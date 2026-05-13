// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/KSHGameplayTags.h"
#include "PlayerBase.generated.h"

class UGameplayAbility;
struct FOnAttributeChangeData;

/**
 * APlayerBase
 *
 * GAS 기반 플레이어 캐릭터.
 *
 * [피격 상태 시스템]
 *   EHitReactionState 열거형 + DOREPLIFETIME 제거.
 *   대신 GAS 태그(State.HitReaction.*) 를 GE HasDuration 으로 부여.
 *   GAS 가 복제를 담당 → BP 에서 OnDownStateChanged / OnHitReactionChanged 이벤트로 반응.
 *
 * [메테오 스택 시스템]
 *   MeteorStackCount 속성이 3 에 도달하면 Event.MeteorStackFull 을 발송.
 *   BP_GA_PersonalMeteor 를 DefaultAbilities 에 추가하고
 *   해당 GA 의 AbilityTriggers 에 Event.MeteorStackFull 태그를 등록하면 자동 활성화됩니다.
 */
UCLASS()
class KSH_MULTIPLAYER_API APlayerBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APlayerBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ================================================================
	// GAS 피격 상태 이벤트
	//
	// ASC 에 피격 태그가 추가/제거될 때 호출됩니다.
	// BP 에서 구현해 AnimBP 를 구동하세요.
	// ================================================================

	// State.HitReaction.Down 태그 변경 시 호출 (bIsDown=true: 다운, false: 회복)
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|Combat")
	void OnDownStateChanged(bool bIsDown);

	// State.HitReaction.HitReaction 태그 변경 시 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|Combat")
	void OnHitReactionChanged(bool bIsHitReaction);

	// ================================================================
	// 메테오 스택 (외부 읽기용)
	// ================================================================

	UFUNCTION(BlueprintPure, Category = "Combat|MeteorStack")
	int32 GetMeteorStackCount() const;

	// [Authority Only] 스택을 0 으로 Override GE 리셋
	void ResetMeteorStack();

protected:
	virtual void BeginPlay() override;

	// MeteorStackCount 속성 변경 콜백 (PossessedBy 에서 ASC 델리게이트에 등록)
	void OnMeteorStackCountChanged(const FOnAttributeChangeData& Data);

	// GAS 태그 변경 콜백 (BeginPlay 에서 RegisterGameplayTagEvent 로 등록)
	void OnDownTagChanged(const FGameplayTag Tag, int32 Count);
	void OnHitReactionTagChanged(const FGameplayTag Tag, int32 Count);

	// ================================================================
	// GAS 컴포넌트
	// ================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	// 에디터에서 기본 부여할 어빌리티 목록.
	// BP_GA_PersonalMeteor 를 추가하고 AbilityTriggers: Event.MeteorStackFull 설정.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	class UBaseAttributeSet* BaseAttributeSet;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
