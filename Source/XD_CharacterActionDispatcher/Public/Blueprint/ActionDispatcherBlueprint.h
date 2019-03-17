// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "K2Node_CustomEvent.h"
#include "ActionDispatcherBlueprint.generated.h"


/**
 * 
 */
UCLASS(MinimalAPI)
class UActionDispatcherBlueprint : public UBlueprint
{
	GENERATED_BODY()
public:

#if WITH_EDITORONLY_DATA
	UClass* GetBlueprintClass() const override;
	void GetReparentingRules(TSet<const UClass*>& AllowedChildrenOfClasses, TSet<const UClass*>& DisallowedChildrenOfClasses) const override;
	bool AlwaysCompileOnLoad() const override { return Status == EBlueprintStatus::BS_Dirty; }

	UPROPERTY()
	UObject* WhenDispatchStartNode;

	UPROPERTY()
	TArray<FName> FinishTags;
#endif
};
