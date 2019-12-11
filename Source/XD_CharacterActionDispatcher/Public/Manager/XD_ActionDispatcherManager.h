// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <Components/ActorComponent.h>
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_SaveGameInterface.h"
#include "XD_ActionDispatcherManager.generated.h"

class UXD_ActionDispatcherBase;
class ULevel;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherManager : public UActorComponent, public IXD_SaveGameInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UXD_ActionDispatcherManager();

protected:
	// Called when the game starts
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool NeedSave_Implementation() const override { return true; }
	void WhenGameInit_Implementation() override;
	void WhenPreSave_Implementation() override;
	void WhenPostLoad_Implementation() override;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	UPROPERTY(SaveGame)
	TArray<UXD_ActionDispatcherBase*> ActivedDispatchers;

	UPROPERTY(SaveGame)
	TArray<UXD_ActionDispatcherBase*> PendingDispatchers;

public:
	static UXD_ActionDispatcherManager* Get(const UObject* WorldContextObject);
	
private:
#if WITH_EDITOR
	friend class UBpNode_StartDispatcherWithManager;
#endif
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
	void InvokeStartDispatcher(UXD_ActionDispatcherBase* Dispatcher);
protected:
	friend class UXD_ActionDispatcherBase;

	void WhenDispatcherStarted(UXD_ActionDispatcherBase* Dispatcher);
	void WhenDispatcherReactived(UXD_ActionDispatcherBase* Dispatcher);
	void WhenDispatcherDeactived(UXD_ActionDispatcherBase* Dispatcher);
	void WhenDispatcherFinished(UXD_ActionDispatcherBase* Dispatcher);
private:
	uint8 bEnableAutoActivePendingAction : 1;
	int32 ActivePendingActionIdx;

	void InvokeActivePendingActions();
	UFUNCTION()
	void WhenLevelLoadCompleted(ULevel* Level);
	UFUNCTION()
	void WhenPostLevelUnload();

public:
	//尝试强制激活Pending状态的调度器
	void TryActivePendingDispatcher(UXD_ActionDispatcherBase* Dispatcher);
};
