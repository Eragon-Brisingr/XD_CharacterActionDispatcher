// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ActionDispatcherManager.h"
#include "GameFramework/GameStateBase.h"
#include "XD_ActorFunctionLibrary.h"
#include "XD_ActionDispatcher_Log.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_ActionDispatcherGameStateImpl.h"
#include "XD_SaveGameSystemBase.h"
#include "Engine/LevelStreaming.h"

// Sets default values for this component's properties
UXD_ActionDispatcherManager::UXD_ActionDispatcherManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UXD_ActionDispatcherManager::BeginPlay()
{
	Super::BeginPlay();

	// ...
	UXD_SaveGameSystemBase::Get(this)->OnLoadLevelCompleted.AddDynamic(this, &UXD_ActionDispatcherManager::WhenLevelLoadCompleted);

	for (ULevelStreaming* LevelStream : GetWorld()->GetStreamingLevels())
	{
		LevelStream->OnLevelUnloaded.AddDynamic(this, &UXD_ActionDispatcherManager::WhenPostLevelUnload);
	}
}

void UXD_ActionDispatcherManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UXD_SaveGameSystemBase::Get(this)->OnLoadLevelCompleted.RemoveAll(this);
}

void UXD_ActionDispatcherManager::WhenPreSave_Implementation()
{
	for (UXD_ActionDispatcherBase* Dispatcher : ActivedDispatchers)
	{
		Dispatcher->SaveDispatchState();
	}
}

void UXD_ActionDispatcherManager::WhenPostLoad_Implementation()
{
	//https://issues.unrealengine.com/issue/UE-63285
	//很诡异的是直接从PostLoad调用的话软引用指向的Actor不是场景中的，通过TimerManager中转处理下
	FTimerHandle TimeHandle;
	GetWorld()->GetTimerManager().SetTimer(TimeHandle, FTimerDelegate::CreateLambda([this] 
	{
		for (UXD_ActionDispatcherBase* Dispatcher : TArray<UXD_ActionDispatcherBase*>(ActivedDispatchers))
		{
			if (Dispatcher->CanReactiveDispatcher())
			{
				Dispatcher->ReactiveDispatcher();
			}
			else
			{
				ActivedDispatchers.Remove(Dispatcher);
				PendingDispatchers.Add(Dispatcher);
			}
		}

		bEnableAutoActivePendingAction = true;
	}), 0.00001f, false);
}

// Called every frame
void UXD_ActionDispatcherManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (bEnableAutoActivePendingAction)
	{
		double StartTime = FPlatformTime::Seconds();
		double ActivePendingActionsTimeLimit = 0.001f;
		while (ActivePendingActionIdx < PendingDispatchers.Num())
		{
			UXD_ActionDispatcherBase* PendingDispatcher = PendingDispatchers[ActivePendingActionIdx];
			if (PendingDispatcher->IsDispatcherStarted())
			{
				if (PendingDispatcher->CanReactiveDispatcher())
				{
					PendingDispatcher->ReactiveDispatcher();
					PendingDispatchers.RemoveAt(ActivePendingActionIdx);
					WhenDispatcherReactived(PendingDispatcher);
				}
				else
				{
					++ActivePendingActionIdx;
				}
			}
			else
			{
				if (PendingDispatcher->CanExecuteDispatch())
				{
					PendingDispatcher->StartDispatch();
					PendingDispatchers.RemoveAt(ActivePendingActionIdx);
					WhenDispatcherStarted(PendingDispatcher);
				}
				else
				{
					++ActivePendingActionIdx;
				}
			}

			if (FPlatformTime::Seconds() - StartTime > ActivePendingActionsTimeLimit)
			{
				break;
			}
		}
		if (ActivePendingActionIdx >= PendingDispatchers.Num())
		{
			ActivePendingActionIdx = 0;
		}
	}
}

void UXD_ActionDispatcherManager::WhenDispatcherStarted(UXD_ActionDispatcherBase* Dispatcher)
{
	check(!ActivedDispatchers.Contains(Dispatcher));

	ActivedDispatchers.Add(Dispatcher);
}

void UXD_ActionDispatcherManager::WhenDispatcherReactived(UXD_ActionDispatcherBase* Dispatcher)
{
	check(!ActivedDispatchers.Contains(Dispatcher));

	ActivedDispatchers.Add(Dispatcher);
}

void UXD_ActionDispatcherManager::WhenDispatcherAborted(UXD_ActionDispatcherBase* Dispatcher)
{
	check(ActivedDispatchers.Contains(Dispatcher));

	ActivedDispatchers.Remove(Dispatcher);
	PendingDispatchers.Add(Dispatcher);
}

UXD_ActionDispatcherManager* UXD_ActionDispatcherManager::Get(const UObject* WorldContextObject)
{
	AGameStateBase* GameState = WorldContextObject->GetWorld()->GetGameState();
	if (GameState->Implements<UXD_ActionDispatcherGameStateImpl>())
	{
		return IXD_ActionDispatcherGameStateImpl::GetActionDispatcherManager(GameState);
	}
	else if (UXD_ActionDispatcherManager* ActionDispatcherManager = GameState->FindComponentByClass<UXD_ActionDispatcherManager>())
	{
		ActionDispatcher_Warning_LOG("%s中为实现XD_ActionDispatcherGameStateImpl接口，请实现并返回ActionDispatcherManager组件", *UXD_DebugFunctionLibrary::GetDebugName(GameState));
		return ActionDispatcherManager;
	}
	else
	{
		ActionDispatcher_Warning_LOG("%s中不存在ActionDispatcherManager组件，请在类中添加", *UXD_DebugFunctionLibrary::GetDebugName(GameState));
		return UXD_ActorFunctionLibrary::AddComponent<UXD_ActionDispatcherManager>(GameState, TEXT("ActionDispatcher"));
	}
}

void UXD_ActionDispatcherManager::WhenDispatcherFinished(UXD_ActionDispatcherBase* Dispatcher)
{
	check(ActivedDispatchers.Contains(Dispatcher));

	ActivedDispatchers.Remove(Dispatcher);

	InvokeActivePendingActions();
}

void UXD_ActionDispatcherManager::InvokeStartDispatcher(UXD_ActionDispatcherBase* Dispatcher)
{
	if (Dispatcher->CanExecuteDispatch())
	{
		WhenDispatcherStarted(Dispatcher);
		Dispatcher->StartDispatch();
	}
	else
	{
		check(!PendingDispatchers.Contains(Dispatcher));

		PendingDispatchers.Add(Dispatcher);
	}
}

void UXD_ActionDispatcherManager::InvokeActivePendingActions()
{

}

void UXD_ActionDispatcherManager::WhenLevelLoadCompleted(ULevel* Level)
{
	InvokeActivePendingActions();
}

void UXD_ActionDispatcherManager::WhenPostLevelUnload()
{
	for (int32 i = 0; i < ActivedDispatchers.Num();)
	{
		UXD_ActionDispatcherBase* Dispatcher = ActivedDispatchers[i];
		if (Dispatcher->CanExecuteDispatch() == false)
		{
			Dispatcher->bIsActive = false;
			for (UXD_DispatchableActionBase* DispatchableAction : Dispatcher->CurrentActions)
			{
				DispatchableAction->bIsActived = false;
			}

			ActivedDispatchers.RemoveAt(i);
			PendingDispatchers.Add(Dispatcher);
		}
		else
		{
			++i;
		}
	}
}

void UXD_ActionDispatcherManager::TryActivePendingDispatcher(UXD_ActionDispatcherBase* Dispatcher)
{
	if (Dispatcher == nullptr || Dispatcher->bIsActive)
	{
		return;
	}

	bool bCanActive = Dispatcher->IsDispatcherStarted() ? Dispatcher->CanReactiveDispatcher() : Dispatcher->CanExecuteDispatch();
	if (bCanActive)
	{
		int32 Idx = PendingDispatchers.IndexOfByKey(Dispatcher);

		check(Idx != INDEX_NONE);

		if (Idx < ActivePendingActionIdx)
		{
			ActivePendingActionIdx -= 1;
		}

		if (Dispatcher->IsDispatcherStarted())
		{
			Dispatcher->ReactiveDispatcher();
			PendingDispatchers.RemoveAt(Idx);
			WhenDispatcherReactived(Dispatcher);
		}
		else
		{
			Dispatcher->StartDispatch();
			PendingDispatchers.RemoveAt(Idx);
			WhenDispatcherStarted(Dispatcher);
		}
	}
}
