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
	check(Action && !ActivedActions.Contains(Action) && Action->Owner == nullptr);

	Action->Owner = this;
	ActivedActions.Add(Action);
	Action->BindAllFinishedEvent(FinishedEvents);
	Action->ActiveAction();
}

void UXD_ActionDispatcherBase::AbortDispatch()
{
	for (UXD_DispatchableActionBase* Action : ActivedActions)
	{
		if (Action)
		{
			Action->DeactiveAction();
		}
	}
}

void UXD_ActionDispatcherBase::ReactiveDispatch()
{
	for (UXD_DispatchableActionBase* Action : ActivedActions)
	{
		if (Action)
		{
			Action->ReactiveAction();
		}
	}
}

void UXD_ActionDispatcherBase::FinishAction(UXD_DispatchableActionBase* Action)
{
	check(ActivedActions.Contains(Action));

	ActivedActions.Remove(Action);
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
