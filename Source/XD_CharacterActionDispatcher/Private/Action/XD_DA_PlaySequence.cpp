// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DA_PlaySequence.h"
#include "GameFramework/Pawn.h"
#include "XD_ReplicableLevelSequence.h"
#include "XD_ActionDispatcherBase.h"
#include "AIController.h"
#include "XD_ActionDispatcherSettings.h"

UXD_DA_PlaySequenceBase::UXD_DA_PlaySequenceBase()
{
#if WITH_EDITORONLY_DATA
	bIsPluginAction = true;
	bShowInExecuteActionNode = false;
#endif
}

void UXD_DA_PlaySequenceBase::WhenActionActived()
{
	for (const FPlaySequenceActorData& Data : PlaySequenceActorDatas)
	{
		AActor* BindingActor = Data.ActorRef.Get();
		RegisterEntity(BindingActor);
	}
	for (int32 i = 0; i < PlaySequenceMoveToDatas.Num(); ++i)
	{
		const FPlaySequenceMoveToData& Data = PlaySequenceMoveToDatas[i];
		APawn* Mover = Data.PawnRef.Get();
		RegisterEntity(Mover);
		FVector PlayLocation = PlayTransform.TransformPosition(Data.Location);
		FRotator PlayRotation = PlayTransform.TransformRotation(Data.Rotation.Quaternion()).Rotator();
		MoveToSequencePlayLocation(Mover, PlayLocation, PlayRotation, i);
	}
}

void UXD_DA_PlaySequenceBase::WhenActionDeactived()
{
	if (SequencePlayer)
	{
		ULevelSequencePlayer* Player = SequencePlayer->SequencePlayer;
		Player->OnStop.RemoveDynamic(this, &UXD_DA_PlaySequenceBase::WhenPlayFinished);
		Player->Stop();
	}

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
	for (const FPlaySequenceActorData& Data : PlaySequenceActorDatas)
	{
		UnregisterEntity(Data.ActorRef.Get());
	}
	for (const FPlaySequenceMoveToData& Data : PlaySequenceMoveToDatas)
	{
		UnregisterEntity(Data.PawnRef.Get());
	}
	SequencePlayer->Destroy();
}

void UXD_DA_PlaySequenceBase::WhenPlayFinished()
{
	FinishAction();
	WhenPlayCompleted.ExecuteIfBound();
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

		SequencePlayer->Play(LevelSequence.LoadSynchronous(), PlayTransform, PlayData);
		ULevelSequencePlayer* Player = SequencePlayer->SequencePlayer;
		Player->OnStop.AddDynamic(this, &UXD_DA_PlaySequenceBase::WhenPlayFinished);
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
}

UXD_DA_PlaySequenceBase* UXD_DA_PlaySequenceBase::PlaySequence(UXD_ActionDispatcherBase* ActionDispatcher, TSoftObjectPtr<ULevelSequence> Sequence, const TArray<FPlaySequenceActorData>& ActorDatas, const TArray<FPlaySequenceMoveToData>& MoveToDatas, const FTransform& InPlayTransform, FDispatchableActionFinishedEvent InWhenPlayEnd)
{
	UXD_DA_PlaySequenceBase* DA_PlaySequence = NewObject<UXD_DA_PlaySequenceBase>(ActionDispatcher, GetDefault<UXD_ActionDispatcherSettings>()->PlaySequenceImplClass);
	DA_PlaySequence->LevelSequence = Sequence;
	DA_PlaySequence->PlaySequenceActorDatas = ActorDatas;
	DA_PlaySequence->PlaySequenceMoveToDatas = MoveToDatas;
	DA_PlaySequence->PlayTransform = InPlayTransform;
	DA_PlaySequence->WhenPlayCompleted = InWhenPlayEnd;
	ActionDispatcher->ActiveAction(DA_PlaySequence, {});
	return DA_PlaySequence;
}

