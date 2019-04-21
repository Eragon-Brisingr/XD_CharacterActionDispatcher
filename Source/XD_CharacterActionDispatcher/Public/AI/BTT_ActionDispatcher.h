// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_ActionDispatcher.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "行为调度"))
class XD_CHARACTERACTIONDISPATCHER_API UBTT_ActionDispatcher : public UBTTaskNode
{
	GENERATED_BODY()
public:
	FString GetStaticDescription() const override;
protected:
	EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
private:
	EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
