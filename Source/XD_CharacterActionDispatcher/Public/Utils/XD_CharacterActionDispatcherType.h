// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_CharacterActionDispatcherType.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_DELEGATE(FOnDispatchableActionFinished);

USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FOnDispatchableActionFinishedEvent
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FOnDispatchableActionFinished Event;

	void ExecuteIfBound() const { Event.ExecuteIfBound(); }
};

DECLARE_DYNAMIC_DELEGATE(FOnActionDispatcherAborted);
USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FOnActionDispatcherAbortedEvent
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FOnActionDispatcherAborted Event;

	DECLARE_DELEGATE_OneParam(FOnDispatchableActionAborted, const FOnActionDispatcherAborted&);
	FOnDispatchableActionAborted OnDispatchableActionAborted;

	void ExecuteIfBound() const { OnDispatchableActionAborted.ExecuteIfBound(Event); }
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
	FOnDispatchableActionFinishedEvent WhenSelected;

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
	Finished = 2,
	Aborting = 3
};

UENUM()
enum class EActionDispatcherState : uint8
{
	Deactive = 0,
	Active = 1,
	Aborting = 2
};
