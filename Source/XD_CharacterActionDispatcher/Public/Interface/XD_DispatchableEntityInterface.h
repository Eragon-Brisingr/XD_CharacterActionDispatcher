// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XD_DispatchableEntityInterface.generated.h"

class UXD_DispatchableActionBase;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UXD_DispatchableEntityInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class XD_CHARACTERACTIONDISPATCHER_API IXD_DispatchableEntityInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, Category = "行为", BlueprintCallable, meta = (CompactNodeTitle = "GetAction"))
	UXD_DispatchableActionBase* GetCurrentDispatchableAction() const;
	virtual UXD_DispatchableActionBase* GetCurrentDispatchableAction_Implementation() const { return nullptr; }
	static UXD_DispatchableActionBase* GetCurrentDispatchableAction(UObject* Obj) { return IXD_DispatchableEntityInterface::Execute_GetCurrentDispatchableAction(Obj); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	void SetCurrentDispatchableAction(UXD_DispatchableActionBase* Action);
	virtual void SetCurrentDispatchableAction_Implementation(UXD_DispatchableActionBase* Action) {}
	static void SetCurrentDispatchableAction(UObject* Obj, UXD_DispatchableActionBase* Action) { IXD_DispatchableEntityInterface::Execute_SetCurrentDispatchableAction(Obj, Action); }
};
