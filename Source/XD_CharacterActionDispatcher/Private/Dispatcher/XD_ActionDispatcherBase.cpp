// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ActionDispatcherBase.h"
#include "XD_DispatchableActionBase.h"
#include "XD_ActionDispatcherManager.h"
#include "XD_ActionDispatcher_Log.h"
#include "XD_DebugFunctionLibrary.h"

bool UXD_ActionDispatcherBase::ReceiveCanExecuteDispatch_Implementation() const
{
	return true;
}

bool UXD_ActionDispatcherBase::CanExecuteDispatch() const
{
	return ReceiveCanExecuteDispatch();
}

void UXD_ActionDispatcherBase::StartDispatch()
{
	bIsActive = true;
	WhenDispatchStart();
}

void UXD_ActionDispatcherBase::ActiveAction(UXD_DispatchableActionBase* Action)
{
	check(Action && !CurrentActions.Contains(Action));

#if WITH_EDITOR
	check(bIsSubActionDispatcher == false);
#endif

	CurrentActions.Add(Action);
	Action->ActiveAction();
}

void UXD_ActionDispatcherBase::AbortDispatch()
{
	if (bIsActive == true)
	{
		bIsActive = false;
		for (UXD_DispatchableActionBase* Action : CurrentActions)
		{
			if (Action)
			{
				Action->DeactiveAction();
			}
		}
	}
}

bool UXD_ActionDispatcherBase::InvokeReactiveDispatch()
{
	if (CanExecuteDispatch())
	{
		ReactiveDispatcher();
		return true;
	}
	return false;
}

void UXD_ActionDispatcherBase::ReactiveDispatcher()
{
	if (bIsActive == false)
	{
		bIsActive = true;
		for (UXD_DispatchableActionBase* Action : CurrentActions)
		{
			if (Action)
			{
				Action->ReactiveAction();
			}
		}
	}
}

void UXD_ActionDispatcherBase::FinishDispatch(FGameplayTag Tag)
{
	ActionDispatcher_Warning_LOG("还未实现FinishDispatch");

	GetManager()->FinishDispatcher(this);
	OnDispatchFinished.Broadcast(Tag);
}

UXD_ActionDispatcherManager* UXD_ActionDispatcherBase::GetManager() const
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

UXD_ActionDispatcherBase* UXD_ActionDispatcherBase::GetMainActionDispatcher()
{
	if (UXD_ActionDispatcherBase* ActionDispatcher = GetTypedOuter<UXD_ActionDispatcherBase>())
	{
		return ActionDispatcher;
	}
	return this;
}

void UXD_ActionDispatcherBase::ActiveSubActionDispatcher(UXD_ActionDispatcherBase* SubActionDispatcher, FGuid NodeGuid)
{
#if WITH_EDITOR
	SubActionDispatcher->bIsSubActionDispatcher = true;
#endif
	ActivedSubActionDispatchers.Add(NodeGuid, SubActionDispatcher);
	ActionDispatcher_Display_Log("创建子行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(SubActionDispatcher));
	SubActionDispatcher->WhenDispatchStart();
}

bool UXD_ActionDispatcherBase::TryActiveSubActionDispatcher(FGuid NodeGuid)
{
	UXD_ActionDispatcherBase** P_ActionDispatcher = ActivedSubActionDispatchers.Find(NodeGuid);
	if (P_ActionDispatcher)
	{
		UXD_ActionDispatcherBase* ActionDispatcher = *P_ActionDispatcher;
		ActionDispatcher->WhenDispatchStart();
		return true;
	}
	else
	{
		return false;
	}
}
