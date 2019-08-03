// Fill out your copyright notice in the Description page of Project Settings.

#include "DA_CustomBpNodeUtils.h"
#include "BlueprintEditorUtils.h"
#include "XD_ActionDispatcherBase.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "KismetCompiler.h"
#include "EdGraph/EdGraphPin.h"
#include "BpNode_DispatchStartEvent.h"
#include "K2Node.h"
#include "MultiBoxBuilder.h"
#include "UIAction.h"
#include "K2Node_CallFunction.h"
#include "XD_CharacterActionDispatcher_EditorUtility.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

bool DA_NodeUtils::IsActionDispatcherGraph(const UEdGraph* TargetGraph)
{
	const EGraphType GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	bool bIsValidGraphType = GraphType == EGraphType::GT_Ubergraph || GraphType == EGraphType::GT_Macro;
	return bIsValidGraphType && Blueprint->GeneratedClass->IsChildOf(UXD_ActionDispatcherBase::StaticClass());
}

void DA_NodeUtils::UpdateNode(UBlueprint* Blueprint)
{
	if (!Blueprint->bBeingCompiled)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

void DA_NodeUtils::CreateDebugEventEntryPoint(UEdGraphNode* SourceNode, FKismetCompilerContext& CompilerContext, UEdGraphPin* ExecPin, const FName& EventName)
{
	UBpNode_DebugEntryPointEvent* DebugEvent = CompilerContext.SpawnIntermediateEventNode<UBpNode_DebugEntryPointEvent>(SourceNode, nullptr, nullptr);
	DebugEvent->CustomFunctionName = EventName;
	DebugEvent->AllocateDefaultPins();

	UK2Node_CallFunction* CallPreDebugForceExecuteNodeNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SourceNode, SourceNode->GetGraph());
	CallPreDebugForceExecuteNodeNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, PreDebugForceExecuteNode), UXD_ActionDispatcherBase::StaticClass());
	CallPreDebugForceExecuteNodeNode->AllocateDefaultPins();

	CompilerContext.GetSchema()->FindExecutionPin(*DebugEvent, EGPD_Output)->MakeLinkTo(CallPreDebugForceExecuteNodeNode->GetExecPin());
	CallPreDebugForceExecuteNodeNode->GetThenPin()->MakeLinkTo(ExecPin);
}

void DA_NodeUtils::AddDebugMenuSection(const UK2Node* Node, const FGraphNodeContextMenuBuilder& Context, FName EntryPointEventName)
{
	if (Context.bIsDebugging)
	{
		if (UObject * DebuggedObject = Node->GetBlueprint()->GetObjectBeingDebugged())
		{
			Context.MenuBuilder->BeginSection("调试", LOCTEXT("调试", "调试"));
			{
				Context.MenuBuilder->AddMenuEntry(
					LOCTEXT("强制执行", "强制执行"),
					LOCTEXT("强制执行描述", "强制执行该节点"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([=]()
							{
								if (UFunction * EntryPointEvent = DebuggedObject->FindFunction(EntryPointEventName))
								{
									ActionDispatcher_Editor_Display_Log("强制执行调试跳转事件[%s]", *EntryPointEventName.ToString());
									DebuggedObject->ProcessEvent(EntryPointEvent, nullptr);
								}
							}),
						FIsActionChecked()
								)
				);
			}
			Context.MenuBuilder->EndSection();
		}
	}
}

#undef LOCTEXT_NAMESPACE
