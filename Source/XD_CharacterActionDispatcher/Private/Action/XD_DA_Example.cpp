// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DA_Example.h"


UXD_DA_Example::UXD_DA_Example()
{
#if WITH_EDITORONLY_DATA
	bIsPluginAction = true;
#endif
}

bool UXD_DA_Example::IsActionValid() const
{
	return true;
}

void UXD_DA_Example::WhenActionActived()
{
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UXD_DA_Example::WhenTimeFinished, DelayTime);
}

void UXD_DA_Example::WhenActionDeactived()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
}

void UXD_DA_Example::WhenTimeFinished()
{
	ExecuteEventAndFinishAction(OnFinished);
}
