// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Meteor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class KSH_MULTIPLAYER_API AMeteor : public AActor
{
	GENERATED_BODY()

public:
	AMeteor();

protected:

	// 땅(또는 타일)에 부딪혔을 때 호출될 함수
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 폭발 및 타일 검색 로직
	void Explode();

protected:
	virtual void BeginPlay() override;

	// 충돌 판정을 담당할 루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionComp;

	// 눈에 보일 메테오 메쉬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeteorMesh;

	// 부드러운 낙하를 위한 발사체 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	// 폭발 반경 (에디터에서 수정 가능)
	UPROPERTY(EditAnywhere, Category = "Meteor")
	float ExplosionRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteor")
	int32 MeteorDamage;


};
