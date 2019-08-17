// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionDispatcherEditor.h"
#include "ActionDispatcherBlueprint.h"
#include "BlueprintEditorModes.h"
#include "WatchPointViewer.h"

void FActionDispatcherEditor::InitActionDispatcherEditor(const EToolkitMode::Type InMode, const TSharedPtr<class IToolkitHost>& InToolkitHost, UActionDispatcherBlueprint* ActionDispatcherBlueprint)
{
	InitBlueprintEditor(InMode, InToolkitHost, { ActionDispatcherBlueprint }, false);
	UpdatePreviewActor(GetBlueprintObj(), true);
	SetCurrentMode(FBlueprintEditorApplicationModes::BlueprintDefaultsMode);

	WatchViewer::UpdateWatchListFromBlueprint(ActionDispatcherBlueprint);

// 	if (ActionDispatcherBlueprint->EdGraph == nullptr)
// 	{
// 		UEventFlowSystemEditorGraph* EventFlowSystemEditorGraph = CastChecked<UEventFlowSystemEditorGraph>(FBlueprintEditorUtils::CreateNewGraph(ActionDispatcherBlueprint, NAME_None, UEventFlowSystemEditorGraph::StaticClass(), UEventFlowSystemEditorGraphSchema::StaticClass()));
// 		ActionDispatcherBlueprint->EdGraph = EventFlowSystemEditorGraph;
// 		ActionDispatcherBlueprint->EdGraph->bAllowDeletion = false;
// 
// 		//Give the schema a chance to fill out any required nodes (like the results node)
// 		const UEdGraphSchema* Schema = ActionDispatcherBlueprint->EdGraph->GetSchema();
// 		Schema->CreateDefaultNodesForGraph(*ActionDispatcherBlueprint->EdGraph);
// 	}
// 	GetEventFlowGraph()->OwningEditor = this;
// 
// 	UEventFlowGraphBlueprint* EditorGraph_Blueprint = GetEventFlowBlueprint();
// 	if (EditorGraph_Blueprint)
// 	{
// 		EditorGraph_Blueprint->OnCompiled().AddRaw(this, &FEventFlowSystemEditor::BlueprintCompiled);
// 	}
}
