// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ActionDispatcherSettings.h"
#include "XD_DA_PlaySequence.h"
#include "XD_DA_RoleSelectionBase.h"

UXD_ActionDispatcherSettings::UXD_ActionDispatcherSettings()
	:PlaySequenceImplClass(UXD_DA_PlaySequenceBase::StaticClass()),
	RoleSelectionImplClass(UXD_DA_RoleSelectionBase::StaticClass())
{
#if WITH_EDITORONLY_DATA
	bShowPluginNode = true;
	bShowPluginClass = true;
#endif
}
