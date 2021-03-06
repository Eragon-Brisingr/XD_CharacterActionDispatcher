// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <Modules/ModuleManager.h>

class FXD_CharacterActionDispatcherModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
