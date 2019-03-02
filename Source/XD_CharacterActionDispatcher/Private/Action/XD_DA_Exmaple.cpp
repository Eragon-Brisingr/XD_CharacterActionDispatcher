// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DA_Exmaple.h"


UXD_DA_Exmaple::UXD_DA_Exmaple()
{
#if WITH_EDITORONLY_DATA
	bIsPluginAction = true;
#endif
}

void UXD_DA_Exmaple::WhenActionActived()
{
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UXD_DA_Exmaple::WhenTimeFinished, DelayTime);
}

void UXD_DA_Exmaple::WhenActionDeactived()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
}

void UXD_DA_Exmaple::WhenTimeFinished()
{
	FinishAction();
	OnFinished.ExecuteIfBound();
}
