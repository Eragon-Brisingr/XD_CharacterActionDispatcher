// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ReplicableLevelSequence.h"
#include "UnrealNetwork.h"
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
}

void AXD_ReplicableLevelSequence::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
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
		SequencePlayer->Play();
	}
}

void AXD_ReplicableLevelSequence::OnRep_BindingDatas()
{
	if (!HasAuthority() && SequencePlayer && SequencePlayer->IsPlaying())
	{
		SequencePlayer->Stop();
	}

	for (const FReplicableLevelSequenceData& BindingData : BindingDatas)
	{
		if (BindingData.BindingActor)
		{
			AddBinding(BindingData.BindingID, BindingData.BindingActor);
		}
	}

	if (!HasAuthority() && SequencePlayer)
	{
		SequencePlayer->Play();
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
	}
}
