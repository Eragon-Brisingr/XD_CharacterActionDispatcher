// Fill out your copyright notice in the Description page of Project Settings.

#include "LinkToFinishNodeChecker.h"
#include "DA_BpNodeInterface.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "CompilerResultsLog.h"
#include "BpNode_FinishDispatch.h"

void FLinkToFinishNodeChecker::CheckForceConnectFinishNode(UEdGraphNode* Node, FCompilerResultsLog& MessageLog)
{
	FLinkToFinishNodeChecker Checker(MessageLog, false);
	Checker.DoCheckImpl(Node);
}

void FLinkToFinishNodeChecker::CheckForceNotConnectFinishNode(UEdGraphPin* Pin, FCompilerResultsLog& MessageLog)
{
	for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
	{
		if (LinkedPin->GetOwningNode()->IsA<UBpNode_FinishDispatch>())
		{
			MessageLog.Error(TEXT("从@@ 出发的所有节点不可连接 @@ 节点"), Pin, LinkedPin->GetOwningNode());
		}
		else
		{
			FLinkToFinishNodeChecker Checker(MessageLog, true);
			Checker.StartSearchPin = Pin;
			Checker.VisitedNodes.Add(Pin->GetOwningNode());

			Checker.DoCheckImpl(LinkedPin->GetOwningNode());
		}
	}
}

void FLinkToFinishNodeChecker::DoCheckImpl(UEdGraphNode* Node)
{
	if (VisitedNodes.Contains(Node))
	{
		return;
	}

	VisitedNodes.Add(Node);

	if (IDA_BpNodeInterface* DA_BpNodeInterface = Cast<IDA_BpNodeInterface>(Node))
	{
		DA_BpNodeInterface->WhenCheckLinkedFinishNode(*this);
	}
	else
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				CheckPinConnectedFinishNode(Pin);
			}
		}
	}
}

void FLinkToFinishNodeChecker::CheckPinConnectedFinishNode(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.Num() > 0)
	{
		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (bForceNotConnectFinishedNode && LinkedPin->GetOwningNode()->IsA<UBpNode_FinishDispatch>())
			{
				MessageLog.Error(TEXT("从@@ 出发的所有节点不可连接 @@ 节点"), StartSearchPin, Pin->GetOwningNode(), LinkedPin->GetOwningNode());
				break;
			}

			DoCheckImpl(LinkedPin->GetOwningNode());
		}
	}
	else if (bForceNotConnectFinishedNode == false)
	{
		MessageLog.Error(TEXT("@@ 需要连接 结束调度器 节点"), Pin);
	}
}
