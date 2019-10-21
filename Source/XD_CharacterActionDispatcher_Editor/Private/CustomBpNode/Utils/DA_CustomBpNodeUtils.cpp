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
#include "XD_DispatchableActionBase.h"
#include "K2Node_MakeArray.h"
#include "K2Node_CustomEvent.h"
#include "XD_BpNodeFunctionWarpper.h"
#include "BlueprintCompilationManager.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_EnumLiteral.h"

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

FString DA_NodeUtils::PinFinishEventToopTip = TEXT("FinishEvent");
FString DA_NodeUtils::PinNormalEventToopTip = TEXT("NormalEvent");

UEdGraphPin* DA_NodeUtils::CreateFinishEventPin(UK2Node* EdNode, const FName& PinName, const FText& DisplayName)
{
	UEdGraphPin* Pin = EdNode->CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PinName);
	Pin->PinFriendlyName = DisplayName;
	Pin->PinToolTip = PinFinishEventToopTip;
	return Pin;
}

UEdGraphPin* DA_NodeUtils::CreateNormalEventPin(UK2Node* EdNode, const FName& PinName, const FText& DisplayName /*= FText::GetEmpty()*/)
{
	UEdGraphPin* Pin = EdNode->CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PinName);
	Pin->PinFriendlyName = DisplayName;
	Pin->PinToolTip = PinNormalEventToopTip;
	Pin->bAdvancedView = true;
	return Pin;
}

void DA_NodeUtils::CreateActionEventPins(UK2Node* Node, const TSubclassOf<UXD_DispatchableActionBase>& ActionClass)
{
	if (ActionClass)
	{
		TArray<UXD_DispatchableActionBase::FPinNameData> FinishedEventNames = ActionClass.GetDefaultObject()->GetAllFinishedEventName();
		for (const UXD_DispatchableActionBase::FPinNameData& EventName : FinishedEventNames)
		{
			DA_NodeUtils::CreateFinishEventPin(Node, EventName.PinName, EventName.PinDisplayName);
		}
		TArray<UXD_DispatchableActionBase::FPinNameData> NormalEventNames = ActionClass.GetDefaultObject()->GetAllNormalEventName();
		for (const UXD_DispatchableActionBase::FPinNameData& EventName : NormalEventNames)
		{
			DA_NodeUtils::CreateNormalEventPin(Node, EventName.PinName, EventName.PinDisplayName);
		}
		if (NormalEventNames.Num() > 0 && ENodeAdvancedPins::NoPins == Node->AdvancedPinDisplay)
		{
			Node->AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
		}
	}
}

UEdGraphPin* DA_NodeUtils::CreateAllEventNode(const TSubclassOf<UXD_DispatchableActionBase>& ActionClass, UK2Node* Node, UEdGraphPin* LastThen, UEdGraphPin* ActionRefPin, const FName& EntryPointEventName, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	if (ActionClass)
	{
		UXD_DispatchableActionBase* Action = ActionClass.GetDefaultObject();

		UK2Node_CallFunction* BindingEventsNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Node, SourceGraph);
		BindingEventsNode->SetFromFunction(UXD_DispatchableActionBase::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_DispatchableActionBase, BindAllActionEvent)));
		BindingEventsNode->AllocateDefaultPins();
		ActionRefPin->MakeLinkTo(BindingEventsNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
		LastThen->MakeLinkTo(BindingEventsNode->GetExecPin());
		LastThen = BindingEventsNode->GetThenPin();

		UK2Node_MakeArray* MakeFinishedEventArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(Node, SourceGraph);
		{
			TArray<UXD_DispatchableActionBase::FPinNameData> FinishedEventNames = Action->GetAllFinishedEventName();
			MakeFinishedEventArrayNode->NumInputs = FinishedEventNames.Num();
			MakeFinishedEventArrayNode->AllocateDefaultPins();
			MakeFinishedEventArrayNode->GetOutputPin()->MakeLinkTo(BindingEventsNode->FindPinChecked(TEXT("FinishedEvents")));
			MakeFinishedEventArrayNode->PinConnectionListChanged(MakeFinishedEventArrayNode->GetOutputPin());

			for (int32 i = 0; i < FinishedEventNames.Num(); ++i)
			{
				const FName& FinishedEventName = FinishedEventNames[i].PinName;
				UEdGraphPin* ElementPin = MakeFinishedEventArrayNode->FindPinChecked(FString::Printf(TEXT("[%d]"), i));

				UK2Node_CallFunction* MakeDispatchableActionEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Node, SourceGraph);
				MakeDispatchableActionEventNode->SetFromFunction(UXD_BpNodeFunctionWarpper::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_BpNodeFunctionWarpper, MakeDispatchableActionFinishedEvent)));
				MakeDispatchableActionEventNode->AllocateDefaultPins();
				UEdGraphPin* MakeEventPin = MakeDispatchableActionEventNode->FindPinChecked(TEXT("Event"));

				//创建委托
				UK2Node_CustomEvent* FinishedEventNode = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(Node, MakeEventPin, SourceGraph);
				FinishedEventNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *EntryPointEventName.ToString(), *FinishedEventName.ToString());
				FinishedEventNode->AllocateDefaultPins();

				FinishedEventNode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName)->MakeLinkTo(MakeEventPin);
				MakeDispatchableActionEventNode->GetReturnValuePin()->MakeLinkTo(ElementPin);

				CompilerContext.MovePinLinksToIntermediate(*Node->FindPinChecked(FinishedEventName, EGPD_Output), *FinishedEventNode->FindPinChecked(UEdGraphSchema_K2::PN_Then));
			}
		}

		UK2Node_MakeArray* MakeNormalEventArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(Node, SourceGraph);
		{
			TArray<UXD_DispatchableActionBase::FPinNameData> NormalEventNames = Action->GetAllNormalEventName();
			MakeNormalEventArrayNode->NumInputs = NormalEventNames.Num();
			MakeNormalEventArrayNode->AllocateDefaultPins();
			MakeNormalEventArrayNode->GetOutputPin()->MakeLinkTo(BindingEventsNode->FindPinChecked(TEXT("NormalEvents")));
			MakeNormalEventArrayNode->PinConnectionListChanged(MakeNormalEventArrayNode->GetOutputPin());

			for (int32 i = 0; i < NormalEventNames.Num(); ++i)
			{
				const FName& NormalEventName = NormalEventNames[i].PinName;
				UEdGraphPin* ElementPin = MakeNormalEventArrayNode->FindPinChecked(FString::Printf(TEXT("[%d]"), i));

				UK2Node_CallFunction* MakeDispatchableActionEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Node, SourceGraph);
				MakeDispatchableActionEventNode->SetFromFunction(UXD_BpNodeFunctionWarpper::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_BpNodeFunctionWarpper, MakeDispatchableNormalEvent)));
				MakeDispatchableActionEventNode->AllocateDefaultPins();
				UEdGraphPin* MakeEventPin = MakeDispatchableActionEventNode->FindPinChecked(TEXT("Event"));

				//创建委托
				UK2Node_CustomEvent* FinishedEventNode = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(Node, MakeEventPin, SourceGraph);
				FinishedEventNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *EntryPointEventName.ToString(), *NormalEventName.ToString());
				FinishedEventNode->AllocateDefaultPins();

				FinishedEventNode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName)->MakeLinkTo(MakeEventPin);
				MakeDispatchableActionEventNode->GetReturnValuePin()->MakeLinkTo(ElementPin);

				CompilerContext.MovePinLinksToIntermediate(*Node->FindPinChecked(NormalEventName, EGPD_Output), *FinishedEventNode->FindPinChecked(UEdGraphSchema_K2::PN_Then));
			}
		}
	}
	return LastThen;
}

UEdGraphPin* DA_NodeUtils::CreateInvokeActiveActionNode(UK2Node* ActionNode, UEdGraphPin* LastThen, UK2Node_CallFunction* GetMainActionDispatcherNode, UEdGraphPin* ActionRefPin, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UK2Node_CallFunction* ActiveActionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(ActionNode, SourceGraph);
	ActiveActionNode->SetFromFunction(UXD_ActionDispatcherBase::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, InvokeActiveAction)));
	ActiveActionNode->AllocateDefaultPins();
	GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(ActiveActionNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
	ActionRefPin->MakeLinkTo(ActiveActionNode->FindPinChecked(TEXT("Action")));

	LastThen->MakeLinkTo(ActiveActionNode->GetExecPin());
	LastThen = ActiveActionNode->GetThenPin();
	return LastThen;
}

UEdGraphPin* DA_NodeUtils::GenerateAssignmentNodes(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UK2Node_CallFunction* CallBeginSpawnNode, UEdGraphNode* SpawnNode, UEdGraphPin* CallBeginResult, const UClass* ForClass)
{
	static const FName ObjectParamName(TEXT("Object"));
	static const FName ValueParamName(TEXT("Value"));
	static const FName PropertyNameParamName(TEXT("PropertyName"));

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	UEdGraphPin* LastThen = CallBeginSpawnNode->GetThenPin();

	// Create 'set var by name' nodes and hook them up
	for (int32 PinIdx = 0; PinIdx < SpawnNode->Pins.Num(); PinIdx++)
	{
		// Only create 'set param by name' node if this pin is linked to something
		UEdGraphPin* OrgPin = SpawnNode->Pins[PinIdx];
		const bool bHasDefaultValue = !OrgPin->DefaultValue.IsEmpty() || !OrgPin->DefaultTextValue.IsEmpty() || OrgPin->DefaultObject;
		if (NULL == CallBeginSpawnNode->FindPin(OrgPin->PinName) &&
			(OrgPin->LinkedTo.Num() > 0 || bHasDefaultValue))
		{
			UProperty* Property = FindField<UProperty>(ForClass, OrgPin->PinName);
			// NULL property indicates that this pin was part of the original node, not the 
			// class we're assigning to:
			if (!Property)
			{
				continue;
			}
			// 必须存在MD_ExposeOnSpawn元数据才创建节点
			if (!Property->GetBoolMetaData(FBlueprintMetadata::MD_ExposeOnSpawn))
			{
				continue;
			}

			if (OrgPin->LinkedTo.Num() == 0)
			{

				// We don't want to generate an assignment node unless the default value 
				// differs from the value in the CDO:
				FString DefaultValueAsString;

				if (FBlueprintCompilationManager::GetDefaultValue(ForClass, Property, DefaultValueAsString))
				{
					if (Schema->DoesDefaultValueMatch(*OrgPin, DefaultValueAsString))
					{
						continue;
					}
				}
				else if (ForClass->ClassDefaultObject)
				{
					FBlueprintEditorUtils::PropertyValueToString(Property, (uint8*)ForClass->ClassDefaultObject, DefaultValueAsString);

					if (DefaultValueAsString == OrgPin->GetDefaultAsString())
					{
						continue;
					}
				}
			}

			UFunction* SetByNameFunction = Schema->FindSetVariableByNameFunction(OrgPin->PinType);
			if (SetByNameFunction)
			{
				UK2Node_CallFunction* SetVarNode = nullptr;
				if (OrgPin->PinType.IsArray())
				{
					SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(SpawnNode, SourceGraph);
				}
				else
				{
					SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
				}
				SetVarNode->SetFromFunction(SetByNameFunction);
				SetVarNode->AllocateDefaultPins();

				// Connect this node into the exec chain
				Schema->TryCreateConnection(LastThen, SetVarNode->GetExecPin());
				LastThen = SetVarNode->GetThenPin();

				// Connect the new actor to the 'object' pin
				UEdGraphPin* ObjectPin = SetVarNode->FindPinChecked(ObjectParamName);
				CallBeginResult->MakeLinkTo(ObjectPin);

				// Fill in literal for 'property name' pin - name of pin is property name
				UEdGraphPin* PropertyNamePin = SetVarNode->FindPinChecked(PropertyNameParamName);
				PropertyNamePin->DefaultValue = OrgPin->PinName.ToString();

				UEdGraphPin* ValuePin = SetVarNode->FindPinChecked(ValueParamName);
				if (OrgPin->LinkedTo.Num() == 0 &&
					OrgPin->DefaultValue != FString() &&
					OrgPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte &&
					OrgPin->PinType.PinSubCategoryObject.IsValid() &&
					OrgPin->PinType.PinSubCategoryObject->IsA<UEnum>())
				{
					// Pin is an enum, we need to alias the enum value to an int:
					UK2Node_EnumLiteral* EnumLiteralNode = CompilerContext.SpawnIntermediateNode<UK2Node_EnumLiteral>(SpawnNode, SourceGraph);
					EnumLiteralNode->Enum = CastChecked<UEnum>(OrgPin->PinType.PinSubCategoryObject.Get());
					EnumLiteralNode->AllocateDefaultPins();
					EnumLiteralNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ValuePin);

					UEdGraphPin* InPin = EnumLiteralNode->FindPinChecked(UK2Node_EnumLiteral::GetEnumInputPinName());
					check(InPin);
					InPin->DefaultValue = OrgPin->DefaultValue;
				}
				else
				{
					// For non-array struct pins that are not linked, transfer the pin type so that the node will expand an auto-ref that will assign the value by-ref.
					if (OrgPin->PinType.IsArray() == false && OrgPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && OrgPin->LinkedTo.Num() == 0)
					{
						ValuePin->PinType.PinCategory = OrgPin->PinType.PinCategory;
						ValuePin->PinType.PinSubCategory = OrgPin->PinType.PinSubCategory;
						ValuePin->PinType.PinSubCategoryObject = OrgPin->PinType.PinSubCategoryObject;
						CompilerContext.MovePinLinksToIntermediate(*OrgPin, *ValuePin);
					}
					else
					{
						CompilerContext.MovePinLinksToIntermediate(*OrgPin, *ValuePin);
						SetVarNode->PinConnectionListChanged(ValuePin);
					}

				}
			}
		}
	}

	return LastThen;
}

#undef LOCTEXT_NAMESPACE
