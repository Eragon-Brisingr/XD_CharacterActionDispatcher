// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "ActionDispatcherBlueprint.generated.h"


/**
 * 
 */
UCLASS(MinimalAPI)
class UActionDispatcherBlueprint : public UBlueprint
{
	GENERATED_BODY()
public:

#if WITH_EDITOR
	UClass* GetBlueprintClass() const override;
	void GetReparentingRules(TSet<const UClass*>& AllowedChildrenOfClasses, TSet<const UClass*>& DisallowedChildrenOfClasses) const override;
	bool AlwaysCompileOnLoad() const override { return Status == EBlueprintStatus::BS_Dirty; }
#endif
};
