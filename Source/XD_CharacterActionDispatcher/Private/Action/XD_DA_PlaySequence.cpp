// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/XD_DA_PlaySequence.h"
#include <GameFramework/Pawn.h>
#include <AIController.h>
#include "Actors/XD_ReplicableLevelSequence.h"
#include "Dispatcher/XD_ActionDispatcherBase.h"

UXD_DA_PlaySequenceBase::UXD_DA_PlaySequenceBase()
{
#if WITH_EDITORONLY_DATA
	bIsPluginAction = true;
	bShowInExecuteActionNode = false;
#endif
}

TSet<AActor*> UXD_DA_PlaySequenceBase::GetAllRegistableEntities() const
{
	TSet<AActor*> Entities;
	for (const FPlaySequenceActorData& Data : PlaySequenceActorDatas)
	{
		Entities.Add(Data.ActorRef.Get());
	}
	for (const FPlaySequenceMoveToData& Data : PlaySequenceMoveToDatas)
	{
		Entities.Add(Data.PawnRef.Get());
	}
	return Entities;
}

bool UXD_DA_PlaySequenceBase::IsActionValid() const
{
	for (const FPlaySequenceActorData& Data : PlaySequenceActorDatas)
	{
		if (Data.ActorRef.IsValid() == false)
		{
			return false;
		}
	}
	for (const FPlaySequenceMoveToData& Data : PlaySequenceMoveToDatas)
	{
		if (Data.PawnRef.IsValid() == false)
		{
			return false;
		}
	}
	return true;
}

void UXD_DA_PlaySequenceBase::WhenActionActived()
{
	for (int32 i = 0; i < PlaySequenceMoveToDatas.Num(); ++i)
	{
		const FPlaySequenceMoveToData& Data = PlaySequenceMoveToDatas[i];
		APawn* Mover = Data.PawnRef.Get();
		FVector PlayLocation = PlayTransform.TransformPosition(Data.Location);
		FRotator PlayRotation = PlayTransform.TransformRotation(Data.Rotation.Quaternion()).Rotator();
		MoveToSequencePlayLocation(Mover, PlayLocation, PlayRotation, i);
	}
}

void UXD_DA_PlaySequenceBase::WhenActionDeactived()
{
	StopSequencePlayer();

	for (const FPlaySequenceMoveToData& Data : PlaySequenceMoveToDatas)
	{
		APawn* Mover = Data.PawnRef.Get();
		if (AAIController* AIController = Cast<AAIController>(Mover->GetController()))
		{
			AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
			AIController->StopMovement();
		}
	}
}

void UXD_DA_PlaySequenceBase::WhenActionFinished()
{

}

void UXD_DA_PlaySequenceBase::WhenSequencerPlayFinished()
{
	SequencePlayer->SequencePlayer->OnStop.RemoveDynamic(this, &UXD_DA_PlaySequenceBase::WhenSequencerPlayFinished);
	SequencePlayer->Destroy();

	ExecuteEventAndFinishAction(WhenPlayCompleted);
}

bool UXD_DA_PlaySequenceBase::MoveToSequencePlayLocation(APawn* Mover, const FVector& PlayLocation, const FRotator& PlayRotation, int32 MoverIdx)
{
	if (AAIController* AIController = Cast<AAIController>(Mover->GetController()))
	{
		AIController->MoveToLocation(PlayLocation);
		AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &UXD_DA_PlaySequenceBase::WhenMoveFinished, MoverIdx);
		return true;
	}
	return false;
}

void UXD_DA_PlaySequenceBase::WhenMoveReached(int32 MoverIdx)
{
	PlaySequenceMoveToDatas[MoverIdx].bIsReached = true;
	if (!PlaySequenceMoveToDatas.ContainsByPredicate([](const FPlaySequenceMoveToData& E) {return E.bIsReached == false; }))
	{
		TArray<FReplicableLevelSequenceData> PlayData;
		for (const FPlaySequenceActorData& Data : PlaySequenceActorDatas)
		{
			AActor* BindingActor = Data.ActorRef.Get();
			PlayData.Add(FReplicableLevelSequenceData(Data.BindingID, BindingActor));
		}
		for (FPlaySequenceMoveToData& Data : PlaySequenceMoveToDatas)
		{
			APawn* BindingPawn = Data.PawnRef.Get();
			PlayData.Add(FReplicableLevelSequenceData(Data.BindingID, BindingPawn));
		}

		if (!SequencePlayer)
		{
			SequencePlayer = GetWorld()->SpawnActor<AXD_ReplicableLevelSequence>();
		}
		PrePlaySequencer();
		SequencePlayer->Play(LevelSequence.LoadSynchronous(), PlayTransform, PlayData);
		ULevelSequencePlayer* Player = SequencePlayer->SequencePlayer;
		Player->OnStop.AddUniqueDynamic(this, &UXD_DA_PlaySequenceBase::WhenSequencerPlayFinished);
	}
}

void UXD_DA_PlaySequenceBase::WhenMoveCanNotReached(int32 MoverIdx)
{
	if (State == EDispatchableActionState::Active && State != EDispatchableActionState::Finished)
	{
		ExecuteEventAndFinishAction(WhenCanNotPlay);
	}
}

void UXD_DA_PlaySequenceBase::StopSequencePlayer()
{
	if (SequencePlayer)
	{
		ULevelSequencePlayer* Player = SequencePlayer->SequencePlayer;
		if (Player->IsPlaying())
		{
			Player->OnStop.RemoveDynamic(this, &UXD_DA_PlaySequenceBase::WhenSequencerPlayFinished);
			Player->Stop();
		}
	}
}

void UXD_DA_PlaySequenceBase::WhenMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result, int32 MoverIdx)
{
	const FPlaySequenceMoveToData& MoveData = PlaySequenceMoveToDatas[MoverIdx];
	if (AAIController* AIController = Cast<AAIController>(MoveData.PawnRef->GetController()))
	{
		AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
	}
	if (Result.Code == EPathFollowingResult::Success)
	{
		WhenMoveReached(MoverIdx);
	}
	else
	{
		WhenMoveCanNotReached(MoverIdx);
	}
}

UXD_DA_PlaySequenceBase* UXD_DA_PlaySequenceBase::CreatePlaySequenceAction(TSubclassOf<UXD_DA_PlaySequenceBase> SequenceType, UXD_ActionDispatcherBase* ActionDispatcher, TSoftObjectPtr<ULevelSequence> Sequence, const TArray<FPlaySequenceActorData>& ActorDatas, const TArray<FPlaySequenceMoveToData>& MoveToDatas)
{
	UXD_DA_PlaySequenceBase* DA_PlaySequence = NewObject<UXD_DA_PlaySequenceBase>(ActionDispatcher, SequenceType);
	DA_PlaySequence->LevelSequence = Sequence;
	DA_PlaySequence->PlaySequenceActorDatas = ActorDatas;
	DA_PlaySequence->PlaySequenceMoveToDatas = MoveToDatas;
	return DA_PlaySequence;
}

