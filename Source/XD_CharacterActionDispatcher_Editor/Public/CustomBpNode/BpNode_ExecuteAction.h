// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "DA_BpNodeInterface.h"
#include "BpNode_ExecuteAction.generated.h"

class UXD_DispatchableActionBase;

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_ExecuteAction : public UK2Node_ConstructObjectFromClass, public IDA_BpNodeInterface
{
	GENERATED_BODY()
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	FName GetCornerIcon() const override { return TEXT("Graph.Latent.LatentIcon"); }
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const override;

	void AllocateDefaultPins() override;
	void PinDefaultValueChanged(UEdGraphPin* ChangedPin) override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
protected:
	UClass* GetClassPinBaseClass() const override;

	void ReflushFinishExec();

	UPROPERTY()
	TSubclassOf<UXD_DispatchableActionBase> ActionClass;

public:
	bool ShouldShowNodeProperties() const override { return true; }

	UPROPERTY(EditAnywhere, Category = "调试")
	FName EntryPointEventName;
private:
	void PostPlacedNewNode() override;

};
