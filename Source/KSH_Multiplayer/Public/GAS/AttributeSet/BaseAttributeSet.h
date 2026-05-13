// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.generated.h"


#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * UBaseAttributeSet
 *
 * 플레이어와 보스가 공유하는 GAS 어트리뷰트 세트.
 * Health, MaxHealth: 체력 시스템
 * MeteorStackCount: 보스 밀치기 피격 스택 카운터
 */
UCLASS()
class KSH_MULTIPLAYER_API UBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBaseAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 체력 회복이나 오버히트에서 내려갈 수 있도록 범위를 제한(Clamp)하는 함수
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// ================================================================
	// 어트리뷰트 값 선언
	// ================================================================

	// 1. 현재 체력
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Health)

	// 2. 최대 체력
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHealth)

	// 3. 메테오 스택 카운터 (보스 밀치기 피격 시 +1, 3개 도달 시 개인 메테오 발동)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MeteorStackCount)
	FGameplayAttributeData MeteorStackCount;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MeteorStackCount)

	// ================================================================
	// RepNotify 함수
	// ================================================================

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_MeteorStackCount(const FGameplayAttributeData& OldMeteorStackCount);
};
