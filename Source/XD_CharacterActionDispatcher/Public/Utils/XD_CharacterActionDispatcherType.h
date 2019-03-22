// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_CharacterActionDispatcherType.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_DELEGATE(FDispatchableActionFinished);

USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FDispatchableActionFinishedEvent
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FDispatchableActionFinished Event;

	void ExecuteIfBound() const { Event.ExecuteIfBound(); }
};

USTRUCT(BlueprintType, BlueprintInternalUseOnly)
struct XD_CHARACTERACTIONDISPATCHER_API FDA_RoleSelectionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame, BlueprintReadWrite, meta = (DisplayName = "文本"))
	FText Selection;
};

USTRUCT(BlueprintType, BlueprintInternalUseOnly)
struct XD_CHARACTERACTIONDISPATCHER_API FDA_RoleSelection : public FDA_RoleSelectionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FDispatchableActionFinishedEvent WhenSelected;

	DECLARE_DELEGATE(FNativeOnSelected);
	FNativeOnSelected NativeOnSelected;

	void ExecuteIfBound() const;
};

USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FDA_DisplaySelection : public FDA_RoleSelectionBase
{
	GENERATED_BODY()
public:
	FDA_DisplaySelection() = default;
	FDA_DisplaySelection(const FDA_RoleSelection& Selection)
		:FDA_RoleSelectionBase(static_cast<const FDA_RoleSelectionBase&>(Selection))
	{}

	UPROPERTY()
	int32 SelectionIdx;
};

UENUM()
enum class EDispatchableActionState : uint8
{
	Deactive = 0,
	Active = 1,
	Finished = 2
};
