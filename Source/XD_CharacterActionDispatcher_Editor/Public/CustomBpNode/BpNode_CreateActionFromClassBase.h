// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "BpNode_CreateActionFromClassBase.generated.h"

class FBlueprintActionDatabaseRegistrar;
class UEdGraph;

UCLASS(abstract)
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_CreateActionFromClassBase : public UK2Node
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface.
	void AllocateDefaultPins() override;
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	FText GetTooltipText() const override;
	FText GetKeywords() const override;
	bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	void PinConnectionListChanged(UEdGraphPin* Pin) override;
	void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	void PostPlacedNewNode() override;
	void AddSearchMetaDataInfo(TArray<struct FSearchTagDataPair>& OutTaggedMetaData) const override;
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface
	bool IsNodeSafeToIgnore() const override { return true; }
	void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	void GetNodeAttributes( TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes ) const override;
	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	FText GetMenuCategory() const override;
	//~ End UK2Node Interface

	/** Create new pins to show properties on archetype */
	void CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins = nullptr);

	/** See if this is a spawn variable pin, or a 'default' pin */
	virtual bool IsSpawnVarPin(UEdGraphPin* Pin) const;

	/** Get the then output pin */
	UEdGraphPin* GetThenPin() const;
	/** Get the blueprint input pin */	
	UEdGraphPin* GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;
	/** Get the world context input pin, can return NULL */
	UEdGraphPin* GetWorldContextPin() const;
	/** Get the result output pin */
	UEdGraphPin* GetResultPin() const;
	/** Get the result input pin */
	UEdGraphPin* GetOuterPin() const;

	/** Get the class that we are going to spawn, if it's defined as default value */
	UClass* GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;

	/** Returns if the node uses World Object Context input */
	virtual bool UseWorldContext() const;

	/** Returns if the node uses Outer input */
	virtual bool UseOuter() const { return false; }

protected:
	/** Gets the default node title when no class is selected */
	virtual FText GetBaseNodeTitle() const;
	/** Gets the node title when a class has been selected. */
	virtual FText GetNodeTitleFormat() const;
	/** Gets base class to use for the 'class' pin.  UObject by default. */
	virtual UClass* GetClassPinBaseClass() const;

	/**
	 * Takes the specified "MutatablePin" and sets its 'PinToolTip' field (according
	 * to the specified description)
	 * 
	 * @param   MutatablePin	The pin you want to set tool-tip text on
	 * @param   PinDescription	A string describing the pin's purpose
	 */
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const;

	/** Refresh pins when class was changed */
	void OnClassPinChanged();

	/** Tooltip text for this node. */
	FText NodeTooltip;

	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTextCache CachedNodeTitle;
};