// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
