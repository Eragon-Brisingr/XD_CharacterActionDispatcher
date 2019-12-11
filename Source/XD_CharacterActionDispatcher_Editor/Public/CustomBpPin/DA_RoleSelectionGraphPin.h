// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <SGraphPin.h>
#include <EdGraphUtilities.h>

/**
 * 
 */
class FDA_RoleSelectionGraphPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};

class XD_CHARACTERACTIONDISPATCHER_EDITOR_API SDA_RoleSelectionGraphPin : public SGraphPin
{
public:
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	TSharedRef<SWidget>	GetDefaultValueWidget() override;
};
