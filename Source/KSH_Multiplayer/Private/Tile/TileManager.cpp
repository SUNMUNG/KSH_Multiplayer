// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/TileManager.h"
#include "Tile/RaidTile.h"

ATileManager::ATileManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TileSize = 1000.f; // 임시 타일 크기 (블루프린트에서 큐브 크기에 맞춰 조절)
}

void ATileManager::BeginPlay()
{
	Super::BeginPlay();

	// 맵(타일) 생성은 무조건 서버에서만 수행해야 합니다.
	if (HasAuthority())
	{
		GenerateGrid();
	}
}

void ATileManager::GenerateGrid()
{
	if (!TileClass) return;

	// 3x3 그리드 루프
	for (int32 x = -1; x <= 1; ++x)
	{
		for (int32 y = -1; y <= 1; ++y)
		{
			// 1. 매니저 중심으로부터의 상대적인 거리(Local Offset)를 먼저 계산합니다.
			FVector LocalOffset = FVector(x * TileSize, y * TileSize, 0.f);

			// 2. [중요] 매니저의 현재 트랜스폼을 기준으로 로컬 좌표를 월드 좌표로 변환합니다.
			// 이렇게 하면 매니저가 45도 돌아가 있으면 위치값도 자동으로 45도 회전되어 계산됩니다.
			FVector SpawnLocation = GetActorTransform().TransformPosition(LocalOffset);

			// 3. 타일 자체의 회전도 매니저의 회전과 맞춥니다.
			// 매니저를 45도 돌릴 것이므로, 타일들도 매니저를 따라 45도 돌아간 상태로 스폰됩니다.
			FRotator SpawnRotation = GetActorRotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;

			ARaidTile* NewTile = GetWorld()->SpawnActor<ARaidTile>(TileClass, SpawnLocation, SpawnRotation, SpawnParams);

			if (NewTile)
			{
				SpawnedTiles.Add(NewTile);

				// (0,0) 좌표는 중앙 타일
				if (x == 0 && y == 0)
				{
					NewTile->Tags.Add(FName("CenterTile"));
					NewTile->InitHealth(13); // 중앙 타일은 13대 맞아야 부서짐
				}
				else
				{
					NewTile->InitHealth(3); // 일반 타일은 3대 맞아야 부서짐
				}
			}
		}
	}
}