// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BossAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;

/**
 * ABossAIController
 *
 * 보스 캐릭터를 제어하는 AI 컨트롤러.
 * OnPossess 시 에디터에서 지정한 BehaviorTree를 자동으로 실행한다.
 *
 * [에디터 설정]
 * 1. BP_BossAIController 생성 (부모: ABossAIController)
 * 2. Behavior Tree Asset 항목에 BT_Boss 지정
 * 3. BP_BossBase의 Pawn → AI Controller Class에 BP_BossAIController 지정
 */
UCLASS()
class KSH_MULTIPLAYER_API ABossAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABossAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// 에디터에서 BT_Boss 에셋을 드래그해 지정하세요
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;
};
