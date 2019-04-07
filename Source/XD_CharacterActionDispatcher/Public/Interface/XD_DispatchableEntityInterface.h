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

	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	UXD_ActionDispatcherBase* GetCurrentDispatcher() const;
	virtual UXD_ActionDispatcherBase* GetCurrentDispatcher_Implementation() const { return nullptr; }
	static UXD_ActionDispatcherBase* GetCurrentDispatcher(UObject* Obj) { return IXD_DispatchableEntityInterface::Execute_GetCurrentDispatcher(Obj); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	void SetCurrentDispatcher(UXD_ActionDispatcherBase* Dispatcher);
	virtual void SetCurrentDispatcher_Implementation(UXD_ActionDispatcherBase* Dispatcher) {}
	static void SetCurrentDispatcher(UObject* Obj, UXD_ActionDispatcherBase* Dispatcher) { IXD_DispatchableEntityInterface::Execute_SetCurrentDispatcher(Obj, Dispatcher); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	bool CanExecuteDispatchableAction() const;
	virtual bool CanExecuteDispatchableAction_Implementation() const { return true; }
	static bool CanExecuteDispatchableAction(UObject* Obj) { return IXD_DispatchableEntityInterface::Execute_CanExecuteDispatchableAction(Obj); }
};
