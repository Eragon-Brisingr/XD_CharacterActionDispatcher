// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "XD_BpNodeFunctionWarpper.h"
#include "XD_CharacterActionDispatcher.h"
#include "LevelSequence/Public/LevelSequenceActor.h"
#include "MovieSceneSequencePlayer.h"

void UXD_BpNodeFunctionWarpper::PlayLevelSequence(ALevelSequenceActor* LevelSequenceActor)
{
	LevelSequenceActor->SequencePlayer->Play();
}

FDispatchableActionNormalEvent UXD_BpNodeFunctionWarpper::MakeDispatchableNormalEvent(const FDispatchableActionEventDelegate& Event)
{
	FDispatchableActionNormalEvent DispatchableActionEvent;
	DispatchableActionEvent.Event = Event;
	return DispatchableActionEvent;
}

FOnDispatchableActionFinishedEvent UXD_BpNodeFunctionWarpper::MakeDispatchableActionFinishedEvent(const FDispatchableActionEventDelegate& Event)
{
	FOnDispatchableActionFinishedEvent DispatchableActionFinishedEvent;
	DispatchableActionFinishedEvent.Event = Event;
	return DispatchableActionFinishedEvent;
}
