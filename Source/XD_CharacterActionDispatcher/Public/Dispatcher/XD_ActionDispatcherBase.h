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

UCLASS(abstract)
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherBase : public UObject
{
	GENERATED_BODY()
public:
	UXD_ActionDispatcherBase();

	UFUNCTION(BlueprintCallable, Category = "行为")
	bool CanExecuteDispatch() const;
	UFUNCTION(BlueprintNativeEvent, Category = "行为")
	bool ReceiveCanExecuteDispatch() const;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void StartDispatch();
	UFUNCTION(BlueprintImplementableEvent, Category = "行为")
	void WhenDispatchStart();

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	void ActiveAction(UXD_DispatchableActionBase* Action);

	void AbortDispatch();
	//With Check
	bool InvokeReactiveDispatch();
	void ReactiveDispatcher();

	UPROPERTY(EditAnywhere, Category = "行为")
	uint8 bCheckAllSoftReferenceValidate : 1;

private:
	bool IsAllSoftReferenceValid() const;
	//结束调度器
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	void FinishDispatch(FGameplayTag Tag);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDispatchFinished, const FGameplayTag&, Tag);
	UPROPERTY(BlueprintAssignable)
	FOnDispatchFinished OnDispatchFinished;

	DECLARE_DYNAMIC_DELEGATE_OneParam(FWhenDispatchFinished, const FName&, Tag);
	UPROPERTY(SaveGame)
	FWhenDispatchFinished WhenDispatchFinished;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	void BindWhenDispatchFinished(const FWhenDispatchFinished& DispatchFinishedEvent) { WhenDispatchFinished = DispatchFinishedEvent; }

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "编译")
	TArray<FName> FinishTags;
	TArray<FName> GetAllFinishTags() const;
#endif
public:
	UPROPERTY(SaveGame)
	TArray<UXD_DispatchableActionBase*> CurrentActions;

	UPROPERTY(BlueprintReadOnly, Category = "行为调度器")
	uint8 bIsActive : 1;

	// 调度器可能不存在管理器
	// e.g. 玩家开机关的行为，调度器直接交给机关管理
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
	bool IsSubActionDispatcher() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = "true"))
	UXD_ActionDispatcherBase* GetMainActionDispatcher();

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	void ActiveSubActionDispatcher(UXD_ActionDispatcherBase* SubActionDispatcher, FGuid NodeGuid);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	bool TryActiveSubActionDispatcher(FGuid NodeGuid);

	UPROPERTY(SaveGame)
	TMap<FGuid, UXD_ActionDispatcherBase*> ActivedSubActionDispatchers;
};
