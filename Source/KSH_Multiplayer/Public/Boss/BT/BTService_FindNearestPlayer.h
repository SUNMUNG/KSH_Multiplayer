// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FindNearestPlayer.generated.h"

/**
 * UBTService_FindNearestPlayer
 *
 * 0.5초마다 월드에서 가장 가까운 살아있는 APlayerBase를 찾아
 * 블랙보드의 "TargetPlayer" 키에 저장한다.
 *
 * [에디터 설정]
 * BT 노드에 붙인 뒤 Target Player Key → "TargetPlayer" (Object) 선택
 */
UCLASS()
class KSH_MULTIPLAYER_API UBTService_FindNearestPlayer : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_FindNearestPlayer();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

public:
	// 블랙보드 키 선택기 — 에디터에서 "TargetPlayer" (Object) 키를 선택하세요
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetPlayerKey;
};
