// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PushAttack.generated.h"

/**
 * UBTTask_PushAttack
 *
 * 보스의 PerformPushAttack()을 호출하고, 쿨다운 시간만큼 대기한 뒤 Succeeded를 반환.
 * BT에서 이 태스크가 끝나야 다음 노드(MoveTo)로 넘어가므로
 * CooldownTime이 사실상 공격 쿨다운 역할을 함.
 *
 * [에디터 설정]
 * BT에서 이 태스크 노드를 선택한 뒤 Cooldown Time을 조정하세요 (기본 3초).
 */
UCLASS()
class KSH_MULTIPLAYER_API UBTTask_PushAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PushAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 공격 후 이 시간이 지나야 태스크가 완료되어 BT가 다음으로 진행됨
	UPROPERTY(EditAnywhere, Category = "Push Attack", meta = (ClampMin = "0.5"))
	float CooldownTime = 3.0f;

private:
	// 태스크 진행 시간 추적용 (NodeMemory 대신 멤버로 사용)
	float ElapsedTime = 0.f;
};
