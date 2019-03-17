// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DispatchableActionBase.h"
#include "XD_ActionDispatcherBase.h"
#include "XD_DispatchableEntityInterface.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_ActionDispatcher_Log.h"

UXD_DispatchableActionBase::UXD_DispatchableActionBase()
{
#if WITH_EDITORONLY_DATA
	bIsPluginAction = false;
	bShowInExecuteActionNode = true;
#endif
}

UWorld* UXD_DispatchableActionBase::GetWorld() const
{
	return GetOwner() ? GetOwner()->GetWorld() : nullptr;
}

void UXD_DispatchableActionBase::ActiveAction()
{
	check(bIsActived == false && bIsFinished == false);

	ActionDispatcher_Display_Log("激活%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
	bIsActived = true;
	WhenActionActived();
}

void UXD_DispatchableActionBase::DeactiveAction()
{
	check(bIsActived == true && bIsFinished == false);

	bIsActived = false;
	SaveState();
	WhenActionDeactived();
	ActionDispatcher_Display_Log("反激活%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}

void UXD_DispatchableActionBase::ReactiveAction()
{
	check(bIsActived == false && bIsFinished == false);

	ActionDispatcher_Display_Log("再次激活%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
	bIsActived = true;
	WhenActionReactived();
}

void UXD_DispatchableActionBase::WhenActionReactived()
{
	WhenActionActived();
}

void UXD_DispatchableActionBase::FinishAction()
{
	check(GetOwner()->CurrentActions.Contains(this));
	check(bIsActived && bIsFinished == false);

	bIsFinished = true;
	UXD_ActionDispatcherBase* ActionDispatcher = GetOwner();
	ActionDispatcher->CurrentActions.Remove(this);
	WhenActionFinished();
	ActionDispatcher_Display_Log("结束%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}

void UXD_DispatchableActionBase::SaveState()
{
	WhenSaveState();
}

UXD_ActionDispatcherBase* UXD_DispatchableActionBase::GetOwner() const
{
	return CastChecked<UXD_ActionDispatcherBase>(GetOuter());
}

TArray<UXD_DispatchableActionBase::FPinNameData> UXD_DispatchableActionBase::GetAllFinishedEventName() const
{
	TArray<FPinNameData> Res;
#if WITH_EDITOR
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		UStructProperty* Struct = *It;
		if (Struct->Struct->IsChildOf(FDispatchableActionFinishedEvent::StaticStruct()))
		{
			FPinNameData Data;
			Data.PinName = *Struct->GetName();
			Data.PinDisplayName = Struct->GetDisplayNameText();
			Res.Add(Data);
		}
	}
#endif
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
			if (PreAction != this)
			{
				//调度器内的行为抢占式跳转移除前一个进行时节点
				if (PreAction->GetOwner() == GetOwner())
				{
					PreAction->DeactiveAction();
					GetOwner()->CurrentActions.Remove(PreAction);
				}
				else
				{
					//非同一调度器先将另一个调度器中断
					PreAction->GetOwner()->AbortDispatch();
				}
				IXD_DispatchableEntityInterface::SetCurrentDispatchableAction(Actor, this);
			}
		}
		else
		{
			IXD_DispatchableEntityInterface::SetCurrentDispatchableAction(Actor, this);
		}
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

void UXD_DispatchableActionBase::AbortDispatcher()
{
	GetOwner()->AbortDispatch();
}

bool UXD_DispatchableActionBase::CanReactiveDispatcher() const
{
	return GetOwner()->CanReactiveDispatcher();
}

bool UXD_DispatchableActionBase::InvokeReactiveDispatcher()
{
	return GetOwner()->InvokeReactiveDispatch();
}

void UXD_DispatchableActionBase::ReactiveDispatcher()
{
	GetOwner()->ReactiveDispatcher();
}
