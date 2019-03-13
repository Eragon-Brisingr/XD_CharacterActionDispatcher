// Fill out your copyright notice in the Description page of Project Settings.

#include "DA_BpNodeInterface.h"
#include "LinkToFinishNodeChecker.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"

void IDA_BpNodeInterface::WhenCheckLinkedFinishNode(FLinkToFinishNodeChecker& Checker) const
{
	if (const UEdGraphNode* Node = CastChecked<UEdGraphNode>(this))
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->PinName != UEdGraphSchema_K2::PN_Then)
			{
				Checker.CheckPin(Pin);
			}
		}
	}
}
