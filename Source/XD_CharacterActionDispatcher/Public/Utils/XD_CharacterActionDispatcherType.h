// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XD_CharacterActionDispatcherType.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_DELEGATE(FDispatchableActionEventDelegate);

USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FDispatchableActionEventBase
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FDispatchableActionEventDelegate Event;
};

USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FOnDispatchableActionFinishedEvent : public FDispatchableActionEventBase
{
	GENERATED_BODY()
private:
	friend class UXD_DispatchableActionBase;
	void ExecuteIfBound() const { Event.ExecuteIfBound(); }
};

USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FDispatchableActionNormalEvent : public FDispatchableActionEventBase
{
	GENERATED_BODY()
public:
	void ExecuteIfBound() const { Event.ExecuteIfBound(); }
};

DECLARE_DYNAMIC_DELEGATE(FOnDispatcherAborted);

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
