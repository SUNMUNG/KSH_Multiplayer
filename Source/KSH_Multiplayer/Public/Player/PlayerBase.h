// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "PlayerBase.generated.h"

class UGameplayAbility;

UENUM(BlueprintType)
enum class EHitReactionState : uint8
{
	Normal			UMETA(DisplayName = "Normal"),			// 평상시
	Flinch			UMETA(DisplayName = "Flinch"),			// 단순 경직 (움찔)
	HitReaction		UMETA(DisplayName = "Hit Reaction"),	// 피격이상 (넉백, 에어본 등 물리 이동)
	Down			UMETA(DisplayName = "Down"),			// 다운 (바닥에 누움, 기상기 필요)
	StatusEffect	UMETA(DisplayName = "Status Effect")	// 상태이상 (기절, 동결 등)
};

UCLASS()
class KSH_MULTIPLAYER_API APlayerBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerBase();

	// [추가] GAS 인터페이스 필수 오버라이드 함수
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// [추가] 서버에서 컴포넌트를 초기화하기 위한 오버라이드
	virtual void PossessedBy(AController* NewController) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


protected:
	// [추가] 어빌리티 시스템 컴포넌트 본체
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* AbilitySystemComponent;

	// 에디터에서 부여할 기술 리스트를 고를 수 있게 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	class UBaseAttributeSet* BaseAttributeSet;

};
