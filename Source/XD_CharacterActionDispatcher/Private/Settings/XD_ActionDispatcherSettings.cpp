// Fill out your copyright notice in the Description page of Project Settings.

#include "Settings/XD_ActionDispatcherSettings.h"
#include "Action/XD_DA_PlaySequence.h"
#include "Action/XD_DA_RoleSelectionBase.h"

UXD_ActionDispatcherSettings::UXD_ActionDispatcherSettings()
{
#if WITH_EDITORONLY_DATA
	bShowPluginNode = true;
	bShowPluginClass = true;
#endif
}
