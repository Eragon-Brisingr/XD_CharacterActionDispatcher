// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTT_ActionDispatcher.h"
#include <AIController.h>
#include "Interface/XD_DispatchableEntityInterface.h"
#include "Action/XD_DispatchableActionBase.h"

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
		//TODO 现在只要有一个Action被Abort了就会结束节点的Abort状态，之后要考虑所有情况
		auto OnActionAbort = UXD_DispatchableActionBase::FOnActionAborted::CreateWeakLambda(this, [=, P_OwnerComp = &OwnerComp]()
		{
			FinishLatentAbort(*P_OwnerComp);
		});

		TArray<UXD_DispatchableActionBase*>& Actions = IXD_DispatchableEntityInterface::GetCurrentDispatchableActions(Pawn);
		for (UXD_DispatchableActionBase* Action : Actions)
		{
			switch (Action->State)
			{
			case EDispatchableActionState::Active:
				Action->AbortDispatcher(OnActionAbort);
				return EBTNodeResult::InProgress;
			case EDispatchableActionState::Aborting:
				Action->OnActionAborted = OnActionAbort;
				return EBTNodeResult::InProgress;
			case EDispatchableActionState::Deactive:
				return EBTNodeResult::Aborted;
			}
		}
	}
	return EBTNodeResult::Aborted;
}

EBTNodeResult::Type UBTT_ActionDispatcher::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}
