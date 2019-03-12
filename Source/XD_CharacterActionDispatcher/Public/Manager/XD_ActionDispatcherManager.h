// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_SaveGameInterface.h"
#include "XD_ActionDispatcherManager.generated.h"

class UXD_ActionDispatcherBase;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherManager : public UActorComponent, public IXD_SaveGameInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UXD_ActionDispatcherManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	bool NeedSave_Implementation() const override { return true; }
	void WhenPreSave_Implementation() override;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = true))
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
