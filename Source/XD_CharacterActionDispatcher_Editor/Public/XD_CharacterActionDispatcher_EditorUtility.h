// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
DECLARE_LOG_CATEGORY_EXTERN(XD_CharacterActionDispatcher_EditorLog, Log, All);
#define CharacterActionDispatcher_Editor_Display_Log(Format, ...) UE_LOG(XD_CharacterActionDispatcher_EditorLog, Log, TEXT(Format), ##__VA_ARGS__)
#define CharacterActionDispatcher_Editor_Warning_LOG(Format, ...) UE_LOG(XD_CharacterActionDispatcher_EditorLog, Warning, TEXT(Format), ##__VA_ARGS__)
#define CharacterActionDispatcher_Editor_Error_Log(Format, ...) UE_LOG(XD_CharacterActionDispatcher_EditorLog, Error, TEXT(Format), ##__VA_ARGS__)