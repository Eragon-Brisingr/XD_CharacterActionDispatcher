// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "BpNode_ExecuteAction.generated.h"

class UXD_DispatchableActionBase;

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_ExecuteAction : public UK2Node_ConstructObjectFromClass
{
	GENERATED_BODY()
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	FName GetCornerIcon() const override { return TEXT("Graph.Latent.LatentIcon"); }
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	void AllocateDefaultPins() override;
	void PinDefaultValueChanged(UEdGraphPin* ChangedPin) override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
protected:
	UClass* GetClassPinBaseClass() const override;

	void ReflushFinishExec();

	UPROPERTY()
	TSubclassOf<UXD_DispatchableActionBase> ActionClass;
};
