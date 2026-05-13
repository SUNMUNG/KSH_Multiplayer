// Fill out your copyright notice in the Description page of Project Settings.

#include "Tile/TileManager.h"
#include "Tile/RaidTile.h"

ATileManager::ATileManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TileSize = 1000.f; // 임시 타일 크기 (에디터에서 타일 메시 크기에 맞게 조정)
}

void ATileManager::BeginPlay()
{
	Super::BeginPlay();

	// 타일(맵) 생성은 서버에서만 수행해야 함
	if (HasAuthority())
	{
		GenerateGrid();
	}
}

void ATileManager::GenerateGrid()
{
	if (!TileClass) return;

	// 3x3 그리드 생성
	for (int32 x = -1; x <= 1; ++x)
	{
		for (int32 y = -1; y <= 1; ++y)
		{
			// 1. 매니저 중심으로부터의 로컬 오프셋 계산
			FVector LocalOffset = FVector(x * TileSize, y * TileSize, 0.f);

			// 2. [중요] 매니저의 월드 트랜스폼을 적용하여 로컬 좌표를 월드 좌표로 변환
			// 이렇게 하면 매니저를 45도 기울인 경우 타일도 자동으로 45도 회전된 위치에 배치됨
			FVector SpawnLocation = GetActorTransform().TransformPosition(LocalOffset);

			// 3. 타일 회전은 매니저의 회전을 따름
			// (매니저가 45도 회전되어 있으면 타일도 동일하게 배치됨)
			FRotator SpawnRotation = GetActorRotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;

			ARaidTile* NewTile = GetWorld()->SpawnActor<ARaidTile>(TileClass, SpawnLocation, SpawnRotation, SpawnParams);

			if (NewTile)
			{
				SpawnedTiles.Add(NewTile);

				// (0,0) 좌표의 중앙 타일
				if (x == 0 && y == 0)
				{
					NewTile->Tags.Add(FName("CenterTile"));
					NewTile->InitHealth(13); // 중앙 타일은 13회 맞아야 파괴
				}
				else
				{
					NewTile->InitHealth(3); // 일반 타일은 3회 맞아야 파괴
				}
			}
		}
	}
}
