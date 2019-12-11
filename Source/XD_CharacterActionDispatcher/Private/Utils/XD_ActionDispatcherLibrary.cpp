// Fill out your copyright notice in the Description page of Project Settings.

#include "Utils/XD_ActionDispatcherLibrary.h"
#include "Dispatcher/XD_ActionDispatcherBase.h"
#include "Manager/XD_ActionDispatcherManager.h"

UXD_ActionDispatcherBase* UXD_ActionDispatcherLibrary::GetOrCreateDispatcherWithOwner(UObject* Owner, TSubclassOf<UXD_ActionDispatcherBase> Dispatcher, UXD_ActionDispatcherBase*& Dispatcher_MemberVar)
{
	if (Dispatcher_MemberVar)
	{
		return Dispatcher_MemberVar;
	}
	Dispatcher_MemberVar = NewObject<UXD_ActionDispatcherBase>(Owner, Dispatcher);
	return Dispatcher_MemberVar;
}

UXD_ActionDispatcherManager* UXD_ActionDispatcherLibrary::GetActionDispatcherManager(const UObject* WorldContextObject)
{
	return UXD_ActionDispatcherManager::Get(WorldContextObject);
}
