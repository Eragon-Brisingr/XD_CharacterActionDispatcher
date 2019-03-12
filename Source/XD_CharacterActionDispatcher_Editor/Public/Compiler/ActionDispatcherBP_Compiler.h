// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KismetCompiler.h"

class UActionDispatcherBlueprint;
class FCompilerResultsLog;
struct FKismetCompilerOptions;

/**
 * 
 */
class FActionDispatcherBP_Compiler : public FKismetCompilerContext
{
protected:
	typedef FKismetCompilerContext Super;

public:
	FActionDispatcherBP_Compiler(UActionDispatcherBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded);
	~FActionDispatcherBP_Compiler() override;

	// FKismetCompilerContext
	void PreCompile() override;
	void SpawnNewClass(const FString& NewClassName) override;
	void FinishCompilingClass(UClass* Class) override;
	// End FKismetCompilerContext

	UActionDispatcherBlueprint* ActionDispatcherBlueprint;
};
