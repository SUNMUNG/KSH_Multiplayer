// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/YellowMeteor.h"

AYellowMeteor::AYellowMeteor()
{
	// 아브렐슈드의 노란 메테오는 범위가 매우 넓고, 타일에 3의 데미지를 줍니다.
	ExplosionRadius = 1700.0f;

	// 타일 3칸의 체력을 한 번에 날려버립니다.
	MeteorDamage = 3;
}