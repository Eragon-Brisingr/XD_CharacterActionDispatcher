// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ActionDispatcherBase.h"
#include "XD_DispatchableActionBase.h"
#include "XD_ActionDispatcherManager.h"
#include "XD_ActionDispatcher_Log.h"
#include "XD_DebugFunctionLibrary.h"

UXD_ActionDispatcherBase::UXD_ActionDispatcherBase()
	:bCheckAllSoftReferenceValidate(true)
{

}

bool UXD_ActionDispatcherBase::ReceiveCanExecuteDispatch_Implementation() const
{
	return true;
}

bool UXD_ActionDispatcherBase::CanExecuteDispatch() const
{
	if (bCheckAllSoftReferenceValidate)
	{
		return ReceiveCanExecuteDispatch() && IsAllSoftReferenceValid();
	}
	return ReceiveCanExecuteDispatch();
}

void UXD_ActionDispatcherBase::StartDispatch()
{
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		bIsActive = true;
		WhenDispatchStart();
	}
	else
	{
		ActionDispatcher_Error_Log("只能在服务端执行行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(this));
	}
}

void UXD_ActionDispatcherBase::ActiveAction(UXD_DispatchableActionBase* Action)
{
	check(Action && !CurrentActions.Contains(Action));
	check(IsSubActionDispatcher() == false);

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

bool UXD_ActionDispatcherBase::IsAllSoftReferenceValid() const
{
	for (TFieldIterator<USoftObjectProperty> It(GetClass()); It; ++It)
	{
		USoftObjectProperty* SoftObjectProperty = *It;
		FSoftObjectPtr SoftObjectPtr = SoftObjectProperty->GetPropertyValue(SoftObjectProperty->ContainerPtrToValuePtr<uint8>(this));
		if (SoftObjectPtr.Get() == nullptr)
		{
			return false;
		}
	}
	return true;
}

void UXD_ActionDispatcherBase::FinishDispatch(FGameplayTag Tag)
{
	if (IsSubActionDispatcher())
	{
		ActionDispatcher_Display_Log("结束子行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(this));
	}
	else
	{
		ActionDispatcher_Display_Log("结束行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(this));
	}

	if (UXD_ActionDispatcherManager* Manager = GetManager())
	{
		Manager->FinishDispatcher(this);
	}
	OnDispatchFinished.Broadcast(Tag);
	if (WhenDispatchFinished.IsBound())
	{
		WhenDispatchFinished.Execute(Tag.GetTagName());
	}
}

#if WITH_EDITOR
TArray<FName> UXD_ActionDispatcherBase::GetAllFinishTags() const
{
	return FinishTags;
}
#endif

UXD_ActionDispatcherManager* UXD_ActionDispatcherBase::GetManager() const
{
	return Cast<UXD_ActionDispatcherManager>(GetOuter());
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

bool UXD_ActionDispatcherBase::IsSubActionDispatcher() const
{
	return GetOuter()->IsA<UXD_ActionDispatcherBase>();
}

UXD_ActionDispatcherBase* UXD_ActionDispatcherBase::GetMainActionDispatcher()
{
	UXD_ActionDispatcherBase* ActionDispatcher = this;
	for (UObject* NextOuter = GetOuter(); NextOuter != NULL; NextOuter = NextOuter->GetOuter())
	{
		if (UXD_ActionDispatcherBase* OuterActionDispatcher = Cast<UXD_ActionDispatcherBase>(NextOuter))
		{
			ActionDispatcher = OuterActionDispatcher;
		}
		else
		{
			break;
		}
	}
	return ActionDispatcher;
}

void UXD_ActionDispatcherBase::ActiveSubActionDispatcher(UXD_ActionDispatcherBase* SubActionDispatcher, FGuid NodeGuid)
{
	ActivedSubActionDispatchers.Add(NodeGuid, SubActionDispatcher);
	ActionDispatcher_Display_Log("启动子行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(SubActionDispatcher));
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
