// Fill out your copyright notice in the Description page of Project Settings.


#include "BpNode_CheckStateTag.h"
#include "DA_CustomBpNodeUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GraphEditAction.h"
#include "KismetCompiler.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_CallFunction.h"
#include "XD_DispatchableEntityInterface.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

FName UBpNode_CheckStateTag::TagExistPinName = TEXT("TagExist");
FName UBpNode_CheckStateTag::TargetPinName = TEXT("Target");
FName UBpNode_CheckStateTag::TagPinName = TEXT("Tag");
FName UBpNode_CheckStateTag::TagNotExistPinName = TEXT("TagNotExist");
FName UBpNode_CheckStateTag::LinkStateTagPinName = TEXT("LinkStateTag");

FText UBpNode_CheckStateTag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Check State Tag Title", "Check State Tag [检查Tag]");
}

FText UBpNode_CheckStateTag::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

void UBpNode_CheckStateTag::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool UBpNode_CheckStateTag::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_CheckStateTag::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, AActor::StaticClass(), UBpNode_CheckStateTag::TargetPinName);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FGameplayTag::StaticStruct(), UBpNode_CheckStateTag::TagPinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UBpNode_CheckStateTag::TagNotExistPinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UBpNode_CheckStateTag::TagExistPinName);
	CreatePin(EGPD_Output, UBpNode_CheckStateTag::LinkStateTagPinName, UBpNode_CheckStateTag::LinkStateTagPinName);

	UEdGraph* Graph = GetGraph();
	Graph->RemoveOnGraphChangedHandler(OnGraphNodeChangeHandle);
	OnGraphNodeChangeHandle = Graph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateLambda([=](const FEdGraphEditAction& Action)
		{
			if (Action.Action == GRAPHACTION_RemoveNode)
			{
				if (Action.Nodes.Contains(this))
				{
					Graph->RemoveNode(LinkedAddStateTagNode);
					Graph->RemoveOnGraphChangedHandler(OnGraphNodeChangeHandle);
				}
			}
		}));
}

void UBpNode_CheckStateTag::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* TagNotExistPin = FindPinChecked(TagNotExistPinName);
	bool IsLinkedAddTagNode = false;
	UEdGraphNode* TargetNode = FindPinChecked(LinkStateTagPinName)->LinkedTo[0]->GetOwningNode();
	if (TagNotExistPin->LinkedTo.Num() > 0)
	{
		struct
		{
			bool Check(UEdGraphNode* Node, UEdGraphNode* InTargetNode)
			{
				TargetNode = InTargetNode;
				return CheckImpl(Node);
			}
		private:
			TArray<UEdGraphNode*> VisitedNodes;
			UEdGraphNode* TargetNode;

			bool CheckImpl(UEdGraphNode* Node)
			{
				if (Node == TargetNode)
				{
					return true;
				}
				if (VisitedNodes.Contains(Node))
				{
					return false;
				}
				VisitedNodes.Add(Node);

				for (UEdGraphPin* Pin : Node->Pins)
				{
					if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
					{
						for (UEdGraphPin* LinkToPin : Pin->LinkedTo)
						{
							UEdGraphNode* NextNode = LinkToPin->GetOwningNode();
							if (CheckImpl(NextNode))
							{
								return true;
							}
						}
					}
				}
				return false;
			}
		} NodeValidChecker;
		IsLinkedAddTagNode = NodeValidChecker.Check(TagNotExistPin->LinkedTo[0]->GetOwningNode(), TargetNode);
	}
	if (!IsLinkedAddTagNode)
	{
		CompilerContext.MessageLog.Error(TEXT("@@ 后续节点都未连接至 @@"), this, TargetNode);
		return;
	}
	UEdGraphPin* TragetPin = FindPinChecked(TargetPinName);
	if (TragetPin->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(TEXT("@@ @@ 目标不得为空"), this, TragetPin);
		return;
	}
	UEdGraphPin* TagPin = FindPinChecked(TagPinName);
	if (TragetPin->LinkedTo.Num() == 0)
	{
		if (TragetPin->DefaultValue.IsEmpty())
		{
			CompilerContext.MessageLog.Error(TEXT("@@ @@ 必须为有效的Tag"), this, TagPin);
			return;
		}
	}

	UK2Node_IfThenElse* BranchNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	BranchNode->AllocateDefaultPins();
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *BranchNode->GetExecPin());
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(TagNotExistPinName), *BranchNode->GetElsePin());
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(TagExistPinName), *BranchNode->GetThenPin());

	UK2Node_CallFunction* HasStateTagNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	HasStateTagNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_DA_StateTagUtils, HasStateTag), UXD_DA_StateTagUtils::StaticClass());
	HasStateTagNode->AllocateDefaultPins();
	HasStateTagNode->FindPinChecked(TEXT("Obj"))->MakeLinkTo(TragetPin->LinkedTo[0]);
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(TagPinName), *HasStateTagNode->FindPinChecked(TEXT("Tag")));
	HasStateTagNode->GetReturnValuePin()->MakeLinkTo(BranchNode->GetConditionPin());
}

void UBpNode_CheckStateTag::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	FGraphNodeCreator<UBpNode_AddStateTag> AddStateTagNodeCreator(*GetGraph());
	UBpNode_AddStateTag* AddStateTagNode = AddStateTagNodeCreator.CreateNode();
	AddStateTagNode->AllocateDefaultPins();
	AddStateTagNode->NodePosX = NodePosX + 400.f;
	AddStateTagNode->NodePosY = NodePosY;
	FindPinChecked(UBpNode_CheckStateTag::TagNotExistPinName)->MakeLinkTo(AddStateTagNode->GetExecPin());
	FindPinChecked(UBpNode_CheckStateTag::LinkStateTagPinName)->MakeLinkTo(AddStateTagNode->FindPinChecked(UBpNode_CheckStateTag::LinkStateTagPinName));
	LinkedAddStateTagNode = AddStateTagNode;
	AddStateTagNode->LinkedAddStateTagNode = this;
	AddStateTagNodeCreator.Finalize();
}

void UBpNode_CheckStateTag::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);
	if (Pin && Pin->PinName == LinkStateTagPinName)
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			Pin->MakeLinkTo(LinkedAddStateTagNode->FindPinChecked(LinkStateTagPinName));
		}
	}
}

FText UBpNode_AddStateTag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Add State Tag Title", "Add State Tag [增加Tag]");
}

FText UBpNode_AddStateTag::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

bool UBpNode_AddStateTag::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph) && DA_NodeUtils::IsActionDispatcherGraph(TargetGraph);
}

void UBpNode_AddStateTag::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UBpNode_CheckStateTag::LinkStateTagPinName, UBpNode_CheckStateTag::LinkStateTagPinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

void UBpNode_AddStateTag::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UBpNode_CheckStateTag* BpNode_CheckStateTag = CastChecked<UBpNode_CheckStateTag>(FindPinChecked(UBpNode_CheckStateTag::LinkStateTagPinName)->LinkedTo[0]->GetOwningNode());

	UK2Node_CallFunction* AddStateTagNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	AddStateTagNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UXD_DA_StateTagUtils, AddStateTag), UXD_DA_StateTagUtils::StaticClass());
	AddStateTagNode->AllocateDefaultPins();
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *AddStateTagNode->GetExecPin());
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Then), *AddStateTagNode->GetThenPin());
	UEdGraphPin* TargetPin = BpNode_CheckStateTag->FindPinChecked(UBpNode_CheckStateTag::TargetPinName);
	if (TargetPin->LinkedTo.Num() > 0)
	{
		AddStateTagNode->FindPinChecked(TEXT("Obj"))->MakeLinkTo(TargetPin->LinkedTo[0]);
	}
	UEdGraphPin* TagPin = BpNode_CheckStateTag->FindPinChecked(UBpNode_CheckStateTag::TagPinName);
	if (TagPin->LinkedTo.Num() > 0)
	{
		AddStateTagNode->FindPinChecked(TEXT("Tag"))->MakeLinkTo(TagPin->LinkedTo[0]);
	}
	else
	{
		AddStateTagNode->FindPinChecked(TEXT("Tag"))->DefaultValue = TagPin->DefaultValue;
	}
}

void UBpNode_AddStateTag::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);
	if (Pin && Pin->PinName == UBpNode_CheckStateTag::LinkStateTagPinName)
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			Pin->MakeLinkTo(LinkedAddStateTagNode->FindPinChecked(UBpNode_CheckStateTag::LinkStateTagPinName));
		}
	}
}

#undef LOCTEXT_NAMESPACE
