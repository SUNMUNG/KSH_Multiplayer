// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// ================================================================
// 피격 상태 태그
//
// GE HasDuration 으로 타겟 ASC 에 부여됩니다.
// GAS 가 복제를 담당하므로 별도 DOREPLIFETIME 불필요.
// AnimBP 는 ASC->HasMatchingGameplayTag() 로 상태를 읽습니다.
// ================================================================
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_HitReaction_HitReaction) // 밀치기/피격 이상
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_HitReaction_Down)        // 다운 상태

// ================================================================
// 메테오 스택 태그
//
// TAG_State_Debuff_MeteorStack : UI 표시용 루즈 태그 (카운트 = 스택 수)
// TAG_Event_MeteorStackFull    : 스택 3 달성 시 SendGameplayEventToActor 로 발송
//                                GA_PersonalMeteor 의 AbilityTriggers 에 이 태그 등록 필요
// ================================================================
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Debuff_MeteorStack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_MeteorStackFull)
