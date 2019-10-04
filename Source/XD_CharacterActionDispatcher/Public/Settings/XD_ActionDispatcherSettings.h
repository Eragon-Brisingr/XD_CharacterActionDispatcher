// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XD_ActionDispatcherSettings.generated.h"

class UXD_DA_PlaySequenceBase;
class UXD_DA_RoleSelectionBase;
class UXD_DA_ShowSelectionsImplBase;

/**
 * 
 */
UCLASS(Config = "XD_ActionDispatcherSettings", defaultconfig)
class XD_CHARACTERACTIONDISPATCHER_API UXD_ActionDispatcherSettings : public UObject
{
	GENERATED_BODY()
public:
	UXD_ActionDispatcherSettings();

	UPROPERTY(EditAnywhere, Category = "设置", Config)
	TSubclassOf<UXD_DA_PlaySequenceBase> PlaySequenceImplClass;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "设置", Config)
	uint8 bShowPluginNode : 1;

	UPROPERTY(EditAnywhere, Category = "设置", Config)
	uint8 bShowPluginClass : 1;
#endif
};
