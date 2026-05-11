// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileManager.generated.h"

class ARaidTile;

UCLASS()
class KSH_MULTIPLAYER_API ATileManager : public AActor
{
	GENERATED_BODY()
	
public:
	ATileManager();

protected:
	virtual void BeginPlay() override;

	// 블루프린트에서 스폰할 타일(방금 만든 RaidTile)을 지정할 수 있게 합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Tile System")
	TSubclassOf<ARaidTile> TileClass;

	// 타일 하나의 크기 (에디터에서 수정 가능)
	UPROPERTY(EditAnywhere, Category = "Tile System")
	float TileSize;

	// 생성된 9개의 타일을 보관하는 배열
	UPROPERTY()
	TArray<ARaidTile*> SpawnedTiles;

	// 타일을 스폰하는 함수
	UFUNCTION(BlueprintCallable, Category = "Tile System")
	void GenerateGrid();

};
