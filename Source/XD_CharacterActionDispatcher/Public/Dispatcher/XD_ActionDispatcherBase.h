// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XD_CharacterActionDispatcherType.h"
#include "XD_ActionDispatcherBase.generated.h"

class UXD_DispatchableActionBase;
class UXD_ActionDispatcherManager;

/**
 * 
 */
UCLASS(abstract, Blueprintable)
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherBase : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "行为")
	bool CanExecuteDispatch() { return ReceiveCanExecuteDispatch(); }
	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	bool ReceiveCanExecuteDispatch();

	UFUNCTION(BlueprintCallable, Category = "行为")
	void StartDispatch() { WhenDispatchStart(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为")
	void WhenDispatchStart();

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	void ActiveAction(UXD_DispatchableActionBase* Action, const TArray<FDispatchableActionFinishedEvent>& FinishedEvents);

	UFUNCTION(BlueprintCallable, Category = "行为")
	void AbortDispatch();

	UFUNCTION(BlueprintCallable, Category = "行为")
	void ReactiveDispatch();

	void FinishAction(UXD_DispatchableActionBase* Action);
public:
	UPROPERTY(SaveGame)
	TArray<UXD_DispatchableActionBase*> ActivedActions;

	UXD_ActionDispatcherManager* GetOwner() const;

	UWorld* GetWorld() const override;
};
