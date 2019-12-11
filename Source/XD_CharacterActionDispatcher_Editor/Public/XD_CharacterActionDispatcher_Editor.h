// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <Modules/ModuleManager.h>

class FSlateStyleSet;
class FActionDispatcher_AssetActions;

class FXD_CharacterActionDispatcher_EditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	static TSharedPtr<FKismetCompilerContext> GetCompilerForBP(UBlueprint* BP, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions);

	TSharedPtr<FSlateStyleSet> StyleSet;
	TSharedPtr<FActionDispatcher_AssetActions> ActionDispatcherAssetActions;
};
