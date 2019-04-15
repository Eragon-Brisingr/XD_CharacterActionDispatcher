// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "VisualLogger.h"

/**
 * 
 */
DECLARE_LOG_CATEGORY_EXTERN(XD_ActionDispatcher_Log, Log, All);
#define ActionDispatcher_Display_Log(Format, ...) UE_LOG(XD_ActionDispatcher_Log, Display, TEXT(Format), ##__VA_ARGS__)
#define ActionDispatcher_Warning_LOG(Format, ...) UE_LOG(XD_ActionDispatcher_Log, Warning, TEXT(Format), ##__VA_ARGS__)
#define ActionDispatcher_Error_Log(Format, ...) UE_LOG(XD_ActionDispatcher_Log, Error, TEXT(Format), ##__VA_ARGS__)

#define ActionDispatcher_Display_VLog(LogOwner, FMT, ...) UE_VLOG(LogOwner, XD_ActionDispatcher_Log, Display, TEXT(FMT), ##__VA_ARGS__)
#define ActionDispatcher_Warning_VLOG(LogOwner, FMT, ...) UE_VLOG(LogOwner, XD_ActionDispatcher_Log, Warning, TEXT(FMT), ##__VA_ARGS__)
#define ActionDispatcher_Error_VLog(LogOwner, FMT, ...) UE_VLOG(LogOwner, XD_ActionDispatcher_Log, Error, TEXT(FMT), ##__VA_ARGS__)
