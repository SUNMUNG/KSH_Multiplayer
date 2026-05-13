// Fill out your copyright notice in the Description page of Project Settings.

#include "Boss/BossAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

ABossAIController::ABossAIController()
{
	// AIController는 PlayerState가 필요 없음
	bWantsPlayerState = false;
}

void ABossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		// RunBehaviorTree: Blackboard 초기화 + BT 실행을 한 번에 처리
		RunBehaviorTree(BehaviorTreeAsset);
		UE_LOG(LogTemp, Log, TEXT("[BossAIController] BehaviorTree 실행 시작: %s"), *BehaviorTreeAsset->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossAIController] BehaviorTreeAsset 미설정! BP_BossAIController 디테일 패널에서 지정하세요."));
	}
}

void ABossAIController::OnUnPossess()
{
	Super::OnUnPossess();
	StopMovement();
}
