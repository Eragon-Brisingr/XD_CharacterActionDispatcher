// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionDispatcherBP_Compiler.h"
#include "ActionDispatcherGeneratedClass.h"
#include "KismetReinstanceUtilities.h"
#include "ActionDispatcherBlueprint.h"
#include "XD_ActionDispatcherBase.h"

#include "LinkToFinishNodeChecker.h"
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
			if ((Flags & CPF_Edit) == CPF_Edit)
			{
				continue;
			}

			if ((Flags & CPF_DisableEditOnInstance) == CPF_DisableEditOnInstance)
			{
				EPropertyFlags CheckFlag = CPF_ExposeOnSpawn | CPF_SaveGame;
				if ((Flags & CheckFlag) == CheckFlag)
				{
					MessageLog.Error(*FString::Printf(TEXT("[%s] 暴露变量需添加标记SaveGame与ExposeOnSpawn"), *BPVariableDescription.VarName.ToString()));
				}
			}
			else
			{
				if ((Flags & CPF_BlueprintReadOnly) == CPF_BlueprintReadOnly)
				{
					if ((Flags & CPF_SaveGame) == CPF_SaveGame)
					{
						MessageLog.Error(*FString::Printf(TEXT("[%s] 非暴露变量需添加标记SaveGame或标记为BlueprintReadOnly"), *BPVariableDescription.VarName.ToString()));
					}
				}
			}
		}		
		
		if (UK2Node_Event* WhenDispatchStartNode = Cast<UK2Node_Event>(ActionDispatcherBlueprint->WhenDispatchStartNode))
		{
			FLinkToFinishNodeChecker::DoCheck(WhenDispatchStartNode, MessageLog);
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
