// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_CharacterActionDispatcherType.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_DELEGATE(FDispatchableActionFinished);

USTRUCT(BlueprintType, BlueprintInternalUseOnly)
struct XD_CHARACTERACTIONDISPATCHER_API FDispatchableActionFinishedEvent
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FDispatchableActionFinished Event;

	void ExecuteIfBound() const { Event.ExecuteIfBound(); }
};

USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FDA_RoleSelection
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame, BlueprintReadWrite, meta = (DisplayName = "文本"))
	FText Selection;

	UPROPERTY(SaveGame)
	FDispatchableActionFinishedEvent WhenSelected;

	DECLARE_DELEGATE(FNativeOnSelected);
	FNativeOnSelected NativeOnSelected;

	void ExecuteIfBound() const;
};

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherTypeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "行为")
	static void ExecuteSelectedEvent(const FDA_RoleSelection& Event);
};
