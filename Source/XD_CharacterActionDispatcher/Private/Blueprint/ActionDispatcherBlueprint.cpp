// Fill out your copyright notice in the Description page of Project Settings.

#include "Blueprint/ActionDispatcherBlueprint.h"
#include <UObject/UObjectHash.h>
#include "Blueprint/ActionDispatcherGeneratedClass.h"
#include "Dispatcher/XD_ActionDispatcherBase.h"

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