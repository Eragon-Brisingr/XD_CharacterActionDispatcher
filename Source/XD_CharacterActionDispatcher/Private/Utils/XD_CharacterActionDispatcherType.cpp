// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_CharacterActionDispatcherType.h"

void FDA_RoleSelection::ExecuteIfBound() const
{
	NativeOnSelected.ExecuteIfBound();
	WhenSelected.ExecuteIfBound();
}
