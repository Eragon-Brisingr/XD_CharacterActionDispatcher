// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FCompilerResultsLog;
class UEdGraphNode;
class UK2Node_MacroInstance;

struct FNodeLinkCheckerBase
{
public:
	TSet<UEdGraphNode*> VisitedNodes;

	FNodeLinkCheckerBase(){}
	virtual ~FNodeLinkCheckerBase(){}
protected:
	bool CheckNextNode(UEdGraphNode* Node);
	virtual bool ExecuteCheck(UEdGraphNode* Node) = 0;

	void ConvertRetargetPin(UEdGraphPin*& Pin, bool& bShowErrorOnLinkedPin);
	//Key为结束的Tunnel节点
	TMap<UK2Node_Tunnel*, UK2Node_MacroInstance*> MacroNodeLinkers;
};

struct FLinkToFinishNodeChecker : public FNodeLinkCheckerBase
{
public:
	static FLinkToFinishNodeChecker CheckForceConnectFinishNode(UEdGraphNode* Node, FCompilerResultsLog& MessageLog);
	static void CheckForceNotConnectFinishNode(UEdGraphPin* Pin, FCompilerResultsLog& MessageLog);

	void CheckPinConnectedFinishNode(UEdGraphPin* Pin);
	FCompilerResultsLog& MessageLog;
private:
	FLinkToFinishNodeChecker(FCompilerResultsLog& MessageLog, bool bForceNotConnectFinishedNode)
		:MessageLog(MessageLog), bForceNotConnectFinishedNode(bForceNotConnectFinishedNode)
	{}

	uint8 bForceNotConnectFinishedNode : 1;
	UEdGraphPin* StartSearchPin;

	bool ExecuteCheck(UEdGraphNode* Node) override;
};

