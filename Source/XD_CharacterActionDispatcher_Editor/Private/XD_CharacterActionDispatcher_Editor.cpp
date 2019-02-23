// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "XD_CharacterActionDispatcher_Editor.h"
#include "PropertyEditorModule.h"
#include "BpNode_PlayLevelSequencer.h"

#define LOCTEXT_NAMESPACE "FXD_CharacterActionDispatcher_EditorModule"

void FXD_CharacterActionDispatcher_EditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyTypeLayout(FSequencerBindingOption::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSequencerBindingOption_Customization::MakeInstance));
}

void FXD_CharacterActionDispatcher_EditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FXD_CharacterActionDispatcher_EditorModule, XD_CharacterActionDispatcher_Editor)