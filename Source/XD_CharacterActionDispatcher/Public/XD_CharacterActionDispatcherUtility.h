// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
DECLARE_LOG_CATEGORY_EXTERN(XD_CharacterActionDispatcher_Log, Log, All);
#define CharacterActionDispatcher_Display_Log(Format, ...) UE_LOG(XD_CharacterActionDispatcher_Log, Log, TEXT(Format), ##__VA_ARGS__)
#define CharacterActionDispatcher_Warning_LOG(Format, ...) UE_LOG(XD_CharacterActionDispatcher_Log, Warning, TEXT(Format), ##__VA_ARGS__)
#define CharacterActionDispatcher_Error_Log(Format, ...) UE_LOG(XD_CharacterActionDispatcher_Log, Error, TEXT(Format), ##__VA_ARGS__)