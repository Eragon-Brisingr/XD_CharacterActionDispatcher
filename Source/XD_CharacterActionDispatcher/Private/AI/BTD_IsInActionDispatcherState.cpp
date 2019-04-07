// Fill out your copyright notice in the Description page of Project Settings.

#include "BTD_IsInActionDispatcherState.h"
#include "AIController.h"
#include "XD_DispatchableEntityInterface.h"
#include "XD_DispatchableActionBase.h"
#include "XD_ActionDispatcherBase.h"

UBTD_IsInActionDispatcherState::UBTD_IsInActionDispatcherState()
{
	FlowAbortMode = EBTFlowAbortMode::Self;
}

bool UBTD_IsInActionDispatcherState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIOwner = OwnerComp.GetAIOwner();
	APawn* Pawn = AIOwner->GetPawn();
	if (Pawn->Implements<UXD_DispatchableEntityInterface>())
	{
		if (UXD_DispatchableActionBase* Action = IXD_DispatchableEntityInterface::GetCurrentDispatchableAction(Pawn))
		{
			if (Action->GetOwner()->State == EActionDispatcherState::Active)
			{
				return true;
			}
		}
	}
	return false;
}

FString UBTD_IsInActionDispatcherState::GetStaticDescription() const
{
	return TEXT("在动作调度状态中");
}
