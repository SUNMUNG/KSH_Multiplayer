#include "Tile/RaidTile.h"
#include "Net/UnrealNetwork.h"

ARaidTile::ARaidTile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	RootComponent = TileMesh;

	// 기본값 세팅 (매니저가 다시 덮어씌울 예정)
	MaxHealth = 3;
	CurrentHealth = 3;
	CurrentState = ETileState::Normal;
}

void ARaidTile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 체력 변수들도 네트워크 동기화에 추가
	DOREPLIFETIME(ARaidTile, MaxHealth);
	DOREPLIFETIME(ARaidTile, CurrentHealth);
	DOREPLIFETIME(ARaidTile, CurrentState);
}

void ARaidTile::InitHealth(int32 InMaxHealth)
{
	if (HasAuthority())
	{
		MaxHealth = InMaxHealth;
		CurrentHealth = InMaxHealth;
		UpdateStateBasedOnHealth();
	}
}

void ARaidTile::TakeDamage(int32 DamageAmount)
{
	// 서버에서만, 그리고 체력이 0보다 클 때만 데미지를 입습니다.
	if (HasAuthority() && CurrentHealth > 0)
	{
		// 1씩 깎던 것을 들어온 데미지만큼 깎도록 변경
		CurrentHealth -= DamageAmount;
		UpdateStateBasedOnHealth();
	}
}

// [핵심] 체력 비율(%)에 따른 상태 자동 계산 로직
void ARaidTile::UpdateStateBasedOnHealth()
{
	ETileState NewState = ETileState::Normal;

	if (CurrentHealth <= 0)
	{
		NewState = ETileState::Destroyed;
		SetActorEnableCollision(false); // 부서지면 충돌 끄기
	}
	else
	{
		// 남은 체력의 비율 계산 (0.0 ~ 1.0)
		float HealthRatio = (float)CurrentHealth / (float)MaxHealth;

		// [비율 로직 분기]
		// 3체력: 1남음(0.33) -> Cracked / 2남음(0.66) -> LittleCracked / 3남음(1.0) -> Normal
		// 13체력: 1~4남음(0.07~0.3) -> Cracked / 5~9남음(0.38~0.69) -> LittleCracked / 10~13남음 -> Normal

		if (HealthRatio <= 0.35f)
		{
			// 체력이 35% 이하일 때 (심하게 파손)
			NewState = ETileState::Cracked;
		}
		else if (HealthRatio <= 0.70f)
		{
			// 체력이 35% 초과 ~ 70% 이하일 때 (살짝 파손)
			NewState = ETileState::LittleCracked;
		}
		else
		{
			// 체력이 70% 초과일 때 (정상)
			NewState = ETileState::Normal;
		}
	}

	// 상태가 변했을 때만 네트워크 갱신
	if (CurrentState != NewState)
	{
		CurrentState = NewState;
		OnRep_TileState();
	}
}

void ARaidTile::OnRep_TileState()
{
	UpdateTileVisuals(CurrentState);
}