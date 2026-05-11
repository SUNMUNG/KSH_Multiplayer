// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RaidTile.generated.h"

// 타일의 상태를 나타내는 열거형 (블루프린트에서 사용 가능)
UENUM(BlueprintType)
enum class ETileState : uint8
{
	Normal		UMETA(DisplayName = "Normal"),
	Cracked		UMETA(DisplayName = "Cracked"),
	Destroyed	UMETA(DisplayName = "Destroyed")
};

UCLASS()
class KSH_MULTIPLAYER_API ARaidTile : public AActor
{
	GENERATED_BODY()

public:
	ARaidTile();

	// 네트워크 동기화를 위한 필수 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* TileMesh;

	// [핵심] 타일 상태가 변하면 OnRep_TileState 함수가 클라이언트에서 자동 호출됩니다.
	UPROPERTY(ReplicatedUsing = OnRep_TileState, BlueprintReadOnly, Category = "Tile State")
	ETileState CurrentState;

	UFUNCTION()
	void OnRep_TileState();

public:
	// 메테오가 떨어졌을 때 서버에서만 호출될 함수
	UFUNCTION(BlueprintCallable)
	void TakeDamage();

	// 머티리얼 색상 변경이나 파괴 이펙트 등은 블루프린트에서 편하게 작업하도록 열어둡니다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Tile State")
	void UpdateTileVisuals(ETileState NewState);
};