// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "BpNode_CheckStateTag.generated.h"

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_CheckStateTag : public UK2Node
{
	GENERATED_BODY()
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	void AllocateDefaultPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	bool CanDuplicateNode() const override { return false; }
	void PostPlacedNewNode() override;
	void PinConnectionListChanged(UEdGraphPin* Pin) override;
protected:
	friend class UBpNode_AddStateTag;
	static FName TagExistPinName;
	static FName TargetPinName;
	static FName TagPinName;
	static FName TagNotExistPinName;
	static FName LinkStateTagPinName;

	FDelegateHandle OnGraphNodeChangeHandle;

	UPROPERTY()
	UBpNode_AddStateTag* LinkedAddStateTagNode;
};

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_AddStateTag : public UK2Node
{
	GENERATED_BODY()
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	void AllocateDefaultPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	bool CanDuplicateNode() const override { return false; }
	bool CanUserDeleteNode() const override { return false; }
	void PinConnectionListChanged(UEdGraphPin* Pin) override;
protected:
	friend class UBpNode_CheckStateTag;

	UPROPERTY()
	UBpNode_CheckStateTag* LinkedAddStateTagNode;
};
