// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_IsInActionDispatcherState.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "在动作调度状态中"))
class XD_CHARACTERACTIONDISPATCHER_API UBTD_IsInActionDispatcherState : public UBTDecorator
{
	GENERATED_BODY()
public:
	UBTD_IsInActionDispatcherState();

	bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	FString GetStaticDescription() const override;
};
