// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DispatchableActionBase.h"
#include "XD_ActionDispatcherBase.h"

UWorld* UXD_DispatchableActionBase::GetWorld() const
{
	return Owner ? Owner->GetWorld() : nullptr;
}

void UXD_DispatchableActionBase::FinishAction()
{
	Owner->FinishAction(this);
}

TArray<FName> UXD_DispatchableActionBase::GetAllFinishedEventName() const
{
	TArray<FName> Res;
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		UStructProperty* Struct = *It;
		if (Struct->Struct->IsChildOf(FDispatchableActionFinishedEvent::StaticStruct()))
		{
			Res.Add(*Struct->GetDisplayNameText().ToString());
		}
	}
	return Res;
}

void UXD_DispatchableActionBase::BindAllFinishedEvent(const TArray<FDispatchableActionFinishedEvent>& FinishedEvents)
{
	int32 BindIdx = 0;
	for (TFieldIterator<UStructProperty> It(GetClass()); It; ++It)
	{
		UStructProperty* Struct = *It;
		if (Struct->Struct->IsChildOf(FDispatchableActionFinishedEvent::StaticStruct()))
		{
			FDispatchableActionFinishedEvent* Value = Struct->ContainerPtrToValuePtr<FDispatchableActionFinishedEvent>(this);
			*Value = FinishedEvents[BindIdx++];
		}
	}
}

void UXD_DispatchableActionExample::WhenActionActived()
{
	auto view = GetClass()->FindPropertyByName(TEXT("OnFinished"));

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UXD_DispatchableActionExample::WhenTimeFinished, DelayTime);
}

void UXD_DispatchableActionExample::WhenActionDeactived()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
}

void UXD_DispatchableActionExample::WhenTimeFinished()
{
	FinishAction();
	OnFinished.ExecuteIfBound();
}
