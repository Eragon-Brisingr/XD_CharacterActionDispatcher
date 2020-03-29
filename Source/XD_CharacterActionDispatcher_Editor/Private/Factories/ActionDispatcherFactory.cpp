// Fill out your copyright notice in the Description page of Project Settings.
#include "Factories/ActionDispatcherFactory.h"
#include <Kismet2/KismetEditorUtilities.h>
#include <AssetTypeCategories.h>
#include <Modules/ModuleManager.h>
#include <KismetCompilerModule.h>
#include <ClassViewerFilter.h>
#include <ClassViewerModule.h>
#include <Kismet2/SClassPickerDialog.h>
#include <EdGraph/EdGraph.h>
#include <K2Node_Event.h>
#include <ObjectEditorUtils.h>
#include <EdGraphSchema_K2.h>
#include <Classes/EditorStyleSettings.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <K2Node_CallParentFunction.h>

#include "Dispatcher/XD_ActionDispatcherBase.h"
#include "Blueprint/ActionDispatcherBlueprint.h"
#include "Blueprint/ActionDispatcherGeneratedClass.h"
#include "Settings/XD_ActionDispatcherSettings.h"
#include "CustomBpNode/BpNode_DispatchStartEvent.h"
#include "GraphEditor/EdGraph_ActionDispatcher.h"
#include "GraphEditor/EdGraphSchema_ActionDispatcher.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

UActionDispatcherFactory::UActionDispatcherFactory()
{
	SupportedClass = UActionDispatcherBlueprint::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UActionDispatcherFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UClass* BlueprintClass = nullptr;
	UClass* BlueprintGeneratedClass = nullptr;

	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
	KismetCompilerModule.GetBlueprintTypesForClass(ActionDispatcherClass, BlueprintClass, BlueprintGeneratedClass);

	UActionDispatcherBlueprint* NewBP = CastChecked<UActionDispatcherBlueprint>(FKismetEditorUtilities::CreateBlueprint(ActionDispatcherClass, InParent, Name, EBlueprintType::BPTYPE_Normal, UActionDispatcherBlueprint::StaticClass(), BlueprintGeneratedClass, CallingContext));
	
	UEdGraph* MainGraph = FBlueprintEditorUtils::CreateNewGraph(NewBP, TEXT("Action Dispatcher Graph"), UEdGraph_ActionDispatcher::StaticClass(), UEdGraphSchema_ActionDispatcher::StaticClass());
	MainGraph->bAllowDeletion = false;
	MainGraph->bAllowRenaming = false;
	FBlueprintEditorUtils::RemoveGraphs(NewBP, NewBP->UbergraphPages);
	FBlueprintEditorUtils::AddUbergraphPage(NewBP, MainGraph);
	NewBP->LastEditedDocuments.Add(MainGraph);

	struct FActionDispatcherEventUtil
	{
		static UBpNode_DispatchStartEvent* AddDefaultEventNode(UBlueprint* InBlueprint, UEdGraph* InGraph, FName InEventName, UClass* InEventClass, int32& InOutNodePosY)
		{
			UBpNode_DispatchStartEvent* NewEventNode = nullptr;

			FMemberReference EventReference;
			EventReference.SetExternalMember(InEventName, InEventClass);

			// Prevent events that are hidden in the Blueprint's class from being auto-generated.
			if (!FObjectEditorUtils::IsFunctionHiddenFromClass(EventReference.ResolveMember<UFunction>(InBlueprint), InBlueprint->ParentClass))
			{
				const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

				// Add the event
				NewEventNode = NewObject<UBpNode_DispatchStartEvent>(InGraph);
				NewEventNode->EventReference = EventReference;

				// Snap the new position to the grid
				const UEditorStyleSettings* StyleSettings = GetDefault<UEditorStyleSettings>();
				if (StyleSettings)
				{
					const uint32 GridSnapSize = StyleSettings->GridSnapSize;
					InOutNodePosY = GridSnapSize * FMath::RoundFromZero(InOutNodePosY / (float)GridSnapSize);
				}

				// add update event graph
				NewEventNode->bOverrideFunction = true;
				NewEventNode->CreateNewGuid();
				NewEventNode->PostPlacedNewNode();
				NewEventNode->SetFlags(RF_Transactional);
				NewEventNode->AllocateDefaultPins();
				NewEventNode->bCommentBubblePinned = true;
				NewEventNode->bCommentBubbleVisible = true;
				NewEventNode->NodePosY = InOutNodePosY;
				UEdGraphSchema_K2::SetNodeMetaData(NewEventNode, FNodeMetadata::DefaultGraphNode);
				InOutNodePosY = NewEventNode->NodePosY + NewEventNode->NodeHeight + 200;

				InGraph->AddNode(NewEventNode);

				// Get the function that the event node or function entry represents
				FFunctionFromNodeHelper FunctionFromNode(NewEventNode);
				if (FunctionFromNode.Function && Schema->GetCallableParentFunction(FunctionFromNode.Function))
				{
					UFunction* ValidParent = Schema->GetCallableParentFunction(FunctionFromNode.Function);
					FGraphNodeCreator<UK2Node_CallParentFunction> FunctionNodeCreator(*InGraph);
					UK2Node_CallParentFunction* ParentFunctionNode = FunctionNodeCreator.CreateNode();
					ParentFunctionNode->SetFromFunction(ValidParent);
					ParentFunctionNode->AllocateDefaultPins();

					for (UEdGraphPin* EventPin : NewEventNode->Pins)
					{
						if (UEdGraphPin* ParentPin = ParentFunctionNode->FindPin(EventPin->PinName, EGPD_Input))
						{
							ParentPin->MakeLinkTo(EventPin);
						}
					}
					ParentFunctionNode->GetExecPin()->MakeLinkTo(NewEventNode->FindPin(UEdGraphSchema_K2::PN_Then));

					ParentFunctionNode->NodePosX = FunctionFromNode.Node->NodePosX + FunctionFromNode.Node->NodeWidth + 200;
					ParentFunctionNode->NodePosY = FunctionFromNode.Node->NodePosY;
					UEdGraphSchema_K2::SetNodeMetaData(ParentFunctionNode, FNodeMetadata::DefaultGraphNode);
					FunctionNodeCreator.Finalize();

					ParentFunctionNode->MakeAutomaticallyPlacedGhostNode();
				}

				NewEventNode->MakeAutomaticallyPlacedGhostNode();
			}

			return NewEventNode;
		}

	};

	int32 NodePosY = 0;
	UBpNode_DispatchStartEvent* WhenDispatchStartNode = FActionDispatcherEventUtil::AddDefaultEventNode(NewBP, MainGraph, GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, WhenDispatchStart), UXD_ActionDispatcherBase::StaticClass(), NodePosY);
	NewBP->WhenDispatchStartNode = WhenDispatchStartNode;
	return NewBP;
}

UObject* UActionDispatcherFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(InClass, InParent, InName, Flags, Context, Warn, NAME_None);
}

FText UActionDispatcherFactory::GetDisplayName() const
{
	return LOCTEXT("创建行为调度器标题", "行为调度器");
}

bool UActionDispatcherFactory::ConfigureProperties()
{
	class FActionDispatcherFilterViewer : public IClassViewerFilter
	{
	public:
		EClassFlags AllowedClassFlags = CLASS_Abstract;

		bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<class FClassViewerFilterFuncs> InFilterFuncs) override
		{
			if (GetDefault<UXD_ActionDispatcherSettings>()->bShowPluginClass == false)
			{
				if (InClass == UXD_ActionDispatcherBase::StaticClass())
				{
					return false;
				}
			}
			return InClass->HasAnyClassFlags(AllowedClassFlags) && InClass->IsChildOf<UXD_ActionDispatcherBase>();
		}

		bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const class IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<class FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return InUnloadedClassData->HasAnyClassFlags(AllowedClassFlags) && InUnloadedClassData->IsChildOf(UXD_ActionDispatcherBase::StaticClass());
		}
	};

	ActionDispatcherClass = nullptr;

	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
	FClassViewerInitializationOptions Options;

	Options.Mode = EClassViewerMode::ClassPicker;
	Options.ClassFilter = MakeShareable<FActionDispatcherFilterViewer>(new FActionDispatcherFilterViewer);
	Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::Dynamic;

	const FText TitleText = LOCTEXT("选择调度器类型", "选择调度器类型");
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UXD_ActionDispatcherBase::StaticClass());

	if (bPressedOk)
	{
		ActionDispatcherClass = ChosenClass ? ChosenClass : UXD_ActionDispatcherBase::StaticClass();
	}

	return bPressedOk;
}

#undef LOCTEXT_NAMESPACE
