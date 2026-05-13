// Fill out your copyright notice in the Description page of Project Settings.

#include "Boss/BossBase.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSet/BaseAttributeSet.h"
#include "GAS/KSHGameplayTags.h"
#include "Player/PlayerBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayEffect.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"

ABossBase::ABossBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	// AI 이동 시 캐릭터가 이동 방향을 바라보도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	// AI 컨트롤러 회전을 직접 사용하지 않음 (OrientToMovement가 담당)
	bUseControllerRotationYaw = false;

	// --- GAS 컴포넌트 초기화 ---
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// 보스는 Mixed 모드: 자신의 어빌리티 이펙트는 자신에게만, 타겟에 적용하는 GE는 전체 복제
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 플레이어와 동일한 AttributeSet — HP, MaxHP를 동일 시스템으로 관리
	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));

	// --- 아브렐슈드 메테오 패턴 기본값 ---
	// 로스트아크 아브렐슈드 6관문 기준 수치에서 영감을 받아 설정
	InitialPatternDelay  = 5.0f;   // 보스 등장 연출 후 5초
	MeteorPatternInterval = 20.0f; // 20초 주기로 반복
	MeteorTargetCount    = 1;      // 기본 1명 (에디터에서 조정)

	bIsMeteorPatternActive = false;
}

UAbilitySystemComponent* ABossBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABossBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// AI 보스는 BeginPlay 시점에 AIController가 이미 Possess 되어있으므로
		// 여기서 ASC를 초기화하는 것이 안전함
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 메테오 패턴 타이머 자동 시작
		StartMeteorPattern();
	}
}

void ABossBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// BeginPlay 이전에 Possess가 발생하는 엣지 케이스 대비 (안전 초기화)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

// ================================================================
// 패턴 제어
// ================================================================

void ABossBase::StartMeteorPattern()
{
	// 이 함수는 서버 권한 컨텍스트(BeginPlay + HasAuthority)에서만 호출됨
	if (!HasAuthority() || bIsMeteorPatternActive) return;

	bIsMeteorPatternActive = true;

	// 보스 캐스팅 시작 연출 알림 (BP에서 모션/사운드 처리)
	OnMeteorPatternStarted();

	// InitialPatternDelay 후 첫 실행 → 이후 MeteorPatternInterval마다 자동 반복
	GetWorldTimerManager().SetTimer(
		MeteorPatternTimerHandle,
		this,
		&ABossBase::SelectAndTriggerMeteorTargets,
		MeteorPatternInterval,
		true,               // bLoop = true: 자동 반복
		InitialPatternDelay // FirstDelay: 첫 실행 전 딜레이
	);

	UE_LOG(LogTemp, Log, TEXT("[BossBase] 메테오 패턴 시작 — 초기 딜레이: %.1fs, 반복 주기: %.1fs"),
		InitialPatternDelay, MeteorPatternInterval);
}

void ABossBase::StopMeteorPattern()
{
	if (!HasAuthority()) return;

	GetWorldTimerManager().ClearTimer(MeteorPatternTimerHandle);
	bIsMeteorPatternActive = false;

	// 패턴 종료 연출 알림 (BP에서 페이즈 전환 처리)
	OnMeteorPatternEnded();

	UE_LOG(LogTemp, Log, TEXT("[BossBase] 메테오 패턴 정지"));
}

// ================================================================
// 핵심 로직: 타겟 선정 → 블루프린트 이벤트 발생
// ================================================================

void ABossBase::SelectAndTriggerMeteorTargets()
{
	// 이 함수는 SetTimer 콜백이므로 항상 서버에서 실행됨
	// HasAuthority() 재확인은 방어적 프로그래밍
	if (!HasAuthority()) return;

	TArray<APlayerBase*> AlivePlayers = GetAlivePlayers();

	if (AlivePlayers.IsEmpty())
	{
		// 살아있는 플레이어가 없으면 (전멸) 패턴 자동 종료
		UE_LOG(LogTemp, Warning, TEXT("[BossBase] 살아있는 플레이어 없음 — 패턴 종료"));
		StopMeteorPattern();
		return;
	}

	// ──────────────────────────────────────────────────────────
	// Fisher-Yates 셔플: 동일 플레이어 중복 타겟팅 방지
	//
	// 아브렐슈드 패턴: 매 사이클마다 새로운 랜덤 플레이어를 선정하므로
	// 배열 전체를 셔플한 뒤 앞에서 MeteorTargetCount개를 가져옴
	// ──────────────────────────────────────────────────────────
	for (int32 i = AlivePlayers.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		AlivePlayers.Swap(i, j);
	}

	// 실제 타겟 수: MeteorTargetCount와 생존 플레이어 수 중 작은 값
	const int32 SelectCount = FMath::Min(MeteorTargetCount, AlivePlayers.Num());

	for (int32 i = 0; i < SelectCount; ++i)
	{
		APlayerBase* Target = AlivePlayers[i];

		// ──────────────────────────────────────────────────────
		// 핵심 이벤트 발생 (서버에서 Blueprint로 제어권 넘김)
		// Blueprint 구현 예시:
		//   1. Target 위치에 Warning Decal/Actor 스폰 (bReplicates=true)
		//   2. Delay 3~4초
		//   3. Spawn AMeteor at Target.Location + Z(500)
		// ──────────────────────────────────────────────────────
		OnMeteorTargetSelected(Target);

		// 디버그 출력 (화면 + 로그)
		const FString Msg = FString::Printf(
			TEXT("[Boss] 메테오 타겟 선정: %s (%.0f HP)"),
			*Target->GetName(),
			Target->GetAbilitySystemComponent()
				? Target->GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute())
				: -1.f
		);
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Purple, Msg);
		UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	}
}

// ================================================================
// 밀치기 공격 & 메테오 스택 부여
// ================================================================

void ABossBase::PerformPushAttack()
{
	if (!HasAuthority()) return;

	const FVector BossLocation  = GetActorLocation();
	const FVector BossForward   = GetActorForwardVector();

	// ──────────────────────────────────────────────────────────────
	// 감지 박스 배치 전략
	//
	// 박스의 중심점을 보스 앞(Forward 방향)으로 BoxDepth/2 만큼 이동.
	// OverlapMultiByObjectType은 박스 중심 + 회전으로 월드 공간 박스를 정의.
	// 결과: 보스를 기준으로 정면 BoxDepth × 좌우 BoxWidth × 높이 BoxHeight의
	//       직육면체 범위에서 Pawn을 감지.
	// ──────────────────────────────────────────────────────────────
	const FVector BoxCenter   = BossLocation + BossForward * (PushDetectionDepth * 0.5f);
	const FQuat   BoxRotation = GetActorQuat();
	const FCollisionShape BoxShape = FCollisionShape::MakeBox(
		FVector(PushDetectionDepth * 0.5f, PushDetectionWidth * 0.5f, PushDetectionHeight * 0.5f));

	// 디버그 시각화 (3초간 노랑 박스)
	DrawDebugBox(GetWorld(), BoxCenter, BoxShape.GetBox(), BoxRotation,
		FColor::Yellow, false, 3.0f, 0, 4.0f);

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults, BoxCenter, BoxRotation, QueryParams, BoxShape);

	if (!bHit) return;

	// 같은 플레이어의 여러 컴포넌트(Capsule, Mesh 등)가 중복 감지되는 것을 방지
	TSet<APlayerBase*> ProcessedPlayers;

	for (const FOverlapResult& Result : OverlapResults)
	{
		APlayerBase* HitPlayer = Cast<APlayerBase>(Result.GetActor());
		if (!HitPlayer || ProcessedPlayers.Contains(HitPlayer)) continue;
		if (!HitPlayer->GetAbilitySystemComponent()) continue;

		ProcessedPlayers.Add(HitPlayer);

		// ──────────────────────────────────────────────────────────────
		// 밀치기 방향: 보스 중심 → 플레이어 방향 (아레나 가장자리 방향)
		//
		// GetSafeNormal2D(): Z를 무시한 수평 단위벡터
		// 보스가 아레나 중앙에 있을 때, 이 방향이 자연스럽게 가장자리를 향함
		// ──────────────────────────────────────────────────────────────
		const FVector PushDir = (HitPlayer->GetActorLocation() - BossLocation).GetSafeNormal2D();
		const FVector LaunchVelocity = PushDir * PushStrength + FVector::UpVector * PushUpwardStrength;
		HitPlayer->LaunchCharacter(LaunchVelocity, true, true);

		// GAS 피격 태그 부여: State.HitReaction.HitReaction (1.5초 HasDuration GE)
		// GAS 가 복제 담당, 만료 시 자동 제거
		if (UAbilitySystemComponent* HitASC = HitPlayer->GetAbilitySystemComponent())
		{
			UGameplayEffect* HitGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
			HitGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
			HitGE->DurationMagnitude = FScalableFloat(1.5f);
			HitGE->InheritableGrantedTagsContainer.Added.AddTag(TAG_State_HitReaction_HitReaction);

			FGameplayEffectContextHandle HitCtx = AbilitySystemComponent->MakeEffectContext();
			FGameplayEffectSpec HitSpec(HitGE, HitCtx, 1.0f);
			HitASC->ApplyGameplayEffectSpecToSelf(HitSpec);
		}

		// 메테오 스택 +1 부여 → APlayerBase 콜백에서 3 도달 시 개인 메테오 발동
		ApplyMeteorStackToPlayer(HitPlayer);
	}
}

void ABossBase::ApplyMeteorStackToPlayer(APlayerBase* HitPlayer)
{
	UAbilitySystemComponent* TargetASC = HitPlayer->GetAbilitySystemComponent();
	if (!TargetASC) return;

	// ──────────────────────────────────────────────────────────────────
	// Dynamic Instant GE: MeteorStackCount + 1
	//
	// 이 GE가 적용되면 APlayerBase의 OnMeteorStackCountChanged 콜백이 발생.
	// 콜백에서 스택 3 도달 여부를 판단하고 개인 메테오를 처리함.
	//
	// 보스의 ASC를 Context 소스로 등록하여 추후 GE 어트리뷰션 추적 가능.
	// ──────────────────────────────────────────────────────────────────
	UGameplayEffect* StackGE = NewObject<UGameplayEffect>(
		GetTransientPackage(), FName(TEXT("MeteorStackIncrement")));
	StackGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Mod = StackGE->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute  = UBaseAttributeSet::GetMeteorStackCountAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FScalableFloat(1.0f);

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);  // 소스: 보스

	FGameplayEffectSpec Spec(StackGE, Context, 1.0f);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);

	// 스택 적용 후 현재 값 (콜백보다 먼저 읽힐 수 있음)
	const float CurrentStack = TargetASC->GetNumericAttribute(
		UBaseAttributeSet::GetMeteorStackCountAttribute());

	GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Orange,
		FString::Printf(TEXT("[Boss] %s 메테오 스택: %.0f / 3"), *HitPlayer->GetName(), CurrentStack));
	UE_LOG(LogTemp, Log, TEXT("[BossBase] ApplyMeteorStack → %s (%.0f/3)"),
		*HitPlayer->GetName(), CurrentStack);
}

// ================================================================
// 살아있는 플레이어 목록 수집
// ================================================================

TArray<APlayerBase*> ABossBase::GetAlivePlayers() const
{
	TArray<APlayerBase*> Result;
	TArray<AActor*> FoundActors;

	// 월드에서 APlayerBase 파생 모든 액터 검색
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerBase::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		APlayerBase* Player = Cast<APlayerBase>(Actor);

		// 유효성 검사: null, 파괴 중인 액터 제외
		if (!Player || !IsValid(Player) || Player->IsActorBeingDestroyed()) continue;

		UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();
		if (!PlayerASC) continue;

		// HP > 0인 플레이어만 타겟 풀에 포함 (Down 상태여도 HP가 있으면 타겟 가능)
		const float CurrentHP = PlayerASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
		if (CurrentHP > 0.f)
		{
			Result.Add(Player);
		}
	}

	return Result;
}
