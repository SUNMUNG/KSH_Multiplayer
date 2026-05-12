// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Meteor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Tile/RaidTile.h"
#include "DrawDebugHelpers.h"

AMeteor::AMeteor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 메테오의 위치와 폭발은 서버가 통제합니다.
	SetReplicateMovement(true);

	// 1. 충돌 컴포넌트 세팅
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(50.0f);
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = CollisionComp;

	// 충돌 이벤트 바인딩
	CollisionComp->OnComponentHit.AddDynamic(this, &AMeteor::OnHit);

	// 2. 메쉬 컴포넌트 세팅
	MeteorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeteorMesh"));
	MeteorMesh->SetupAttachment(RootComponent);

	// 3. 발사체 낙하 무브먼트 세팅 (직접 Tick으로 내리지 않고 엔진 기능 활용)
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 250.0f; // 떨어지는 속도
	ProjectileMovement->MaxSpeed = 250.0f;
	ProjectileMovement->ProjectileGravityScale = 1.0f;

	// 아브렐슈드 6관문을 위한 폭발 반경 (타일이 1000 사이즈일 때 기준)
	ExplosionRadius = 600.0f;

	MeteorDamage = 1;
}

void AMeteor::BeginPlay()
{
	Super::BeginPlay();

	// 생성되자마자 수직 아래(Z축)로 떨어지게 만듭니다.
	ProjectileMovement->Velocity = FVector(0.f, 0.f, -1.f) * ProjectileMovement->InitialSpeed;
}

void AMeteor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 충돌 연산과 타일 파괴는 오직 '서버'에서만 수행해야 합니다.
	if (HasAuthority())
	{
		Explode();
	}
}

void AMeteor::Explode()
{
	FVector ExplosionLocation = GetActorLocation();

	DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 32, FColor::Red, false, 2.0f);

	// 충돌 결과를 담을 배열
	TArray<FOverlapResult> OverlapResults;

	// 폭발 반경 설정 (구 모양)
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(ExplosionRadius);

	// 타일들이 WorldDynamic 또는 WorldStatic 채널이라고 가정
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	// 폭발 위치를 중심으로 구형 충돌 검사 실행
	bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		ExplosionLocation,
		FQuat::Identity,
		ObjectQueryParams,
		SphereShape
	);

	if (bHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			ARaidTile* HitTile = Cast<ARaidTile>(Result.GetActor());
			if (HitTile)
			{
				// [수정] 무조건 1이 아니라, 자신의 데미지 변수를 전달합니다.
				HitTile->TakeDamage(MeteorDamage);
			}
		}
	}

	// TODO: 블루프린트에서 파괴 이펙트(나이아가라)나 사운드를 재생할 수 있도록 멀티캐스트 로직 추가 가능

	// 폭발이 끝났으므로 메테오 삭제
	Destroy();
}