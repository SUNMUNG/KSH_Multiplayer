// Fill out your copyright notice in the Description page of Project Settings.

#include "Boss/BT/BTTask_PushAttack.h"
#include "Boss/BossBase.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_PushAttack::UBTTask_PushAttack()
{
	NodeName = TEXT("Boss Push Attack");

	// TickTask를 사용하려면 반드시 true 설정 (InProgress 상태에서 매 프레임 호출)
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_PushAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	ABossBase* Boss = Cast<ABossBase>(AIController->GetPawn());
	if (!Boss) return EBTNodeResult::Failed;

	// 즉시 밀치기 공격 실행
	Boss->PerformPushAttack();

	// 쿨다운 타이머 초기화
	ElapsedTime = 0.f;

	// InProgress 반환 → TickTask에서 CooldownTime 후에 Succeeded 반환
	return EBTNodeResult::InProgress;
}

void UBTTask_PushAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	ElapsedTime += DeltaSeconds;

	if (ElapsedTime >= CooldownTime)
	{
		// 쿨다운 완료 → BT가 다음 노드로 진행
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
