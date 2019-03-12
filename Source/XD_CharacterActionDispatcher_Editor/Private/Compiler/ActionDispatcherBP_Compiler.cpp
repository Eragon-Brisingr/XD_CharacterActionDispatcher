// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionDispatcherBP_Compiler.h"
#include "ActionDispatcherGeneratedClass.h"
#include "KismetReinstanceUtilities.h"
#include "ActionDispatcherBlueprint.h"
#include "XD_ActionDispatcherBase.h"

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

	if (CompileOptions.CompileType != EKismetCompileType::SkeletonOnly)
	{
		UXD_ActionDispatcherBase* CDO = Class->GetDefaultObject<UXD_ActionDispatcherBase>();
		for (TFieldIterator<UProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			UProperty* Property = *It;
			if (Property->HasAnyPropertyFlags(CPF_Edit) == false)
			{
				continue;
			}

			if (Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance) == false)
			{
				Property->SetPropertyFlags(CPF_ExposeOnSpawn);
				if (Property->HasAllPropertyFlags(CPF_SaveGame) == false)
				{
					MessageLog.Error(TEXT("@@ ExposeOnSpawn变量需添加标记SaveGame"), Property);
				}
			}
			else
			{
				if (Property->HasAllPropertyFlags(CPF_BlueprintReadOnly) == false)
				{
					if (Property->HasAllPropertyFlags(CPF_SaveGame) == false)
					{
						MessageLog.Error(TEXT("@@ 非暴露变量需添加标记SaveGame或标记为BlueprintReadOnly"), Property);
					}
				}
			}
		}

		if (UK2Node_Event* WhenDispatchStartNode = Cast<UK2Node_Event>(ActionDispatcherBlueprint->WhenDispatchStartNode))
		{
			
		}
	}
}
