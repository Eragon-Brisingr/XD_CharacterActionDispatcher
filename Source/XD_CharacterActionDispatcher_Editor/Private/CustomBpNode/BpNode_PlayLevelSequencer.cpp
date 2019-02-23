// Fill out your copyright notice in the Description page of Project Settings.

#include "BpNode_PlayLevelSequencer.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "MovieSceneSequence.h"
#include "SlateIconFinder.h"
#include "MovieSceneSubSection.h"
#include "MovieSceneCinematicShotSection.h"
#include "EditorStyleSet.h"
#include "MovieSceneSubTrack.h"
#include "BlueprintEditorUtils.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "STextBlock.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "LevelSequence/Public/LevelSequenceActor.h"
#include "K2Node_Knot.h"
#include "XD_BpNodeFunctionWarpper.h"

#define LOCTEXT_NAMESPACE "CharacterActionDispatcher"

void FSequencerBindingOption_Customization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	TSharedPtr<IPropertyHandle> DisplayName_PropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSequencerBindingOption, DisplayName));
	TSharedPtr<IPropertyHandle> bIsPin_PropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSequencerBindingOption, bIsPin));

	FString DisplayName;
	DisplayName_PropertyHandle->GetValue(DisplayName);

	HeaderRow.FilterString(StructPropertyHandle->GetPropertyDisplayName())
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(DisplayName))
		]
	.ValueContent()
		.HAlign(HAlign_Fill)
		[
			bIsPin_PropertyHandle->CreatePropertyValueWidget(false)
		];
}

/** Node that represents an object binding, or a sub sequence (where the guid is zero) */
struct FSequenceBindingNode
{
	FSequenceBindingNode(FText InDisplayString, FMovieSceneObjectBindingID InBindingID, FSlateIcon InIcon)
		: BindingID(InBindingID)
		, DisplayString(InDisplayString)
		, Icon(InIcon)
		, bIsSpawnable(false)
	{}

	/** Add a child */
	void AddChild(TSharedRef<FSequenceBindingNode> Child)
	{
		Child->ParentID = BindingID;
		Children.Add(Child);
	}

	/** This object's ID, and its parent's */
	FMovieSceneObjectBindingID BindingID, ParentID;
	/** The display string that represents this node */
	FText DisplayString;
	/** A representative icon for the node */
	FSlateIcon Icon;
	/** Whether this is a spawnable or not */
	bool bIsSpawnable;
	/** Array holding this node's children */
	TArray<TSharedRef<FSequenceBindingNode>> Children;
};

struct FSequenceBindingNodeIterator
{
	template<typename FFunctor>
	static void ForEachChild(const TSharedRef<FSequenceBindingNode>& SequenceBindingNode, const FFunctor& Functor)
	{
		for (TSharedRef<FSequenceBindingNode>& Child : SequenceBindingNode->Children)
		{
			Functor(Child);
			ForEachChild(Child, Functor);
		}
	}
};

/** Stack of sequence IDs from parent to child */
struct FSequenceIDStack
{
	/** Get the current accumulated sequence ID */
	FMovieSceneSequenceID GetCurrent() const
	{
		FMovieSceneSequenceID ID = MovieSceneSequenceID::Root;
		for (int32 Index = IDs.Num() - 1; Index >= 0; --Index)
		{
			ID = ID.AccumulateParentID(IDs[Index]);
		}
		return ID;
	}

	/** Push a sequence ID onto the stack */
	void Push(FMovieSceneSequenceID InSequenceID) { IDs.Add(InSequenceID); }

	/** Pop the last sequence ID off the stack */
	void Pop() { IDs.RemoveAt(IDs.Num() - 1, 1, false); }

private:
	TArray<FMovieSceneSequenceID> IDs;
};

/** Data structure used internally to represent the bindings of a sequence recursively */
struct FSequenceBindingTree
{
	/**
	 * Construct the tree structure from the specified sequence.
	 *
	 * @param InSequence			The sequence to generate the tree for
	 * @param InActiveSequence		A sequence at which point we can start to generate localally resolving IDs
	 * @param InActiveSequenceID	The sequence ID for the above sequence within the root context
	 */
	void Build(UMovieSceneSequence* InSequence, FObjectKey InActiveSequence, FMovieSceneSequenceID InActiveSequenceID)
	{
		bIsEmpty = true;

		// Reset state
		ActiveSequenceID = InActiveSequenceID;
		ActiveSequence = InActiveSequence;
		Hierarchy.Reset();

		ActiveSequenceNode = nullptr;

		// Create a node for the root sequence
		FMovieSceneObjectBindingID RootSequenceID;
		TSharedRef<FSequenceBindingNode> RootSequenceNode = MakeShared<FSequenceBindingNode>(FText(), RootSequenceID, FSlateIcon());
		Hierarchy.Add(RootSequenceID, RootSequenceNode);

		TopLevelNode = RootSequenceNode;

		if (InSequence)
		{
			RootSequenceNode->DisplayString = FText::FromString(InSequence->GetName());
			RootSequenceNode->Icon = FSlateIconFinder::FindIconForClass(InSequence->GetClass());

			// Build the tree
			FSequenceIDStack SequenceIDStack;
			Build(InSequence, SequenceIDStack);

			// Sort the tree
			Sort(RootSequenceNode);

			// We don't show cross-references to the same sequence since this would result in erroneous mixtures of absolute and local bindings
			if (ActiveSequenceNode.IsValid() && ActiveSequenceNode != RootSequenceNode)
			{
				// Remove it from its parent, and put it at the root for quick access
				TSharedPtr<FSequenceBindingNode> ActiveParent = Hierarchy.FindChecked(ActiveSequenceNode->ParentID);
				ActiveParent->Children.Remove(ActiveSequenceNode.ToSharedRef());

				// Make a new top level node (with an invalid ID)
				FMovieSceneObjectBindingID TopLevelID = FMovieSceneObjectBindingID(FGuid(), FMovieSceneSequenceID());
				TopLevelNode = MakeShared<FSequenceBindingNode>(FText(), TopLevelID, FSlateIcon());

				// Override the display string and icon
				ActiveSequenceNode->DisplayString = LOCTEXT("ThisSequenceText", "This Sequence");
				ActiveSequenceNode->Icon = FSlateIcon();

				TopLevelNode->Children.Add(ActiveSequenceNode.ToSharedRef());
				TopLevelNode->Children.Add(RootSequenceNode);
			}
		}
	}

	/** Get the root of the tree */
	TSharedRef<FSequenceBindingNode> GetRootNode() const
	{
		return TopLevelNode.ToSharedRef();
	}

	/** Find a node in the tree */
	TSharedPtr<FSequenceBindingNode> FindNode(FMovieSceneObjectBindingID BindingID) const
	{
		return Hierarchy.FindRef(BindingID);
	}

	bool IsEmpty() const
	{
		return bIsEmpty;
	}

private:

	/** Recursive sort helper for a sequence binding node */
	static void Sort(TSharedRef<FSequenceBindingNode> Node)
	{
		Node->Children.Sort(
			[](TSharedRef<FSequenceBindingNode> A, TSharedRef<FSequenceBindingNode> B)
		{
			// Sort shots first
			if (A->BindingID.IsValid() != B->BindingID.IsValid())
			{
				return !A->BindingID.IsValid();
			}
			return A->DisplayString.CompareToCaseIgnored(B->DisplayString) < 0;
		}
		);

		for (TSharedRef<FSequenceBindingNode> Child : Node->Children)
		{
			Sort(Child);
		}
	}

	/** Recursive builder function that iterates into sub sequences */
	void Build(UMovieSceneSequence* InSequence, FSequenceIDStack& SequenceIDStack)
	{
		check(InSequence);

		UMovieScene* MovieScene = InSequence->GetMovieScene();
		if (!MovieScene)
		{
			return;
		}

		if (ActiveSequence == InSequence)
		{
			// Don't allow cross-references to the same sequence (ie, re-entrant references)
			if (SequenceIDStack.GetCurrent() != ActiveSequenceID)
			{
				return;
			}

			// Keep track of the active sequence node
			ActiveSequenceNode = Hierarchy.FindChecked(FMovieSceneObjectBindingID(FGuid(), SequenceIDStack.GetCurrent()));
		}

		// Iterate all sub sections
		for (const UMovieSceneTrack* MasterTrack : MovieScene->GetMasterTracks())
		{
			const UMovieSceneSubTrack* SubTrack = Cast<const UMovieSceneSubTrack>(MasterTrack);
			if (SubTrack)
			{
				for (UMovieSceneSection* Section : SubTrack->GetAllSections())
				{
					UMovieSceneSubSection* SubSection = Cast<UMovieSceneSubSection>(Section);
					UMovieSceneSequence* SubSequence = SubSection ? SubSection->GetSequence() : nullptr;
					if (SubSequence)
					{
						// Hold onto the current parent ID before adding our ID onto the stack
						FMovieSceneSequenceID ParentID = SequenceIDStack.GetCurrent();
						SequenceIDStack.Push(SubSection->GetSequenceID());

						FMovieSceneObjectBindingID CurrentID(FGuid(), SequenceIDStack.GetCurrent());

						UMovieSceneCinematicShotSection* ShotSection = Cast<UMovieSceneCinematicShotSection>(Section);
						FText DisplayString = ShotSection ? FText::FromString(ShotSection->GetShotDisplayName()) : FText::FromName(SubSequence->GetFName());
						FSlateIcon Icon(FEditorStyle::GetStyleSetName(), ShotSection ? "Sequencer.Tracks.CinematicShot" : "Sequencer.Tracks.Sub");

						TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(DisplayString, CurrentID, Icon);
						ensure(!Hierarchy.Contains(CurrentID));
						Hierarchy.Add(CurrentID, NewNode);

						EnsureParent(FGuid(), MovieScene, ParentID)->AddChild(NewNode);

						Build(SubSequence, SequenceIDStack);

						SequenceIDStack.Pop();
					}
				}
			}
		}

		FMovieSceneSequenceID CurrentSequenceID = SequenceIDStack.GetCurrent();

		// Add all spawnables first (since possessables can be children of spawnables)
		int32 SpawnableCount = MovieScene->GetSpawnableCount();
		for (int32 Index = 0; Index < SpawnableCount; ++Index)
		{
			const FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(Index);

			FMovieSceneObjectBindingID ID(Spawnable.GetGuid(), CurrentSequenceID);

			FSlateIcon Icon = FSlateIconFinder::FindIconForClass(Spawnable.GetObjectTemplate()->GetClass());
			TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(MovieScene->GetObjectDisplayName(Spawnable.GetGuid()), ID, Icon);
			NewNode->bIsSpawnable = true;

			EnsureParent(FGuid(), MovieScene, CurrentSequenceID)->AddChild(NewNode);
			ensure(!Hierarchy.Contains(ID));
			Hierarchy.Add(ID, NewNode);

			bIsEmpty = false;
		}

		// Add all possessables
		const int32 PossessableCount = MovieScene->GetPossessableCount();
		for (int32 Index = 0; Index < PossessableCount; ++Index)
		{
			const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(Index);
			if (InSequence->CanRebindPossessable(Possessable))
			{
				FMovieSceneObjectBindingID ID(Possessable.GetGuid(), CurrentSequenceID);

				FSlateIcon Icon = FSlateIconFinder::FindIconForClass(Possessable.GetPossessedObjectClass());
				TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(MovieScene->GetObjectDisplayName(Possessable.GetGuid()), ID, Icon);

				EnsureParent(Possessable.GetParent(), MovieScene, CurrentSequenceID)->AddChild(NewNode);
				ensure(!Hierarchy.Contains(ID));
				Hierarchy.Add(ID, NewNode);

				bIsEmpty = false;
			}
		}
	}

	/** Ensure that a parent node exists for the specified object */
	TSharedRef<FSequenceBindingNode> EnsureParent(const FGuid& InParentGuid, UMovieScene* InMovieScene, FMovieSceneSequenceID SequenceID)
	{
		FMovieSceneObjectBindingID ParentPtr(InParentGuid, SequenceID);

		// If the node already exists
		TSharedPtr<FSequenceBindingNode> Parent = Hierarchy.FindRef(ParentPtr);
		if (Parent.IsValid())
		{
			return Parent.ToSharedRef();
		}

		// Non-object binding nodes should have already been added externally to EnsureParent
		check(InParentGuid.IsValid());

		// Need to add it
		FGuid AddToGuid;
		if (FMovieScenePossessable* GrandParentPossessable = InMovieScene->FindPossessable(InParentGuid))
		{
			AddToGuid = GrandParentPossessable->GetGuid();
		}

		// Deduce the icon for the node
		FSlateIcon Icon;
		bool bIsSpawnable = false;
		{
			const FMovieScenePossessable* Possessable = InMovieScene->FindPossessable(InParentGuid);
			const FMovieSceneSpawnable* Spawnable = Possessable ? nullptr : InMovieScene->FindSpawnable(InParentGuid);
			if (Possessable || Spawnable)
			{
				Icon = FSlateIconFinder::FindIconForClass(Possessable ? Possessable->GetPossessedObjectClass() : Spawnable->GetObjectTemplate()->GetClass());
			}

			bIsSpawnable = Spawnable != nullptr;
		}

		TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(InMovieScene->GetObjectDisplayName(InParentGuid), ParentPtr, Icon);
		NewNode->bIsSpawnable = bIsSpawnable;

		ensure(!Hierarchy.Contains(ParentPtr));
		Hierarchy.Add(ParentPtr, NewNode);
		bIsEmpty = false;

		EnsureParent(AddToGuid, InMovieScene, SequenceID)->AddChild(NewNode);

		return NewNode;
	}

private:

	/** The ID of the currently 'active' sequence from which to generate relative IDs */
	FMovieSceneSequenceID ActiveSequenceID;
	/** The currently 'active' sequence from which to generate relative IDs */
	FObjectKey ActiveSequence;
	/** The node relating to the currently active sequence ID (if any) */
	TSharedPtr<FSequenceBindingNode> ActiveSequenceNode;
	/** The top level (root) node in the tree */
	TSharedPtr<FSequenceBindingNode> TopLevelNode;
	/** Map of hierarchical information */
	TMap<FMovieSceneObjectBindingID, TSharedPtr<FSequenceBindingNode>> Hierarchy;
	/** Whether the tree is considered empty */
	bool bIsEmpty;
};

FName SequenceActorPinName = TEXT("LevelSequence");

void UBpNode_PlayLevelSequencer::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, NAME_None, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, NAME_None, UEdGraphSchema_K2::PN_Then);

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, ALevelSequenceActor::StaticClass(), SequenceActorPinName);

	for (const FSequencerBindingOption& Option : BindingOptions)
	{
		UpdatePinInfo(Option);
	}
}

void UBpNode_PlayLevelSequencer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UBpNode_PlayLevelSequencer, Sequencer))
	{
		if (UMovieSceneSequence* MovieSceneSequence = Sequencer.LoadSynchronous())
		{
			if (!DataTree.IsValid())
			{
				DataTree = MakeShared<FSequenceBindingTree>();
			}
			DataTree->Build(MovieSceneSequence, MovieSceneSequence, MovieSceneSequenceID::Root);

			TArray<FSequencerBindingOption> PreBindingOptions = BindingOptions;
			BindingOptions.Empty();

			FSequenceBindingNodeIterator::ForEachChild(DataTree->GetRootNode(), [&](const TSharedRef<FSequenceBindingNode>& SequenceBindingNode)
			{
				FSequencerBindingOption BindingOption;
				BindingOption.DisplayName = SequenceBindingNode->DisplayString.ToString();
				BindingOption.Binding = SequenceBindingNode->BindingID;

				if (FSequencerBindingOption* Option = PreBindingOptions.FindByPredicate([&](const FSequencerBindingOption& E) {return E.Binding == BindingOption.Binding; }))
				{
					BindingOption.bIsPin = Option->bIsPin;
				}

				BindingOptions.Add(BindingOption);
			});

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
}

void UBpNode_PlayLevelSequencer::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSequencerBindingOption, bIsPin))
	{
		int32 ModifyIndex = PropertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_STRING_CHECKED(UBpNode_PlayLevelSequencer, BindingOptions));
		const FSequencerBindingOption& Option = BindingOptions[ModifyIndex];

		UpdatePinInfo(Option);
		ReflushNode();
	}
}

void UBpNode_PlayLevelSequencer::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UBpNode_PlayLevelSequencer::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UK2Node_Knot* SequencerPinNode = CompilerContext.SpawnIntermediateNode<UK2Node_Knot>(this, SourceGraph);
	SequencerPinNode->AllocateDefaultPins();
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(SequenceActorPinName), *SequencerPinNode->GetInputPin());

	UK2Node_CallFunction* SetSequencerFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	SetSequencerFunction->SetFromFunction(ALevelSequenceActor::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(ALevelSequenceActor, SetSequence)));
	SetSequencerFunction->AllocateDefaultPins();
	SequencerPinNode->GetOutputPin()->MakeLinkTo(SetSequencerFunction->FindPinChecked(UEdGraphSchema_K2::PN_Self));
	SetSequencerFunction->FindPinChecked(TEXT("InSequence"))->DefaultObject = Sequencer.Get();

	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *SetSequencerFunction->GetExecPin());

	UEdGraphPin* PreThenPin = SetSequencerFunction->GetThenPin();
	for (FSequencerBindingOption& Option : BindingOptions)
	{
		if (Option.bIsPin)
		{
			UK2Node_CallFunction* AddBindingFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			AddBindingFunction->SetFromFunction(ALevelSequenceActor::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(ALevelSequenceActor, AddBinding)));
			AddBindingFunction->AllocateDefaultPins();

			UEdGraphPin* BindingPin = AddBindingFunction->FindPinChecked(TEXT("Binding"), EGPD_Input);
			FMovieSceneObjectBindingID::StaticStruct()->ExportText(BindingPin->DefaultValue, &Option.Binding, nullptr, nullptr, 0, nullptr);

			PreThenPin->MakeLinkTo(AddBindingFunction->GetExecPin());
			PreThenPin = AddBindingFunction->GetThenPin();

			SequencerPinNode->GetOutputPin()->MakeLinkTo(AddBindingFunction->FindPinChecked(UEdGraphSchema_K2::PN_Self));
			CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(*Option.DisplayName), *AddBindingFunction->FindPinChecked(TEXT("Actor")));
		}
	}

	UK2Node_CallFunction* LevelSequencerPlayFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	LevelSequencerPlayFunction->SetFromFunction(UXD_BpNodeFunctionWarpper::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UXD_BpNodeFunctionWarpper, PlayLevelSequence)));
	LevelSequencerPlayFunction->AllocateDefaultPins();

	PreThenPin->MakeLinkTo(LevelSequencerPlayFunction->GetExecPin());
	SequencerPinNode->GetOutputPin()->MakeLinkTo(LevelSequencerPlayFunction->FindPinChecked(TEXT("LevelSequenceActor")));

	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Then), *LevelSequencerPlayFunction->GetThenPin());
}

void UBpNode_PlayLevelSequencer::UpdatePinInfo(const FSequencerBindingOption &Option)
{
	FName PinName = *Option.DisplayName;
	UEdGraphPin* Pin = FindPin(PinName, EGPD_Input);
	if (Option.bIsPin)
	{
		if (Pin == nullptr)
		{
			CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, AActor::StaticClass(), PinName);
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
	if (!GetBlueprint()->bBeingCompiled)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

#undef LOCTEXT_NAMESPACE
