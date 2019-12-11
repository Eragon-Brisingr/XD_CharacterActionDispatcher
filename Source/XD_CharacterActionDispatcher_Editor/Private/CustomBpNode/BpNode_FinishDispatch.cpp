// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomBpNode/BpNode_FinishDispatch.h"
#include "CustomBpNode/Utils/DA_CustomBpNodeUtils.h"
#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <K2Node_CallFunction.h>
#include "Dispatcher/XD_ActionDispatcherBase.h"
#include <KismetCompiler.h>

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FText UBpNode_FinishDispatch::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return LOCTEXT("结束调度器标题", "Finish Action Dispatcher");
	}
	else
	{
		return FText::Format(LOCTEXT("结束调度器详细标题", "结束调度器 事件Tag为[{0}]"), FText::FromName(Tag.GetTagName()));
	}
}

FLinearColor UBpNode_FinishDispatch::GetNodeTitleColor() const
{
	return FLinearColor::Red;
}

FText UBpNode_FinishDispatch::GetTooltipText() const
{
	return LOCTEXT("结束调度器提示描述", "调度器所有事件流完成后必须连接该节点");
}

FText UBpNode_FinishDispatch::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

void UBpNode_FinishDispatch::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool UBpNode_FinishDispatch::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_FinishDispatch::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
}

void UBpNode_FinishDispatch::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UK2Node_CallFunction* FinishDispatchNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	FinishDispatchNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, FinishDispatch), UXD_ActionDispatcherBase::StaticClass());
	FinishDispatchNode->AllocateDefaultPins();
	DA_NodeUtils::SetPinStructValue(FinishDispatchNode->FindPinChecked(TEXT("Tag")), Tag);
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *FinishDispatchNode->GetExecPin());
}

#undef LOCTEXT_NAMESPACE
