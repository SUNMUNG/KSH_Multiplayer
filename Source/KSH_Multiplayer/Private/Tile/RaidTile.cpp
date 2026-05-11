// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/RaidTile.h"
#include "Net/UnrealNetwork.h"

ARaidTile::ARaidTile()
{
	PrimaryActorTick.bCanEverTick = false; // 타일은 매 프레임 Tick 연산이 필요 없으므로 최적화
	bReplicates = true; // 멀티플레이어 환경에서 액터 복제 허용

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	RootComponent = TileMesh;

	CurrentState = ETileState::Normal;
}

void ARaidTile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// CurrentState 변수를 멀티플레이 네트워크로 동기화 등록
	DOREPLIFETIME(ARaidTile, CurrentState);
}

void ARaidTile::TakeDamage()
{
	// 오직 서버(Authority)에서만 타일의 상태를 변경할 수 있습니다.
	if (HasAuthority())
	{
		if (CurrentState == ETileState::Normal)
		{
			CurrentState = ETileState::Cracked; // 1번 맞으면 금가기
		}
		else if (CurrentState == ETileState::Cracked)
		{
			CurrentState = ETileState::Destroyed; // 2번 맞으면 파괴
			// 타일이 부서졌을 때 충돌 판정을 꺼서 플레이어가 떨어지게 만듭니다.
			SetActorEnableCollision(false);
		}

		// 방장(서버 플레이어)의 화면도 갱신해주기 위해 수동으로 한 번 호출
		OnRep_TileState();
	}
}

void ARaidTile::OnRep_TileState()
{
	// 서버에서 상태가 변해 클라이언트로 동기화되면 이 함수가 실행됩니다.
	// 블루프린트의 시각 효과(이펙트, 머티리얼 변경 등) 이벤트를 호출합니다.
	UpdateTileVisuals(CurrentState);
}