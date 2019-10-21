// Fill out your copyright notice in the Description page of Project Settings.

#include "BpNode_RoleSelection.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "DA_CustomBpNodeUtils.h"
#include "EdGraphSchema_K2.h"
#include "XD_DA_RoleSelectionBase.h"
#include "XD_ActionDispatcherSettings.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "XD_BpNodeFunctionWarpper.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "GameFramework/Pawn.h"
#include "K2Node_Self.h"
#include "XD_ActionDispatcherBase.h"
#include "PropertyPortFlags.h"
#include "CoreGlobals.h"
#include "FeedbackContext.h"
#include "UIAction.h"
#include "EdGraph/EdGraphNode.h"
#include "MultiBoxBuilder.h"
#include "K2Node_IfThenElse.h"
#include "Kismet/GameplayStatics.h"
#include "K2Node_Knot.h"
#include "Kismet/KismetMathLibrary.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

UBpNode_RoleSelection::UBpNode_RoleSelection()
{
	SelectionStructType = FDA_RoleSelection::StaticStruct();
	ActionClass = UXD_DA_RoleSelectionBase::StaticClass();
}

FText UBpNode_RoleSelection::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	static FText EmptyName = LOCTEXT("None", "None");
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return FText::Format(LOCTEXT("Role Selection Title", "Execute Action {0} [{1}]"), ActionClass ? FText::FromString(ActionClass->GetName()) : EmptyName, ActionClass ? ActionClass->GetDisplayNameText() : EmptyName);
	}
	else
	{
		return FText::Format(LOCTEXT("Role Selection Detail Title", "[{0}]({1})"), ActionClass ? ActionClass->GetDisplayNameText() : EmptyName, SelectionNum);
	}
}

FText UBpNode_RoleSelection::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

FName UBpNode_RoleSelection::GetCornerIcon() const
{
	return TEXT("Graph.Latent.LatentIcon");
}

bool UBpNode_RoleSelection::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_RoleSelection::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	Super::GetContextMenuActions(Context);
	if (!Context.bIsDebugging)
	{
		Context.MenuBuilder->BeginSection("选项", LOCTEXT("选项", "选项"));
		{
			Context.MenuBuilder->AddMenuEntry(
				LOCTEXT("添加选项", "添加选项"),
				LOCTEXT("添加选项描述", "添加选项"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateUObject(this, &UBpNode_RoleSelection::AddSelection),
					FIsActionChecked()
				)
			);

			if (Context.Pin && SelectionNum > 1 && SelectionPins.ContainsByPredicate([&](const FSelectionPin& E) {return E.SelectionPin == Context.Pin;}))
			{
				Context.MenuBuilder->AddMenuEntry(
					LOCTEXT("移除选项", "移除选项"),
					LOCTEXT("移除选项描述", "移除选项"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateUObject(this, &UBpNode_RoleSelection::RemoveSelection, Context.Pin),
						FIsActionChecked()
					)
				);
			}
		}
		Context.MenuBuilder->EndSection();
	}

	DA_NodeUtils::AddDebugMenuSection(this, Context, EntryPointEventName);
}

void UBpNode_RoleSelection::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	SelectionPins.Empty();
}

void UBpNode_RoleSelection::ShowExtendPins(UClass* UseSpawnClass)
{
	Super::ShowExtendPins(UseSpawnClass);
	for (int32 i = 0; i < SelectionNum; ++i)
	{
		AddSelectionImpl(i);
	}

	CreateResultPin(UseSpawnClass);
}

void UBpNode_RoleSelection::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UClass* ClassToSpawn = GetClassToSpawn();
	if (ClassToSpawn == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("角色选择节点_类型为空Error", "ICE: @@类型不得为空").ToString(), this);
		return;
	}

	DA_NodeUtils::CreateDebugEventEntryPoint(this, CompilerContext, GetExecPin(), EntryPointEventName);

	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, SpawnObject), UGameplayStatics::StaticClass());
	CallCreateNode->AllocateDefaultPins();
	CallCreateNode->GetReturnValuePin()->PinType = GetResultPin()->PinType;
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CallCreateNode->GetExecPin());
	CompilerContext.MovePinLinksToIntermediate(*GetClassPin(), *CallCreateNode->FindPinChecked(TEXT("ObjectClass")));
	CompilerContext.MovePinLinksToIntermediate(*GetResultPin(), *CallCreateNode->GetReturnValuePin());
	UEdGraphPin* LastThen = DA_NodeUtils::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, this, CallCreateNode->GetReturnValuePin(), ClassToSpawn);
	UK2Node_CallFunction* GetMainActionDispatcherNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	{
		GetMainActionDispatcherNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, GetMainActionDispatcher), UXD_ActionDispatcherBase::StaticClass());
		GetMainActionDispatcherNode->AllocateDefaultPins();
	}
	GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(CallCreateNode->FindPin(TEXT("Outer")));

	UK2Node_CallFunction* CallShowSelectionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallShowSelectionNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_DA_RoleSelectionBase, ShowSelection), UXD_DA_RoleSelectionBase::StaticClass());
	CallShowSelectionNode->AllocateDefaultPins();
	CallShowSelectionNode->FindPinChecked(UEdGraphSchema_K2::PN_Self)->MakeLinkTo(CallCreateNode->GetReturnValuePin());
	GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(CallShowSelectionNode->FindPinChecked(TEXT("ActionDispatcher")));
	LastThen->MakeLinkTo(CallShowSelectionNode->GetExecPin());

	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Then), *CallShowSelectionNode->GetThenPin());

	UK2Node_MakeArray* MakeSelectionsArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	MakeSelectionsArrayNode->NumInputs = SelectionNum;
	MakeSelectionsArrayNode->AllocateDefaultPins();
	MakeSelectionsArrayNode->GetOutputPin()->MakeLinkTo(CallShowSelectionNode->FindPinChecked(TEXT("InSelections")));
	MakeSelectionsArrayNode->PinConnectionListChanged(MakeSelectionsArrayNode->GetOutputPin());

	for (int32 i = 0; i < SelectionNum; ++i)
	{
		UK2Node_CallFunction* MakeDispatchableActionFinishedEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		MakeDispatchableActionFinishedEventNode->SetFromFunction(UXD_BpNodeFunctionWarpper::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_BpNodeFunctionWarpper, MakeDispatchableActionFinishedEvent)));
		MakeDispatchableActionFinishedEventNode->AllocateDefaultPins();
		UEdGraphPin* MakeEventPin = MakeDispatchableActionFinishedEventNode->FindPinChecked(TEXT("Event"));

		//创建委托
		UK2Node_CustomEvent* FinishedEventNode = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(this, MakeEventPin, SourceGraph);
		FinishedEventNode->CustomFunctionName = *FString::Printf(TEXT("WhenSelected_[%d]_[%s]"), i, *CompilerContext.GetGuid(this));
		FinishedEventNode->AllocateDefaultPins();
		CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(GetExecPinName(i), EGPD_Output), *FinishedEventNode->FindPinChecked(UEdGraphSchema_K2::PN_Then));

		UK2Node_CallFunction* SetWhenSelectedEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		SetWhenSelectedEventNode->SetFromFunction(UXD_DA_RoleSelectionBase::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_DA_RoleSelectionBase, SetWhenSelectedEvent)));
		SetWhenSelectedEventNode->AllocateDefaultPins();

		FinishedEventNode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName)->MakeLinkTo(MakeEventPin);
		MakeDispatchableActionFinishedEventNode->GetReturnValuePin()->MakeLinkTo(SetWhenSelectedEventNode->FindPinChecked(TEXT("Event")));

		CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(GetSelectionPinName(i), EGPD_Input), *SetWhenSelectedEventNode->FindPinChecked(TEXT("Selection")));

		MakeSelectionsArrayNode->FindPinChecked(*FString::Printf(TEXT("[%d]"), i), EGPD_Input)->MakeLinkTo(SetWhenSelectedEventNode->GetReturnValuePin());
	}

	UK2Node_MakeArray* MakeShowSelectionConditionsArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	MakeShowSelectionConditionsArrayNode->NumInputs = SelectionNum;
	MakeShowSelectionConditionsArrayNode->AllocateDefaultPins();
	MakeShowSelectionConditionsArrayNode->GetOutputPin()->MakeLinkTo(CallShowSelectionNode->FindPinChecked(TEXT("ShowSelectionConditions")));
	MakeShowSelectionConditionsArrayNode->PinConnectionListChanged(MakeShowSelectionConditionsArrayNode->GetOutputPin());
	for (int32 i = 0; i < SelectionNum; ++i)
	{
		CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(GetShowSelectionConditionName(i), EGPD_Input), *MakeShowSelectionConditionsArrayNode->FindPinChecked(*FString::Printf(TEXT("[%d]"), i), EGPD_Input));
	}

	BreakAllNodeLinks();
}

void UBpNode_RoleSelection::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// 4.23-BUG
	// FixupPinStringDataReferences中207行会设置Pin->DefaultValue，导致就算是DynamicDelegate的绑定为空也会存入None，引发
	// DelegatePropertyTools::ImportDelegateFromText中215行ErrorText->Logf(ELogVerbosity::Warning, TEXT("Cannot import unqualified delegate name; no object to search"))报错
	// 导致蓝图编译失败
	for (int32 Idx = 0; Idx < SelectionNum; ++Idx)
	{
		UScriptStruct* DA_RoleSelectionType = FDA_RoleSelection::StaticStruct();

		UEdGraphPin* SelectionPin = FindPin(GetSelectionPinName(Idx));
		if (SelectionPin && SelectionPin->DefaultValue.Len() > 0)
		{
			FDA_RoleSelection Selection = FDA_RoleSelection();
			DA_RoleSelectionType->ImportText(*SelectionPin->DefaultValue, &Selection, nullptr, EPropertyPortFlags::PPF_None, GWarn, DA_RoleSelectionType->GetName());
			FDA_RoleSelection DefaultSelection = FDA_RoleSelection();
			FString NewSelectionSourceString;
			DA_RoleSelectionType->ExportText(NewSelectionSourceString, &Selection, &DefaultSelection, nullptr, EPropertyPortFlags::PPF_None, nullptr);
			SelectionPin->DefaultValue = NewSelectionSourceString;
		}
	}
}

UClass* UBpNode_RoleSelection::GetClassPinBaseClass() const
{
	return UXD_DA_RoleSelectionBase::StaticClass();
}

void UBpNode_RoleSelection::AddSelection()
{
	SelectionNum += 1;
	ReconstructNode();
	DA_NodeUtils::UpdateNode(GetBlueprint());
}

void UBpNode_RoleSelection::AddSelectionImpl(int32 Idx)
{
	UEdGraphPin* SelectionPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, SelectionStructType, GetSelectionPinName(Idx));
	UEdGraphPin* ShowSelectionConditionPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, GetShowSelectionConditionName(Idx));
	ShowSelectionConditionPin->DefaultValue = TEXT("True");
	ShowSelectionConditionPin->bAdvancedView = true;
	UEdGraphPin* ExecPin = DA_NodeUtils::CreateFinishEventPin(this, GetExecPinName(Idx));

	FSelectionPin SelectionPinData;
	SelectionPinData.SelectionPin = SelectionPin;
	SelectionPinData.ExecPin = ExecPin;
	SelectionPins.Add(SelectionPinData);
}

void UBpNode_RoleSelection::RemoveSelection(const UEdGraphPin* SelectionPin)
{
	int32 Index = SelectionPins.IndexOfByPredicate([&](const FSelectionPin& E) {return E.SelectionPin == SelectionPin; });
	if (Index != INDEX_NONE)
	{
		SelectionNum -= 1;
		ReconstructNode();
		DA_NodeUtils::UpdateNode(GetBlueprint());
	}
}

FName UBpNode_RoleSelection::GetSelectionPinName(int32 Idx) const
{
	return *FString::Printf(TEXT("选项[%d]"), Idx + 1);
}

FName UBpNode_RoleSelection::GetShowSelectionConditionName(int32 Idx) const
{
	return *FString::Printf(TEXT("显示选项[%d]"), Idx + 1);
}

FName UBpNode_RoleSelection::GetExecPinName(int32 Idx) const
{
	return *FString::Printf(TEXT("选择了[%d]"), Idx + 1);
}

void UBpNode_RoleSelection::PostPasteNode()
{
	Super::PostPasteNode();
	EntryPointEventName = *FString::Printf(TEXT("RoleSelection_%d"), FMath::Rand());
}

void UBpNode_RoleSelection::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();
	EntryPointEventName = *FString::Printf(TEXT("RoleSelection_%d"), FMath::Rand());
}

#undef LOCTEXT_NAMESPACE
