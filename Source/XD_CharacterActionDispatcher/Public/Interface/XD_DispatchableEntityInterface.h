// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <GameplayTagContainer.h>
#include "XD_DispatchableEntityInterface.generated.h"

class UXD_DispatchableActionBase;

USTRUCT(BlueprintType, BlueprintInternalUseOnly, meta = (HasNativeMake = "XD_DispatchableActionListUtils.MakeDispatchableActionList", HasNativeBreak = "XD_DispatchableActionListUtils.BreakDispatchableActionList"))
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
	const TArray<UXD_DispatchableActionBase*>& operator*() const
	{
		check(List != nullptr);
		return *List;
	}
};

// This class does not need to be modified.
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_DispatchableActionListUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "行为", meta = (NativeMakeFunc))
	static FXD_DispatchableActionList MakeDispatchableActionList(const TArray<UXD_DispatchableActionBase*>& Actions)
	{
		return FXD_DispatchableActionList(const_cast<TArray<UXD_DispatchableActionBase*>&>(Actions));
	}

	UFUNCTION(BlueprintPure, Category = "行为", meta = (NativeBreakFunc))
	static TArray<UXD_DispatchableActionBase*> BreakDispatchableActionList(const FXD_DispatchableActionList& ActionList)
	{
		return *ActionList;
	}
};

// This class does not need to be modified.
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_DA_StateTagUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = true))
	static bool HasStateTag(UObject* Obj, FGameplayTag Tag);
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	static void AddStateTag(UObject* Obj, FGameplayTag Tag);
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
	UXD_ActionDispatcherBase* GetCurrentMainDispatcher() const;
	virtual UXD_ActionDispatcherBase* GetCurrentMainDispatcher_Implementation() const { return nullptr; }
	static UXD_ActionDispatcherBase* GetCurrentMainDispatcher(UObject* Obj) { return IXD_DispatchableEntityInterface::Execute_GetCurrentMainDispatcher(Obj); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	void SetCurrentMainDispatcher(UXD_ActionDispatcherBase* Dispatcher);
	virtual void SetCurrentMainDispatcher_Implementation(UXD_ActionDispatcherBase* Dispatcher) {}
	static void SetCurrentMainDispatcher(UObject* Obj, UXD_ActionDispatcherBase* Dispatcher) { IXD_DispatchableEntityInterface::Execute_SetCurrentMainDispatcher(Obj, Dispatcher); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	bool CanExecuteDispatcher() const;
	virtual bool CanExecuteDispatcher_Implementation() const { return true; }
	static bool CanExecuteDispatcher(UObject* Obj) { return IXD_DispatchableEntityInterface::Execute_CanExecuteDispatcher(Obj); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	bool AD_HasStateTag(const FGameplayTag& Tag) const;
	virtual bool AD_HasStateTag_Implementation(const FGameplayTag& Tag) const { return true; }
	static bool AD_HasStateTag(UObject* Obj, const FGameplayTag& Tag) { return IXD_DispatchableEntityInterface::Execute_AD_HasStateTag(Obj, Tag); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	void AD_AddStateTag(const FGameplayTag& Tag);
	virtual void AD_AddStateTag_Implementation(const FGameplayTag& Tag) {}
	static void AD_AddStateTag(UObject* Obj, const FGameplayTag& Tag) { IXD_DispatchableEntityInterface::Execute_AD_AddStateTag(Obj, Tag); }
};
