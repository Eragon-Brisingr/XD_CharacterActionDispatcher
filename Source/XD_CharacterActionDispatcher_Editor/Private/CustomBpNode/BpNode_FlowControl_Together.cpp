// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomBpNode/BpNode_FlowControl_Together.h"
#include <Kismet2/BlueprintEditorUtils.h>
#include <BlueprintNodeSpawner.h>
#include <BlueprintActionDatabaseRegistrar.h>
#include <EdGraphSchema_K2.h>
#include <K2Node_CallFunction.h>
#include <KismetCompiler.h>
#include "Dispatcher/XD_ActionDispatcherBase.h"
#include <K2Node_IfThenElse.h>
#include <K2Node_Knot.h>
#include <EdGraph/EdGraphNode.h>
#include <Framework/MultiBox/MultiBoxBuilder.h>
#include <ToolMenu.h>
#include <ToolMenuSection.h>

#include "CustomBpNode/Utils/DA_CustomBpNodeUtils.h"
#include "Compiler/LinkToFinishNodeChecker.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FName TogetherEventPinName = TEXT("Together");

FText UBpNode_FlowControl_Together::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return LOCTEXT("TogetherEvent Node Title", "Together Event");
	}
	else
	{
		return FText::Format(LOCTEXT("TogetherEvent node detail title", "共同事件[{0}]"), TogetherEventCount);
	}
}

FText UBpNode_FlowControl_Together::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

FName UBpNode_FlowControl_Together::GetCornerIcon() const
{
	return TEXT("Graph.Latent.LatentIcon");
}

void UBpNode_FlowControl_Together::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool UBpNode_FlowControl_Together::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_FlowControl_Together::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	if (!Context->bIsDebugging)
	{
		FToolMenuSection& Section = Menu->AddSection(TEXT("FlowControl Together"), LOCTEXT("FlowControl Together", "FlowControl Together"));
		Section.AddMenuEntry(
			TEXT("Add FlowControl Together Event"),
			LOCTEXT("FlowControl Together Add Pin Desc", "添加共同事件输入"),
			LOCTEXT("FlowControl Together Add Pin Desc", "添加共同事件输入"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(this, &UBpNode_FlowControl_Together::AddExecPin),
				FIsActionChecked()
			)
		);

		if (Context->Pin && TogetherEventCount > 2 && TogetherPins.Contains(Context->Pin))
		{
			Section.AddMenuEntry(
				TEXT("Remove FlowControl Together Event"),
				LOCTEXT("FlowControl Together Remove Pin Desc", "移除共同事件输入"),
				LOCTEXT("FlowControl Together Remove Pin Desc", "移除共同事件输入"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateUObject(this, &UBpNode_FlowControl_Together::RemoveExecPin, Context->Pin),
					FIsActionChecked()
				)
			);
		}
	}
}

void UBpNode_FlowControl_Together::AllocateDefaultPins()
{
	DA_NodeUtils::CreateFinishEventPin(this, TogetherEventPinName, LOCTEXT("可执行共同事件引脚描述", "可执行共同事件"));
	for (int32 i = 0; i < TogetherEventCount; ++i)
	{
		AddExecPinImpl(i);
	}
}

void UBpNode_FlowControl_Together::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UK2Node_Knot* FinishedNode = CompilerContext.SpawnIntermediateNode<UK2Node_Knot>(this, SourceGraph);
	FinishedNode->AllocateDefaultPins();
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(TogetherEventPinName), *FinishedNode->GetOutputPin());

	UK2Node_CallFunction* GetMainActionDispatcherNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	{
		GetMainActionDispatcherNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, GetMainActionDispatcher), UXD_ActionDispatcherBase::StaticClass());
		GetMainActionDispatcherNode->AllocateDefaultPins();
	}

	for (int32 i = 0; i < TogetherEventCount; ++i)
	{
		UK2Node_CallFunction* CallEnterTogetherFlowControl = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallEnterTogetherFlowControl->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, EnterTogetherFlowControl), UXD_ActionDispatcherBase::StaticClass());
		CallEnterTogetherFlowControl->AllocateDefaultPins();
		GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(CallEnterTogetherFlowControl->FindPinChecked(UEdGraphSchema_K2::PN_Self));
		DA_NodeUtils::SetPinStructValue(CallEnterTogetherFlowControl->FindPinChecked(TEXT("NodeGuid")), NodeGuid);
		CallEnterTogetherFlowControl->FindPinChecked(TEXT("Index"))->DefaultValue = FString::FromInt(i);
		CallEnterTogetherFlowControl->FindPinChecked(TEXT("TogetherCount"))->DefaultValue = FString::FromInt(TogetherEventCount);

		CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(GetExecPinName(i), EGPD_Input), *CallEnterTogetherFlowControl->GetExecPin());

		UK2Node_IfThenElse* BranchNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
		BranchNode->AllocateDefaultPins();
		CallEnterTogetherFlowControl->FindPinChecked(UEdGraphSchema_K2::PN_Then)->MakeLinkTo(BranchNode->GetExecPin());
		BranchNode->GetConditionPin()->MakeLinkTo(CallEnterTogetherFlowControl->GetReturnValuePin());
		BranchNode->GetThenPin()->MakeLinkTo(FinishedNode->GetInputPin());

		CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(GetWaitPinName(i), EGPD_Output), *BranchNode->GetElsePin());
	}

	BreakAllNodeLinks();
}

void UBpNode_FlowControl_Together::WhenCheckLinkedFinishNode(FLinkToFinishNodeChecker& Checker) const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
		{
			if (Pin->PinName == TogetherEventPinName)
			{
				Checker.CheckPinConnectedFinishNode(Pin);
			}
			else
			{
				if (Pin->LinkedTo.Num() > 0)
				{
					FLinkToFinishNodeChecker::CheckForceNotConnectFinishNode(Pin, Checker.MessageLog);
				}
				else
				{
					Checker.MessageLog.Warning(TEXT("@@ 推荐添加角色等待行为"), Pin);
				}
			}
		}
	}
}

void UBpNode_FlowControl_Together::AddExecPin()
{
	AddExecPinImpl(TogetherEventCount);
	TogetherEventCount += 1; 
	DA_NodeUtils::UpdateNode(GetBlueprint());
}

void UBpNode_FlowControl_Together::AddExecPinImpl(int32 Idx)
{
	UEdGraphPin* ExecPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, GetExecPinName(Idx));
	UEdGraphPin* ThenPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, GetWaitPinName(Idx));
	TogetherPins.Add(ExecPin) = ThenPin;
}

void UBpNode_FlowControl_Together::RemoveExecPin(const UEdGraphPin* Pin)
{
	if (UEdGraphPin** p_ThenPin = TogetherPins.Find(Pin))
	{
		UEdGraphPin* ThenPin = *p_ThenPin;
		RemovePin(const_cast<UEdGraphPin*>(Pin));
		RemovePin(ThenPin);
		TogetherPins.Remove(Pin);
		TogetherEventCount -= 1;
		ReconstructNode();
		DA_NodeUtils::UpdateNode(GetBlueprint());
	}
}

FName UBpNode_FlowControl_Together::GetExecPinName(int32 Idx)
{
	return *FString::Printf(TEXT("申请执行[%d]"), Idx + 1);
}

FName UBpNode_FlowControl_Together::GetWaitPinName(int32 Idx)
{
	return *FString::Printf(TEXT("[%d]等待状态"), Idx + 1);
}

#undef LOCTEXT_NAMESPACE