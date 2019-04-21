// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionDispatcherBlueprint.h"
#include "ActionDispatcherGeneratedClass.h"
#include "XD_ActionDispatcherBase.h"
#include "UObjectHash.h"

#if WITH_EDITOR
UClass* UActionDispatcherBlueprint::GetBlueprintClass() const
{
	return UActionDispatcherGeneratedClass::StaticClass();
}

void UActionDispatcherBlueprint::GetReparentingRules(TSet<const UClass*>& AllowedChildrenOfClasses, TSet<const UClass*>& DisallowedChildrenOfClasses) const
{
	AllowedChildrenOfClasses.Add(UXD_ActionDispatcherBase::StaticClass());
}
#endif