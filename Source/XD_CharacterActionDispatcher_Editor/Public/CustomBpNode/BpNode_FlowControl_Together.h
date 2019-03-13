// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "DA_BpNodeInterface.h"
#include "BpNode_FlowControl_Together.generated.h"

class UEdGraphPin;

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_FlowControl_Together : public UK2Node, public IDA_BpNodeInterface
{
	GENERATED_BODY()
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	FName GetCornerIcon() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const override;

	void AllocateDefaultPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
private:
	UPROPERTY()
	int32 TogetherEventCount = 2;

	void AddExecPin();
	void AddExecPinImpl(int32 Idx);

	void RemoveExecPin(const UEdGraphPin* Pin);

	void ReflushExecPinName();

	FName GetExecPinName(int32 Idx);
	FName GetWaitPinName(int32 Idx);

	TMap<UEdGraphPin*, UEdGraphPin*> TogetherPins;

	void ReflushNode();
};
