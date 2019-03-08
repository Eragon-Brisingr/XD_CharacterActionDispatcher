// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XD_CharacterActionDispatcherType.h"
#include "GameplayTagContainer.h"
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
	void FinishDispatch(FGameplayTag Tag);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDispatchFinished, const FGameplayTag&, Tag);
	UPROPERTY(BlueprintAssignable)
	FOnDispatchFinished OnDispatchFinished;

	TArray<FGameplayTag> GetAllFinishTags() const { return {}; }
public:
	UPROPERTY(SaveGame)
	TArray<UXD_DispatchableActionBase*> CurrentActions;

	uint8 bIsActive : 1;

	UXD_ActionDispatcherManager* GetManager() const;

	UWorld* GetWorld() const override;
	
	//共同行为调度器
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	bool EnterTogetherFlowControl(FGuid NodeGuid, int32 Index, int32 TogetherCount);

	TMap<FGuid, FTogetherFlowControl> ActivedTogetherControl;

	//子流程，允许逻辑分层
	//若没有需要储存的状态更推荐使用公共宏代替
public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(SaveGame)
	uint8 bIsSubActionDispatcher : 1;
#endif

	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = "true"))
	UXD_ActionDispatcherBase* GetMainActionDispatcher();

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	void ActiveSubActionDispatcher(UXD_ActionDispatcherBase* SubActionDispatcher, FGuid NodeGuid);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	bool TryActiveSubActionDispatcher(FGuid NodeGuid);

	UPROPERTY(SaveGame)
	TMap<FGuid, UXD_ActionDispatcherBase*> ActivedSubActionDispatchers;
};
