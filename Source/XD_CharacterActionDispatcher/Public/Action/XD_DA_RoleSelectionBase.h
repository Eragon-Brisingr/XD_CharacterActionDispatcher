// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/XD_DispatchableActionBase.h"
#include "XD_DA_RoleSelectionBase.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "显示选项_基础"))
class XD_CHARACTERACTIONDISPATCHER_API UXD_DA_RoleSelectionBase : public UXD_DispatchableActionBase
{
	GENERATED_BODY()
public:
	UXD_DA_RoleSelectionBase();

	TSet<AActor*> GetAllRegistableEntities() const override;
	bool IsActionValid() const override;
	void WhenActionActived() override;
	void WhenActionDeactived() override;
	void WhenActionFinished() override;

public:
	UPROPERTY(SaveGame)
	TArray<FDA_RoleSelection> Selections;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (ExposeOnSpawn = "true"))
	TSoftObjectPtr<APawn> Role;

	UFUNCTION(BlueprintCallable, Category = "行为|选择")
	void ExecuteSelection(const FDA_DisplaySelection& Selection);

	UFUNCTION(BlueprintCallable, Category = "行为|选择")
	static void ExecuteRoleSelected(APawn* InRole, const FDA_DisplaySelection& Selection);
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void ShowSelection(UXD_ActionDispatcherBase* ActionDispatcher, bool SaveAction, FGuid ActionGuid, const TArray<FDA_RoleSelection>& InSelections, const TArray<bool>& ShowSelectionConditions);

	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = true))
	static FDA_RoleSelection& SetWhenSelectedEvent(FDA_RoleSelection Selection, const FOnDispatchableActionFinishedEvent& Event);
private:
	void AddSelections(const TArray<FDA_RoleSelection>& InSelections);

	void ExecuteSelection(const FDA_RoleSelection& RoleSelection);
};
