// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_ActionDispatcherLibrary.generated.h"

class UXD_ActionDispatcherBase;


UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "行为", BlueprintAuthorityOnly, meta = (BlueprintInternalUseOnly = true))
	static UXD_ActionDispatcherBase* GetOrCreateDispatcherWithOwner(UObject* Owner, TSubclassOf<UXD_ActionDispatcherBase> Dispatcher, UPARAM(Ref)UXD_ActionDispatcherBase*& Dispatcher_MemberVar);

	UFUNCTION(BlueprintPure, Category = "行为", meta = (WorldContext = "WorldContextObject"))
	static UXD_ActionDispatcherManager* GetActionDispatcherManager(const UObject* WorldContextObject);
};
