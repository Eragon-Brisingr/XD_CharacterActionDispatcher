// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_Event.h"
#include "BpNode_DispatchStartEvent.generated.h"

/**
 * 
 */
UCLASS()
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API UBpNode_DispatchStartEvent : public UK2Node_Event
{
	GENERATED_BODY()
public:
	bool CanUserDeleteNode() const override { return false; }
};
