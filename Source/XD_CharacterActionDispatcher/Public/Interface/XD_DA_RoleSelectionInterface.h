// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XD_CharacterActionDispatcherType.h"
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
	void ExecuteSelect(const TArray<FDA_RoleSelection>& Selections);
	virtual void ExecuteSelect_Implementation(const TArray<FDA_RoleSelection>& Selections) {}
	static void ExecuteSelect(UObject* Role, const TArray<FDA_RoleSelection>& Selections) { IXD_DA_RoleSelectionInterface::Execute_ExecuteSelect(Role, Selections); }
};
