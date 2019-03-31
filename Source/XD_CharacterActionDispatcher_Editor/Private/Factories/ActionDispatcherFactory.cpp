// Fill out your copyright notice in the Description page of Project Settings.
#include "ActionDispatcherFactory.h"
#include "KismetEditorUtilities.h"
#include "XD_ActionDispatcherBase.h"
#include "ActionDispatcherBlueprint.h"
#include "ActionDispatcherGeneratedClass.h"
#include "AssetTypeCategories.h"
#include "ModuleManager.h"
#include "KismetCompilerModule.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "SClassPickerDialog.h"
#include "XD_ActionDispatcherSettings.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

UActionDispatcherFactory::UActionDispatcherFactory()
{
	SupportedClass = UActionDispatcherBlueprint::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UActionDispatcherFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UClass* BlueprintClass = nullptr;
	UClass* BlueprintGeneratedClass = nullptr;

	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
	KismetCompilerModule.GetBlueprintTypesForClass(ActionDispatcherClass, BlueprintClass, BlueprintGeneratedClass);

	UActionDispatcherBlueprint* NewBP = CastChecked<UActionDispatcherBlueprint>(FKismetEditorUtilities::CreateBlueprint(ActionDispatcherClass, InParent, Name, EBlueprintType::BPTYPE_Normal, UActionDispatcherBlueprint::StaticClass(), BlueprintGeneratedClass, CallingContext));
	FName WhenDispatchStartName = GET_FUNCTION_NAME_CHECKED(UXD_ActionDispatcherBase, WhenDispatchStart);
	int32 NodePosY = 0;
	UFunction* WhenDispatchStartFunction = UXD_ActionDispatcherBase::StaticClass()->FindFunctionByName(WhenDispatchStartName);
	UK2Node_Event* WhenDispatchStartNode = FKismetEditorUtilities::AddDefaultEventNode(NewBP, NewBP->UbergraphPages[0], WhenDispatchStartName, UXD_ActionDispatcherBase::StaticClass(), NodePosY);
	NewBP->WhenDispatchStartNode = WhenDispatchStartNode;
	return NewBP;
}

UObject* UActionDispatcherFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(InClass, InParent, InName, Flags, Context, Warn, NAME_None);
}

FText UActionDispatcherFactory::GetDisplayName() const
{
	return LOCTEXT("创建行为调度器标题", "行为调度器");
}

uint32 UActionDispatcherFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Blueprint;
}

bool UActionDispatcherFactory::ConfigureProperties()
{
	class FActionDispatcherFilterViewer : public IClassViewerFilter
	{
	public:
		EClassFlags AllowedClassFlags = CLASS_Abstract;

		bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<class FClassViewerFilterFuncs> InFilterFuncs) override
		{
			if (GetDefault<UXD_ActionDispatcherSettings>()->bShowPluginClass == false)
			{
				if (InClass == UXD_ActionDispatcherBase::StaticClass())
				{
					return false;
				}
			}
			return InClass->HasAnyClassFlags(AllowedClassFlags) && InClass->IsChildOf<UXD_ActionDispatcherBase>();
		}

		bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const class IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<class FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return InUnloadedClassData->HasAnyClassFlags(AllowedClassFlags) && InUnloadedClassData->IsChildOf(UXD_ActionDispatcherBase::StaticClass());
		}
	};

	ActionDispatcherClass = nullptr;

	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
	FClassViewerInitializationOptions Options;

	Options.Mode = EClassViewerMode::ClassPicker;
	Options.ClassFilter = MakeShareable<FActionDispatcherFilterViewer>(new FActionDispatcherFilterViewer);

	const FText TitleText = LOCTEXT("选择调度器类型", "选择调度器类型");
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UXD_ActionDispatcherBase::StaticClass());

	if (bPressedOk)
	{
		ActionDispatcherClass = ChosenClass ? ChosenClass : UXD_ActionDispatcherBase::StaticClass();
	}

	return bPressedOk;
}

#undef LOCTEXT_NAMESPACE
