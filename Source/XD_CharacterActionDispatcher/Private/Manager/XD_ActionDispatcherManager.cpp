// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ActionDispatcherManager.h"
#include "GameFramework/GameStateBase.h"
#include "XD_ActorFunctionLibrary.h"
#include "XD_ActionDispatcher_Log.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_ActionDispatcherGameStateImpl.h"

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
	
}


void UXD_ActionDispatcherManager::WhenPreSave_Implementation()
{
	for (UXD_ActionDispatcherBase* Dispatcher : ActivedDispatchers)
	{
		Dispatcher->AbortDispatch();
	}
}

// Called every frame
void UXD_ActionDispatcherManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UXD_ActionDispatcherManager::InvokeStartDispatcher(UXD_ActionDispatcherBase* Dispatcher)
{
	check(!(ActivedDispatchers.Contains(Dispatcher) && PendingDispatchers.Contains(Dispatcher)));

	if (Dispatcher->CanExecuteDispatch())
	{
		ActivedDispatchers.Add(Dispatcher);
		Dispatcher->StartDispatch();
	}
	else
	{
		PendingDispatchers.Add(Dispatcher);
	}
}

void UXD_ActionDispatcherManager::InvokeAbortDispatcher(UXD_ActionDispatcherBase* Dispatcher)
{
	check(ActivedDispatchers.Contains(Dispatcher));

	ActivedDispatchers.Remove(Dispatcher);
	PendingDispatchers.Add(Dispatcher);
	Dispatcher->AbortDispatch();
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

void UXD_ActionDispatcherManager::FinishDispatcher(UXD_ActionDispatcherBase* Dispatcher)
{
	check(ActivedDispatchers.Contains(Dispatcher));

	ActivedDispatchers.Remove(Dispatcher);
}
