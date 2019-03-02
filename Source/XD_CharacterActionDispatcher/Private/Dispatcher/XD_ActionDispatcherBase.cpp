// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ActionDispatcherBase.h"
#include "XD_DispatchableActionBase.h"
#include "XD_ActionDispatcherManager.h"

bool UXD_ActionDispatcherBase::ReceiveCanExecuteDispatch_Implementation()
{
	return true;
}

void UXD_ActionDispatcherBase::ActiveAction(UXD_DispatchableActionBase* Action, const TArray<FDispatchableActionFinishedEvent>& FinishedEvents)
{
	check(Action && !CurrentActions.Contains(Action) && Action->Owner == nullptr);

	Action->Owner = this;
	CurrentActions.Add(Action);
	Action->BindAllFinishedEvent(FinishedEvents);
	Action->ActiveAction();
}

void UXD_ActionDispatcherBase::AbortDispatch()
{
	for (UXD_DispatchableActionBase* Action : CurrentActions)
	{
		if (Action)
		{
			Action->DeactiveAction();
		}
	}
}

void UXD_ActionDispatcherBase::ReactiveDispatch()
{
	for (UXD_DispatchableActionBase* Action : CurrentActions)
	{
		if (Action)
		{
			Action->ReactiveAction();
		}
	}
}

void UXD_ActionDispatcherBase::FinishAction(UXD_DispatchableActionBase* Action)
{
	check(CurrentActions.Contains(Action));

	CurrentActions.Remove(Action);
}

bool UXD_ActionDispatcherBase::EnterTogetherFlowControl(FGuid NodeGuid, int32 Index, int32 TogetherCount)
{
	FTogetherFlowControl& TogetherFlowControl = ActivedTogetherControl.FindOrAdd(NodeGuid);
	TogetherFlowControl.CheckList.SetNum(TogetherCount);
	TogetherFlowControl.CheckList[Index] = true;
	bool IsTogether = !TogetherFlowControl.CheckList.Contains(false);
	if (IsTogether)
	{
		ActivedTogetherControl.Remove(NodeGuid);
		return true;
	}
	else
	{
		return false;
	}
}

UXD_ActionDispatcherManager* UXD_ActionDispatcherBase::GetOwner() const
{
	return CastChecked<UXD_ActionDispatcherManager>(GetOuter());
}

UWorld* UXD_ActionDispatcherBase::GetWorld() const
{
#if WITH_EDITOR
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
#endif
	return GetOuter()->GetWorld();
}
