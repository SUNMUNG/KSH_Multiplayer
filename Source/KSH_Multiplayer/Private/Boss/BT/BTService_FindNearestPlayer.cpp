// Fill out your copyright notice in the Description page of Project Settings.

#include "Boss/BT/BTService_FindNearestPlayer.h"
#include "Player/PlayerBase.h"
#include "GAS/AttributeSet/BaseAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"

UBTService_FindNearestPlayer::UBTService_FindNearestPlayer()
{
	NodeName = TEXT("Find Nearest Player");

	// 0.5초마다 가장 가까운 플레이어 탐색 (±0.1초 랜덤 편차 적용)
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTService_FindNearestPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	APawn* BossPawn = AIController->GetPawn();
	if (!BossPawn) return;

	// 월드에서 모든 APlayerBase 수집
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerBase::StaticClass(), FoundActors);

	APlayerBase* NearestPlayer  = nullptr;
	float        NearestDistSq  = FLT_MAX;

	for (AActor* Actor : FoundActors)
	{
		APlayerBase* Player = Cast<APlayerBase>(Actor);
		if (!Player || !IsValid(Player) || Player->IsActorBeingDestroyed()) continue;

		// HP > 0 인 살아있는 플레이어만 타겟으로 선정
		UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();
		if (PlayerASC)
		{
			const float HP = PlayerASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
			if (HP <= 0.f) continue;
		}

		const float DistSq = FVector::DistSquared(BossPawn->GetActorLocation(), Player->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestPlayer = Player;
		}
	}

	// 블랙보드에 가장 가까운 플레이어 저장 (없으면 nullptr → IsSet 조건 실패)
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsObject(TargetPlayerKey.SelectedKeyName, NearestPlayer);
	}
}
