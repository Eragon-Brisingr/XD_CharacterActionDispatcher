﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomBpNode/BpNode_ExecuteAction.h"
#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <KismetCompiler.h>
#include <K2Node_CallFunction.h>
#include <K2Node_Self.h>
#include <K2Node_MakeArray.h>
#include <K2Node_CustomEvent.h>
#include <EdGraphSchema_K2_Actions.h>
#include <Kismet2/BlueprintEditorUtils.h>

#include "XD_BpNodeFunctionWarpper.h"
#include "Action/XD_DispatchableActionBase.h"
#include "Dispatcher/XD_ActionDispatcherBase.h"
#include "CustomBpNode/Utils/DA_CustomBpNodeUtils.h"
#include "Settings/XD_ActionDispatcherSettings.h"
#include "XD_ObjectFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FText UBpNode_ExecuteAction::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return FText::Format(LOCTEXT("Execute Action Title", "Execute Action {0} [{1}]"), ActionClass ? FText::FromString(ActionClass->GetName()) : LOCTEXT("None", "None"), ActionClass ? ActionClass->GetDisplayNameText() : LOCTEXT("None", "None"));
	}
	else if (UClass* ClassToSpawn = GetClassToSpawn())
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ClassName"), ClassToSpawn->GetDisplayNameText());
			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("执行行为_Title", "{ClassName}"), Args), this);
		}
		return CachedNodeTitle;
	}
	return LOCTEXT("执行行为_Title_NONE", "执行行为 [NONE]");
}

FText UBpNode_ExecuteAction::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

void UBpNode_ExecuteAction::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	const bool ShowPluginNode = GetDefault<UXD_ActionDispatcherSettings>()->bShowPluginNode;

	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		for (UClass* It : UXD_ObjectFunctionLibrary::GetAllSubclass<UXD_DispatchableActionBase>())
		{
			UXD_DispatchableActionBase* Action = It->GetDefaultObject<UXD_DispatchableActionBase>();
			if (!Action->bShowInExecuteActionNode)
			{
				continue;
			}

			if (!ShowPluginNode && Action->bIsPluginAction)
			{
				continue;
			}

			UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
			check(NodeSpawner != nullptr);

			NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda([=](UEdGraphNode* NewNode, bool bIsTemplateNode)
			{
				UBpNode_ExecuteAction* ExecuteActionNode = CastChecked<UBpNode_ExecuteAction>(NewNode);
				ExecuteActionNode->ActionClass = It;
			});
			ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
		}
	}
}

bool UBpNode_ExecuteAction::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_ExecuteAction::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	DA_NodeUtils::AddDebugMenuSection(this, Menu, Context, EntryPointEventName);
}

void UBpNode_ExecuteAction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
}

void UBpNode_ExecuteAction::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();
	EntryPointEventName = *FString::Printf(TEXT("%s_%d"), *ActionClass->GetName(), FMath::Rand());
}

void UBpNode_ExecuteAction::PostPasteNode()
{
	Super::PostPasteNode();
	EntryPointEventName = *FString::Printf(TEXT("%s_%d"), *ActionClass->GetName(), FMath::Rand());
}

void UBpNode_ExecuteAction::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (ActionClass == nullptr)
	{
		return;
	}

	DA_NodeUtils::CreateDebugEventEntryPoint(this, CompilerContext, GetExecPin(), EntryPointEventName);

	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, CreateAction), UXD_ActionDispatcherBase::StaticClass());
	CallCreateNode->AllocateDefaultPins();
	CallCreateNode->GetReturnValuePin()->PinType.PinSubCategoryObject = *ActionClass;

	// store off the class to spawn before we mutate pin connections:
	UClass* ClassToSpawn = GetClassToSpawn();

	UK2Node_CallFunction* GetMainActionDispatcherNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	{
		GetMainActionDispatcherNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, GetMainActionDispatcher), UXD_ActionDispatcherBase::StaticClass());
		GetMainActionDispatcherNode->AllocateDefaultPins();
	}

	bool bSucceeded = true;
	//connect exe
	{
		UEdGraphPin* SpawnExecPin = GetExecPin();
		UEdGraphPin* CallExecPin = CallCreateNode->GetExecPin();
		bSucceeded &= SpawnExecPin && CallExecPin && CompilerContext.MovePinLinksToIntermediate(*SpawnExecPin, *CallExecPin).CanSafeConnect();
	}

	//connect class
	{
		UEdGraphPin* SpawnClassPin = GetClassPin();
		UEdGraphPin* CallClassPin = CallCreateNode->FindPin(TEXT("ObjectClass"));
		bSucceeded &= SpawnClassPin && CallClassPin && CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallClassPin).CanSafeConnect();
	}

	//connect outer
	{
		GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(CallCreateNode->FindPinChecked(TEXT("Outer")));
	}

	UEdGraphPin* LastThen = DA_NodeUtils::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, this, CallCreateNode->GetReturnValuePin(), ClassToSpawn);

	//创建所有事件的委托
	LastThen = CreateAllEventNode(LastThen, CallCreateNode->GetReturnValuePin(), EntryPointEventName, CompilerContext, SourceGraph);

	LastThen = CreateInvokeActiveActionNode(LastThen, GetMainActionDispatcherNode, CallCreateNode->GetReturnValuePin(), CompilerContext, SourceGraph);
	LinkResultPin(GetMainActionDispatcherNode, CompilerContext, SourceGraph);

	bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *LastThen).CanSafeConnect();

	BreakAllNodeLinks();

	if (!bSucceeded)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("ExecuteAction_Error", "ICE: ExecuteAction error @@").ToString(), this);
	}
}

void UBpNode_ExecuteAction::ShowExtendPins(UClass* UseSpawnClass)
{
	Super::ShowExtendPins(UseSpawnClass);
	CreateActionEventPins(UseSpawnClass);
	CreateResultPin(UseSpawnClass);
}

UClass* UBpNode_ExecuteAction::GetClassPinBaseClass() const
{
	return UXD_DispatchableActionBase::StaticClass();
}

bool UBpNode_ExecuteAction::CanShowActionClass(bool ShowPluginNode, UXD_DispatchableActionBase* Action) const
{
	return Super::CanShowActionClass(ShowPluginNode, Action) && Action->bShowInExecuteActionNode;
}

#undef LOCTEXT_NAMESPACE
