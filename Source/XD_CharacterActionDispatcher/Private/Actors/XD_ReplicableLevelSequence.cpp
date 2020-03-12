// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/XD_ReplicableLevelSequence.h"
#include <Net/UnrealNetwork.h>
#include "DefaultLevelSequenceInstanceData.h"

AXD_ReplicableLevelSequence::AXD_ReplicableLevelSequence(const FObjectInitializer& Init)
	:Super(Init)
{
	bReplicates = true;
	SetReplicatingMovement(true);
	bOverrideInstanceData = true;
	bReplicatePlayback = true;

	UDefaultLevelSequenceInstanceData* LevelSequenceInstanceData = Cast<UDefaultLevelSequenceInstanceData>(DefaultInstanceData);
	LevelSequenceInstanceData->TransformOriginActor = this;

	SequencePlayer->OnStop.AddDynamic(this, &AXD_ReplicableLevelSequence::WhenPlayEnd);
}

void AXD_ReplicableLevelSequence::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AXD_ReplicableLevelSequence, LevelSequenceRef);
	DOREPLIFETIME(AXD_ReplicableLevelSequence, BindingDatas);
}

void AXD_ReplicableLevelSequence::BeginPlay()
{
	Super::BeginPlay();

}

void AXD_ReplicableLevelSequence::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

}

void AXD_ReplicableLevelSequence::OnRep_LevelSequence()
{
	if (LevelSequenceRef)
	{
		SetSequence(LevelSequenceRef);
	}
}

void AXD_ReplicableLevelSequence::OnRep_BindingDatas()
{
	ensure(!SequencePlayer->IsPlaying());

	for (const FReplicableLevelSequenceData& BindingData : BindingDatas)
	{
		if (BindingData.BindingActor)
		{
			AddBinding(BindingData.BindingID, BindingData.BindingActor);
		}
	}
}

void AXD_ReplicableLevelSequence::Play(ULevelSequence* Sequence, const FTransform& PlayTransform, const TArray<FReplicableLevelSequenceData>& Data)
{
	if (Sequence)
	{
		SetActorTransform(PlayTransform);
		LevelSequenceRef = Sequence;
		BindingDatas = Data;
		OnRep_BindingDatas();
		OnRep_LevelSequence();
		SequencePlayer->Play();
	}
}
