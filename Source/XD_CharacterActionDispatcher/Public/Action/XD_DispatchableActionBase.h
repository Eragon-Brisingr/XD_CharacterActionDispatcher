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
protected:
	friend class UXD_ActionDispatcherBase;

	void ActiveAction();
	//当行为被第一次激活时的实现，别忘记调用RegisterEntity
	UFUNCTION()
	virtual void WhenActionActived(){}

	void DeactiveAction();
	//当行为被中断时的实现
	UFUNCTION()
	virtual void WhenActionDeactived(){}

	void ReactiveAction();
	//当行为被再次激活时的实现
	UFUNCTION()
	virtual void WhenActionReactived();

	void FinishAction();
	//当行为成功结束时的实现，一般用作UnregisterEntity
	UFUNCTION()
	virtual void WhenActionFinished(){}
public:
	UXD_ActionDispatcherBase* GetOwner() const;

	UPROPERTY()
	uint8 bIsActived : 1;
	UPROPERTY()
	uint8 bIsFinished : 1;
public:
	struct FPinNameData
	{
		FName PinName;
		FText PinDisplayName;
	};
	virtual TArray<FPinNameData> GetAllFinishedEventName() const;
	// TODO 可以不为运行时行为，ExpandNode时根据类型绑定上回调，这样还可以支持参数
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	virtual void BindAllFinishedEvent(const TArray<FDispatchableActionFinishedEvent>& FinishedEvents);

protected:
	//所有执行Action的实体在Active时注册
	void RegisterEntity(AActor* Actor);
	//所有执行Action的实体在Finish时反注册
	void UnregisterEntity(AActor* Actor);

public:
	UFUNCTION(BlueprintCallable, Category = "行为")
	void AbortDispatcher();

	UFUNCTION(BlueprintCallable, Category = "行为")
	bool CanReactiveDispatcher() const;

	//With Check
	UFUNCTION(BlueprintCallable, Category = "行为")
	bool InvokeReactiveDispatcher();

	UFUNCTION(BlueprintCallable, Category = "行为")
	void ReactiveDispatcher();
};
