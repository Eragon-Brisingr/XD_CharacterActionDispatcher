// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionDispatcherBP_Compiler.h"
#include "ActionDispatcherGeneratedClass.h"
#include "KismetReinstanceUtilities.h"
#include "ActionDispatcherBlueprint.h"
#include "XD_ActionDispatcherBase.h"

FActionDispatcherBP_Compiler::FActionDispatcherBP_Compiler(UActionDispatcherBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded)
	: FKismetCompilerContext(SourceSketch, InMessageLog, InCompilerOptions, InObjLoaded)
{

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

	UXD_ActionDispatcherBase* CDO = Class->GetDefaultObject<UXD_ActionDispatcherBase>();
	for (TFieldIterator<UProperty> It(Class); It; ++It)
	{
		UProperty* Property = *It;
		if (Property->HasAnyPropertyFlags(CPF_Edit) && Property->HasAnyPropertyFlags(CPF_Protected) == false)
		{
			if (Property->HasAllPropertyFlags(CPF_ExposeOnSpawn | CPF_SaveGame) == false)
			{
				MessageLog.Error(TEXT("[%s]，非Private变量需添加标记SaveGame与ExposeOnSpawn"), *Property->GetName());
			}
		}

// 		FSoftObjectPtr SoftObjectPtr = SoftObjectProperty->GetPropertyValue(SoftObjectProperty->ContainerPtrToValuePtr<uint8>(CDO));
// 		if (SoftObjectPtr.Get() == nullptr)
// 		{
// 
// 		}
	}
}
