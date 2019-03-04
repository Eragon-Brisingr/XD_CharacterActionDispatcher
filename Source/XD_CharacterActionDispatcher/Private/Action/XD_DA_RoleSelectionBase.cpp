// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_DA_RoleSelectionBase.h"
#include "XD_DispatchableEntityInterface.h"
#include "XD_DA_RoleSelectionInterface.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_ActionDispatcher_Log.h"

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

	for (FDA_RoleSelection& Selection : Selections)
	{
		Selection.NativeOnSelected.BindUObject(this, &UXD_DA_RoleSelectionBase::WhenSelected);
	}

	if (Pawn->Implements<UXD_DA_RoleSelectionInterface>())
	{
		IXD_DA_RoleSelectionInterface::ExecuteSelect(Pawn, Selections);
	}
	else
	{
		int32 SelectIdx = FMath::RandHelper(Selections.Num());
		ActionDispatcher_Display_Log("%s未实现XD_DA_RoleSelectionInterface，随机选择了选项[%s]", *UXD_DebugFunctionLibrary::GetDebugName(Pawn), *Selections[SelectIdx].Selection.ToString());
		Selections[SelectIdx].ExecuteIfBound();
	}
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
	//若当前已经是选择状态了
	APawn* Role = InRole.Get();
	if (Role->Implements<UXD_DispatchableEntityInterface>())
	{
		if (UXD_DA_RoleSelectionBase* RoleSelection = Cast<UXD_DA_RoleSelectionBase>(IXD_DispatchableEntityInterface::GetCurrentDispatchableAction(Role)))
		{
			RoleSelection->Selections.Append(InSelections);
			return RoleSelection;
		}
	}

	//非选择状态激活选择
	UXD_DA_RoleSelectionBase* RoleSelection = NewObject<UXD_DA_RoleSelectionBase>(ActionDispatcher);
	RoleSelection->Role = InRole;
	RoleSelection->Selections = InSelections;
	ActionDispatcher->ActiveAction(RoleSelection);
	return RoleSelection;
}

FDA_RoleSelection& UXD_DA_RoleSelectionBase::SetWhenSelectedEvent(FDA_RoleSelection Selection, const FDispatchableActionFinishedEvent& Event)
{
	Selection.WhenSelected = Event;
	return Selection;
}

void UXD_DA_RoleSelectionBase::WhenSelected()
{
	FinishAction();
}
