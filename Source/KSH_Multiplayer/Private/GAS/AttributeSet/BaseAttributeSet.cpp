// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/AttributeSet/BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"

UBaseAttributeSet::UBaseAttributeSet()
{
	// 기본값 초기화
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMeteorStackCount(0.f);
}

void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 항상 클라이언트에 복제되도록 설정 (REPNOTIFY_Always: 값이 같아도 OnRep 호출)
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Health,           COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHealth,        COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MeteorStackCount, COND_None, REPNOTIFY_Always);
}

void UBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 체력이 변화할 때, 0 ~ MaxHealth 사이로 값을 강제로 클램핑
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	// 스택 카운트는 0 ~ 10 범위로 클램핑 (Override GE로 리셋 시 음수 방지)
	else if (Attribute == GetMeteorStackCountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 10.0f);
	}
}

void UBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Health, OldHealth);
}

void UBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxHealth, OldMaxHealth);
}

void UBaseAttributeSet::OnRep_MeteorStackCount(const FGameplayAttributeData& OldMeteorStackCount)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MeteorStackCount, OldMeteorStackCount);
}
