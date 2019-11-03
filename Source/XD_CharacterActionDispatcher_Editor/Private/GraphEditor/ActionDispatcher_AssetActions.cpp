// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionDispatcher_AssetActions.h"
#include "MessageDialog.h"
#include "ActionDispatcherBlueprint.h"
#include "ActionDispatcherEditor.h"

#define LOCTEXT_NAMESPACE "ActionDispatcher_Editor"

FText FActionDispatcher_AssetActions::GetName() const
{
	return Super::GetName();
}

UClass* FActionDispatcher_AssetActions::GetSupportedClass() const
{
	return UActionDispatcherBlueprint::StaticClass();
}

FColor FActionDispatcher_AssetActions::GetTypeColor() const
{
	return Super::GetTypeColor();
}

uint32 FActionDispatcher_AssetActions::GetCategories()
{
	return EAssetTypeCategories::Gameplay;
}

bool FActionDispatcher_AssetActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return Super::HasActions(InObjects);
}

void FActionDispatcher_AssetActions::OpenAssetEditor(const TArray<UObject *>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor /* = TSharedPtr<IToolkitHost>() */)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			bool bLetOpen = true;
			if (!Blueprint->SkeletonGeneratedClass || !Blueprint->GeneratedClass)
			{
				bLetOpen = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("FailedToLoadBlueprintWithContinue", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed! Do you want to continue (it can crash the editor)?"));
			}
			if (bLetOpen)
			{
				auto CustomBP = Cast<UActionDispatcherBlueprint>(Object);
				if (CustomBP != nullptr)
				{
					TSharedRef<FActionDispatcherEditor> ActionDispatcherEditor = MakeShareable(new FActionDispatcherEditor());
					ActionDispatcherEditor->InitActionDispatcherEditor(Mode, EditWithinLevelEditor, CustomBP);
				}
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToLoadBlueprint", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

#undef LOCTEXT_NAMESPACE
