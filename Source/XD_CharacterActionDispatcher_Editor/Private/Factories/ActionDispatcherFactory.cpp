// Fill out your copyright notice in the Description page of Project Settings.
#include "ActionDispatcherFactory.h"
#include "KismetEditorUtilities.h"
#include "XD_ActionDispatcherBase.h"
#include "ActionDispatcherBlueprint.h"
#include "ActionDispatcherGeneratedClass.h"
#include "AssetTypeCategories.h"

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher"

UActionDispatcherFactory::UActionDispatcherFactory()
{
	SupportedClass = UActionDispatcherBlueprint::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UActionDispatcherFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UActionDispatcherGeneratedClass* NewBP = CastChecked<UActionDispatcherGeneratedClass>(FKismetEditorUtilities::CreateBlueprint(UXD_ActionDispatcherBase::StaticClass(), InParent, Name, EBlueprintType::BPTYPE_Normal, UActionDispatcherBlueprint::StaticClass(), UActionDispatcherGeneratedClass::StaticClass(), CallingContext));
	return NewBP;
}

UObject* UActionDispatcherFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(InClass, InParent, InName, Flags, Context, Warn, NAME_None);
}

FText UActionDispatcherFactory::GetDisplayName() const
{
	return LOCTEXT("创建行为调度器标题", "创建行为调度器");
}

uint32 UActionDispatcherFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Blueprint;
}

#undef LOCTEXT_NAMESPACE
