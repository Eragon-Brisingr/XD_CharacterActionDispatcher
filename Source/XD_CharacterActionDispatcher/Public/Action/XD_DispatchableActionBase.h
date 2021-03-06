﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <UObject/NoExportTypes.h>
#include "Utils/XD_CharacterActionDispatcherType.h"
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
	void AbortAction();
	void DeactiveAction();
	void ReactiveAction();
protected:
	//当行为被第一次激活时的实现
	virtual void WhenActionActived(){}

	//确认行为是否可以执行
	virtual bool IsActionValid() const;

	//当行为被中断时的实现，一般为NPC主动想中断该行为
	virtual void WhenActionAborted();

	//当行为反激活时的实现，一般用作清理委托
	virtual void WhenActionDeactived(){}

	//当行为被再次激活时的实现
	virtual void WhenActionReactived();

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
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static void BindFinishedEvent(UPARAM(Ref)FOnDispatchableActionFinishedEvent& FinishedEvent, const FDispatchableActionEventDelegate& InEvent);
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static void BindNormalEvent(UPARAM(Ref)FDispatchableActionNormalEvent& NormalEvent, const FDispatchableActionEventDelegate& InEvent);
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
public:
	//DeactiveRequestAction为true则不会调用Abort行为，考虑动画被打断的情况
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
};

UCLASS(abstract, Blueprintable)
class XD_CHARACTERACTIONDISPATCHER_API UXD_DispatchAction_ScriptBase : public UXD_DispatchableActionBase
{
	GENERATED_BODY()
public:
	UXD_DispatchAction_ScriptBase(){}

	void WhenActionActived() override { ReceiveWhenActionActived(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "WhenActionActived"))
	void ReceiveWhenActionActived();

	bool IsActionValid() const override { return ReceiveIsActionValid(); }
	UFUNCTION(BlueprintImplementableEvent, Category = "行为", meta = (DisplayName = "IsActionValid"))
	bool ReceiveIsActionValid() const;

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
