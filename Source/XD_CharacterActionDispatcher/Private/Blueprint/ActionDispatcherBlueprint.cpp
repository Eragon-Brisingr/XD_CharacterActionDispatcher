// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionDispatcherBlueprint.h"
#include "ActionDispatcherGeneratedClass.h"

#if WITH_EDITOR
UClass* UActionDispatcherBlueprint::GetBlueprintClass() const
{
	return UActionDispatcherGeneratedClass::StaticClass();
}

void UActionDispatcherBlueprint::GetReparentingRules(TSet<const UClass*>& AllowedChildrenOfClasses, TSet<const UClass*>& DisallowedChildrenOfClasses) const
{
	AllowedChildrenOfClasses.Add(UActionDispatcherGeneratedClass::StaticClass());
}
#endif