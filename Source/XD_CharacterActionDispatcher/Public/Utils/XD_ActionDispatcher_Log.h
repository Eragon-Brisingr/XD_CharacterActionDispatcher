// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * 
 */
DECLARE_LOG_CATEGORY_EXTERN(XD_ActionDispatcher_Log, Log, All);
#define ActionDispatcher_Display_Log(Format, ...) UE_LOG(XD_ActionDispatcher_Log, Log, TEXT(Format), ##__VA_ARGS__)
#define ActionDispatcher_Warning_LOG(Format, ...) UE_LOG(XD_ActionDispatcher_Log, Warning, TEXT(Format), ##__VA_ARGS__)
#define ActionDispatcher_Error_Log(Format, ...) UE_LOG(XD_ActionDispatcher_Log, Error, TEXT(Format), ##__VA_ARGS__)