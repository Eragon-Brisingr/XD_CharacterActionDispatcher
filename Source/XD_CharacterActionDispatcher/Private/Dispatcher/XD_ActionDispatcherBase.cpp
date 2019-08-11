// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ActionDispatcherBase.h"
#if WITH_EDITOR
#include "Engine/BlueprintGeneratedClass.h"
#include "ActionDispatcherBlueprint.h"
#endif

#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "XD_DispatchableActionBase.h"
#include "XD_ActionDispatcherManager.h"
#include "XD_ActionDispatcher_Log.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_DispatchableEntityInterface.h"
#include "XD_SaveGameSystemBase.h"

UXD_ActionDispatcherBase::UXD_ActionDispatcherBase()
	:bIsMainDispatcher(true)
{

}

bool UXD_ActionDispatcherBase::CanStartDispatcher() const
{
	return IsDispatcherValid() && ReceiveCanStartDispatcher();
}

bool UXD_ActionDispatcherBase::ReceiveCanStartDispatcher_Implementation() const
{
	return true;
}

bool UXD_ActionDispatcherBase::IsDispatcherValid() const
{
	check(IsSubActionDispatcher() == false);

	if (DispatcherLeader.IsNull() || DispatcherLeader.IsValid())
	{
		return ReceiveIsDispatcherValid() && IsAllSoftReferenceValid();
	}
	return false;
}

bool UXD_ActionDispatcherBase::ReceiveIsDispatcherValid_Implementation() const
{
	return true;
}

void UXD_ActionDispatcherBase::StartDispatch()
{
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		check(State == EActionDispatcherState::Deactive);

		State = EActionDispatcherState::Active;
		ActiveDispatcher();
		ActionDispatcher_Display_Log("开始行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(this));
		WhenDispatchStart();
	}
	else
	{
		ActionDispatcher_Error_Log("只能在服务端执行行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(this));
	}
}

void UXD_ActionDispatcherBase::StartDispatchWithEvent(const FOnDispatchDeactiveNative& OnDispatchDeactive)
{
	OnDispatchDeactiveNative = OnDispatchDeactive;
	StartDispatch();
}

void UXD_ActionDispatcherBase::InitLeader(AActor* InDispatcherLeader)
{
	check(DispatcherLeader == nullptr);

	if (InDispatcherLeader)
	{
		APawn* Pawn = Cast<APawn>(InDispatcherLeader);
		if (Pawn && Pawn->IsPlayerControlled())
		{
			DispatcherLeader = InDispatcherLeader;
		}
		else
		{
			DispatcherLeader = InDispatcherLeader->GetLevel();
		}
		ActionDispatcher_Display_Log("调度器%s领导者设置为%s", *UXD_DebugFunctionLibrary::GetDebugName(this), *UXD_DebugFunctionLibrary::GetDebugName(DispatcherLeader.Get()));
	}
	else
	{
		DispatcherLeader = nullptr;
	}
}

void UXD_ActionDispatcherBase::InvokeActiveAction(UXD_DispatchableActionBase* Action)
{
	check(Action && !CurrentActions.Contains(Action));
	check(IsSubActionDispatcher() == false);

	//TODO 这边用Action是否有效来判断是否要中断可能有问题，再考虑下
	CurrentActions.Add(Action);
	if (State == EActionDispatcherState::Active)
	{
		if (Action->IsActionValid())
		{
			Action->ActiveAction();
		}
		else
		{
			AbortDispatch();
		}
	}
}

void UXD_ActionDispatcherBase::Tick(float DeltaTime)
{
	for (UXD_DispatchableActionBase* Action : CurrentActions)
	{
		if (Action->bTickable)
		{
			Action->WhenTick(DeltaTime);
		}
	}
}

bool UXD_ActionDispatcherBase::IsTickable() const
{
	return State != EActionDispatcherState::Deactive;
}

void UXD_ActionDispatcherBase::ExecuteAbortedDelegate()
{
	OnDispatcherAborted.ExecuteIfBound();
	OnDispatcherAbortedNative.ExecuteIfBound();
}

void UXD_ActionDispatcherBase::AbortDispatch(const FOnDispatcherAborted& Event, UXD_DispatchableActionBase* DeactiveRequestAction)
{
	OnDispatcherAborted = Event;
	AbortDispatch(DeactiveRequestAction);
}

void UXD_ActionDispatcherBase::AbortDispatch(UXD_DispatchableActionBase* DeactiveRequestAction /*= nullptr*/)
{
	check(State == EActionDispatcherState::Active);

	State = EActionDispatcherState::Aborting;

	for (UXD_DispatchableActionBase* Action : CurrentActions)
	{
		if (Action->IsActionValid())
		{
			if (DeactiveRequestAction == Action)
			{
				Action->DeactiveAction();
			}
			else
			{
				Action->AbortAction();
			}
		}
		else
		{
			Action->State = EDispatchableActionState::Deactive;
		}
	}
	WhenActionAborted();
	ActionDispatcher_Display_Log("中断行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(this));
}

void UXD_ActionDispatcherBase::AbortDispatch(const FOnDispatcherAbortedNative& Event, UXD_DispatchableActionBase* DeactiveRequestAction /*= nullptr*/)
{
	OnDispatcherAbortedNative = Event;
	AbortDispatch(DeactiveRequestAction);
}

void UXD_ActionDispatcherBase::BP_AbortDispatch(const FOnDispatcherAborted& Event)
{
	if (State == EActionDispatcherState::Active)
	{
		AbortDispatch(Event);
	}
}

void UXD_ActionDispatcherBase::AssignOnDispatcherAbort(const FOnDispatcherAborted& Event)
{
	OnDispatcherAborted = Event;
}

void UXD_ActionDispatcherBase::WhenActionAborted()
{
	if (State != EActionDispatcherState::Deactive)
	{
		if (CurrentActions.ContainsByPredicate([](UXD_DispatchableActionBase* Action) {return Action->State != EDispatchableActionState::Deactive; }) == false)
		{
			ActionDispatcher_Display_Log("中断%s调度器", *UXD_DebugFunctionLibrary::GetDebugName(this));
			DeactiveDispatcher(false);
			if (UXD_ActionDispatcherManager * Manager = GetManager())
			{
				Manager->WhenDispatcherDeactived(this);
			}
			ExecuteAbortedDelegate();
		}
	}
}

void UXD_ActionDispatcherBase::DeactiveDispatcher(bool IsFinsihedCompleted)
{
	check(State != EActionDispatcherState::Deactive);
	State = EActionDispatcherState::Deactive;

	if (bIsMainDispatcher)
	{
		for (USoftObjectProperty* SoftObjectProperty : GetSoftObjectPropertys())
		{
			FSoftObjectPtr SoftObjectPtr = SoftObjectProperty->GetPropertyValue(SoftObjectProperty->ContainerPtrToValuePtr<uint8>(this));
			UObject* Obj = SoftObjectPtr.Get();
			if (Obj && Obj->Implements<UXD_DispatchableEntityInterface>())
			{
				if (IXD_DispatchableEntityInterface::GetCurrentMainDispatcher(Obj) == this)
				{
					IXD_DispatchableEntityInterface::SetCurrentMainDispatcher(Obj, nullptr);
				}
			}
		}
	}

	if (UObject* Leader = DispatcherLeader.Get())
	{
		if (APawn* Pawn = Cast<APawn>(Leader))
		{
			Pawn->OnEndPlay.RemoveDynamic(this, &UXD_ActionDispatcherBase::WhenPlayerLeaderDestroyed);
		}
		else
		{
			UXD_SaveGameSystemBase::Get(this)->OnPreLevelUnload.RemoveAll(this);
		}
	}
	WhenDeactived(IsFinsihedCompleted);
	OnDispatchDeactiveNative.ExecuteIfBound(IsFinsihedCompleted);
}

void UXD_ActionDispatcherBase::SaveDispatchState()
{
	for (UXD_DispatchableActionBase* Action : CurrentActions)
	{
		Action->SaveState();
	}
}

bool UXD_ActionDispatcherBase::InvokeReactiveDispatch()
{
	if (CanStartDispatcher())
	{
		ReactiveDispatcher();
		return true;
	}
	return false;
}

bool UXD_ActionDispatcherBase::CanReactiveDispatcher() const
{
	if (CanStartDispatcher())
	{
		for (UXD_DispatchableActionBase* Action : CurrentActions)
		{
			if (Action->IsActionValid() == false)
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

const TArray<USoftObjectProperty*>& UXD_ActionDispatcherBase::GetSoftObjectPropertys() const
{
	return GetClass()->GetDefaultObject<UXD_ActionDispatcherBase>()->SoftObjectPropertys;
}

void UXD_ActionDispatcherBase::PostCDOContruct()
{
	Super::PostCDOContruct();
	for (TFieldIterator<USoftObjectProperty> It(GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		SoftObjectPropertys.Add(*It);
	}
}

void UXD_ActionDispatcherBase::WhenPlayerLeaderDestroyed(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (State != EActionDispatcherState::Deactive)
	{
		ActionDispatcher_Display_Log("因玩家%s离开导至%s终止", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(this));
		AbortDispatch();
	}
}

void UXD_ActionDispatcherBase::WhenLevelLeaderDestroyed(ULevel* Level)
{
	if (State != EActionDispatcherState::Deactive)
	{
		ULevel* CurLevel = Cast<ULevel>(DispatcherLeader.Get());
		if (CurLevel == Level)
		{
			ActionDispatcher_Display_Log("因关卡%s卸载导至%s终止", *UXD_DebugFunctionLibrary::GetDebugName(Level), *UXD_DebugFunctionLibrary::GetDebugName(this));
			AbortDispatch();
			UXD_SaveGameSystemBase::Get(this)->OnPreLevelUnload.RemoveAll(this);
		}
	}
}

void UXD_ActionDispatcherBase::ReactiveDispatcher()
{
	check(State == EActionDispatcherState::Deactive);

	State = EActionDispatcherState::Active;
	ActiveDispatcher();
	for (UXD_DispatchableActionBase* Action : TArray<UXD_DispatchableActionBase*>(CurrentActions))
	{
		if (Action)
		{
			Action->ReactiveAction();
		}
	}
}

void UXD_ActionDispatcherBase::ActiveDispatcher()
{
	if (bIsMainDispatcher)
	{
		for (USoftObjectProperty* SoftObjectProperty : GetSoftObjectPropertys())
		{
			FSoftObjectPtr SoftObjectPtr = SoftObjectProperty->GetPropertyValue(SoftObjectProperty->ContainerPtrToValuePtr<uint8>(this));
			UObject* Obj = SoftObjectPtr.Get();
			if (Obj && Obj->Implements<UXD_DispatchableEntityInterface>())
			{
				IXD_DispatchableEntityInterface::SetCurrentMainDispatcher(Obj, this);
			}
		}
	}

	if (UObject* Leader = DispatcherLeader.Get())
	{
		if (APawn* Pawn = Cast<APawn>(Leader))
		{
			Pawn->OnEndPlay.AddUniqueDynamic(this, &UXD_ActionDispatcherBase::WhenPlayerLeaderDestroyed);
		}
		else
		{
			UXD_SaveGameSystemBase::Get(this)->OnPreLevelUnload.AddUObject(this, &UXD_ActionDispatcherBase::WhenLevelLeaderDestroyed);
		}
	}
	WhenActived();
}

bool UXD_ActionDispatcherBase::IsAllSoftReferenceValid() const
{
	for (USoftObjectProperty* SoftObjectProperty : GetSoftObjectPropertys())
	{
		FSoftObjectPtr SoftObjectPtr = SoftObjectProperty->GetPropertyValue(SoftObjectProperty->ContainerPtrToValuePtr<uint8>(this));
#if WITH_EDITOR
		if (SoftObjectPtr.IsNull())
		{
			ActionDispatcher_Error_Log("调度器%s中的软引用[%s]为空，该调度器永远不会触发", *UXD_DebugFunctionLibrary::GetDebugName(this), *SoftObjectProperty->GetDisplayNameText().ToString());
		}
#endif
		if (UObject* Obj = SoftObjectPtr.Get())
		{
			if (Obj->Implements<UXD_DispatchableEntityInterface>())
			{
				if (IXD_DispatchableEntityInterface::CanExecuteDispatcher(Obj) == false)
				{
					return false;
				}
				if (UXD_ActionDispatcherBase* Dispatcher = IXD_DispatchableEntityInterface::GetCurrentMainDispatcher(Obj))
				{
					//当主调度器（抢占式）存在的时候不能再启用别的调度器
 					if (Dispatcher != this)
 					{
 						return false;
 					}
				}
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

void UXD_ActionDispatcherBase::FinishDispatch(FGameplayTag Tag)
{
	check(State == EActionDispatcherState::Active);
	DeactiveDispatcher(true);
	if (IsSubActionDispatcher())
	{
		ActionDispatcher_Display_Log("结束子行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(this));
	}
	else
	{
		ActionDispatcher_Display_Log("结束行为调度器%s", *UXD_DebugFunctionLibrary::GetDebugName(this));
		if (UXD_ActionDispatcherManager * Manager = GetManager())
		{
			Manager->WhenDispatcherFinished(this);
		}
	}

	OnDispatchFinished.Broadcast(Tag);
	WhenDispatchFinished.ExecuteIfBound(Tag.GetTagName());
	WhenDispatchFinishedNative.ExecuteIfBound(Tag.GetTagName());
}

#if WITH_EDITOR
TArray<FName> UXD_ActionDispatcherBase::GetAllFinishTags() const
{
	UActionDispatcherBlueprint* AD_Blueprint = CastChecked<UActionDispatcherBlueprint>(CastChecked<UBlueprintGeneratedClass>(GetClass())->ClassGeneratedBy);
	return AD_Blueprint->FinishTags;
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
	for (UObject* NextOuter = GetOuter(); NextOuter != nullptr; NextOuter = NextOuter->GetOuter())
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
	check(SubActionDispatcher->State != EActionDispatcherState::Active);
	SubActionDispatcher->State = EActionDispatcherState::Active;

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

void UXD_ActionDispatcherBase::WhenDeactived(bool IsFinsihedCompleted)
{
	ReceiveWhenDeactived(IsFinsihedCompleted);
}

void UXD_ActionDispatcherBase::PreDebugForceExecuteNode()
{

}
