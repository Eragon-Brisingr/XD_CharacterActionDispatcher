// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "XD_CharacterActionDispatcher_Editor.h"
#include "PropertyEditorModule.h"
#include "BpNode_PlayLevelSequencer.h"
#include "EdGraphUtilities.h"
#include "DA_RoleSelectionGraphPin.h"
#include "ActionDispatcherBP_Compiler.h"
#include "ActionDispatcherBlueprint.h"
#include "SlateStyle.h"
#include "IPluginManager.h"
#include "SlateImageBrush.h"
#include "SlateStyleRegistry.h"
#include "AssetToolsModule.h"
#include "ActionDispatcher_AssetActions.h"

#define LOCTEXT_NAMESPACE "FXD_CharacterActionDispatcher_EditorModule"

void FXD_CharacterActionDispatcher_EditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyTypeLayout(FSequencerBindingOption::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSequencerBindingOption_Customization::MakeInstance));

	FEdGraphUtilities::RegisterVisualPinFactory(MakeShared<FDA_RoleSelectionGraphPinFactory>());

	// 注册资源操作
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	ActionDispatcherAssetActions = MakeShareable(new FActionDispatcher_AssetActions());
	AssetTools.RegisterAssetTypeActions(ActionDispatcherAssetActions.ToSharedRef());

	FKismetCompilerContext::RegisterCompilerForBP(UActionDispatcherBlueprint::StaticClass(), &FXD_CharacterActionDispatcher_EditorModule::GetCompilerForBP);

	{
		// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

		StyleSet = MakeShareable(new FSlateStyleSet("ActionDispatcherStyle"));

		//Content path of this plugin
		FString ContentDir = IPluginManager::Get().FindPlugin("XD_CharacterActionDispatcher")->GetBaseDir();

		//The image we wish to load is located inside the Resources folder inside the Base Directory
		//so let's set the content dir to the base dir and manually switch to the Resources folder:
		StyleSet->SetContentRoot(ContentDir);

		//Create a brush from the icon
		FSlateImageBrush* ThumbnailBrush = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/Icon128"), TEXT(".png")), FVector2D(128.f, 128.f));

		if (ThumbnailBrush)
		{
			//In order to bind the thumbnail to our class we need to type ClassThumbnail.X where X is the name of the C++ class of the asset
			StyleSet->Set("ClassThumbnail.XD_ActionDispatcherBase", ThumbnailBrush);

			//Reguster the created style
			FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
		}
	}
}

void FXD_CharacterActionDispatcher_EditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	//Unregister the style
	FSlateStyleRegistry::UnRegisterSlateStyle(StyleSet->GetStyleSetName());

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(ActionDispatcherAssetActions.ToSharedRef());
	}
}

TSharedPtr<FKismetCompilerContext> FXD_CharacterActionDispatcher_EditorModule::GetCompilerForBP(UBlueprint* BP, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions)
{
	return TSharedPtr<FKismetCompilerContext>(new FActionDispatcherBP_Compiler(CastChecked<UActionDispatcherBlueprint>(BP), InMessageLog, InCompileOptions));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FXD_CharacterActionDispatcher_EditorModule, XD_CharacterActionDispatcher_Editor)