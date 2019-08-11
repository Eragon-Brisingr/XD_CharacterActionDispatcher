// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XD_CharacterActionDispatcherType.h"
#include "XD_DispatchableActionBase.generated.h"

class UXD_ActionDispatcherBase;

/**
 * 
 */
UCLASS(abstract, BlueprintType, Within = "XD_ActionDispatcherBase")
class XD_CHARACTERACTIONDISPATCHER_API UXD_DispatchableActionBase : public UObject
{
	GENERATED_BODY()
public:
	UXD_DispatchableActionBase();

	UWorld* GetWorld() const override;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "设置")
	uint8 bIsPluginAction : 1;
	UPROPERTY(EditAnywhere, Category = "设置")
	uint8 bShowInExecuteActionNode : 1;
#endif
	DECLARE_DELEGATE(FOnActionAborted);
	FOnActionAborted OnActionAborted;
protected:
	friend class UXD_ActionDispatcherBase;

	void ActiveAction();
	//当行为被第一次激活时的实现
	virtual void WhenActionActived(){}

	virtual bool CanActiveAction() const { return IsActionValid(); }

	void AbortAction();
	//当行为被中断时的实现，一般为NPC主动想中断该行为
	virtual void WhenActionAborted();

	void DeactiveAction();
	//当行为反激活时的实现，一般用作清理委托
	virtual void WhenActionDeactived(){}

	void ReactiveAction();
	//当行为被再次激活时的实现
	virtual void WhenActionReactived();

	//行为是否能和其他行为同时执行
	virtual bool IsCompatibleWith(UXD_DispatchableActionBase* Action) const { return false; }

	UPROPERTY(EditDefaultsOnly, Category = "设置")
	uint8 bTickable : 1;
	//需开启bTickable
	virtual void WhenTick(float DeltaSeconds) {}
private:
	void FinishAction();
public:
	//当行为成功结束时的实现
	UFUNCTION()
	virtual void WhenActionFinished(){}

	void SaveState();
	//当请求保存该行为时的实现，行为被中断和保存游戏时会被调用
	UFUNCTION()
	virtual void WhenSaveState(){}
public:
	UFUNCTION(BlueprintCallable, Category = "行为")
	UXD_ActionDispatcherBase* GetOwner() const;

	UPROPERTY(BlueprintReadOnly, Category = "行为", SaveGame)
	EDispatchableActionState State;
public:
	struct FPinNameData
	{
		FName PinName;
		FText PinDisplayName;
	};
	TArray<FPinNameData> GetAllFinishedEventName() const;
	TArray<FPinNameData> GetAllNormalEventName() const;
	// TODO 可以不为运行时行为，ExpandNode时根据类型绑定上回调，这样还可以支持参数
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	void BindAllActionEvent(const TArray<FOnDispatchableActionFinishedEvent>& FinishedEvents, const TArray<FDispatchableActionNormalEvent>& NormalEvents);
protected:
	//返回行为中所有需要注册的实体
	virtual TSet<AActor*> GetAllRegistableEntities() const;
private:
	//所有执行Action的实体在Active时注册
	UFUNCTION(BlueprintCallable, Category = "行为")
	void RegisterEntity(AActor* Actor);
	//所有执行Action的实体在Finish时反注册
	UFUNCTION(BlueprintCallable, Category = "行为")
	void UnregisterEntity(AActor* Actor);

protected:
	//执行下一个事件
	UFUNCTION(BlueprintCallable, Category = "行为")
	void ExecuteEventAndFinishAction(const FOnDispatchableActionFinishedEvent& Event);

	virtual bool IsActionValid() const;
public:
	UFUNCTION(BlueprintCallable, Category = "行为")
	void AbortDispatcher(const FOnDispatcherAborted& Event, bool DeactiveRequestAction = false);
	void AbortDispatcher(const FOnActionAborted& Event, bool DeactiveRequestAction = false);
	void AbortDispatcher(bool DeactiveRequestAction = false);

	UFUNCTION(BlueprintCallable, Category = "行为")
	bool CanReactiveDispatcher() const;

	UFUNCTION(BlueprintCallable, Category = "行为")
	bool InvokeReactiveDispatcher();

	UFUNCTION(BlueprintCallable, Category = "行为")
	void ReactiveDispatcher();

	UPROPERTY(SaveGame, meta = (DisplayName = "行为激活"))
	FDispatchableActionNormalEvent OnActionActived;
	UPROPERTY(SaveGame, meta = (DisplayName = "行为反激活"))
	FDispatchableActionNormalEvent OnActionDeactived;
private:
	void PostInitProperties() override;
	TArray<UStructProperty*> CachedAllFinishedEvents;
	TArray<UStructProperty*> CachedAllNormalEvents;
	const TArray<UStructProperty*>& GetAllFinishedEvents() const;
	const TArray<UStructProperty*>& GetAllNormalEvents() const;
};

UCLASS(abstract, Blueprintable)
class XD_CHARACTERACTIONDISPATCHER_API UXD_DA_BlueprintBase : public UXD_DispatchableActionBase
{
	GENERATED_BODY()
public:
	UXD_DA_BlueprintBase(){}

	void WhenActionActived() override { ReceiveWhenActionActived(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "WhenActionActived"))
	void ReceiveWhenActionActived();

	bool IsActionValid() const override { return ReceiveIsActionValid(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "IsActionValid"))
	bool ReceiveIsActionValid() const;

	bool CanActiveAction() const override { return IsActionValid() && ReceiveCanActiveAction(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "CanActiveAction"))
	bool ReceiveCanActiveAction() const;

	void WhenActionDeactived() override { ReceiveWhenActionDeactived(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "WhenActionDeactived"))
	void ReceiveWhenActionDeactived();

	void WhenActionFinished() override { ReceiveWhenActionFinished(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "WhenActionFinished"))
	void ReceiveWhenActionFinished();

	void WhenSaveState() override { ReceiveWhenSaveState(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "WhenSaveState"))
	void ReceiveWhenSaveState();
};
