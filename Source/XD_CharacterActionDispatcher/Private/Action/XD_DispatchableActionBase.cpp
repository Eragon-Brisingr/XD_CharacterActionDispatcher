﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/XD_DispatchableActionBase.h"
#include <UObject/Package.h>

#include "XD_DebugFunctionLibrary.h"
#include "Dispatcher/XD_ActionDispatcherBase.h"
#include "Interface/XD_DispatchableEntityInterface.h"
#include "Utils/XD_ActionDispatcher_Log.h"

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
#if WITH_EDITOR
	// 编辑器下修复SoftObject运行时的指向
	const int32 PIEInstanceID = GetWorld()->GetOutermost()->PIEInstanceID;
	for (TFieldIterator<FSoftObjectProperty> PropertyIt(GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		TSoftObjectPtr<UObject>* SoftObjectPtr = PropertyIt->ContainerPtrToValuePtr<TSoftObjectPtr<UObject>>(this);
		FSoftObjectPath& SoftObjectPath = const_cast<FSoftObjectPath&>(SoftObjectPtr->ToSoftObjectPath());
		if (SoftObjectPath.FixupForPIE(PIEInstanceID))
		{
			*SoftObjectPtr = SoftObjectPath.ResolveObject();
		}
	}
#endif

	check(State != EDispatchableActionState::Active);
	ActionDispatcher_Display_VLog(GetOwner(), "激活%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
	State = EDispatchableActionState::Active;
	for (AActor* Entity : GetAllRegistableEntities())
	{
		RegisterEntity(Entity);
	}
	WhenActionActived();
	OnActionActived.ExecuteIfBound();

	//可能在行为激活的过程中某一个行为中断了调度器，那么后续的行为就不激活了
	if (GetOwner()->State != EActionDispatcherState::Active)
	{
		DeactiveAction();
	}
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

	for (AActor* Entity : GetAllRegistableEntities())
	{
		UnregisterEntity(Entity);
	}

	WhenActionDeactived();
	OnActionDeactived.ExecuteIfBound();
}

void UXD_DispatchableActionBase::ReactiveAction()
{
	check(State != EDispatchableActionState::Active);

	ActionDispatcher_Display_VLog(GetOwner(), "再次激活%s中的行为%s", *UXD_DebugFunctionLibrary::GetDebugName(GetOwner()), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
	State = EDispatchableActionState::Active;
	for (AActor* Entity : GetAllRegistableEntities())
	{
		RegisterEntity(Entity);
	}
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

	for (AActor* Entity : GetAllRegistableEntities())
	{
		UnregisterEntity(Entity);
	}

	//结束前也调用下反激活
	WhenActionDeactived();
	OnActionDeactived.ExecuteIfBound();
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

void UXD_DispatchableActionBase::BindFinishedEvent(FOnDispatchableActionFinishedEvent& FinishedEvent, const FDispatchableActionEventDelegate& InEvent)
{
	FinishedEvent.Event = InEvent;
}

void UXD_DispatchableActionBase::BindNormalEvent(FDispatchableActionNormalEvent& NormalEvent, const FDispatchableActionEventDelegate& InEvent)
{
	NormalEvent.Event = InEvent;
}

TSet<AActor*> UXD_DispatchableActionBase::GetAllRegistableEntities() const
{
	unimplemented();
	return {};
}

void UXD_DispatchableActionBase::RegisterEntity(AActor* Actor)
{
	check((Actor->GetWorld()->AreActorsInitialized()));

	if (Actor && Actor->Implements<UXD_DispatchableEntityInterface>())
	{
		TArray<UXD_DispatchableActionBase*>& Actions = IXD_DispatchableEntityInterface::GetCurrentDispatchableActions(Actor);
		for (UXD_DispatchableActionBase* PreAction : TArray<UXD_DispatchableActionBase*>(Actions))
		{
			check(PreAction != this);

			//调度器内的行为抢占式跳转移除前一个进行时节点
			UXD_ActionDispatcherBase* SelfDispatcher = GetOwner();
			UXD_ActionDispatcherBase* PreDispatcher = PreAction->GetOwner();

			const bool IsBothCompatible = SelfDispatcher->ActionIsBothCompatible(this, PreAction);
			if (!IsBothCompatible)
			{
				if (PreDispatcher == SelfDispatcher)
				{
					if (PreAction->State == EDispatchableActionState::Active)
					{
						PreAction->DeactiveAction();
						SelfDispatcher->CurrentActions.Remove(PreAction);
					}
				}
				else
				{
					// 若不为被领导的调度器
					if (!SelfDispatcher->ActionDispatcherLeader)
					{
						if (PreDispatcher->State == EActionDispatcherState::Active)
						{
						 	//非同一调度器先将另一个调度器中断
						 	PreDispatcher->AbortDispatch(PreAction);
						}
					}
				}
			}
		}
		Actions.Add(this);
	}
	ActionDispatcher_Display_VLog(Actor, "%s执行行为%s", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(GetClass()));
}

void UXD_DispatchableActionBase::UnregisterEntity(AActor* Actor)
{
	if (Actor && Actor->Implements<UXD_DispatchableEntityInterface>())
	{
		TArray<UXD_DispatchableActionBase*>& Actions = IXD_DispatchableEntityInterface::GetCurrentDispatchableActions(Actor);
		int32 RemoveNum = Actions.Remove(this);
		check(RemoveNum != 0);
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
	for (AActor* Entity : GetAllRegistableEntities())
	{
		if (!Entity)
		{
			return false;
		}
	}
	return true;
}

void UXD_DispatchableActionBase::AbortDispatcher(const FOnDispatcherAborted& Event, bool DeactiveRequestAction)
{
	UXD_ActionDispatcherBase* Owner = GetOwner();
	if (Owner->State == EActionDispatcherState::Active)
	{
		Owner->AbortDispatch(Event, DeactiveRequestAction ? this : nullptr);
	}
}

void UXD_DispatchableActionBase::AbortDispatcher(const FOnActionAborted& Event, bool DeactiveRequestAction /*= false*/)
{
	OnActionAborted = Event;
	UXD_ActionDispatcherBase* Owner = GetOwner();
	if (Owner->State == EActionDispatcherState::Active)
	{
		Owner->AbortDispatch(DeactiveRequestAction ? this : nullptr);
	}
}

void UXD_DispatchableActionBase::AbortDispatcher(bool DeactiveRequestAction /*= false*/)
{
	UXD_ActionDispatcherBase* Owner = GetOwner();
	if (Owner->State == EActionDispatcherState::Active)
	{
		Owner->AbortDispatch(DeactiveRequestAction ? this : nullptr);
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
