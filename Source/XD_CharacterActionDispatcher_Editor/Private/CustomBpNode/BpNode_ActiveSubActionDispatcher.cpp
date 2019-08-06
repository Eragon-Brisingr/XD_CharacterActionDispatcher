// Fill out your copyright notice in the Description page of Project Settings.

#include "BpNode_ActiveSubActionDispatcher.h"
#include "DA_CustomBpNodeUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "XD_ActionDispatcherBase.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "Kismet/GameplayStatics.h"
#include "K2Node_Self.h"
#include "XD_BpNodeFunctionWarpper.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Knot.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_SwitchName.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FName UBpNode_ActiveSubActionDispatcher::DefaultPinName = TEXT("Default");

FText UBpNode_ActiveSubActionDispatcher::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return LOCTEXT("激活子调度器标题", "Active Sub Action Dispatcher");
	}
	else if (UClass* ClassToSpawn = GetClassToSpawn())
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ClassName"), ClassToSpawn->GetDisplayNameText());
			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("激活子调度器详细标题", "激活子调度器 [{ClassName}]"), Args), this);
		}
		return CachedNodeTitle;
	}
	return LOCTEXT("激活子调度器详细标题_NONE", "激活子调度器 [NONE]");
}

FText UBpNode_ActiveSubActionDispatcher::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

void UBpNode_ActiveSubActionDispatcher::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool UBpNode_ActiveSubActionDispatcher::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_ActiveSubActionDispatcher::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	GetClassPin()->DefaultObject = ActionDispatcherClass;

	ReflushFinishExec();
	//调整节点顺序
	RemovePin(GetThenPin());
	RemovePin(GetResultPin());
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, GetClassPinBaseClass(), UEdGraphSchema_K2::PN_ReturnValue);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

void UBpNode_ActiveSubActionDispatcher::PinDefaultValueChanged(UEdGraphPin* ChangedPin)
{
	Super::PinDefaultValueChanged(ChangedPin);
	if (ChangedPin && (ChangedPin->PinName == TEXT("Class")))
	{
		ActionDispatcherClass = GetClassToSpawn();
		if (ActionDispatcherClass)
		{
			FinishedTags = ActionDispatcherClass.GetDefaultObject()->GetAllFinishTags();
		}
		else
		{
			FinishedTags.Empty();
		}
		ReconstructNode();
	}
}

void UBpNode_ActiveSubActionDispatcher::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (ActionDispatcherClass == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("激活子调度器_类型为空Error", "ICE: @@类型不得为空").ToString(), this);
		return;
	}

	UK2Node_Knot* FinishedNode = CompilerContext.SpawnIntermediateNode<UK2Node_Knot>(this, SourceGraph);
	FinishedNode->AllocateDefaultPins();
	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *FinishedNode->GetOutputPin());

	bool bSucceeded = true;
	UK2Node_CallFunction* GetMainActionDispatcherNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	{
		GetMainActionDispatcherNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, GetMainActionDispatcher), UXD_ActionDispatcherBase::StaticClass());
		GetMainActionDispatcherNode->AllocateDefaultPins();
	}

	UK2Node_CallFunction* TryActiveSubActionDispatcherNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	TryActiveSubActionDispatcherNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, TryActiveSubActionDispatcher), UXD_ActionDispatcherBase::StaticClass());
	TryActiveSubActionDispatcherNode->AllocateDefaultPins();
	bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *TryActiveSubActionDispatcherNode->GetExecPin()).CanSafeConnect();
	DA_NodeUtils::SetPinStructValue(TryActiveSubActionDispatcherNode->FindPinChecked(TEXT("NodeGuid")), NodeGuid);
	GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(TryActiveSubActionDispatcherNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));

	UK2Node_IfThenElse* BranchNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	BranchNode->AllocateDefaultPins();
	BranchNode->GetConditionPin()->MakeLinkTo(TryActiveSubActionDispatcherNode->GetReturnValuePin());
	TryActiveSubActionDispatcherNode->GetThenPin()->MakeLinkTo(BranchNode->GetExecPin());
	BranchNode->GetThenPin()->MakeLinkTo(FinishedNode->GetInputPin());

	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, SpawnObject), UGameplayStatics::StaticClass());
	CallCreateNode->AllocateDefaultPins();
	BranchNode->GetElsePin()->MakeLinkTo(CallCreateNode->GetExecPin());

	// store off the class to spawn before we mutate pin connections:
	UClass* ClassToSpawn = GetClassToSpawn();
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
		UEdGraphPin* SpawnOuterPin = GetMainActionDispatcherNode->GetReturnValuePin();
		UEdGraphPin* CallOuterPin = CallCreateNode->FindPin(TEXT("Outer"));
		SpawnOuterPin->MakeLinkTo(CallOuterPin);
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

	UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, this, CallResultPin, ClassToSpawn);
	{
		UK2Node_CallFunction* ActiveSubActionDispatcherNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		ActiveSubActionDispatcherNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, ActiveSubActionDispatcher), UXD_ActionDispatcherBase::StaticClass());
		ActiveSubActionDispatcherNode->AllocateDefaultPins();
		DA_NodeUtils::SetPinStructValue(ActiveSubActionDispatcherNode->FindPinChecked(TEXT("NodeGuid")), NodeGuid);
		ActiveSubActionDispatcherNode->FindPinChecked(TEXT("SubActionDispatcher"))->MakeLinkTo(CallCreateNode->GetReturnValuePin());
		GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(ActiveSubActionDispatcherNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
		LastThen->MakeLinkTo(ActiveSubActionDispatcherNode->GetExecPin());
		LastThen = ActiveSubActionDispatcherNode->GetThenPin();

		UK2Node_CallFunction* BindWhenSubDispatchFinishedNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		BindWhenSubDispatchFinishedNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, BindWhenDispatchFinished), UXD_ActionDispatcherBase::StaticClass());
		BindWhenSubDispatchFinishedNode->AllocateDefaultPins();
		LastThen->MakeLinkTo(BindWhenSubDispatchFinishedNode->GetExecPin());
		LastThen = BindWhenSubDispatchFinishedNode->GetThenPin();
		CallCreateNode->GetReturnValuePin()->MakeLinkTo(BindWhenSubDispatchFinishedNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));

		UEdGraphPin* SubDispatchFinishedEventPin = BindWhenSubDispatchFinishedNode->FindPinChecked(TEXT("DispatchFinishedEvent"));
		UK2Node_CustomEvent* FinishedEventNode = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(this, SubDispatchFinishedEventPin, SourceGraph);
		FinishedEventNode->CustomFunctionName = *FString::Printf(TEXT("WhenSubDispatcherFinished_[%s]"), *CompilerContext.GetGuid(this));
		FinishedEventNode->AllocateDefaultPins();
		const UDelegateProperty* DelegateProperty = CastChecked<UDelegateProperty>(UXD_ActionDispatcherBase::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UXD_ActionDispatcherBase, WhenDispatchFinished)));
		for (TFieldIterator<UProperty> PropIt(DelegateProperty->SignatureFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			const UProperty* Param = *PropIt;
			if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				FEdGraphPinType PinType;
				CompilerContext.GetSchema()->ConvertPropertyToPinType(Param, /*out*/ PinType);
				FinishedEventNode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output);
			}
		}

		FinishedEventNode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName)->MakeLinkTo(SubDispatchFinishedEventPin);

		UK2Node_SwitchName* FinishedSwitchNameNode = CompilerContext.SpawnIntermediateNode<UK2Node_SwitchName>(this, SourceGraph);
		FinishedSwitchNameNode->PinNames = FinishedTags;
		FinishedSwitchNameNode->AllocateDefaultPins();
		FinishedSwitchNameNode->FindPinChecked(TEXT("Selection"))->MakeLinkTo(FinishedEventNode->FindPinChecked(TEXT("Tag")));
		FinishedEventNode->FindPinChecked(UEdGraphSchema_K2::PN_Then)->MakeLinkTo(FinishedSwitchNameNode->GetExecPin());

		CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(DefaultPinName, EGPD_Output), *FinishedSwitchNameNode->FindPinChecked(DefaultPinName));
		for (const FName& Tag : FinishedTags)
		{
			CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(Tag, EGPD_Output), *FinishedSwitchNameNode->FindPinChecked(Tag));
		}
	}

	//connect then
	{
		LastThen->MakeLinkTo(FinishedNode->GetInputPin());
	}

	BreakAllNodeLinks();

	if (!bSucceeded)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("激活子调度器_Error", "ICE: 激活子调度器 error @@").ToString(), this);
	}
}

UClass* UBpNode_ActiveSubActionDispatcher::GetClassPinBaseClass() const
{
	return UXD_ActionDispatcherBase::StaticClass();
}

void UBpNode_ActiveSubActionDispatcher::ReflushFinishExec()
{
	for (const FName& Tag : FinishedTags)
	{
		DA_NodeUtils::CreateFinishEventPin(this, Tag);
	}
	DA_NodeUtils::CreateFinishEventPin(this, DefaultPinName);
}

#undef LOCTEXT_NAMESPACE
