// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTD_IsInActionDispatcherState.h"
#include <AIController.h>
#include "Interface/XD_DispatchableEntityInterface.h"
#include "Action/XD_DispatchableActionBase.h"
#include "Dispatcher/XD_ActionDispatcherBase.h"

UBTD_IsInActionDispatcherState::UBTD_IsInActionDispatcherState()
{
	FlowAbortMode = EBTFlowAbortMode::Self;
	bNotifyTick = true;
}

bool UBTD_IsInActionDispatcherState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIOwner = OwnerComp.GetAIOwner();
	APawn* Pawn = AIOwner->GetPawn();
	if (Pawn->Implements<UXD_DispatchableEntityInterface>())
	{
		UXD_ActionDispatcherBase* MainDispatcher = IXD_DispatchableEntityInterface::GetCurrentMainDispatcher(Pawn);
		return MainDispatcher && MainDispatcher->State == EActionDispatcherState::Active;
	}
	return false;
}

void UBTD_IsInActionDispatcherState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const bool bIsOnActiveBranch = OwnerComp.IsExecutingBranch(GetMyNode(), GetChildIndex());
	if (bIsOnActiveBranch && !CalculateRawConditionValue(OwnerComp, NodeMemory))
	{
		OwnerComp.RequestExecution(this);
	}
}

FString UBTD_IsInActionDispatcherState::GetStaticDescription() const
{
	return TEXT("在动作调度状态中");
}
