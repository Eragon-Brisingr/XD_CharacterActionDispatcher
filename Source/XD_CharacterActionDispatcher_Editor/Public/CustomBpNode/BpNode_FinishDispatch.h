// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "GameplayTagContainer.h"
#include "BpNode_FinishDispatch.generated.h"

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_FinishDispatch : public UK2Node
{
	GENERATED_BODY()
public:
	bool ShouldShowNodeProperties() const override { return true; }
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FLinearColor GetNodeTitleColor() const;
	FText GetTooltipText() const override;
	FText GetMenuCategory() const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;

	void AllocateDefaultPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

protected:
	UPROPERTY(EditAnywhere, Category = "行为", meta = (DisplayName = "结束事件标签"))
	FGameplayTag Tag;
};
