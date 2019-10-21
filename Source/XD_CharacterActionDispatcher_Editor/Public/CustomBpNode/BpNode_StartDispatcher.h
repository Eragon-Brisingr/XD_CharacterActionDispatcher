// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DA_BpNodeInterface.h"
#include "BpNode_CreateActionFromClassBase.h"
#include "BpNode_StartDispatcher.generated.h"

class UXD_ActionDispatcherBase;

/**
 * 
 */
UCLASS(abstract)
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_StartDispatcherBase : public UBpNode_AD_CreateObjectBase
{
	GENERATED_BODY()
public:
	UBpNode_StartDispatcherBase();
	void AllocateDefaultPins() override;
	void ShowExtendPins(UClass* UseSpawnClass) override;
	void PinDefaultValueChanged(UEdGraphPin* ChangedPin) override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
protected:
	UClass* GetClassPinBaseClass() const override;
	bool IsSpawnVarPin(UEdGraphPin* Pin) const override;

	UPROPERTY()
	TSubclassOf<UXD_ActionDispatcherBase> ActionDispatcherClass;

	UPROPERTY()
	TArray<FName> FinishedTags;

	void ReflushFinishExec();

	void GenerateFinishEvent(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* DispatchFinishedEventPin, const FString& EventName);

	static FName DefaultPinName;
};

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_StartDispatcherWithManager : public UBpNode_StartDispatcherBase
{
	GENERATED_BODY()
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	FName GetCornerIcon() const override { return TEXT("Graph.Latent.LatentIcon"); }
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;

	void AllocateDefaultPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

protected:
	bool IsSpawnVarPin(UEdGraphPin* Pin) const override;
	static FName LeaderPinName;
};

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_StartDispatcherWithOwner : public UBpNode_StartDispatcherBase, public IDA_BpNodeInterface
{
	GENERATED_BODY()
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	FName GetCornerIcon() const override { return TEXT("Graph.Latent.LatentIcon"); }
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;

	void AllocateDefaultPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

protected:
	static FName Dispatcher_MemberVarPinName;
	static FName OwnerPinName;

	bool IsSpawnVarPin(UEdGraphPin* Pin) const override;
};
