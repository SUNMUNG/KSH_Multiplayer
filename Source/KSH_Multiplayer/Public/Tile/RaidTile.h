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
	LittleCracked		UMETA(DisplayName = "LittleCracked"),
	Cracked		UMETA(DisplayName = "Cracked"),
	Destroyed	UMETA(DisplayName = "Destroyed")
};

UCLASS()
class KSH_MULTIPLAYER_API ARaidTile : public AActor
{
	GENERATED_BODY()

public:
	ARaidTile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 매니저가 스폰 직후 체력을 설정할 수 있도록 여는 함수
	void InitHealth(int32 InMaxHealth);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* TileMesh;

	// [추가] 타일의 체력 데이터
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tile State")
	int32 MaxHealth;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tile State")
	int32 CurrentHealth;

	UPROPERTY(ReplicatedUsing = OnRep_TileState, BlueprintReadOnly, Category = "Tile State")
	ETileState CurrentState;

	UFUNCTION()
	void OnRep_TileState();

	// [추가] 체력 비율에 따라 상태를 결정하는 내부 함수
	void UpdateStateBasedOnHealth();

public:
	UFUNCTION(BlueprintCallable)
	void TakeDamage(int32 DamageAmount = 1);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tile State")
	void UpdateTileVisuals(ETileState NewState);
};