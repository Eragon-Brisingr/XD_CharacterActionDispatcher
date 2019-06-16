// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/XD_DispatchableActionBase.h"
#include "MovieSceneObjectBindingID.h"
#include "AITypes.h"
#include "XD_DA_PlaySequence.generated.h"

class AActor;
class APawn;
class ULevelSequence;
class AXD_ReplicableLevelSequence;
class UXD_ActionDispatcherBase;
struct FPathFollowingResult;

/**
 * 
 */
USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FPlaySequenceActorData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TSoftObjectPtr<AActor> ActorRef;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FMovieSceneObjectBindingID BindingID;
};

USTRUCT(BlueprintType)
struct XD_CHARACTERACTIONDISPATCHER_API FPlaySequenceMoveToData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TSoftObjectPtr<APawn> PawnRef;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FMovieSceneObjectBindingID BindingID;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FVector Location;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FRotator Rotation;

	UPROPERTY()
	uint8 bIsReached : 1;
};

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API UXD_DA_PlaySequenceBase : public UXD_DispatchableActionBase
{
	GENERATED_BODY()
public:
	UXD_DA_PlaySequenceBase();

	TArray<AActor*> GetAllRegistableEntities() const override;
	bool IsActionValid() const override;
	void WhenActionActived() override;
	void WhenActionDeactived() override;
	void WhenActionFinished() override;

	UPROPERTY(SaveGame)
	FOnDispatchableActionFinishedEvent WhenPlayCompleted;

	UPROPERTY(SaveGame)
	FOnDispatchableActionFinishedEvent WhenCanNotPlay;

	UFUNCTION()
	void WhenPlayFinished();
protected:
	virtual bool MoveToSequencePlayLocation(APawn* Mover, const FVector& PlayLocation, const FRotator& PlayRotation, int32 MoverIdx);

	void WhenMoveReached(int32 MoverIdx);

	void WhenMoveCanNotReached(int32 MoverIdx);

	void StopSequencePlayer();
private:
	void WhenMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result, int32 MoverIdx);
public:
	UPROPERTY(SaveGame)
	TSoftObjectPtr<ULevelSequence> LevelSequence;

	UPROPERTY(SaveGame)
	TArray<FPlaySequenceActorData> PlaySequenceActorDatas;

	UPROPERTY(SaveGame)
	TArray<FPlaySequenceMoveToData> PlaySequenceMoveToDatas;

	UPROPERTY()
	AXD_ReplicableLevelSequence* SequencePlayer;

	UPROPERTY(SaveGame)
	FTransform PlayTransform;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UXD_DA_PlaySequenceBase* PlaySequence(UXD_ActionDispatcherBase* ActionDispatcher, TSoftObjectPtr<ULevelSequence> Sequence, const TArray<FPlaySequenceActorData>& ActorDatas, const TArray<FPlaySequenceMoveToData>& MoveToDatas, const FTransform& InPlayTransform, const FOnDispatchableActionFinishedEvent& InWhenPlayEnd, const FOnDispatchableActionFinishedEvent& InWhenCanNotPlay);
};
