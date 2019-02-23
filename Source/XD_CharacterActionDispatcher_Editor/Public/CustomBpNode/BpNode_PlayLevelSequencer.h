// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "MovieSceneObjectBindingID.h"
#include "IPropertyTypeCustomization.h"
#include "BpNode_PlayLevelSequencer.generated.h"

class UMovieSceneSequence;

/**
 * 
 */
USTRUCT()
struct FSequencerBindingOption
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, Category = "Sequencer")
	FString DisplayName;

	UPROPERTY()
	FMovieSceneObjectBindingID Binding;

	UPROPERTY(EditAnywhere, Category = "Sequencer")
	uint8 bIsPin : 1;
};

struct FSequencerBindingOption_Customization : public IPropertyTypeCustomization
{
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FSequencerBindingOption_Customization());
	}

	void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override{}
};

UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_PlayLevelSequencer : public UK2Node
{
	GENERATED_BODY()
public:
 	UPROPERTY(EditAnywhere, Category = "Sequence", meta = (DisplayName = "播放的Sequencer"))
 	TSoftObjectPtr<UMovieSceneSequence> Sequencer;

	UPROPERTY(EditAnywhere, Category = "Sequence", EditFixedSize, meta = (DisplayName = "可绑定对象"))
	TArray<FSequencerBindingOption> BindingOptions;

	TSharedPtr<struct FSequenceBindingTree> DataTree;

	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
public:
	bool ShouldShowNodeProperties() const override { return true; }
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	void AllocateDefaultPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
private:
	void UpdatePinInfo(const FSequencerBindingOption &Option);

	void ReflushNode();
};
