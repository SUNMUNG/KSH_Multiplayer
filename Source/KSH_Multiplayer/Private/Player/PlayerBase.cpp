// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerBase.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSet/BaseAttributeSet.h"

// Sets default values
APlayerBase::APlayerBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	// [추가] 어빌리티 시스템 컴포넌트 생성 및 네트워크 설정
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// 플레이어 캐릭터이므로 Mixed 모드 권장 (자신의 이펙트는 즉시, 타인은 서버를 통해)
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    BaseAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("BaseAttributeSet"));
}

UAbilitySystemComponent* APlayerBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// [추가] 서버에서 캐릭터 빙의 시 ASC 초기화
void APlayerBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);

        // [추가] 서버에서만 기본 기술들을 부여합니다.
        if (HasAuthority())
        {
            for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
            {
                if (AbilityClass)
                {
                    // 기술 레벨 1, 입력 ID 없음(-1)으로 부여
                    AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, -1));
                }
            }
        }
    }
}

// Called when the game starts or when spawned
void APlayerBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

