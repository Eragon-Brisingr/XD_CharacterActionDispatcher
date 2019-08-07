// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UEdGraphNode;
class FKismetCompilerContext;
class UEdGraphPin;
class UEdGraph;
class UK2Node;
struct FGraphNodeContextMenuBuilder;
class UXD_DispatchableActionBase;

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

	// 创建Debug用的跳转事件
	static void CreateDebugEventEntryPoint(UEdGraphNode* SourceNode, FKismetCompilerContext& CompilerContext, UEdGraphPin* ExecPin, const FName& EventName);

	static void AddDebugMenuSection(const UK2Node* Node, const FGraphNodeContextMenuBuilder& Context, FName EntryPointEventName);

	static FString PinFinishEventToopTip;
	static FString PinNormalEventToopTip;

	static UEdGraphPin* CreateFinishEventPin(UK2Node* EdNode, const FName& PinName, const FText& DisplayName = FText::GetEmpty());
	static UEdGraphPin* CreateNormalEventPin(UK2Node* EdNode, const FName& PinName, const FText& DisplayName = FText::GetEmpty());

	static void CreateActionEventPins(UK2Node* Node, const TSubclassOf<UXD_DispatchableActionBase>& ActionClass);

	static UEdGraphPin* CreateAllEventNode(const TSubclassOf<UXD_DispatchableActionBase>& ActionClass, UK2Node* ActionNode, UEdGraphPin* LastThen, UEdGraphPin* ActionRefPin, const FName& EntryPointEventName,
		FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	static UEdGraphPin* CreateInvokeActiveActionNode(UK2Node* ActionNode, UEdGraphPin* LastThen, UK2Node_CallFunction* GetMainActionDispatcherNode, UEdGraphPin* ActionRefPin, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

};
