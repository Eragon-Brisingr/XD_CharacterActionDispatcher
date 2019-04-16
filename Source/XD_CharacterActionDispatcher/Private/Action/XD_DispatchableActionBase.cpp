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
#if WITH_EDITOR
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
#endif
	return GetOwner() ? GetOwner()->GetWorld() : nullptr;
}

void UXD_DispatchableActionBase::ActiveAction()
{
	check(State != EDispatchableActionState::Active);

	ActionDispatcher_Display_VLog(GetOwner(), "激活%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
	State = EDispatchableActionState::Active;
	WhenActionActived();
}

void UXD_DispatchableActionBase::AbortAction()
{
	check(State != EDispatchableActionState::Aborting);

	ActionDispatcher_Display_VLog(GetOwner(), "中断%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
	State = EDispatchableActionState::Aborting;
	WhenActionAborted();
}

void UXD_DispatchableActionBase::WhenActionAborted()
{
	DeactiveAction();
}

void UXD_DispatchableActionBase::DeactiveAction()
{
	check(State != EDispatchableActionState::Deactive);

	bool isFromAbort = State == EDispatchableActionState::Aborting;

	State = EDispatchableActionState::Deactive;
	ActionDispatcher_Display_VLog(GetOwner(), "反激活%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));

	if (isFromAbort)
	{
		GetOwner()->WhenActionAborted();
		OnActionAborted.ExecuteIfBound();
		OnActionAborted.Unbind();
	}

	SaveState();
	WhenActionDeactived();
}

void UXD_DispatchableActionBase::ReactiveAction()
{
	check(State != EDispatchableActionState::Active);

	ActionDispatcher_Display_VLog(GetOwner(), "再次激活%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
	State = EDispatchableActionState::Active;
	WhenActionReactived();
}

void UXD_DispatchableActionBase::WhenActionReactived()
{
	WhenActionActived();
}

void UXD_DispatchableActionBase::FinishAction()
{
	check(GetOwner()->CurrentActions.Contains(this));
	check(State == EDispatchableActionState::Active);

	State = EDispatchableActionState::Finished;
	UXD_ActionDispatcherBase* ActionDispatcher = GetOwner();
	ActionDispatcher->CurrentActions.Remove(this);
	WhenActionFinished();
	ActionDispatcher_Display_VLog(GetOwner(), "结束%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
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
		if (Struct->Struct->IsChildOf(FOnDispatchableActionFinishedEvent::StaticStruct()))
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

void UXD_DispatchableActionBase::BindAllFinishedEvent(const TArray<FOnDispatchableActionFinishedEvent>& FinishedEvents)
{
	if (FinishedEvents.Num() == 0)
	{
		return;
	}

	int32 BindIdx = 0;
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		UStructProperty* Struct = *It;
		if (Struct->Struct->IsChildOf(FOnDispatchableActionFinishedEvent::StaticStruct()))
		{
			FOnDispatchableActionFinishedEvent* Value = Struct->ContainerPtrToValuePtr<FOnDispatchableActionFinishedEvent>(this);
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
	check((Actor->GetWorld()->AreActorsInitialized()));

	if (Actor && Actor->Implements<UXD_DispatchableEntityInterface>())
	{
		if (UXD_DispatchableActionBase* PreAction = IXD_DispatchableEntityInterface::GetCurrentDispatchableAction(Actor))
		{
			if (PreAction != this)
			{
				//调度器内的行为抢占式跳转移除前一个进行时节点
				if (PreAction->GetOwner() == GetOwner())
				{
					if (PreAction->State == EDispatchableActionState::Active)
					{
						PreAction->DeactiveAction();
						GetOwner()->CurrentActions.Remove(PreAction);
					}
				}
				else
				{
					if (PreAction->GetOwner()->State == EActionDispatcherState::Active)
					{
						//非同一调度器先将另一个调度器中断
						PreAction->GetOwner()->AbortDispatch();
					}
				}
				IXD_DispatchableEntityInterface::SetCurrentDispatchableAction(Actor, this);
			}
		}
		else
		{
			IXD_DispatchableEntityInterface::SetCurrentDispatchableAction(Actor, this);
		}
	}
	ActionDispatcher_Display_VLog(Actor, "%s执行行为%s", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}

void UXD_DispatchableActionBase::UnregisterEntity(AActor* Actor)
{
	if (Actor && Actor->Implements<UXD_DispatchableEntityInterface>())
	{
		check(IXD_DispatchableEntityInterface::GetCurrentDispatchableAction(Actor) == this);

		IXD_DispatchableEntityInterface::SetCurrentDispatchableAction(Actor, nullptr);
	}
	ActionDispatcher_Display_VLog(Actor, "%s停止执行行为%s", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}

void UXD_DispatchableActionBase::ExecuteEventAndFinishAction(const FOnDispatchableActionFinishedEvent& Event)
{
	FinishAction();
	Event.ExecuteIfBound();
}

bool UXD_DispatchableActionBase::IsActionValid() const
{
	return false;
}

void UXD_DispatchableActionBase::AbortDispatcher(const FOnDispatcherAborted& Event, bool DeactiveRequestAction)
{
	UXD_ActionDispatcherBase* Owner = GetOwner();
	if (Owner->State == EActionDispatcherState::Active)
	{
		Owner->AbortDispatch(Event, DeactiveRequestAction ? this : nullptr);
	}
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
