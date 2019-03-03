// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;

/**
 * 
 */
struct DA_NodeUtils
{
	template<typename T>
	static void SetPinStructValue(UEdGraphPin* Pin, const T& Value)
	{
		Pin->DefaultValue.Empty();
		TBaseStructure<T>::Get()->ExportText(Pin->DefaultValue, &Value, nullptr, nullptr, 0, nullptr);
	}

	template<>
	static void SetPinStructValue<FVector>(UEdGraphPin* Pin, const FVector& Value)
	{
		Pin->DefaultValue = FString::Printf(TEXT("%f, %f, %f"), Value.X, Value.Y, Value.Z);
	}

	template<>
	static void SetPinStructValue<FRotator>(UEdGraphPin* Pin, const FRotator& Value)
	{
		Pin->DefaultValue = FString::Printf(TEXT("%f, %f, %f"), Value.Pitch, Value.Yaw, Value.Roll);
	}

	static bool IsActionDispatcherGraph(const UEdGraph* TargetGraph);

	static void UpdateNode(UBlueprint* Blueprint);
};