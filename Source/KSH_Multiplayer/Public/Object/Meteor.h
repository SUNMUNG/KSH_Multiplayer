// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Meteor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class APlayerBase;

UCLASS()
class KSH_MULTIPLAYER_API AMeteor : public AActor
{
	GENERATED_BODY()

public:
	AMeteor();

protected:
	// 충돌(또는 타일)에 닿았을 때 호출되는 함수
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 폭발 후 타일 검색·처리
	void Explode();

protected:
	virtual void BeginPlay() override;

	// 충돌 범위를 처리하는 스피어 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionComp;

	// 시각 표현용 스태틱 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeteorMesh;

	// 부드럽게 낙하하는 발사체 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	// 폭발 반경 (에디터에서 조정 가능)
	UPROPERTY(EditAnywhere, Category = "Meteor")
	float ExplosionRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteor")
	int32 MeteorDamage;

	// 플레이어 MaxHealth 대비 데미지 비율 (0.8 = 80%)
	UPROPERTY(EditAnywhere, Category = "Meteor|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamagePercentage;

	// 넉백 수평 강도 (cm/s)
	UPROPERTY(EditAnywhere, Category = "Meteor|Knockback")
	float KnockbackStrength;

	// 넉백 수직 강도 (cm/s) — 위로 튕겨지는 정도
	UPROPERTY(EditAnywhere, Category = "Meteor|Knockback")
	float KnockbackUpwardStrength;

private:
	// [서버 전용] 단일 플레이어에게 GAS 데미지와 넉백 + 피격 태그 GE 적용
	void ApplyDamageToPlayer(APlayerBase* HitPlayer, const FVector& ExplosionLocation);
};
