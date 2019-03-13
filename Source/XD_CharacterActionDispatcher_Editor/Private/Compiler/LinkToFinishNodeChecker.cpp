// Fill out your copyright notice in the Description page of Project Settings.

#include "LinkToFinishNodeChecker.h"
#include "DA_BpNodeInterface.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "CompilerResultsLog.h"

void FLinkToFinishNodeChecker::DoCheck(UEdGraphNode* Node, FCompilerResultsLog& MessageLog)
{
	FLinkToFinishNodeChecker Checker(MessageLog);
	Checker.DoCheckImpl(Node);
}

void FLinkToFinishNodeChecker::DoCheckImpl(UEdGraphNode* Node)
{
	if (VisitedNodes.Contains(Node))
	{
		return;
	}

	VisitedNodes.Add(Node);

	if (Node->Implements<UDA_BpNodeInterface>())
	{
		((IDA_BpNodeInterface*)Node)->WhenCheckLinkedFinishNode(*this);
	}
	else
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				CheckPin(Pin);
			}
		}
	}
}

void FLinkToFinishNodeChecker::CheckPin(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.Num() > 0)
	{
		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			DoCheckImpl(LinkedPin->GetOwningNode());
		}
	}
	else
	{
		MessageLog.Error(TEXT("@@ 需要连接 结束调度器 节点"), Pin);
	}
}
