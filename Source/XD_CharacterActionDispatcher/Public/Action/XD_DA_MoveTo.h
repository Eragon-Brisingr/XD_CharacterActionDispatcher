// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XD_DispatchableActionBase.h"
#include "AITypes.h"
#include "XD_DA_MoveTo.generated.h"

class APawn;
struct FPathFollowingResult;

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_DA_MoveTo : public UXD_DispatchableActionBase
{
	GENERATED_BODY()
public:
	UXD_DA_MoveTo();

	bool IsActionValid() const override;
	void WhenActionActived() override;
	void WhenActionDeactived() override;
	void WhenActionFinished() override;

protected:
	UPROPERTY(SaveGame)
	FOnDispatchableActionFinishedEvent WhenReached;

	UPROPERTY(SaveGame)
	FOnDispatchableActionFinishedEvent WhenCanNotReached;

	void WhenRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);
public:
	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (ExposeOnSpawn = "true"))
	TSoftObjectPtr<APawn> Pawn;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (ExposeOnSpawn = "true"))
	FVector Location;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (ExposeOnSpawn = "true"))
	TSoftObjectPtr<AActor> Goal;
};
