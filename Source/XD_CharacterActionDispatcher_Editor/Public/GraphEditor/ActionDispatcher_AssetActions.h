// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <AssetTypeActions/AssetTypeActions_Blueprint.h>

/**
 * 
 */
class XD_CHARACTERACTIONDISPATCHER_EDITOR_API FActionDispatcher_AssetActions : public FAssetTypeActions_Blueprint
{
	using Super = FAssetTypeActions_Blueprint;
public:
	FText GetName() const override;
	UClass* GetSupportedClass() const override;
	FColor GetTypeColor() const override;
	uint32 GetCategories() override;
	bool HasActions(const TArray<UObject*>& InObjects) const override;

	void OpenAssetEditor(const TArray<UObject *>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor /* = TSharedPtr<IToolkitHost>() */) override;
};
