// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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
	UFUNCTION(BlueprintCallable, Category = "行为")
	void InvokeStartDispatcher(UXD_ActionDispatcherBase* Dispatcher);

	UFUNCTION(BlueprintCallable, Category = "行为")
	void InvokeAbortDispatcher(UXD_ActionDispatcherBase* Dispatcher);

	UPROPERTY(SaveGame)
	TArray<UXD_ActionDispatcherBase*> ActivedDispatchers;

	UPROPERTY(SaveGame)
	TArray<UXD_ActionDispatcherBase*> PendingDispatchers;
};
