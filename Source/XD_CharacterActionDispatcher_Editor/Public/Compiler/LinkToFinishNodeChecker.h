// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FCompilerResultsLog;
class UEdGraphNode;


struct FLinkToFinishNodeChecker
{
	FCompilerResultsLog& MessageLog;

	static void DoCheck(UEdGraphNode* Node, FCompilerResultsLog& MessageLog);

	void CheckPin(UEdGraphPin* Pin);
private:
	FLinkToFinishNodeChecker(FCompilerResultsLog& MessageLog)
		:MessageLog(MessageLog)
	{}
	TSet<UEdGraphNode*> VisitedNodes;

	void DoCheckImpl(UEdGraphNode* Node);
};


