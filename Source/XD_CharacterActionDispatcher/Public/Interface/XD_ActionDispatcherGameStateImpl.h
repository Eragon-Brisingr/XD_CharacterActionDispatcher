// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XD_ActionDispatcherGameStateImpl.generated.h"

class UXD_ActionDispatcherManager;
class AGameStateBase;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UXD_ActionDispatcherGameStateImpl : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class XD_CHARACTERACTIONDISPATCHER_API IXD_ActionDispatcherGameStateImpl
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, Category = "管理器")
	UXD_ActionDispatcherManager* GetActionDispatcherManager() const;
	virtual UXD_ActionDispatcherManager* GetActionDispatcherManager_Implementation() const { return nullptr; }
	static UXD_ActionDispatcherManager* GetActionDispatcherManager(const AGameStateBase* GameState) { return Execute_GetActionDispatcherManager((UObject*)GameState); }
};
