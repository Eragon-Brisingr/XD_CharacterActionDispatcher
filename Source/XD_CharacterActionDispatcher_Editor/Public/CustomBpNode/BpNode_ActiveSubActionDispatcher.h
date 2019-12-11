// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/DA_BpNodeInterface.h"
#include "CustomBpNode/BpNode_CreateActionFromClassBase.h"
#include "BpNode_ActiveSubActionDispatcher.generated.h"

class UXD_ActionDispatcherBase;

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_ActiveSubActionDispatcher : public UBpNode_AD_CreateObjectBase, public IDA_BpNodeInterface
{
	GENERATED_BODY()
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	FName GetCornerIcon() const override { return TEXT("Graph.Latent.LatentIcon"); }
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;

	void AllocateDefaultPins() override;
	void ShowExtendPins(UClass* UseSpawnClass) override;
	void PinDefaultValueChanged(UEdGraphPin* ChangedPin) override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
protected:
	UClass* GetClassPinBaseClass() const override;

	UPROPERTY()
	TSubclassOf<UXD_ActionDispatcherBase> ActionDispatcherClass;

	UPROPERTY()
	TArray<FName> FinishedTags;

	void ReflushFinishExec();

	static FName DefaultPinName;
};
