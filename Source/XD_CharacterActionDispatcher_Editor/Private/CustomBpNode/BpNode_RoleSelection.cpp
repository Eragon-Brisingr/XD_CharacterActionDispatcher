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

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FName UBpNode_RoleSelection::RetureValuePinName = TEXT("ReturnValue");
FName UBpNode_RoleSelection::RolePinName = TEXT("Role");

UBpNode_RoleSelection::UBpNode_RoleSelection()
{
	SelectionStructType = FDA_RoleSelection::StaticStruct();
}

FText UBpNode_RoleSelection::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return LOCTEXT("Role Selection Title", "Show Selection");
	}
	else
	{
		return FText::Format(LOCTEXT("Role Selection Detail Title", "显示选项[{0}]"), SelectionNum);
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

void UBpNode_RoleSelection::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
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

	FCreatePinParams CreatePinParams;
	CreatePinParams.bIsReference = true;

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_SoftObject, APawn::StaticClass(), RolePinName, CreatePinParams);

	SelectionPins.Empty();
	for (int32 i = 0; i < SelectionNum; ++i)
	{
		AddSelectionImpl(i);
	}

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, GetDefault<UXD_ActionDispatcherSettings>()->RoleSelectionImplClass, RetureValuePinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

void UBpNode_RoleSelection::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	DA_NodeUtils::CreateDebugEventEntryPoint(this, CompilerContext, GetExecPin(), EntryPointEventName);

	UK2Node_CallFunction* CallRoleSelectionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallRoleSelectionNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_DA_RoleSelectionBase, ShowSelection), UXD_DA_RoleSelectionBase::StaticClass());
	CallRoleSelectionNode->AllocateDefaultPins();

	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CallRoleSelectionNode->GetExecPin());
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Then), *CallRoleSelectionNode->GetThenPin());
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(RolePinName), *CallRoleSelectionNode->FindPinChecked(TEXT("InRole")));

	UK2Node_CallFunction* GetMainActionDispatcherNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	{
		GetMainActionDispatcherNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, GetMainActionDispatcher), UXD_ActionDispatcherBase::StaticClass());
		GetMainActionDispatcherNode->AllocateDefaultPins();
	}
	GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(CallRoleSelectionNode->FindPinChecked(TEXT("ActionDispatcher")));

	UK2Node_MakeArray* MakeActorDatasArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	MakeActorDatasArrayNode->NumInputs = SelectionNum;
	MakeActorDatasArrayNode->AllocateDefaultPins();
	MakeActorDatasArrayNode->GetOutputPin()->MakeLinkTo(CallRoleSelectionNode->FindPinChecked(TEXT("InSelections")));
	MakeActorDatasArrayNode->PinConnectionListChanged(MakeActorDatasArrayNode->GetOutputPin());

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

		MakeActorDatasArrayNode->FindPinChecked(*FString::Printf(TEXT("[%d]"), i), EGPD_Input)->MakeLinkTo(SetWhenSelectedEventNode->GetReturnValuePin());
	}
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

void UBpNode_RoleSelection::AddSelection()
{
	SelectionNum += 1;
	ReconstructNode();
	DA_NodeUtils::UpdateNode(GetBlueprint());
}

void UBpNode_RoleSelection::AddSelectionImpl(int32 Idx)
{
	UEdGraphPin* SelectionPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, SelectionStructType, GetSelectionPinName(Idx));
	UEdGraphPin* ExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, DA_NodeUtils::PinFinishEventSubCategoryName, GetExecPinName(Idx));

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

FName UBpNode_RoleSelection::GetSelectionPinName(int32 Idx)
{
	return *FString::Printf(TEXT("选项[%d]"), Idx + 1);
}

FName UBpNode_RoleSelection::GetExecPinName(int32 Idx)
{
	return *FString::Printf(TEXT("选择了[%d]"), Idx + 1);
}

void UBpNode_RoleSelection::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();
	EntryPointEventName = *FString::Printf(TEXT("RoleSelection_%d"), FMath::Rand());
}

#undef LOCTEXT_NAMESPACE
