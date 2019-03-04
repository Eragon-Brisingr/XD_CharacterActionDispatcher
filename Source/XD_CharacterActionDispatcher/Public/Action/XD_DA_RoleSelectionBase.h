// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/XD_DispatchableActionBase.h"
#include "XD_DA_RoleSelectionBase.generated.h"

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_DA_RoleSelectionBase : public UXD_DispatchableActionBase
{
	GENERATED_BODY()
public:
	UXD_DA_RoleSelectionBase();

	void WhenActionActived() override;
	void WhenActionDeactived() override;
	void WhenActionFinished() override;

public:
	UPROPERTY(SaveGame)
	TArray<FDA_RoleSelection> Selections;

	UPROPERTY(SaveGame)
	TSoftObjectPtr<APawn> Role;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UXD_DA_RoleSelectionBase* ShowSelection(UXD_ActionDispatcherBase* ActionDispatcher, const TSoftObjectPtr<APawn>& InRole, const TArray<FDA_RoleSelection>& InSelections);

	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = "true"))
	static FDA_RoleSelection& SetWhenSelectedEvent(FDA_RoleSelection Selection, const FDispatchableActionFinishedEvent& Event);

private:
	void WhenSelected();
};
