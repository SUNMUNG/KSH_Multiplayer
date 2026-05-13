// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "BossBase.generated.h"

class UAbilitySystemComponent;
class UBaseAttributeSet;
class APlayerBase;

/**
 * ABossBase
 *
 * 보스 캐릭터의 C++ 기반 클래스.
 * - ACharacter 상속: 이동, 애니메이션, 콜리전 사용 가능
 * - IAbilitySystemInterface 구현: GAS 파이프라인 완전 통합
 * - UBaseAttributeSet 공유: 플레이어와 동일한 스탯 시스템 사용
 * - 서버 전용 타이머로 아브렐슈드 스타일 메테오 패턴을 주기적으로 실행
 */
UCLASS()
class KSH_MULTIPLAYER_API ABossBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABossBase();

	// IAbilitySystemInterface 필수 구현
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// ================================================================
	// GAS 컴포넌트 (플레이어와 동일한 구조)
	// ================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbilitySystemComponent* AbilitySystemComponent;

	// 플레이어와 동일한 UBaseAttributeSet 사용 — HP, MaxHP 공유
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UBaseAttributeSet* BaseAttributeSet;

	// ================================================================
	// 아브렐슈드 메테오 패턴 설정
	// 에디터 디테일 패널에서 조정 가능
	// ================================================================

	// 전투 시작 후 첫 패턴까지의 딜레이 (초)
	// 아브렐슈드 기준: 약 5초 (보스 등장 연출 이후)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Meteor Pattern",
		meta = (ClampMin = "0.0", ToolTip = "전투 시작 후 첫 번째 메테오 패턴까지의 딜레이(초)"))
	float InitialPatternDelay;

	// 패턴 반복 간격 (초)
	// 아브렐슈드 기준: 약 20초마다 메테오 패턴 반복
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Meteor Pattern",
		meta = (ClampMin = "1.0", ToolTip = "메테오 패턴의 반복 주기(초)"))
	float MeteorPatternInterval;

	// 한 번의 패턴에서 동시에 타겟팅할 플레이어 수
	// 1 = 일반 난이도, 2 = 하드 모드 (아브렐슈드 하드는 동시 2명 타겟)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Meteor Pattern",
		meta = (ClampMin = "1", ClampMax = "4", ToolTip = "동시 메테오 타겟 수 (1~4)"))
	int32 MeteorTargetCount;

	// ================================================================
	// 패턴 제어 함수 (Authority Only)
	// ================================================================

public:
	// ================================================================
	// 밀치기 공격 (아브렐슈드 푸시 패턴)
	// ================================================================

	/**
	 * 보스 정면 박스 범위 내 플레이어를 감지하여 아레나 가장자리로 밀쳐냄.
	 * 피격 플레이어에게 State.Debuff.MeteorStack +1 적용.
	 * 3회 피격 시 APlayerBase에서 자동으로 개인 메테오 발동.
	 *
	 * BTTask_PushAttack 및 Blueprint 애니메이션 노티파이에서 호출.
	 */
	UFUNCTION(BlueprintCallable, Category = "Boss|Push Attack")
	void PerformPushAttack();

	// ──────────────────────────────────────────────
	// 밀치기 공격 파라미터 (에디터 조정 가능)
	// ──────────────────────────────────────────────

	// 감지 박스 깊이 (보스 정면 방향, cm)
	UPROPERTY(EditAnywhere, Category = "Boss|Push Attack", meta = (ClampMin = "100.0"))
	float PushDetectionDepth = 600.f;

	// 감지 박스 너비 (보스 좌우 방향, cm)
	UPROPERTY(EditAnywhere, Category = "Boss|Push Attack", meta = (ClampMin = "100.0"))
	float PushDetectionWidth = 400.f;

	// 감지 박스 높이 (cm)
	UPROPERTY(EditAnywhere, Category = "Boss|Push Attack", meta = (ClampMin = "50.0"))
	float PushDetectionHeight = 300.f;

	// 밀치기 수평 강도 (cm/s)
	UPROPERTY(EditAnywhere, Category = "Boss|Push Attack")
	float PushStrength = 1800.f;

	// 밀치기 수직 강도 (cm/s)
	UPROPERTY(EditAnywhere, Category = "Boss|Push Attack")
	float PushUpwardStrength = 500.f;

	// 패턴 타이머 시작 — BeginPlay에서 자동 호출, 블루프린트에서도 수동 호출 가능
	UFUNCTION(BlueprintCallable, Category = "Boss|Meteor Pattern")
	void StartMeteorPattern();

	// 패턴 타이머 정지 (보스 사망, 페이즈 전환 등)
	UFUNCTION(BlueprintCallable, Category = "Boss|Meteor Pattern")
	void StopMeteorPattern();

	// 현재 패턴이 활성 상태인지 반환
	UFUNCTION(BlueprintPure, Category = "Boss|Meteor Pattern")
	bool IsMeteorPatternActive() const { return bIsMeteorPatternActive; }

	// ================================================================
	// 블루프린트 이벤트 (Blueprint에서 구현)
	// ================================================================

	/**
	 * [핵심 이벤트] 서버가 메테오 타겟을 선정했을 때 호출됨.
	 *
	 * 블루프린트에서 이 이벤트를 오버라이드하여 다음 시퀀스를 구현:
	 *   1. TargetPlayer 위치에 경고 마커(Warning Decal/Widget) 표시
	 *   2. Delay 노드로 3~4초 대기 (경고 연출)
	 *   3. Spawn Actor (AMeteor 또는 AYellowMeteor) at TargetPlayer 위 500유닛
	 *
	 * [멀티플레이어 주의]
	 * 이 이벤트는 서버에서만 실행됩니다.
	 * 경고 마커를 모든 클라이언트에 보이게 하려면 bReplicates=true인 경고 Actor를
	 * Spawn하거나, Multicast RPC를 별도로 추가하세요.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Meteor Pattern")
	void OnMeteorTargetSelected(APlayerBase* TargetPlayer);

	/**
	 * 메테오 패턴이 시작될 때 호출 (보스 캐스팅 모션, 사운드 재생 등)
	 * 이 이벤트도 서버에서만 실행됩니다. 전신 연출이 필요하면 Multicast RPC 사용.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Meteor Pattern")
	void OnMeteorPatternStarted();

	/**
	 * 메테오 패턴이 정지될 때 호출 (보스 사망, 페이즈 전환 등)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Meteor Pattern")
	void OnMeteorPatternEnded();

private:
	// 타이머 핸들 — ClearTimer에 사용
	FTimerHandle MeteorPatternTimerHandle;

	// 패턴 중복 시작 방지 플래그
	bool bIsMeteorPatternActive;

	// [타이머 콜백] 살아있는 플레이어를 선정하여 OnMeteorTargetSelected 호출
	UFUNCTION()
	void SelectAndTriggerMeteorTargets();

	// 현재 월드에서 살아있는(HP > 0) APlayerBase 액터 목록 반환
	TArray<APlayerBase*> GetAlivePlayers() const;

	// 단일 플레이어에게 MeteorStack +1 GE를 적용하는 헬퍼 (PerformPushAttack 내부용)
	void ApplyMeteorStackToPlayer(APlayerBase* HitPlayer);
};
