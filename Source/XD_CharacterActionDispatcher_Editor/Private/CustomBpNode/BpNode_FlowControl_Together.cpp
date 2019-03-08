// Fill out your copyright notice in the Description page of Project Settings.

#include "BpNode_FlowControl_Together.h"
#include "BlueprintEditorUtils.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "XD_ActionDispatcherBase.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Knot.h"
#include "EdGraph/EdGraphNode.h"
#include "MultiBoxBuilder.h"
#include "DA_CustomBpNodeUtils.h"

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

void UBpNode_FlowControl_Together::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	Super::GetContextMenuActions(Context);
	if (!Context.bIsDebugging)
	{
		Context.MenuBuilder->BeginSection("FlowControl Together", LOCTEXT("FlowControl Together", "FlowControl Together"));
		{
			Context.MenuBuilder->AddMenuEntry(
				LOCTEXT("FlowControl Together Add Pin Desc", "添加共同事件输入"),
				LOCTEXT("FlowControl Together Add Pin Desc", "添加共同事件输入"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateUObject(this, &UBpNode_FlowControl_Together::AddExecPin),
					FIsActionChecked()
				)
			);

			if (Context.Pin && TogetherEventCount > 2 && TogetherPins.Contains(Context.Pin))
			{
				Context.MenuBuilder->AddMenuEntry(
					LOCTEXT("FlowControl Together Remove Pin Desc", "移除共同事件输入"),
					LOCTEXT("FlowControl Together Remove Pin Desc", "移除共同事件输入"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateUObject(this, &UBpNode_FlowControl_Together::RemoveExecPin, Context.Pin),
						FIsActionChecked()
					)
				);
			}
		}
		Context.MenuBuilder->EndSection();
	}
}

void UBpNode_FlowControl_Together::AllocateDefaultPins()
{
	UEdGraphPin* TogetherPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TogetherEventPinName);
	TogetherPin->PinFriendlyName = LOCTEXT("可执行共同事件引脚描述", "可执行共同事件");
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

void UBpNode_FlowControl_Together::AddExecPin()
{
	AddExecPinImpl(TogetherEventCount);
	TogetherEventCount += 1;

	ReflushNode();
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
		ReflushExecPinName();
		ReflushNode();
	}
}

void UBpNode_FlowControl_Together::ReflushExecPinName()
{
	int32 Idx = 0;
	for (const TPair<UEdGraphPin*, UEdGraphPin*>& PinCollection : TogetherPins)
	{
		UEdGraphPin* ExecPin = PinCollection.Key;
		UEdGraphPin* ThenPin = PinCollection.Value;
		ExecPin->PinName = GetExecPinName(Idx);
		ThenPin->PinName = GetWaitPinName(Idx);
		Idx += 1;
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

void UBpNode_FlowControl_Together::ReflushNode()
{
	UBlueprint* Blueprint = GetBlueprint();
	if (!Blueprint->bBeingCompiled)
	{
		DA_NodeUtils::UpdateNode(GetBlueprint());
	}
}

#undef LOCTEXT_NAMESPACE