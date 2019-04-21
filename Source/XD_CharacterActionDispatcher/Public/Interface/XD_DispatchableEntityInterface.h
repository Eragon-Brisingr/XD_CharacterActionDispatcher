// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XD_DispatchableEntityInterface.generated.h"

class UXD_DispatchableActionBase;

USTRUCT(BlueprintType, BlueprintInternalUseOnly)
struct XD_CHARACTERACTIONDISPATCHER_API FXD_DispatchableActionList
{
	GENERATED_BODY()
public:
	FXD_DispatchableActionList() = default;
	FXD_DispatchableActionList(TArray<UXD_DispatchableActionBase*>& Actions)
		:List(&Actions)
	{}
private:
	TArray<UXD_DispatchableActionBase*>* List = nullptr;
public:
	operator bool() const { return List != nullptr; }
	TArray<UXD_DispatchableActionBase*>& operator*()
	{
		check(List != nullptr);
		return *List;
	}
};

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
	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	FXD_DispatchableActionList GetCurrentDispatchableActions();
	virtual FXD_DispatchableActionList GetCurrentDispatchableActions_Implementation() { return FXD_DispatchableActionList(); }
	static TArray<UXD_DispatchableActionBase*>& GetCurrentDispatchableActions(UObject* Obj) { return *IXD_DispatchableEntityInterface::Execute_GetCurrentDispatchableActions(Obj); }
	template<typename ActionType>
	static ActionType* GetCurrentDispatchableAction(UObject* Obj)
	{
		for (UXD_DispatchableActionBase* Action : GetCurrentDispatchableActions(Obj))
		{
			if (ActionType* InvokeAction = Cast<ActionType>(Action))
			{
				return InvokeAction;
			}
		}
		return nullptr;
	}

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
