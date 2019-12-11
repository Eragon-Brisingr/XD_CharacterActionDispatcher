// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <BlueprintEditor.h>

class UActionDispatcherBlueprint;

/**
 * 
 */
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API FActionDispatcherEditor : public FBlueprintEditor
{
public:
	void InitActionDispatcherEditor(const EToolkitMode::Type InMode, const TSharedPtr<class IToolkitHost>& InToolkitHost, UActionDispatcherBlueprint* ActionDispatcherBlueprint);
};
