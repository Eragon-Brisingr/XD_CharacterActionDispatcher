// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_CharacterActionDispatcherType.h"

void UXD_ActionDispatcherTypeLibrary::ExecuteSelectedEvent(const FDA_RoleSelection& Event)
{
	Event.ExecuteIfBound();
}

void FDA_RoleSelection::ExecuteIfBound() const
{
	NativeOnSelected.ExecuteIfBound();
	WhenSelected.ExecuteIfBound();
}
