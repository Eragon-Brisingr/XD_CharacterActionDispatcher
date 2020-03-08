// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomBpNode/BpNode_PlayLevelSequencer.h"
#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <LevelSequence.h>
#include <Styling/SlateIconFinder.h>
#include <Sections/MovieSceneSubSection.h>
#include <Sections/MovieSceneCinematicShotSection.h>
#include <EditorStyleSet.h>
#include <Tracks/MovieSceneSubTrack.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <DetailWidgetRow.h>
#include <PropertyHandle.h>
#include <Widgets/Text/STextBlock.h>
#include <KismetCompiler.h>
#include <K2Node_CallFunction.h>
#include <K2Node_Knot.h>
#include <Tracks/MovieScene3DTransformTrack.h>
#include <Channels/MovieSceneFloatChannel.h>
#include <Channels/MovieSceneChannelProxy.h>
#include <K2Node_Self.h>
#include <K2Node_CustomEvent.h>
#include <K2Node_MakeArray.h>
#include <K2Node_MakeStruct.h>

#include "Action/XD_DA_PlaySequence.h"
#include "XD_BpNodeFunctionWarpper.h"
#include "CustomBpNode/Utils/DA_CustomBpNodeUtils.h"
#include "Settings/XD_ActionDispatcherSettings.h"
#include "Dispatcher/XD_ActionDispatcherBase.h"

#define LOCTEXT_NAMESPACE "CharacterActionDispatcher"

const FVector FSequencerBindingOption::InvalidLocation = FVector(FLT_MAX);
const FRotator FSequencerBindingOption::InvalidRotation = FRotator(FLT_MAX);

void FSequencerBindingOption_Customization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	TSharedPtr<IPropertyHandle> PinName_PropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSequencerBindingOption, PinName));
	TSharedPtr<IPropertyHandle> bIsPin_PropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSequencerBindingOption, bIsPin));

	FString PinName;
	PinName_PropertyHandle->GetValue(PinName);

	HeaderRow.FilterString(StructPropertyHandle->GetPropertyDisplayName())
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(PinName))
		]
	.ValueContent()
		.HAlign(HAlign_Fill)
		[
			bIsPin_PropertyHandle->CreatePropertyValueWidget(false)
		];
}

void UBpNode_PlayLevelSequencer::RefreshSequenceData()
{
	if (ULevelSequence* LevelSequenceRef = LevelSequence.LoadSynchronous())
	{
		TArray<FSequencerBindingOption> PreBindingOptions = BindingOptions;
		BindingOptions.Empty();

		//只处理了MovieSceneSequenceID::Root的可绑定信息，若需要拓展参考FSequenceBindingTree
		UMovieScene* MovieScene = LevelSequenceRef->GetMovieScene();
		for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
		{
			const FMovieScenePossessable& Binding = MovieScene->GetPossessable(i);
			if (UClass* Class = const_cast<UClass*>(Binding.GetPossessedObjectClass()))
			{
				FSequencerBindingOption BindingOption;
				BindingOption.PinName = Binding.GetName();
				BindingOption.Binding = FMovieSceneObjectBindingID(Binding.GetGuid(), MovieSceneSequenceID::Root);
				BindingOption.BindingClass = Class;

				if (FSequencerBindingOption* Option = PreBindingOptions.FindByPredicate([&](const FSequencerBindingOption& E) {return E.Binding == BindingOption.Binding; }))
				{
					BindingOption.bIsPin = Option->bIsPin;
				}

				if (const FMovieSceneBinding* SceneBinding = MovieScene->GetBindings().FindByPredicate([&](const FMovieSceneBinding& E) {return E.GetObjectGuid() == Binding.GetGuid(); }))
				{
					if (UMovieScene3DTransformTrack*const* P_TransformTrack = (UMovieScene3DTransformTrack*const*)SceneBinding->GetTracks().FindByPredicate([&](UMovieSceneTrack* E) {return E->IsA<UMovieScene3DTransformTrack>(); }))
					{
						const UMovieScene3DTransformTrack* TransformTrack = *P_TransformTrack;
						const TArray<UMovieSceneSection*>& Sections = TransformTrack->GetAllSections();
						if (Sections.Num() > 0)
						{
							UMovieScene3DTransformSection* Section = Cast<UMovieScene3DTransformSection>(Sections[0]);
							TArrayView<FMovieSceneFloatChannel*> FloatChannels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();

							float X, Y, Z;
							check(FloatChannels[0]->Evaluate(0, X));
							check(FloatChannels[1]->Evaluate(0, Y));
							check(FloatChannels[2]->Evaluate(0, Z));
							BindingOption.Location = FVector(X, Y, Z);

							float Pitch, Yaw, Roll;
							check(FloatChannels[4]->Evaluate(0, Pitch));
							check(FloatChannels[5]->Evaluate(0, Yaw));
							check(FloatChannels[6]->Evaluate(0, Roll));

							BindingOption.Rotation = FRotator(Pitch, Yaw, Roll);
						}
					}
				}

				BindingOptions.Add(BindingOption);
			}
		}
		for (int32 Index = 0; Index < MovieScene->GetSpawnableCount(); ++Index)
		{
			const FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(Index);
			if (const UObject* Template = Spawnable.GetObjectTemplate())
			{
				FSequencerBindingOption BindingOption;
				BindingOption.PinName = TEXT("[生成模板]") + Spawnable.GetName();
				BindingOption.Binding = FMovieSceneObjectBindingID(Spawnable.GetGuid(), MovieSceneSequenceID::Root);
				BindingOption.BindingClass = Template->GetClass();

				if (FSequencerBindingOption* Option = PreBindingOptions.FindByPredicate([&](const FSequencerBindingOption& E) {return E.Binding == BindingOption.Binding; }))
				{
					BindingOption.bIsPin = Option->bIsPin;
				}

				BindingOptions.Add(BindingOption);
			}
		}

		for (FSequencerBindingOption& PreOption : PreBindingOptions)
		{
			if (PreOption.bIsPin && !BindingOptions.ContainsByPredicate([&](const FSequencerBindingOption& E) {return E.Binding == PreOption.Binding; }))
			{
				PreOption.bIsPin = false;
				UpdatePinInfo(PreOption);
			}
		}
		ReflushNode();
	}
}

void UBpNode_PlayLevelSequencer::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	for (const FSequencerBindingOption& Option : BindingOptions)
	{
		UpdatePinInfo(Option);
	}
}

void UBpNode_PlayLevelSequencer::ShowExtendPins(UClass* UseSpawnClass)
{
	Super::ShowExtendPins(UseSpawnClass);
	CreateActionEventPins(UseSpawnClass);
	CreateResultPin(UseSpawnClass);
}

UBpNode_PlayLevelSequencer::UBpNode_PlayLevelSequencer()
{
	ActionClass = UXD_DA_PlaySequenceBase::StaticClass();
}

void UBpNode_PlayLevelSequencer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UBpNode_PlayLevelSequencer, LevelSequence))
	{
		RefreshSequenceData();
	}
}

void UBpNode_PlayLevelSequencer::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSequencerBindingOption, bIsPin))
	{
		int32 ModifyIndex = PropertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_STRING_CHECKED(UBpNode_PlayLevelSequencer, BindingOptions));
		const FSequencerBindingOption& Option = BindingOptions[ModifyIndex];

		UpdatePinInfo(Option);
		ReflushNode();
	}
}

FText UBpNode_PlayLevelSequencer::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	static FText EmptyName = LOCTEXT("None", "None");
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return FText::Format(LOCTEXT("PlaySequence title", "Execute Action {0} [{1}]"), ActionClass ? FText::FromString(ActionClass->GetName()) : EmptyName, ActionClass ? ActionClass->GetDisplayNameText() : EmptyName);
	}
	else
	{
		return FText::Format(LOCTEXT("PlaySequence detail title", "[{0}]({1})"), ActionClass ? ActionClass->GetDisplayNameText() : EmptyName, FText::FromString(LevelSequence.IsNull() ? TEXT("None") : LevelSequence.GetAssetName()));
	}
}

FText UBpNode_PlayLevelSequencer::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

bool UBpNode_PlayLevelSequencer::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_PlayLevelSequencer::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	DA_NodeUtils::AddDebugMenuSection(this, Menu, Context, EntryPointEventName);
}

void UBpNode_PlayLevelSequencer::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (ActionClass == nullptr)
	{
		return;
	}

	DA_NodeUtils::CreateDebugEventEntryPoint(this, CompilerContext, GetExecPin(), EntryPointEventName);

	if (LevelSequence.IsNull())
	{
		CompilerContext.MessageLog.Error(TEXT("@@ LevelSequence 为空"), this);
		return;
	}

	UK2Node_CallFunction* CreatePlaySequenceNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CreatePlaySequenceNode->SetFromFunction(UXD_DA_PlaySequenceBase::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_DA_PlaySequenceBase, CreatePlaySequenceAction)));
	CreatePlaySequenceNode->AllocateDefaultPins();
	DA_NodeUtils::SetPinStructValue(CreatePlaySequenceNode->FindPinChecked(TEXT("Sequence"), EGPD_Input), LevelSequence.ToSoftObjectPath());
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CreatePlaySequenceNode->GetExecPin());
	CompilerContext.MovePinLinksToIntermediate(*GetClassPin(), *CreatePlaySequenceNode->FindPinChecked(TEXT("SequenceType")));
	UEdGraphPin* LastThen = DA_NodeUtils::GenerateAssignmentNodes(CompilerContext, SourceGraph, CreatePlaySequenceNode, this, CreatePlaySequenceNode->GetReturnValuePin(), GetClassToSpawn());

	UK2Node_CallFunction* GetMainActionDispatcherNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	{
		GetMainActionDispatcherNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, GetMainActionDispatcher), UXD_ActionDispatcherBase::StaticClass());
		GetMainActionDispatcherNode->AllocateDefaultPins();
	}
	GetMainActionDispatcherNode->GetReturnValuePin()->MakeLinkTo(CreatePlaySequenceNode->FindPinChecked(TEXT("ActionDispatcher")));

	UK2Node_MakeArray* MakeActorDatasArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	MakeActorDatasArrayNode->NumInputs = 0;
	MakeActorDatasArrayNode->AllocateDefaultPins();
	MakeActorDatasArrayNode->GetOutputPin()->MakeLinkTo(CreatePlaySequenceNode->FindPinChecked(TEXT("ActorDatas")));
	MakeActorDatasArrayNode->PinConnectionListChanged(MakeActorDatasArrayNode->GetOutputPin());

	UK2Node_MakeArray* MakePawnDatasArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	MakePawnDatasArrayNode->NumInputs = 0;
	MakePawnDatasArrayNode->AllocateDefaultPins();
	MakePawnDatasArrayNode->GetOutputPin()->MakeLinkTo(CreatePlaySequenceNode->FindPinChecked(TEXT("MoveToDatas")));
	MakePawnDatasArrayNode->PinConnectionListChanged(MakePawnDatasArrayNode->GetOutputPin());

 	for (FSequencerBindingOption& Option : BindingOptions)
 	{
 		if (Option.bIsPin)
 		{
 			if (Option.BindingClass->IsChildOf<APawn>() && Option.IsPositionValid())
 			{
 				UK2Node_MakeStruct* MakePawnDataNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
 				MakePawnDataNode->StructType = FPlaySequenceMoveToData::StaticStruct();
				MakePawnDataNode->bMadeAfterOverridePinRemoval = true;
 				MakePawnDataNode->AllocateDefaultPins();
 
 				CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(Option.PinName, EGPD_Input), *MakePawnDataNode->FindPinChecked(TEXT("PawnRef"), EGPD_Input));
 				DA_NodeUtils::SetPinStructValue(MakePawnDataNode->FindPinChecked(GET_MEMBER_NAME_CHECKED(FPlaySequenceMoveToData, BindingID), EGPD_Input), Option.Binding);
  				DA_NodeUtils::SetPinStructValue(MakePawnDataNode->FindPinChecked(GET_MEMBER_NAME_CHECKED(FPlaySequenceMoveToData, Location), EGPD_Input), Option.Location);
 				DA_NodeUtils::SetPinStructValue(MakePawnDataNode->FindPinChecked(GET_MEMBER_NAME_CHECKED(FPlaySequenceMoveToData, Rotation), EGPD_Input), Option.Rotation);
 
 				MakePawnDatasArrayNode->AddInputPin();
 				MakePawnDatasArrayNode->FindPinChecked(FString::Printf(TEXT("[%d]"), MakePawnDatasArrayNode->NumInputs - 1))->MakeLinkTo(MakePawnDataNode->FindPinChecked(MakePawnDataNode->StructType->GetFName()));
 			}
 			else
 			{
 				UK2Node_MakeStruct* MakeActorDataNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
 				MakeActorDataNode->StructType = FPlaySequenceActorData::StaticStruct();
				MakeActorDataNode->bMadeAfterOverridePinRemoval = true;
 				MakeActorDataNode->AllocateDefaultPins();
 
 				CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(Option.PinName, EGPD_Input), *MakeActorDataNode->FindPinChecked(TEXT("ActorRef"), EGPD_Input));
 				DA_NodeUtils::SetPinStructValue(MakeActorDataNode->FindPinChecked(GET_MEMBER_NAME_CHECKED(FPlaySequenceActorData, BindingID), EGPD_Input), Option.Binding);
 
 				MakeActorDatasArrayNode->AddInputPin();
 				MakeActorDatasArrayNode->FindPinChecked(FString::Printf(TEXT("[%d]"), MakeActorDatasArrayNode->NumInputs - 1))->MakeLinkTo(MakeActorDataNode->FindPinChecked(MakeActorDataNode->StructType->GetFName()));
 			}
 		}
 	}

	LastThen = CreateAllEventNode(LastThen, CreatePlaySequenceNode->GetReturnValuePin(), EntryPointEventName, CompilerContext, SourceGraph);
	LastThen = CreateInvokeActiveActionNode(LastThen, GetMainActionDispatcherNode, CreatePlaySequenceNode->GetReturnValuePin(), CompilerContext, SourceGraph);
	LinkResultPin(GetMainActionDispatcherNode, CompilerContext, SourceGraph);

	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Then), *LastThen);
}

UClass* UBpNode_PlayLevelSequencer::GetClassPinBaseClass() const
{
	return UXD_DA_PlaySequenceBase::StaticClass();
}

void UBpNode_PlayLevelSequencer::UpdatePinInfo(const FSequencerBindingOption &Option)
{
	FName PinName = *Option.PinName;
	UEdGraphPin* Pin = FindPin(PinName, EGPD_Input);
	if (Option.bIsPin)
	{
		if (Pin == nullptr)
		{
			CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_SoftObject, Option.BindingClass, PinName);
		}
	}
	else
	{
		if (Pin)
		{
			RemovePin(Pin);
		}
	}
}

void UBpNode_PlayLevelSequencer::ReflushNode()
{
	DA_NodeUtils::UpdateNode(GetBlueprint());
}

void UBpNode_PlayLevelSequencer::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();
	EntryPointEventName = *FString::Printf(TEXT("PlayLevelSequencer_%d"), FMath::Rand());
}

void UBpNode_PlayLevelSequencer::PostPasteNode()
{
	Super::PostPasteNode();
	EntryPointEventName = *FString::Printf(TEXT("PlayLevelSequencer_%d"), FMath::Rand());
}

#undef LOCTEXT_NAMESPACE
