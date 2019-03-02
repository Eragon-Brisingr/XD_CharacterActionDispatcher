// Fill out your copyright notice in the Description page of Project Settings.

#include "BpNode_RoleSelection.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "DA_CustomBpNodeUtils.h"
#include "EdGraphSchema_K2.h"
#include "XD_DA_RoleSelectionBase.h"
#include "XD_ActionDispatcherSettings.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FName UBpNode_RoleSelection::RetureValuePinName = TEXT("ReturnValue");

UBpNode_RoleSelection::UBpNode_RoleSelection()
{
	SelectionStruct = FDA_RoleSelection::StaticStruct();
}

FText UBpNode_RoleSelection::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("RoleSelectionTitle", "RoleSelection");
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
}

void UBpNode_RoleSelection::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	for (int32 i = 0; i < SelectionNum; ++i)
	{
		AddSelectionImpl(i);
	}

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, GetDefault<UXD_ActionDispatcherSettings>()->RoleSelectionImplClass, RetureValuePinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

void UBpNode_RoleSelection::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{

}

void UBpNode_RoleSelection::AddSelection()
{
	AddSelectionImpl(SelectionNum);
	SelectionNum += 1;
}

void UBpNode_RoleSelection::AddSelectionImpl(int32 Idx)
{
	UEdGraphPin* SelectionPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, SelectionStruct, GetSelectionPinName(Idx));
	UEdGraphPin* ExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, SelectionStruct, GetExecPinName(Idx));

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
		RemovePin(SelectionPins[Index].SelectionPin);
		RemovePin(SelectionPins[Index].ExecPin);
		SelectionPins.RemoveAt(Index);
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

#undef LOCTEXT_NAMESPACE
