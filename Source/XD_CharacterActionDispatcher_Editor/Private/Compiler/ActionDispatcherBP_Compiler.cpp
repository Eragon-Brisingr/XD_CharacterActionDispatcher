// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionDispatcherBP_Compiler.h"
#include "ActionDispatcherGeneratedClass.h"
#include "KismetReinstanceUtilities.h"
#include "ActionDispatcherBlueprint.h"
#include "XD_ActionDispatcherBase.h"

#include "LinkToFinishNodeChecker.h"
#include "BpNode_FinishDispatch.h"
FActionDispatcherBP_Compiler::FActionDispatcherBP_Compiler(UActionDispatcherBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded)
	: FKismetCompilerContext(SourceSketch, InMessageLog, InCompilerOptions, InObjLoaded)
{
	ActionDispatcherBlueprint = SourceSketch;
}

FActionDispatcherBP_Compiler::~FActionDispatcherBP_Compiler()
{

}

void FActionDispatcherBP_Compiler::PreCompile()
{
	Super::PreCompile();

	if (CompileOptions.CompileType != EKismetCompileType::SkeletonOnly)
	{
		for (const FBPVariableDescription& BPVariableDescription : Blueprint->NewVariables)
		{
			EPropertyFlags Flags = (EPropertyFlags)BPVariableDescription.PropertyFlags;
			if ((Flags & CPF_Edit) != CPF_Edit)
			{
				continue;
			}

			bool isMarkSaveGame = (Flags & CPF_SaveGame) == CPF_SaveGame;
			if ((Flags & CPF_DisableEditOnInstance) != CPF_DisableEditOnInstance)
			{
				if (!(isMarkSaveGame && BPVariableDescription.HasMetaData(FBlueprintMetadata::MD_ExposeOnSpawn)))
				{
					MessageLog.Error(*FString::Printf(TEXT("[%s] 暴露变量需添加标记[保存游戏]SaveGame与[在生成时显示]ExposeOnSpawn"), *BPVariableDescription.VarName.ToString()));
				}
			}
			else
			{
				bool isBlueprintReadOnly = (Flags & CPF_BlueprintReadOnly) == CPF_BlueprintReadOnly;
				if (!(isBlueprintReadOnly || BPVariableDescription.HasMetaData(FBlueprintMetadata::MD_Private)))
				{
					if (!isMarkSaveGame)
					{
						MessageLog.Error(*FString::Printf(TEXT("[%s] 非暴露变量需添加标记SaveGame或标记为[只读蓝图]BlueprintReadOnly或者[私有]Private"), *BPVariableDescription.VarName.ToString()));
					}
				}
			}
		}		
		
		UK2Node_Event* WhenDispatchStartNode = Cast<UK2Node_Event>(ActionDispatcherBlueprint->WhenDispatchStartNode);
		if (WhenDispatchStartNode == nullptr)
		{
			for (UEdGraph* Ubergraph : Blueprint->UbergraphPages)
			{
				for (UEdGraphNode* Node : Ubergraph->Nodes)
				{
					if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
					{
						if (EventNode->GetFunctionName() == GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, WhenDispatchStart))
						{
							WhenDispatchStartNode = EventNode;
							goto FindNode;
						}
					}
				}
			}
		}
	FindNode:
		if (WhenDispatchStartNode)
		{
			FLinkToFinishNodeChecker Checker = FLinkToFinishNodeChecker::CheckForceConnectFinishNode(WhenDispatchStartNode, MessageLog);
			ActionDispatcherBlueprint->FinishTags.Empty();
			for (UEdGraphNode* Node : Checker.VisitedNodes)
			{
				if (UBpNode_FinishDispatch* FinishDispatchNode = Cast<UBpNode_FinishDispatch>(Node))
				{
					ActionDispatcherBlueprint->FinishTags.AddUnique(FinishDispatchNode->Tag.GetTagName());
				}
			}
		}
		else
		{
			MessageLog.Error(TEXT("需要实现[执行调度]WhenDispatchStart事件"));
		}
	}
}

void FActionDispatcherBP_Compiler::SpawnNewClass(const FString& NewClassName)
{
	NewClass = FindObject<UActionDispatcherGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);
	if (NewClass == NULL)
	{
		// If the class hasn't been found, then spawn a new one
		NewClass = NewObject<UActionDispatcherGeneratedClass>(Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		// Already existed, but wasn't linked in the Blueprint yet due to load ordering issues
		NewClass->ClassGeneratedBy = Blueprint;
		FBlueprintCompileReinstancer::Create(NewClass);
	}
}

void FActionDispatcherBP_Compiler::FinishCompilingClass(UClass* Class)
{
	Super::FinishCompilingClass(Class);
}
