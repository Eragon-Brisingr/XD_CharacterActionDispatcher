// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DA_RoleSelectionBase.h"

UXD_DA_RoleSelectionBase::UXD_DA_RoleSelectionBase()
{
#if WITH_EDITORONLY_DATA
	//bShowInExecuteActionNode = false;
#endif
}

void UXD_DA_RoleSelectionBase::WhenActionActived()
{
	APawn* Pawn = Role.Get();
	RegisterEntity(Pawn);

	FinishAction();
	Selections[FMath::RandHelper(Selections.Num())].WhenSelected.ExecuteIfBound();
}

void UXD_DA_RoleSelectionBase::WhenActionDeactived()
{

}

void UXD_DA_RoleSelectionBase::WhenActionFinished()
{
	APawn* Pawn = Role.Get();
	UnregisterEntity(Pawn);
}

UXD_DA_RoleSelectionBase* UXD_DA_RoleSelectionBase::ShowSelection(UXD_ActionDispatcherBase* ActionDispatcher, const TSoftObjectPtr<APawn>& InRole, const TArray<FDA_RoleSelection>& InSelections)
{
	UXD_DA_RoleSelectionBase* RoleSelection = NewObject<UXD_DA_RoleSelectionBase>(ActionDispatcher);
	RoleSelection->Role = InRole;
	RoleSelection->Selections = InSelections;
	ActionDispatcher->ActiveAction(RoleSelection, {});
	return RoleSelection;
}

FDA_RoleSelection& UXD_DA_RoleSelectionBase::SetWhenSelectedEvent(FDA_RoleSelection& Selection, const FDispatchableActionFinishedEvent& Event)
{
	Selection.WhenSelected = Event;
	return Selection;
}
