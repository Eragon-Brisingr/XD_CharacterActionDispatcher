// Fill out your copyright notice in the Description page of Project Settings.

#include "LinkToFinishNodeChecker.h"
#include "DA_BpNodeInterface.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "CompilerResultsLog.h"
#include "BpNode_FinishDispatch.h"
#include "K2Node_Composite.h"
#include "K2Node_MacroInstance.h"
#include "DA_CustomBpNodeUtils.h"

FLinkToFinishNodeChecker FLinkToFinishNodeChecker::CheckForceConnectFinishNode(UEdGraphNode* Node, FCompilerResultsLog& MessageLog)
{
	FLinkToFinishNodeChecker Checker(MessageLog, false);
	Checker.DoCheckImpl(Node);
	return Checker;
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
	UEdGraphPin* RetargetPin = Pin;
	bool bShowErrorOnLinkedPin = false;
	ConvertRetargetPin(RetargetPin, bShowErrorOnLinkedPin);
	UEdGraphPin* ShowErrorPin = bShowErrorOnLinkedPin ? RetargetPin : Pin;

	if (RetargetPin->LinkedTo.Num() > 0)
	{
		for (UEdGraphPin* LinkedPin : RetargetPin->LinkedTo)
		{
			if (bForceNotConnectFinishedNode && LinkedPin->GetOwningNode()->IsA<UBpNode_FinishDispatch>())
			{
				MessageLog.Error(TEXT("从@@ 出发的所有节点不可连接 @@ 节点，因为可能会执行[结束调度器]节点"), StartSearchPin, ShowErrorPin->GetOwningNode(), LinkedPin->GetOwningNode());
				break;
			}

			DoCheckImpl(LinkedPin->GetOwningNode());
		}
	}
	else if (bForceNotConnectFinishedNode == false)
	{
		MessageLog.Error(TEXT("@@ 需要连接[结束调度器]节点"), ShowErrorPin);
	}
}

void FLinkToFinishNodeChecker::ConvertRetargetPin(UEdGraphPin*& Pin, bool& bShowErrorOnLinkedPin)
{
	for (UEdGraphPin* LinkToPin : Pin->LinkedTo)
	{
		if (UK2Node_Tunnel* TunnelNode = Cast<UK2Node_Tunnel>(LinkToPin->GetOwningNode()))
		{
			//宏的输入中转
			if (UK2Node_MacroInstance* MacroInstanceNode = Cast<UK2Node_MacroInstance>(TunnelNode))
			{
				UK2Node_Tunnel* InputTunnelNode = nullptr;
				UK2Node_Tunnel* OutputTunnelNode = nullptr;

				for (UEdGraphNode* Node : MacroInstanceNode->GetMacroGraph()->Nodes)
				{
					if (Node && Node->GetClass() == UK2Node_Tunnel::StaticClass())
					{
						UK2Node_Tunnel* SubTunnelNode = (UK2Node_Tunnel*)Node;
						if (SubTunnelNode->bCanHaveOutputs)
						{
							InputTunnelNode = SubTunnelNode;
						}
						else
						{
							OutputTunnelNode = SubTunnelNode;
						}
					}
				}

				if (OutputTunnelNode)
				{
					MacroNodeLinkers.FindOrAdd(OutputTunnelNode) = MacroInstanceNode;
				}

				if (InputTunnelNode)
				{
					Pin = InputTunnelNode->FindPinChecked(LinkToPin->PinName);
					bShowErrorOnLinkedPin = false;
					ConvertRetargetPin(Pin, bShowErrorOnLinkedPin);
					return;
				}
			}
			else
			{
				//宏的输出中转
				if (UK2Node_MacroInstance** P_MacroInstanceNode = MacroNodeLinkers.Find(TunnelNode))
				{
					MacroInstanceNode = *P_MacroInstanceNode;
					Pin = MacroInstanceNode->FindPinChecked(LinkToPin->PinName);
					bShowErrorOnLinkedPin = true;
					ConvertRetargetPin(Pin, bShowErrorOnLinkedPin);
					return;
				}

				//合并节点处理
				if (TunnelNode->InputSinkNode)
				{
					Pin = TunnelNode->InputSinkNode->FindPinChecked(LinkToPin->PinName);
					bShowErrorOnLinkedPin = LinkToPin->Direction == EGPD_Input;
					ConvertRetargetPin(Pin, bShowErrorOnLinkedPin);
					return;
				}
			}
		}
	}
}
