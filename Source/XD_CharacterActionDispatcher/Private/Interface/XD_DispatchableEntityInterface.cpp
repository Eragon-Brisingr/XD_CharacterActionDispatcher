// Fill out your copyright notice in the Description page of Project Settings.

#include "Interface/XD_DispatchableEntityInterface.h"

// Add default functionality here for any IXD_DispatchableEntityInterface functions that are not pure virtual.

bool UXD_DA_StateTagUtils::HasStateTag(UObject* Obj, FGameplayTag Tag)
{
	if (Obj && Obj->Implements<UXD_DispatchableEntityInterface>())
	{
		return IXD_DispatchableEntityInterface::Execute_AD_HasStateTag(Obj, Tag);
	}
	return false;
}

void UXD_DA_StateTagUtils::AddStateTag(UObject* Obj, FGameplayTag Tag)
{
	if (Obj && Obj->Implements<UXD_DispatchableEntityInterface>())
	{
		return IXD_DispatchableEntityInterface::Execute_AD_AddStateTag(Obj, Tag);
	}
}
