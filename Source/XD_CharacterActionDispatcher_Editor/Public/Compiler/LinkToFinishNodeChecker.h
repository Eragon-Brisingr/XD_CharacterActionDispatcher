// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FCompilerResultsLog;
class UEdGraphNode;


struct FLinkToFinishNodeChecker
{
	FCompilerResultsLog& MessageLog;

	static void CheckForceConnectFinishNode(UEdGraphNode* Node, FCompilerResultsLog& MessageLog);
	static void CheckForceNotConnectFinishNode(UEdGraphPin* Pin, FCompilerResultsLog& MessageLog);

	void CheckPinConnectedFinishNode(UEdGraphPin* Pin);
private:
	FLinkToFinishNodeChecker(FCompilerResultsLog& MessageLog, bool bForceNotConnectFinishedNode)
		:MessageLog(MessageLog), bForceNotConnectFinishedNode(bForceNotConnectFinishedNode)
	{}

	uint8 bForceNotConnectFinishedNode : 1;
	UEdGraphPin* StartSearchPin;

	TSet<UEdGraphNode*> VisitedNodes;

	void DoCheckImpl(UEdGraphNode* Node);
};

