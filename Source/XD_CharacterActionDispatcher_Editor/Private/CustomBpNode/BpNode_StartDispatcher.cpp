﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomBpNode/BpNode_StartDispatcher.h"
#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <KismetCompiler.h>
#include <K2Node_CustomEvent.h>
#include <K2Node_SwitchName.h>
#include <K2Node_CallFunction.h>
#include <Kismet/GameplayStatics.h>

#include "CustomBpNode/Utils/DA_CustomBpNodeUtils.h"
#include "Dispatcher/XD_ActionDispatcherBase.h"
#include "Utils/XD_ActionDispatcherLibrary.h"
#include "Manager/XD_ActionDispatcherManager.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FName UBpNode_StartDispatcherBase::DefaultPinName = TEXT("Default");

UBpNode_StartDispatcherBase::UBpNode_StartDispatcherBase()
{
	
}

void UBpNode_StartDispatcherBase::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* ThenPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, UEdGraphSchema_K2::PN_ReturnValue);

	ReflushFinishExec();
}

void UBpNode_StartDispatcherBase::ShowExtendPins(UClass* UseSpawnClass)
{
	Super::ShowExtendPins(UseSpawnClass);
	GetClassPin()->DefaultObject = ActionDispatcherClass;
	GetResultPin()->PinType.PinSubCategoryObject = UseSpawnClass;
}

void UBpNode_StartDispatcherBase::WhenClassPinChanged(UClass* NewClass)
{
	Super::WhenClassPinChanged(NewClass);
	ActionDispatcherClass = NewClass;
}

void UBpNode_StartDispatcherBase::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
	if (ActionDispatcherClass == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("开始调度器_类型为空Error", "ICE: @@类型不得为空").ToString(), this);
		return;
	}
}

UClass* UBpNode_StartDispatcherBase::GetClassPinBaseClass() const
{
	return UXD_ActionDispatcherBase::StaticClass();
}

bool UBpNode_StartDispatcherBase::IsSpawnVarPin(UEdGraphPin* Pin) const
{
	return Super::IsSpawnVarPin(Pin);
}

void UBpNode_StartDispatcherBase::ReflushFinishExec()
{
	if (ActionDispatcherClass)
	{
		FinishedTags = ActionDispatcherClass.GetDefaultObject()->GetAllFinishTags();
	}
	else
	{
		FinishedTags.Empty();
	}
	for (const FName& Tag : FinishedTags)
	{
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, Tag);
	}
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, DefaultPinName);
}

void UBpNode_StartDispatcherBase::GenerateFinishEvent(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* DispatchFinishedEventPin, const FString& EventName)
{
	UK2Node_CustomEvent* FinishedEventNode = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(this, DispatchFinishedEventPin, SourceGraph);
	FinishedEventNode->CustomFunctionName = *FString::Printf(TEXT("%s_[%s]"), *EventName, *CompilerContext.GetGuid(this));
	FinishedEventNode->AllocateDefaultPins();
	const FDelegateProperty* DelegateProperty = CastFieldChecked<FDelegateProperty>(UXD_ActionDispatcherBase::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UXD_ActionDispatcherBase, WhenDispatchFinished)));
	for (TFieldIterator<FProperty> PropIt(DelegateProperty->SignatureFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		const FProperty* Param = *PropIt;
		if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
		{
			FEdGraphPinType PinType;
			CompilerContext.GetSchema()->ConvertPropertyToPinType(Param, /*out*/ PinType);
			FinishedEventNode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output);
		}
	}

	FinishedEventNode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName)->MakeLinkTo(DispatchFinishedEventPin);

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

//UBpNode_StartDispatcherWithManager

FName UBpNode_StartDispatcherWithManager::LeaderPinName = TEXT("DispatcherLeader");

FText UBpNode_StartDispatcherWithManager::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return LOCTEXT("开始调度器（全局）标题", "Start Global Action Dispatcher");
	}
	else if (UClass* ClassToSpawn = GetClassToSpawn())
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ClassName"), ClassToSpawn->GetDisplayNameText());
			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("开始调度器（全局）详细标题", "开始调度器（全局） [{ClassName}]"), Args), this);
		}
		return CachedNodeTitle;
	}
	return LOCTEXT("开始调度器（全局）详细标题_NONE", "开始调度器（全局） [NONE]");
}

FText UBpNode_StartDispatcherWithManager::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

void UBpNode_StartDispatcherWithManager::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool UBpNode_StartDispatcherWithManager::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && !DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_StartDispatcherWithManager::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* LeaderPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_SoftObject, AActor::StaticClass(), LeaderPinName);
	LeaderPin->PinToolTip = *LOCTEXT("Leader Pin Desc", "调度器的主导者，当主导者无效时则停止调度\n输入的为玩家时主导者为玩家，输入非玩家时则主导者为Level").ToString();
}

void UBpNode_StartDispatcherWithManager::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	bool bSucceeded = true;
	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, SpawnObject), UGameplayStatics::StaticClass());
	CallCreateNode->AllocateDefaultPins();

	UK2Node_CallFunction* GetActionDispatcherManagerNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	GetActionDispatcherManagerNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherLibrary, GetActionDispatcherManager), UXD_ActionDispatcherLibrary::StaticClass());
	GetActionDispatcherManagerNode->AllocateDefaultPins();

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
		UEdGraphPin* SpawnOuterPin = GetActionDispatcherManagerNode->GetReturnValuePin();
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

	UEdGraphPin* LastThen = CallCreateNode->GetThenPin();
	//GenerateAssignmentNodes执行之后节点的LinkTo被清空，MovePinLinksToIntermediate无效
	UK2Node_CallFunction* InitLeaderNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	{
		InitLeaderNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, InitLeader), UXD_ActionDispatcherBase::StaticClass());
		InitLeaderNode->AllocateDefaultPins();
		CallCreateNode->GetReturnValuePin()->MakeLinkTo(InitLeaderNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
		bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(LeaderPinName), *InitLeaderNode->FindPinChecked(TEXT("InDispatcherLeader"))).CanSafeConnect();
		LastThen->MakeLinkTo(InitLeaderNode->GetExecPin());
		LastThen = InitLeaderNode->GetThenPin();
	}

	LastThen = DA_NodeUtils::GenerateAssignmentNodes(CompilerContext, SourceGraph, InitLeaderNode, this, CallResultPin, ClassToSpawn);
	{
		{
			UK2Node_CallFunction* BindWhenDispatchFinishedNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			BindWhenDispatchFinishedNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, BindWhenDispatchFinished), UXD_ActionDispatcherBase::StaticClass());
			BindWhenDispatchFinishedNode->AllocateDefaultPins();
			LastThen->MakeLinkTo(BindWhenDispatchFinishedNode->GetExecPin());
			LastThen = BindWhenDispatchFinishedNode->GetThenPin();
			CallCreateNode->GetReturnValuePin()->MakeLinkTo(BindWhenDispatchFinishedNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));

			UEdGraphPin* DispatchFinishedEventPin = BindWhenDispatchFinishedNode->FindPinChecked(TEXT("DispatchFinishedEvent"));
			GenerateFinishEvent(CompilerContext, SourceGraph, DispatchFinishedEventPin, TEXT("WhenDispatcherFinished"));
		}

		{
			UK2Node_CallFunction* StartDispatcherInManagerNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			StartDispatcherInManagerNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherManager, InvokeStartDispatcher), UXD_ActionDispatcherManager::StaticClass());
			StartDispatcherInManagerNode->AllocateDefaultPins();
			CallCreateNode->GetReturnValuePin()->MakeLinkTo(StartDispatcherInManagerNode->FindPinChecked(TEXT("Dispatcher")));
			GetActionDispatcherManagerNode->GetReturnValuePin()->MakeLinkTo(StartDispatcherInManagerNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
			LastThen->MakeLinkTo(StartDispatcherInManagerNode->GetExecPin());
			LastThen = StartDispatcherInManagerNode->GetThenPin();
		}
	}

	//connect then
	{
		bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *LastThen).CanSafeConnect();
	}

	BreakAllNodeLinks();

	if (!bSucceeded)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("开始调度器_Error", "ICE: 开始调度器 error @@").ToString(), this);
	}
}


bool UBpNode_StartDispatcherWithManager::IsSpawnVarPin(UEdGraphPin* Pin) const
{
	return Super::IsSpawnVarPin(Pin) && Pin->PinName != LeaderPinName;
}

//UBpNode_StartDispatcherWithOwner

FName UBpNode_StartDispatcherWithOwner::Dispatcher_MemberVarPinName = TEXT("Dispatcher_MemberVar");

FName UBpNode_StartDispatcherWithOwner::OwnerPinName = TEXT("Owner");

bool UBpNode_StartDispatcherWithOwner::IsSpawnVarPin(UEdGraphPin* Pin) const
{
	return Super::IsSpawnVarPin(Pin) && 
		Pin->PinName != Dispatcher_MemberVarPinName &&
		Pin->PinName != OwnerPinName;
}

FText UBpNode_StartDispatcherWithOwner::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return LOCTEXT("开始调度器（独立）标题", "Start Independent Action Dispatcher");
	}
	else if (UClass* ClassToSpawn = GetClassToSpawn())
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ClassName"), ClassToSpawn->GetDisplayNameText());
			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("开始调度器（独立）详细标题", "开始调度器（独立） [{ClassName}]"), Args), this);
		}
		return CachedNodeTitle;
	}
	return LOCTEXT("开始调度器（独立）详细标题_NONE", "开始调度器（独立） [NONE]");
}

FText UBpNode_StartDispatcherWithOwner::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

void UBpNode_StartDispatcherWithOwner::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool UBpNode_StartDispatcherWithOwner::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && !DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_StartDispatcherWithOwner::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	FCreatePinParams CreatePinParams;
	CreatePinParams.bIsReference = true;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), OwnerPinName);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, ActionDispatcherClass ? *ActionDispatcherClass : UXD_ActionDispatcherBase::StaticClass(), Dispatcher_MemberVarPinName, CreatePinParams);
}

void UBpNode_StartDispatcherWithOwner::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* OwnerPin = FindPinChecked(OwnerPinName);
	if (OwnerPin->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("开始调度器_Owner为空Error", "ICE: @@Owner不得为空").ToString(), this);
		return;
	}

	bool bSucceeded = true;
	UK2Node_CallFunction* GetOrCreateDispatcherWithOwnerNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	GetOrCreateDispatcherWithOwnerNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherLibrary, GetOrCreateDispatcherWithOwner), UXD_ActionDispatcherLibrary::StaticClass());
	GetOrCreateDispatcherWithOwnerNode->AllocateDefaultPins();
	bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *GetOrCreateDispatcherWithOwnerNode->GetExecPin()).CanSafeConnect();
	bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(Dispatcher_MemberVarPinName), *GetOrCreateDispatcherWithOwnerNode->FindPinChecked(Dispatcher_MemberVarPinName)).CanSafeConnect();

	// store off the class to spawn before we mutate pin connections:
	UClass* ClassToSpawn = GetClassToSpawn();
	//connect exe
	{
		UEdGraphPin* SpawnExecPin = GetExecPin();
		UEdGraphPin* CallExecPin = GetOrCreateDispatcherWithOwnerNode->GetExecPin();
		bSucceeded &= SpawnExecPin && CallExecPin && CompilerContext.MovePinLinksToIntermediate(*SpawnExecPin, *CallExecPin).CanSafeConnect();
	}

	//connect class
	{
		UEdGraphPin* SpawnClassPin = GetClassPin();
		UEdGraphPin* CallClassPin = GetOrCreateDispatcherWithOwnerNode->FindPin(TEXT("Dispatcher"));
		bSucceeded &= SpawnClassPin && CallClassPin && CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallClassPin).CanSafeConnect();
	}

	//connect owner
	{
		UEdGraphPin* CallOuterPin = GetOrCreateDispatcherWithOwnerNode->FindPin(TEXT("Owner"));
		bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*OwnerPin, *CallOuterPin).CanSafeConnect();
	}

	UEdGraphPin* CallResultPin = nullptr;
	//connect result
	{
		UEdGraphPin* SpawnResultPin = GetResultPin();
		CallResultPin = GetOrCreateDispatcherWithOwnerNode->GetReturnValuePin();

		// cast HACK. It should be safe. The only problem is native code generation.
		if (SpawnResultPin && CallResultPin)
		{
			CallResultPin->PinType = SpawnResultPin->PinType;
		}
		bSucceeded &= SpawnResultPin && CallResultPin && CompilerContext.MovePinLinksToIntermediate(*SpawnResultPin, *CallResultPin).CanSafeConnect();
	}

	UEdGraphPin* LastThen = GetOrCreateDispatcherWithOwnerNode->GetThenPin();
	//GenerateAssignmentNodes执行之后节点的LinkTo被清空，MovePinLinksToIntermediate无效
	UK2Node_CallFunction* InitLeaderNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	{
		InitLeaderNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, InitLeader), UXD_ActionDispatcherBase::StaticClass());
		InitLeaderNode->AllocateDefaultPins();
		GetOrCreateDispatcherWithOwnerNode->GetReturnValuePin()->MakeLinkTo(InitLeaderNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
		bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(OwnerPinName), *InitLeaderNode->FindPinChecked(TEXT("InDispatcherLeader"))).CanSafeConnect();
		LastThen->MakeLinkTo(InitLeaderNode->GetExecPin());
		LastThen = InitLeaderNode->GetThenPin();
	}

	LastThen = DA_NodeUtils::GenerateAssignmentNodes(CompilerContext, SourceGraph, InitLeaderNode, this, CallResultPin, ClassToSpawn);
	{
		{
			UK2Node_CallFunction* BindWhenDispatchFinishedNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			BindWhenDispatchFinishedNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, BindWhenDispatchFinished), UXD_ActionDispatcherBase::StaticClass());
			BindWhenDispatchFinishedNode->AllocateDefaultPins();
			LastThen->MakeLinkTo(BindWhenDispatchFinishedNode->GetExecPin());
			LastThen = BindWhenDispatchFinishedNode->GetThenPin();
			GetOrCreateDispatcherWithOwnerNode->GetReturnValuePin()->MakeLinkTo(BindWhenDispatchFinishedNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));

			UEdGraphPin* DispatchFinishedEventPin = BindWhenDispatchFinishedNode->FindPinChecked(TEXT("DispatchFinishedEvent"));
			GenerateFinishEvent(CompilerContext, SourceGraph, DispatchFinishedEventPin, TEXT("WhenDispatcherFinished"));
		}

		{
			UK2Node_CallFunction* StartDispatchNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			StartDispatchNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, StartDispatch), UXD_ActionDispatcherBase::StaticClass());
			StartDispatchNode->AllocateDefaultPins();
			GetOrCreateDispatcherWithOwnerNode->GetReturnValuePin()->MakeLinkTo(StartDispatchNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));
			LastThen->MakeLinkTo(StartDispatchNode->GetExecPin());
			LastThen = StartDispatchNode->GetThenPin();
		}
	}

	//connect then
	{
		bSucceeded &= CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *LastThen).CanSafeConnect();
	}

	BreakAllNodeLinks();

	if (!bSucceeded)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("开始调度器_Error", "ICE: 开始调度器 error @@").ToString(), this);
	}
}

#undef LOCTEXT_NAMESPACE
