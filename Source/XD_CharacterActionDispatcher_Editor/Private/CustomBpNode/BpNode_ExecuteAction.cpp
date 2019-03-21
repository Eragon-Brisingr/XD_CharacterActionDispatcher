// Fill out your copyright notice in the Description page of Project Settings.

#include "BpNode_ExecuteAction.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "XD_ActionDispatcherBase.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "Kismet/GameplayStatics.h"
#include "K2Node_Self.h"
#include "Action/XD_DispatchableActionBase.h"
#include "K2Node_MakeArray.h"
#include "K2Node_CustomEvent.h"
#include "XD_BpNodeFunctionWarpper.h"
#include "EdGraphSchema_K2_Actions.h"
#include "BlueprintEditorUtils.h"
#include "Utils/DA_CustomBpNodeUtils.h"
#include "Settings/XD_ActionDispatcherSettings.h"
#include "XD_ObjectFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FText UBpNode_ExecuteAction::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return FText::Format(LOCTEXT("Execute Action Title", "Execute Action [{0}]"), ActionClass ? ActionClass->GetDisplayNameText() : LOCTEXT("None", "None"));
	}
	else if (UClass* ClassToSpawn = GetClassToSpawn())
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ClassName"), ClassToSpawn->GetDisplayNameText());
			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("执行行为_Title", "执行行为 [{ClassName}]"), Args), this);
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
	bool ShowPluginNode = GetDefault<UXD_ActionDispatcherSettings>()->bShowPluginNode;

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

void UBpNode_ExecuteAction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	GetClassPin()->DefaultObject = ActionClass;
	ReflushFinishExec();

	//调整节点顺序
	RemovePin(GetThenPin());
	RemovePin(GetResultPin());
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, GetClassPinBaseClass(), UEdGraphSchema_K2::PN_ReturnValue);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

void UBpNode_ExecuteAction::ReflushFinishExec()
{
	if (ActionClass)
	{
		TArray<UXD_DispatchableActionBase::FPinNameData> FinishedEventNames = ActionClass.GetDefaultObject()->GetAllFinishedEventName();
		for (const UXD_DispatchableActionBase::FPinNameData& EventName : FinishedEventNames)
		{
			UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, EventName.PinName);
			Pin->PinFriendlyName = EventName.PinDisplayName;
		}
	}
}

void UBpNode_ExecuteAction::PinDefaultValueChanged(UEdGraphPin* ChangedPin)
{
	Super::PinDefaultValueChanged(ChangedPin);
	if (ChangedPin && (ChangedPin->PinName == TEXT("Class")))
	{
		ActionClass = GetClassToSpawn();
		ReconstructNode();
	}
}

void UBpNode_ExecuteAction::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, SpawnObject), UGameplayStatics::StaticClass());
	CallCreateNode->AllocateDefaultPins();

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

	UEdGraphPin* CallResultPin = nullptr;
	//connect result
	{
		UEdGraphPin* SpawnResultPin = GetResultPin();
		CallResultPin = CallCreateNode->GetReturnValuePin();

		// cast HACK. It should be safe. The only problem is native code generation.
		if (SpawnResultPin && CallResultPin)
		{
			CallResultPin->PinType = SpawnResultPin->PinType;
		}
		bSucceeded &= SpawnResultPin && CallResultPin && CompilerContext.MovePinLinksToIntermediate(*SpawnResultPin, *CallResultPin).CanSafeConnect();
	}

	//创建所有事件的委托
	UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, this, CallResultPin, ClassToSpawn);
	{
		if (ActionClass)
		{
			UXD_DispatchableActionBase* Action = ActionClass.GetDefaultObject();
			TArray<UXD_DispatchableActionBase::FPinNameData> FinishedEventNames = Action->GetAllFinishedEventName();

			UK2Node_MakeArray* MakeEventArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
			MakeEventArrayNode->NumInputs = FinishedEventNames.Num();
			MakeEventArrayNode->AllocateDefaultPins();

			UK2Node_CallFunction* BindingEventsNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			BindingEventsNode->SetFromFunction(UXD_DispatchableActionBase::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_DispatchableActionBase, BindAllFinishedEvent)));
			BindingEventsNode->AllocateDefaultPins();
			CallCreateNode->GetReturnValuePin()->MakeLinkTo(BindingEventsNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
			LastThen->MakeLinkTo(BindingEventsNode->GetExecPin());
			LastThen = BindingEventsNode->GetThenPin();

			MakeEventArrayNode->GetOutputPin()->MakeLinkTo(BindingEventsNode->FindPinChecked(TEXT("FinishedEvents")));
			MakeEventArrayNode->PinConnectionListChanged(MakeEventArrayNode->GetOutputPin());

			UK2Node_CallFunction* ActiveActionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			ActiveActionNode->SetFromFunction(UXD_ActionDispatcherBase::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, InvokeActiveAction)));
			ActiveActionNode->AllocateDefaultPins();
			GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(ActiveActionNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
			CallCreateNode->GetReturnValuePin()->MakeLinkTo(ActiveActionNode->FindPinChecked(TEXT("Action")));

			for (int32 i = 0; i < FinishedEventNames.Num(); ++i)
			{
				const FName& FinishedEventName = FinishedEventNames[i].PinName;
				UEdGraphPin* ElementPin = MakeEventArrayNode->FindPinChecked(FString::Printf(TEXT("[%d]"), i));

				UK2Node_CallFunction* MakeDispatchableActionFinishedEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
				MakeDispatchableActionFinishedEventNode->SetFromFunction(UXD_BpNodeFunctionWarpper::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_BpNodeFunctionWarpper, MakeDispatchableActionFinishedEvent)));
				MakeDispatchableActionFinishedEventNode->AllocateDefaultPins();
				UEdGraphPin* MakeEventPin = MakeDispatchableActionFinishedEventNode->FindPinChecked(TEXT("Event"));

				//创建委托
				UK2Node_CustomEvent* FinishedEventNode = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(this, MakeEventPin, SourceGraph);
				FinishedEventNode->CustomFunctionName = *FString::Printf(TEXT("%s_[%s]"), *FinishedEventName.ToString(), *CompilerContext.GetGuid(this));
				FinishedEventNode->AllocateDefaultPins();

				FinishedEventNode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName)->MakeLinkTo(MakeEventPin);
				MakeDispatchableActionFinishedEventNode->GetReturnValuePin()->MakeLinkTo(ElementPin);

				CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(FinishedEventName, EGPD_Output), *FinishedEventNode->FindPinChecked(UEdGraphSchema_K2::PN_Then));
			}

			LastThen->MakeLinkTo(ActiveActionNode->GetExecPin());
			LastThen = ActiveActionNode->GetThenPin();
		}
	}

	//connect then
	{
		UEdGraphPin* SpawnNodeThen = GetThenPin();
		bSucceeded &= SpawnNodeThen && LastThen && CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen, *LastThen).CanSafeConnect();
	}

	BreakAllNodeLinks();

	if (!bSucceeded)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("ExecuteAction_Error", "ICE: ExecuteAction error @@").ToString(), this);
	}
}

UClass* UBpNode_ExecuteAction::GetClassPinBaseClass() const
{
	return UXD_DispatchableActionBase::StaticClass();
}

#undef LOCTEXT_NAMESPACE
