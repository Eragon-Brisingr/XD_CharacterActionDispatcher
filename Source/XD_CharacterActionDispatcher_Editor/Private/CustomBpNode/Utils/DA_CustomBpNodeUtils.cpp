// Fill out your copyright notice in the Description page of Project Settings.

#include "DA_CustomBpNodeUtils.h"
#include "BlueprintEditorUtils.h"
#include "XD_ActionDispatcherBase.h"

bool DA_NodeUtils::IsActionDispatcherGraph(const UEdGraph* TargetGraph)
{
	const EGraphType GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	bool bIsValidGraphType = GraphType == EGraphType::GT_Ubergraph || GraphType == EGraphType::GT_Macro;
	return bIsValidGraphType && Blueprint->GeneratedClass->IsChildOf(UXD_ActionDispatcherBase::StaticClass());
}

void DA_NodeUtils::UpdateNode(UBlueprint* Blueprint)
{
	if (!Blueprint->bBeingCompiled)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}
