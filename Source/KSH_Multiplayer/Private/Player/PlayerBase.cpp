// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/PlayerBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/AttributeSet/BaseAttributeSet.h"
#include "GAS/KSHGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"

// ================================================================
// 복제 속성 등록
// ================================================================

void APlayerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 피격 상태는 GAS 태그로 대체 — 별도 DOREPLIFETIME 불필요
}

// ================================================================
// 생성자
// ================================================================

APlayerBase::APlayerBase()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));
}

UAbilitySystemComponent* APlayerBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// ================================================================
// 컨트롤러 빙의 시 ASC 초기화 (서버)
// ================================================================

void APlayerBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		// 기본 어빌리티 일괄 부여
		// BP_GA_PersonalMeteor 를 DefaultAbilities 에 추가하고
		// 해당 GA 의 AbilityTriggers 를 Event.MeteorStackFull 로 설정하면
		// 스택 3 달성 시 자동 활성화됩니다.
		for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
		{
			if (AbilityClass)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, -1));
			}
		}

		// MeteorStackCount 속성 변경 시 OnMeteorStackCountChanged 콜백 등록
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UBaseAttributeSet::GetMeteorStackCountAttribute()
		).AddUObject(this, &APlayerBase::OnMeteorStackCountChanged);
	}
}

// ================================================================
// OnRep_Controller: 클라이언트 측 ASC 초기화
// PossessedBy 는 서버에서만 발생하므로 클라이언트는 여기서 초기화
// ================================================================

void APlayerBase::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RefreshAbilityActorInfo();
	}
}

// ================================================================
// BeginPlay: GAS 태그 변경 이벤트 등록 (서버 + 클라이언트 모두)
// ================================================================

void APlayerBase::BeginPlay()
{
	Super::BeginPlay();

	if (!AbilitySystemComponent) return;

	// 태그가 추가/제거될 때 BP 이벤트로 AnimBP 에 통지
	AbilitySystemComponent->RegisterGameplayTagEvent(
		TAG_State_HitReaction_Down, EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &APlayerBase::OnDownTagChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(
		TAG_State_HitReaction_HitReaction, EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &APlayerBase::OnHitReactionTagChanged);
}

// ================================================================
// GAS 태그 변경 콜백 → Blueprint 이벤트 전달
// ================================================================

void APlayerBase::OnDownTagChanged(const FGameplayTag Tag, int32 Count)
{
	OnDownStateChanged(Count > 0);
}

void APlayerBase::OnHitReactionTagChanged(const FGameplayTag Tag, int32 Count)
{
	OnHitReactionChanged(Count > 0);
}

// ================================================================
// 메테오 스택 시스템
// ================================================================

int32 APlayerBase::GetMeteorStackCount() const
{
	if (!AbilitySystemComponent) return 0;
	return FMath::RoundToInt(
		AbilitySystemComponent->GetNumericAttribute(UBaseAttributeSet::GetMeteorStackCountAttribute()));
}

void APlayerBase::OnMeteorStackCountChanged(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority()) return;

	const int32 OldCount = FMath::RoundToInt(Data.OldValue);
	const int32 NewCount = FMath::RoundToInt(Data.NewValue);

	// UI 용 루즈 태그 동기화 (State.Debuff.MeteorStack 카운트 = 스택 수)
	const int32 Delta = NewCount - OldCount;
	if (Delta > 0)
		AbilitySystemComponent->AddLooseGameplayTag(TAG_State_Debuff_MeteorStack, Delta);
	else if (Delta < 0)
		AbilitySystemComponent->RemoveLooseGameplayTag(TAG_State_Debuff_MeteorStack, -Delta);

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
		FString::Printf(TEXT("[%s] 메테오 스택: %d → %d"), *GetName(), OldCount, NewCount));

	// ──────────────────────────────────────────────────────────────────
	// 스택 3 도달: GAS GameplayEvent 로 GA_PersonalMeteor 자동 활성화
	//
	// [에디터 설정 필수]
	// BP_GA_PersonalMeteor → AbilityTriggers → Add Element:
	//   Trigger Tag   : Event.MeteorStackFull
	//   Trigger Source: GameplayEvent
	// ──────────────────────────────────────────────────────────────────
	if (NewCount >= 3 && OldCount < 3)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Red,
			FString::Printf(TEXT("[%s] 스택 3! 개인 메테오 발동"), *GetName()));

		ResetMeteorStack();

		FGameplayEventData EventData;
		EventData.Instigator = this;
		EventData.Target     = this;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			this, TAG_Event_MeteorStackFull, EventData);
	}
}

void APlayerBase::ResetMeteorStack()
{
	if (!HasAuthority() || !AbilitySystemComponent) return;

	// Override GE: MeteorStackCount 를 현재 값에 관계없이 0 으로 덮어씀
	UGameplayEffect* ResetGE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("MeteorStackReset")));
	ResetGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Mod = ResetGE->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute         = UBaseAttributeSet::GetMeteorStackCountAttribute();
	Mod.ModifierOp        = EGameplayModOp::Override;
	Mod.ModifierMagnitude = FScalableFloat(0.0f);

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	FGameplayEffectSpec Spec(ResetGE, Context, 1.0f);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(Spec);
}

void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
