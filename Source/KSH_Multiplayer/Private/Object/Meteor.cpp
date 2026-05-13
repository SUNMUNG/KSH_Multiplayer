// Fill out your copyright notice in the Description page of Project Settings.

#include "Object/Meteor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Tile/RaidTile.h"
#include "DrawDebugHelpers.h"
#include "Player/PlayerBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GAS/AttributeSet/BaseAttributeSet.h"
#include "GAS/KSHGameplayTags.h"

AMeteor::AMeteor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	// 1. 충돌 컴포넌트 생성
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(50.0f);
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = CollisionComp;

	// 충돌 이벤트 바인딩
	CollisionComp->OnComponentHit.AddDynamic(this, &AMeteor::OnHit);

	// 2. 메시 컴포넌트 생성
	MeteorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeteorMesh"));
	MeteorMesh->SetupAttachment(RootComponent);

	// 3. 발사체 이동 컴포넌트 생성 (Tick 없이 물리 기반 이동 처리)
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 250.0f;
	ProjectileMovement->MaxSpeed = 250.0f;
	ProjectileMovement->ProjectileGravityScale = 1.0f;

	// 기본 폭발 반경 (타일 6개 정도 커버)
	ExplosionRadius = 600.0f;
	MeteorDamage = 1;

	// 블루 메테오 기본값: MaxHealth의 80% 데미지
	DamagePercentage = 0.8f;

	// 수평 1200 cm/s, 수직 600 cm/s 넉백 (에디터에서 조정 가능)
	KnockbackStrength = 1200.0f;
	KnockbackUpwardStrength = 600.0f;
}

void AMeteor::BeginPlay()
{
	Super::BeginPlay();

	// 스폰되자마자 아래(-Z축)로 낙하 시작
	ProjectileMovement->Velocity = FVector(0.f, 0.f, -1.f) * ProjectileMovement->InitialSpeed;
}

void AMeteor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 충돌 처리는 서버 권한에서만 수행 (타일 파괴, 플레이어 피격 모두 서버에서)
	if (HasAuthority())
	{
		Explode();
	}
}

void AMeteor::Explode()
{
	FVector ExplosionLocation = GetActorLocation();

	DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 32, FColor::Red, false, 2.0f);

	TArray<FOverlapResult> OverlapResults;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(ExplosionRadius);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	// 플레이어(Pawn) 감지를 위해 Pawn 채널 추가
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		ExplosionLocation,
		FQuat::Identity,
		ObjectQueryParams,
		SphereShape
	);

	if (bHit)
	{
		// 같은 액터의 여러 컴포넌트 중복 감지 방지
		TSet<AActor*> HitActors;

		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor || HitActors.Contains(HitActor)) continue;
			HitActors.Add(HitActor);

			// --- 타일 처리 ---
			ARaidTile* HitTile = Cast<ARaidTile>(HitActor);
			if (HitTile)
			{
				HitTile->TakeDamage(MeteorDamage);
				continue;
			}

			// --- 플레이어 처리 (GAS 데미지 + 넉백) ---
			APlayerBase* HitPlayer = Cast<APlayerBase>(HitActor);
			if (HitPlayer)
			{
				ApplyDamageToPlayer(HitPlayer, ExplosionLocation);
			}
		}
	}

	Destroy();
}

void AMeteor::ApplyDamageToPlayer(APlayerBase* HitPlayer, const FVector& ExplosionLocation)
{
	// 이 함수는 OnHit → Explode 경로로만 호출되며, 항상 서버 권한 컨텍스트 안에 있음
	UAbilitySystemComponent* TargetASC = HitPlayer->GetAbilitySystemComponent();
	if (!TargetASC) return;

	// =========================================================
	// [1단계] GAS 퍼센티지 데미지 적용
	//
	// MaxHealth를 직접 읽어 동적 GameplayEffect를 생성.
	// Blueprint GE 에셋 없이 순수 C++로 GAS 파이프라인을 통해 처리하므로
	// PreAttributeChange의 클램핑 등이 모두 정상 작동.
	// =========================================================
	const float MaxHealth    = TargetASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	const float DamageAmount = MaxHealth * DamagePercentage;

	// 런타임에 즉시(Instant) GameplayEffect를 동적으로 생성
	UGameplayEffect* DamageGE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("BlueMeteorInstantDamage")));
	DamageGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Modifier = DamageGE->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute        = UBaseAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp       = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FScalableFloat(-DamageAmount);

	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(DamageGE, Context, 1.0f);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);

	// [DEBUG] 데미지 적용 결과 출력
	const float HealthAfter = TargetASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
		FString::Printf(TEXT("[Meteor] MaxHP=%.0f | Damage=%.0f (%.0f%%) | HP After=%.0f"),
			MaxHealth, DamageAmount, DamagePercentage * 100.f, HealthAfter));

	// =========================================================
	// [2단계] 물리 넉백 — LaunchCharacter
	//
	// 폭발 지점 → 플레이어 방향으로 수평 벡터를 구하고,
	// 별도 수직 강도를 더해 비스듬히 위로 날아가게 만듦.
	// LaunchCharacter는 CharacterMovement를 통해 서버→클라 자동 복제.
	// =========================================================
	FVector KnockbackDir  = (HitPlayer->GetActorLocation() - ExplosionLocation).GetSafeNormal2D();
	FVector LaunchVelocity = KnockbackDir * KnockbackStrength + FVector::UpVector * KnockbackUpwardStrength;

	// bXYOverride=true, bZOverride=true: 현재 속도를 무시하고 완전히 덮어씀
	HitPlayer->LaunchCharacter(LaunchVelocity, true, true);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
		FString::Printf(TEXT("[Meteor] Knockback Speed=%.0f"), LaunchVelocity.Size()));

	// =========================================================
	// [3단계] GAS 피격 태그 부여 (HasDuration GE)
	//
	// DamagePercentage >= 50% → State.HitReaction.Down (3초)
	// 미만                    → State.HitReaction.HitReaction (1.5초)
	// GAS 가 복제 담당, GE 만료 시 태그 자동 제거
	// =========================================================
	const bool   bDown       = (DamagePercentage >= 0.5f);
	const FGameplayTag HitTag = bDown ? TAG_State_HitReaction_Down : TAG_State_HitReaction_HitReaction;
	const float  TagDuration  = bDown ? 3.0f : 1.5f;

	UGameplayEffect* HitGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
	HitGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	HitGE->DurationMagnitude = FScalableFloat(TagDuration);
	HitGE->InheritableGrantedTagsContainer.Added.AddTag(HitTag);

	FGameplayEffectContextHandle HitCtx = TargetASC->MakeEffectContext();
	HitCtx.AddSourceObject(this);
	FGameplayEffectSpec HitSpec(HitGE, HitCtx, 1.0f);
	TargetASC->ApplyGameplayEffectSpecToSelf(HitSpec);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("[Meteor] HitTag → %s (%.1fs) | %s"),
			*HitTag.ToString(), TagDuration, *HitPlayer->GetName()));
}
