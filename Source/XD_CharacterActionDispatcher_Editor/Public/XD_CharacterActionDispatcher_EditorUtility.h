// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
DECLARE_LOG_CATEGORY_EXTERN(XD_ActionDispatcher_EditorLog, Log, All);
#define ActionDispatcher_Editor_Display_Log(Format, ...) UE_LOG(XD_ActionDispatcher_EditorLog, Log, TEXT(Format), ##__VA_ARGS__)
#define ActionDispatcher_Editor_Warning_LOG(Format, ...) UE_LOG(XD_ActionDispatcher_EditorLog, Warning, TEXT(Format), ##__VA_ARGS__)
#define ActionDispatcher_Editor_Error_Log(Format, ...) UE_LOG(XD_ActionDispatcher_EditorLog, Error, TEXT(Format), ##__VA_ARGS__)