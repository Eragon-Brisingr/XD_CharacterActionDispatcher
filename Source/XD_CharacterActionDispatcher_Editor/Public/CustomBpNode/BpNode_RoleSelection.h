// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DA_BpNodeInterface.h"
#include "BpNode_CreateActionFromClassBase.h"
#include "BpNode_RoleSelection.generated.h"

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_RoleSelection : public UBpNode_CreateActionFromClassBase, public IDA_BpNodeInterface
{
	GENERATED_BODY()
public:
	UBpNode_RoleSelection();

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	FName GetCornerIcon() const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const override;

	void AllocateDefaultPins() override;
	void ShowExtendPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	void Serialize(FArchive& Ar) override;

	UClass* GetClassPinBaseClass() const override;
protected:
	UPROPERTY()
	int32 SelectionNum = 2;

	UPROPERTY()
	UScriptStruct* SelectionStructType;

	struct FSelectionPin
	{
		UEdGraphPin* SelectionPin;
		UEdGraphPin* ExecPin;
	};
	TArray<FSelectionPin> SelectionPins;

	void AddSelection();
	void AddSelectionImpl(int32 Idx);
	void RemoveSelection(const UEdGraphPin* SelectionPin);

	FName GetSelectionPinName(int32 Idx) const;
	FName GetShowSelectionConditionName(int32 Idx) const;
	FName GetExecPinName(int32 Idx) const;
public:
	bool ShouldShowNodeProperties() const override { return true; }
	void PostPasteNode() override;
	void PostPlacedNewNode() override;

	UPROPERTY(EditAnywhere, Category = "调试")
	FName EntryPointEventName;
};
