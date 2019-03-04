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
USTRUCT()
struct FTogetherFlowControl
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	TArray<bool> CheckList;

	UPROPERTY(SaveGame)
	FDispatchableActionFinishedEvent TogetherEvent;
};

UCLASS(abstract, Blueprintable)
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherBase : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "行为")
	bool CanExecuteDispatch() const;
	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	bool ReceiveCanExecuteDispatch() const;

	void StartDispatch();
	UFUNCTION(BlueprintImplementableEvent, Category = "行为")
	void WhenDispatchStart();

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	void ActiveAction(UXD_DispatchableActionBase* Action);

	void AbortDispatch();
	//With Check
	bool InvokeReactiveDispatch();
	void ReactiveDispatcher();

	//TODO 完成实现
	UFUNCTION(BlueprintCallable, Category = "行为")
	void FinishDispatch(FName Tag);
public:
	UPROPERTY(SaveGame)
	TArray<UXD_DispatchableActionBase*> CurrentActions;

	UPROPERTY(SaveGame)
	TMap<FGuid, FTogetherFlowControl> ActivedTogetherControl;

	uint8 bIsActive : 1;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	bool EnterTogetherFlowControl(FGuid NodeGuid, int32 Index, int32 TogetherCount);

	UXD_ActionDispatcherManager* GetManager() const;

	UWorld* GetWorld() const override;
};
