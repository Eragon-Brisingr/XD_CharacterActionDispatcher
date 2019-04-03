// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DA_MoveTo.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

UXD_DA_MoveTo::UXD_DA_MoveTo()
{
#if WITH_EDITORONLY_DATA
	bIsPluginAction = true;
#endif
}

bool UXD_DA_MoveTo::IsActionValid() const
{
	return Pawn.Get() ? true : false;
}

void UXD_DA_MoveTo::WhenActionActived()
{
	APawn* Mover = Pawn.Get();
	RegisterEntity(Mover);
	if (AAIController* AIController = Cast<AAIController>(Mover->GetController()))
	{
		EPathFollowingRequestResult::Type Result;
		if (AActor* Target = Goal.Get())
		{
			Result = AIController->MoveToActor(Target);
		}
		else
		{
			Result = AIController->MoveToLocation(Location);
		}

		switch (Result)
		{
		case EPathFollowingRequestResult::AlreadyAtGoal:
			ExecuteEventAndFinishAction(WhenReached);
			break;
		case EPathFollowingRequestResult::RequestSuccessful:
			AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &UXD_DA_MoveTo::WhenRequestFinished);
			break;
		case EPathFollowingRequestResult::Failed:
			ExecuteEventAndFinishAction(WhenCanNotReached);
			break;
		}
	}
}

void UXD_DA_MoveTo::WhenActionDeactived()
{
	APawn* Mover = Pawn.Get();
	if (AAIController* AIController = Cast<AAIController>(Mover->GetController()))
	{
		AIController->StopMovement();
	}
}

void UXD_DA_MoveTo::WhenActionFinished()
{
	APawn* Mover = Pawn.Get();
	UnregisterEntity(Mover);
}

void UXD_DA_MoveTo::WhenRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	APawn* Mover = Pawn.Get();
	if (AAIController* AIController = Cast<AAIController>(Mover->GetController()))
	{
		AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
	}
	if (Result.Code == EPathFollowingResult::Success)
	{
		ExecuteEventAndFinishAction(WhenReached);
	}
	else if (Result.Code != EPathFollowingResult::Aborted)
	{
		ExecuteEventAndFinishAction(WhenCanNotReached);
	}
}
