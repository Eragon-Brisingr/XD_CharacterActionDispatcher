// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Utils/XD_CharacterActionDispatcherType.h"
#include "XD_DA_RoleSelectionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UXD_DA_RoleSelectionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class XD_CHARACTERACTIONDISPATCHER_API IXD_DA_RoleSelectionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, Category = "行为|选择")
	void ExecuteSelect(UXD_DA_RoleSelectionBase* RoleSelection, const TArray<FDA_DisplaySelection>& Selections);
	virtual void ExecuteSelect_Implementation(UXD_DA_RoleSelectionBase* RoleSelection, const TArray<FDA_DisplaySelection>& Selections) {}
	static void ExecuteSelect(UObject* Role, UXD_DA_RoleSelectionBase* RoleSelection, const TArray<FDA_DisplaySelection>& Selections) { IXD_DA_RoleSelectionInterface::Execute_ExecuteSelect(Role, RoleSelection, Selections); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为|选择")
	void ExecuteAddSelects(const TArray<FDA_DisplaySelection>& Selections);
	virtual void ExecuteAddSelects_Implementation(const TArray<FDA_DisplaySelection>& Selections) {}
	static void ExecuteAddSelects(UObject* Role, const TArray<FDA_DisplaySelection>& Selections) { IXD_DA_RoleSelectionInterface::Execute_ExecuteAddSelects(Role, Selections); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为|选择")
	void ExecuteAbortSelect();
	virtual void ExecuteAbortSelect_Implementation() {}
	static void ExecuteAbortSelect(UObject* Role) { IXD_DA_RoleSelectionInterface::Execute_ExecuteAbortSelect(Role); }

	UFUNCTION(BlueprintNativeEvent, Category = "行为|选择")
	void WhenSelected(const FDA_DisplaySelection& Selection);
	virtual void WhenSelected_Implementation(const FDA_DisplaySelection& Selection) {}
};
