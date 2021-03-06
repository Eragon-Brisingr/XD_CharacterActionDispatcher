﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/XD_DispatchableActionBase.h"
#include "XD_DA_Example.generated.h"

/**
 *
 */
UCLASS(meta = (DisplayName = "例子"))
class XD_CHARACTERACTIONDISPATCHER_API UXD_DA_Example : public UXD_DispatchableActionBase
{
	GENERATED_BODY()
public:
	UXD_DA_Example();

	UPROPERTY(SaveGame, BlueprintReadWrite, meta = (DisplayName = "当结束时"))
	FOnDispatchableActionFinishedEvent OnFinished;

	TSet<AActor*> GetAllRegistableEntities() const override { return {}; }
	bool IsActionValid() const override;
	void WhenActionActived() override;
	void WhenActionDeactived() override;
public:
	FTimerHandle TimerHandle;

	UPROPERTY(BlueprintReadOnly, Category = "例子", meta = (ExposeOnSpawn = "true"), SaveGame)
	float DelayTime;

	void WhenTimeFinished();
};
