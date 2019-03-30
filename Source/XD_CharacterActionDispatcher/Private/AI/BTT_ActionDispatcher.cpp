// Fill out your copyright notice in the Description page of Project Settings.

#include "BTT_ActionDispatcher.h"
#include "AIController.h"
#include "XD_DispatchableEntityInterface.h"

FString UBTT_ActionDispatcher::GetStaticDescription() const
{
	return TEXT("行为调度");
}

EBTNodeResult::Type UBTT_ActionDispatcher::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIOwner = OwnerComp.GetAIOwner();
	APawn* Pawn = AIOwner->GetPawn();
	if (Pawn->Implements<UXD_DispatchableEntityInterface>())
	{
		if (UXD_DispatchableActionBase* Action = IXD_DispatchableEntityInterface::GetCurrentDispatchableAction(Pawn))
		{
			Action->AbortDispatcher({});
		}
	}
	return EBTNodeResult::Aborted;
}

EBTNodeResult::Type UBTT_ActionDispatcher::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}
