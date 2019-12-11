// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Utils/XD_CharacterActionDispatcherType.h"
#include "XD_BpNodeFunctionWarpper.generated.h"

class ALevelSequenceActor;

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_BpNodeFunctionWarpper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static void PlayLevelSequence(ALevelSequenceActor* LevelSequenceActor);

	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = "true"))
	static FDispatchableActionNormalEvent MakeDispatchableNormalEvent(const FDispatchableActionEventDelegate& Event);

	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = "true"))
	static FOnDispatchableActionFinishedEvent MakeDispatchableActionFinishedEvent(const FDispatchableActionEventDelegate& Event);
};
