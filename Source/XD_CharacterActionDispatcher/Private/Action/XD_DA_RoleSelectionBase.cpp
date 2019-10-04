// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DA_RoleSelectionBase.h"
#include "XD_DispatchableEntityInterface.h"
#include "XD_DA_RoleSelectionInterface.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_ActionDispatcher_Log.h"
#include "XD_ActionDispatcherSettings.h"
#include "XD_ActionDispatcherBase.h"
#include "GameFramework/Pawn.h"

UXD_DA_RoleSelectionBase::UXD_DA_RoleSelectionBase()
{
#if WITH_EDITORONLY_DATA
	bShowInExecuteActionNode = false;
	bIsPluginAction = true;
#endif
}

TSet<AActor*> UXD_DA_RoleSelectionBase::GetAllRegistableEntities() const
{
	return { Role.Get() };
}

bool UXD_DA_RoleSelectionBase::IsActionValid() const
{
	return Role.IsValid();
}

void UXD_DA_RoleSelectionBase::WhenActionActived()
{
	APawn* Pawn = Role.Get();
	if (Pawn->Implements<UXD_DA_RoleSelectionInterface>())
	{
		TArray<FDA_DisplaySelection> RoleSelectionDisplays;
		for (int32 i = 0; i < Selections.Num(); ++i)
		{
			FDA_DisplaySelection DisplaySelection(Selections[i]);
			DisplaySelection.SelectionIdx = i;
			RoleSelectionDisplays.Add(DisplaySelection);
		}

		IXD_DA_RoleSelectionInterface::ExecuteSelect(Pawn, this, RoleSelectionDisplays);
	}
	else
	{
		int32 SelectIdx = FMath::RandHelper(Selections.Num());
		ActionDispatcher_Warning_LOG("%s未实现XD_DA_RoleSelectionInterface，随机选择了选项[%s]", *UXD_DebugFunctionLibrary::GetDebugName(Pawn), *Selections[SelectIdx].Selection.ToString());
		ExecuteSelection(Selections[SelectIdx]);
	}
}

void UXD_DA_RoleSelectionBase::WhenActionDeactived()
{
	APawn* Pawn = Role.Get();
	if (Pawn->Implements<UXD_DA_RoleSelectionInterface>())
	{
		IXD_DA_RoleSelectionInterface::ExecuteAbortSelect(Pawn);
	}
}

void UXD_DA_RoleSelectionBase::WhenActionFinished()
{

}

void UXD_DA_RoleSelectionBase::ExecuteSelection(const FDA_DisplaySelection& Selection)
{
	if (Selection.SelectionIdx < Selections.Num())
	{
		const FDA_RoleSelection& CurSelection = Selections[Selection.SelectionIdx];
		if (CurSelection.Selection.EqualTo(Selection.Selection))
		{
			ExecuteSelection(CurSelection);
			return;
		}
	}

	ActionDispatcher_Error_Log("选择 [%d][%s] 无效，需查明原因", Selection.SelectionIdx, *Selection.Selection.ToString());
}

void UXD_DA_RoleSelectionBase::ExecuteRoleSelected(APawn* InRole, const FDA_DisplaySelection& Selection)
{
	if (InRole && InRole->Implements<UXD_DA_RoleSelectionInterface>())
	{
		IXD_DA_RoleSelectionInterface::Execute_WhenSelected(InRole, Selection);
	}
}

void UXD_DA_RoleSelectionBase::ShowSelection(UXD_ActionDispatcherBase* ActionDispatcher, const TArray<FDA_RoleSelection>& InSelections, const TArray<bool>& ShowSelectionConditions)
{
	for (int32 Idx = 0; Idx < InSelections.Num(); ++Idx)
	{
		if (ShowSelectionConditions[Idx])
		{
			Selections.Add(InSelections[Idx]);
		}
	}
	ActionDispatcher->InvokeActiveAction(this);
}

FDA_RoleSelection& UXD_DA_RoleSelectionBase::SetWhenSelectedEvent(FDA_RoleSelection Selection, const FOnDispatchableActionFinishedEvent& Event)
{
	Selection.WhenSelected = Event;
	return Selection;
}

void UXD_DA_RoleSelectionBase::AddSelections(const TArray<FDA_RoleSelection>& InSelections)
{
	APawn* Pawn = Role.Get();
	if (Pawn->Implements<UXD_DA_RoleSelectionInterface>())
	{
		TArray<FDA_DisplaySelection> DisplaySelections;
		for (int32 i = 0; i < InSelections.Num(); ++i)
		{
			FDA_DisplaySelection RoleSelectionDisplay(InSelections[i]);
			RoleSelectionDisplay.SelectionIdx = Selections.Num() + i;
			DisplaySelections.Add(RoleSelectionDisplay);
		}

		Selections.Append(InSelections);

		IXD_DA_RoleSelectionInterface::ExecuteAddSelects(Pawn, DisplaySelections);
	}
}

void UXD_DA_RoleSelectionBase::ExecuteSelection(const FDA_RoleSelection& RoleSelection)
{
	ExecuteEventAndFinishAction(RoleSelection.WhenSelected);
}