// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "MovieSceneObjectBindingID.h"
#include "IPropertyTypeCustomization.h"
#include "DA_BpNodeInterface.h"
#include "BpNode_PlayLevelSequencer.generated.h"

class ULevelSequence;

/**
 * 
 */
USTRUCT()
struct FSequencerBindingOption
{
	GENERATED_BODY()
public:
	FSequencerBindingOption()
		:bIsPin(false)
	{}

	UPROPERTY(VisibleAnywhere, Category = "Sequencer")
	FString PinName;

	UPROPERTY()
	FMovieSceneObjectBindingID Binding;

	UPROPERTY()
	UClass* BindingClass;

	UPROPERTY()
	FVector Location = InvalidLocation;

	UPROPERTY()
	FRotator Rotation = InvalidRotation;

	UPROPERTY(EditAnywhere, Category = "Sequencer")
	uint8 bIsPin : 1;

	static const FVector InvalidLocation;
	static const FRotator InvalidRotation;

	bool IsPositionValid() const { return Location != InvalidLocation && Rotation != InvalidRotation; }
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
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_PlayLevelSequencer : public UK2Node, public IDA_BpNodeInterface
{
	GENERATED_BODY()
public:
 	UPROPERTY(EditAnywhere, Category = "Sequence", meta = (DisplayName = "播放的Sequencer"))
 	TSoftObjectPtr<ULevelSequence> LevelSequence;

	UPROPERTY(EditAnywhere, Category = "Sequence", EditFixedSize, meta = (DisplayName = "可绑定对象"))
	TArray<FSequencerBindingOption> BindingOptions;

	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
public:
	bool ShouldShowNodeProperties() const override { return true; }	
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FText GetMenuCategory() const override;
	FName GetCornerIcon() const override { return TEXT("Graph.Latent.LatentIcon"); }
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;

	void AllocateDefaultPins() override;
	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
private:
	void UpdatePinInfo(const FSequencerBindingOption &Option);

	void ReflushNode();

	static FName PlayLocationPinName;
	static FName WhenPlayCompletedPinName;
	static FName WhenCanNotPlayPinName;
	static FName RetureValuePinName;

	//刷新Seqeuence可绑定的数据
	UFUNCTION(Category = "Sequence", meta = (DisplayName = "刷新Sequence数据", CallInEditor = true))
	void RefreshSequenceData();

public:
	UPROPERTY(EditAnywhere, Category = "调试")
	FName EntryPointEventName;
private:
	void PostPlacedNewNode() override;
};
