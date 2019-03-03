// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_ActionDispatcherManager.generated.h"

class UXD_ActionDispatcherBase;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UXD_ActionDispatcherManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	void InvokeStartDispatcher(UXD_ActionDispatcherBase* Dispatcher);

	void InvokeAbortDispatcher(UXD_ActionDispatcherBase* Dispatcher);

	UPROPERTY(SaveGame)
	TArray<UXD_ActionDispatcherBase*> ActivedDispatchers;

	UPROPERTY(SaveGame)
	TArray<UXD_ActionDispatcherBase*> PendingDispatchers;

public:
	static UXD_ActionDispatcherManager* Get(const UObject* WorldContextObject);

protected:
	friend class UXD_ActionDispatcherBase;
	void FinishDispatcher(UXD_ActionDispatcherBase* Dispatcher);
};

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	//TODO 建立K2Node帮助ActionDispatcher初始化
	UFUNCTION(BlueprintCallable, Category = "行为", BlueprintAuthorityOnly, meta = (WorldContext = "WorldContextObject"))
	static void InvokeStartDispatcher(TSubclassOf<UXD_ActionDispatcherBase> ActionDispatcher, const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "行为", meta = (WorldContext = "WorldContextObject"))
	static UXD_ActionDispatcherManager* GetActionDispatcherManager(const UObject* WorldContextObject);
};
