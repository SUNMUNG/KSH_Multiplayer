// Fill out your copyright notice in the Description page of Project Settings.

#include "Object/YellowMeteor.h"

AYellowMeteor::AYellowMeteor()
{
	// 아브렐슈드 노랑 메테오: 폭발 반경이 넓어 타일 약 3개 정도를 커버함
	ExplosionRadius = 1700.0f;

	// 타일 3칸의 체력을 한 번에 깎을 수 있는 데미지
	MeteorDamage = 3;
}
