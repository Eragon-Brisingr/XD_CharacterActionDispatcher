// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DispatchableActionBase.h"
#include "XD_ActionDispatcherBase.h"
#include "XD_DispatchableEntityInterface.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_ActionDispatcher_Log.h"

UWorld* UXD_DispatchableActionBase::GetWorld() const
{
	return Owner ? Owner->GetWorld() : nullptr;
}

void UXD_DispatchableActionBase::ActiveAction()
{
	ActionDispatcher_Display_Log("激活行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
	WhenActionActived();
}

void UXD_DispatchableActionBase::DeactiveAction()
{
	WhenActionDeactived();
	ActionDispatcher_Display_Log("反激活行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}

void UXD_DispatchableActionBase::FinishAction()
{
	Owner->FinishAction(this);
	WhenActionFinished();
	ActionDispatcher_Display_Log("结束行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}

TArray<FName> UXD_DispatchableActionBase::GetAllFinishedEventName() const
{
	TArray<FName> Res;
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		UStructProperty* Struct = *It;
		if (Struct->Struct->IsChildOf(FDispatchableActionFinishedEvent::StaticStruct()))
		{
			Res.Add(*Struct->GetDisplayNameText().ToString());
		}
	}
	return Res;
}

void UXD_DispatchableActionBase::BindAllFinishedEvent(const TArray<FDispatchableActionFinishedEvent>& FinishedEvents)
{
	if (FinishedEvents.Num() == 0)
	{
		return;
	}

	int32 BindIdx = 0;
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		UStructProperty* Struct = *It;
		if (Struct->Struct->IsChildOf(FDispatchableActionFinishedEvent::StaticStruct()))
		{
			FDispatchableActionFinishedEvent* Value = Struct->ContainerPtrToValuePtr<FDispatchableActionFinishedEvent>(this);
			*Value = FinishedEvents[BindIdx++];

			if (FinishedEvents.Num() >= BindIdx)
			{
				break;
			}
		}
	}
}

void UXD_DispatchableActionBase::RegisterEntity(AActor* Actor)
{
	if (Actor && Actor->Implements<UXD_DispatchableEntityInterface>())
	{
		if (UXD_DispatchableActionBase* PreAction = IXD_DispatchableEntityInterface::GetCurrentDispatchableAction(Actor))
		{
			//调度器内的行为抢占式跳转移除前一个进行时节点
			if (PreAction->Owner == Owner)
			{
				PreAction->DeactiveAction();
				Owner->CurrentActions.Remove(PreAction);
			}
			else
			{
				//非同一调度器先将另一个调度器中断
				PreAction->Owner->AbortDispatch();
			}
		}
		IXD_DispatchableEntityInterface::SetCurrentDispatchableAction(Actor, this);
	}
	ActionDispatcher_Display_Log("--%s执行行为%s", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}

void UXD_DispatchableActionBase::UnregisterEntity(AActor* Actor)
{
	if (Actor && Actor->Implements<UXD_DispatchableEntityInterface>())
	{
		check(IXD_DispatchableEntityInterface::GetCurrentDispatchableAction(Actor) == this);

		IXD_DispatchableEntityInterface::SetCurrentDispatchableAction(Actor, nullptr);
	}
	ActionDispatcher_Display_Log("--%s停止执行行为%s", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}
