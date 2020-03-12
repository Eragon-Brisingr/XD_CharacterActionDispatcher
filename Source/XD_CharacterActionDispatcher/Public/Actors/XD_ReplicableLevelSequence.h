// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LevelSequenceActor.h"
#include "XD_ReplicableLevelSequence.generated.h"

class ULevelSequence;

/**
 * 
 */
USTRUCT(BlueprintType, BlueprintInternalUseOnly)
struct XD_CHARACTERACTIONDISPATCHER_API FReplicableLevelSequenceData
{
	GENERATED_BODY()
public:
	FReplicableLevelSequenceData() = default;
	FReplicableLevelSequenceData(const FMovieSceneObjectBindingID& BindingID, AActor* BindingActor)
		:BindingID(BindingID), BindingActor(BindingActor)
	{}

	UPROPERTY(BlueprintReadOnly)
	FMovieSceneObjectBindingID BindingID;

	UPROPERTY(BlueprintReadOnly)
	AActor* BindingActor;
};

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_API AXD_ReplicableLevelSequence : public ALevelSequenceActor
{
	GENERATED_BODY()
public:
	AXD_ReplicableLevelSequence(const FObjectInitializer& Init);

	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;

	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(ReplicatedUsing = OnRep_LevelSequence)
	ULevelSequence* LevelSequenceRef;
	UFUNCTION()
	void OnRep_LevelSequence();

	UPROPERTY(ReplicatedUsing = OnRep_BindingDatas)
	TArray<FReplicableLevelSequenceData> BindingDatas;
	UFUNCTION()
	void OnRep_BindingDatas();

	UFUNCTION(BlueprintCallable)
	void Play(ULevelSequence* Sequence, const FTransform& PlayTransform, const TArray<FReplicableLevelSequenceData>& Data);

	UFUNCTION()
	virtual void WhenPlayEnd() {}
};
