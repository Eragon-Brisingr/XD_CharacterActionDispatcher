// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ActionDispatcherManager.h"

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

