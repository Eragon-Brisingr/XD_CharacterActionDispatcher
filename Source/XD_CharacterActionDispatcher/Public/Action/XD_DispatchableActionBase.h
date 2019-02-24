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
UCLASS(abstract)
class XD_CHARACTERACTIONDISPATCHER_API UXD_DispatchableActionBase : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	UXD_ActionDispatcherBase* Owner;

	UWorld* GetWorld() const override;
public:
	void ActiveAction() { WhenActionActived(); }
	UFUNCTION()
	virtual void WhenActionActived(){}

	void DeactiveAction() { WhenActionDeactived(); }
	UFUNCTION()
	virtual void WhenActionDeactived(){}

	void ReactiveAction() { WhenActionReactived(); }
	UFUNCTION()
	virtual void WhenActionReactived() { WhenActionActived(); }

	UFUNCTION()
	void FinishAction();

	virtual TArray<FName> GetAllFinishedEventName() const;

	// TODO 可以不为运行时行为，ExpandNode时根据类型绑定上回调，这样还可以支持参数
	virtual void BindAllFinishedEvent(const TArray<FDispatchableActionFinishedEvent>& FinishedEvents);
};

UCLASS(meta = (DisplayName = "例子"))
class XD_CHARACTERACTIONDISPATCHER_API UXD_DispatchableActionExample : public UXD_DispatchableActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame, meta = (DisplayName = "当结束时"))
	FDispatchableActionFinishedEvent OnFinished;

	void WhenActionActived() override;
	void WhenActionDeactived() override;
public:
	FTimerHandle TimerHandle;

	UPROPERTY(BlueprintReadOnly, Category = "例子", meta = (ExposeOnSpawn = "true"))
	float DelayTime;

	void WhenTimeFinished();
};
