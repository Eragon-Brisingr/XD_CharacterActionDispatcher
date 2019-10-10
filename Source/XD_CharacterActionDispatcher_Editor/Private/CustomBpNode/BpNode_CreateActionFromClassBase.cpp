// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BpNode_CreateActionFromClassBase.h"
#include "UObject/UnrealType.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "FindInBlueprintManager.h"
#include "XD_ActionDispatcherSettings.h"
#include "XD_ObjectFunctionLibrary.h"
#include "XD_DispatchableActionBase.h"
#include "KismetCompiler.h"

struct FBpNode_CreateActionFromClassHelper
{
	static FName WorldContextPinName;
	static FName ClassPinName;
	static FName OuterPinName;
};

FName FBpNode_CreateActionFromClassHelper::WorldContextPinName(TEXT("WorldContextObject"));
FName FBpNode_CreateActionFromClassHelper::ClassPinName(TEXT("Class"));
FName FBpNode_CreateActionFromClassHelper::OuterPinName(TEXT("Outer"));

#define LOCTEXT_NAMESPACE "XD_CharacterActionDispatcher_CreateActionFromClassBase"

UBpNode_AD_CreateObjectBase::UBpNode_AD_CreateObjectBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

UClass* UBpNode_AD_CreateObjectBase::GetClassPinBaseClass() const
{
	return nullptr;
}

bool UBpNode_AD_CreateObjectBase::UseWorldContext() const
{
	UBlueprint* BP = GetBlueprint();
	const UClass* ParentClass = BP ? BP->ParentClass : nullptr;
	return ParentClass ? ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin) != nullptr : false;
}

void UBpNode_AD_CreateObjectBase::AllocateDefaultPins()
{
	// Add execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// If required add the world context pin
	if (UseWorldContext())
	{
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), FBpNode_CreateActionFromClassHelper::WorldContextPinName);
	}

	// Add blueprint pin
	UEdGraphPin* ClassPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Class, GetClassPinBaseClass(), FBpNode_CreateActionFromClassHelper::ClassPinName);
	
	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, GetClassPinBaseClass(), UEdGraphSchema_K2::PN_ReturnValue);
	
	if (UseOuter())
	{
		UEdGraphPin* OuterPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), FBpNode_CreateActionFromClassHelper::OuterPinName);
	}

	Super::AllocateDefaultPins();
}

UEdGraphPin* UBpNode_AD_CreateObjectBase::GetOuterPin() const
{
	UEdGraphPin* Pin = FindPin(FBpNode_CreateActionFromClassHelper::OuterPinName);
	ensure(nullptr == Pin || Pin->Direction == EGPD_Input);
	return Pin;
}

void UBpNode_AD_CreateObjectBase::SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const
{
	MutatablePin.PinToolTip = UEdGraphSchema_K2::TypeToText(MutatablePin.PinType).ToString();

	UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	if (K2Schema != nullptr)
	{
		MutatablePin.PinToolTip += TEXT(" ");
		MutatablePin.PinToolTip += K2Schema->GetPinDisplayName(&MutatablePin).ToString();
	}

	MutatablePin.PinToolTip += FString(TEXT("\n")) + PinDescription.ToString();
}

void UBpNode_AD_CreateObjectBase::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins)
{
	check(InClass);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	const UObject* const ClassDefaultObject = InClass->GetDefaultObject(false);

	TArray<UProperty*> SortedExposePropertys;

	TArray<UProperty*> CurrentClassPropertys;
	UClass* CurrentClass = InClass;
	for (TFieldIterator<UProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		UClass* PropertyClass = CastChecked<UClass>(Property->GetOuter());

		const bool bIsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
		const bool bIsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
		const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

		if(	bIsExposedToSpawn &&
			!Property->HasAnyPropertyFlags(CPF_Parm) && 
			bIsSettableExternally &&
			Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
			!bIsDelegate &&
			(nullptr == FindPin(Property->GetFName()) ) &&
			FBlueprintEditorUtils::PropertyStillExists(Property))
		{
			if (PropertyClass != CurrentClass)
			{
				CurrentClass = PropertyClass;
				SortedExposePropertys.Insert(CurrentClassPropertys, 0);
				CurrentClassPropertys.Empty();
			}
			CurrentClassPropertys.Add(Property);
		}
	}
	SortedExposePropertys.Insert(CurrentClassPropertys, 0);

	for (UProperty* Property : SortedExposePropertys)
	{
		if (UEdGraphPin* Pin = CreatePin(EGPD_Input, NAME_None, Property->GetFName()))
		{
			K2Schema->ConvertPropertyToPinType(Property, /*out*/ Pin->PinType);
			if (OutClassPins)
			{
				OutClassPins->Add(Pin);
			}
			Pin->PinFriendlyName = Property->GetDisplayNameText();

			if (ClassDefaultObject && K2Schema->PinDefaultValueIsEditable(*Pin))
			{
				FString DefaultValueAsString;
				const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString, this);
				check(bDefaultValueSet);
				K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
			}

			// Copy tooltip from the property.
			K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);
		}
	}

	// Change class of output pin
	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->PinType.PinSubCategoryObject = InClass->GetAuthoritativeClass();
}

UClass* UBpNode_AD_CreateObjectBase::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch /*=NULL*/) const
{
	UClass* UseSpawnClass = nullptr;
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* ClassPin = GetClassPin(PinsToSearch);
	if (ClassPin && ClassPin->DefaultObject && ClassPin->LinkedTo.Num() == 0)
	{
		UseSpawnClass = CastChecked<UClass>(ClassPin->DefaultObject);
	}
	else if (ClassPin && ClassPin->LinkedTo.Num())
	{
		UEdGraphPin* ClassSource = ClassPin->LinkedTo[0];
		UseSpawnClass = ClassSource ? Cast<UClass>(ClassSource->PinType.PinSubCategoryObject.Get()) : nullptr;
	}

	return UseSpawnClass;
}

void UBpNode_AD_CreateObjectBase::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) 
{
	AllocateDefaultPins();
	if (UClass* UseSpawnClass = GetClassToSpawn(&OldPins))
	{
		CreatePinsForClass(UseSpawnClass);
	}
	ShowExtendPins();
	RestoreSplitPins(OldPins);
}

void UBpNode_AD_CreateObjectBase::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	if (UClass* UseSpawnClass = GetClassToSpawn())
	{
		CreatePinsForClass(UseSpawnClass);
	}
	ShowExtendPins();
}

void UBpNode_AD_CreateObjectBase::AddSearchMetaDataInfo(TArray<struct FSearchTagDataPair>& OutTaggedMetaData) const
{
	Super::AddSearchMetaDataInfo(OutTaggedMetaData);
	OutTaggedMetaData.Add(FSearchTagDataPair(FFindInBlueprintSearchTags::FiB_NativeName, CachedNodeTitle.GetCachedText()));
}

bool UBpNode_AD_CreateObjectBase::IsSpawnVarPin(UEdGraphPin* Pin) const
{
	return(	Pin->PinName != UEdGraphSchema_K2::PN_Execute &&
			Pin->PinName != UEdGraphSchema_K2::PN_Then &&
			Pin->PinName != UEdGraphSchema_K2::PN_ReturnValue &&
			Pin->PinName != FBpNode_CreateActionFromClassHelper::ClassPinName &&
			Pin->PinName != FBpNode_CreateActionFromClassHelper::WorldContextPinName &&
			Pin->PinName != FBpNode_CreateActionFromClassHelper::OuterPinName);
}

void UBpNode_AD_CreateObjectBase::OnClassPinChanged()
{
	// Remove all pins related to archetype variables
	TArray<UEdGraphPin*> OldPins = Pins;
	TArray<UEdGraphPin*> OldClassPins;

	for (UEdGraphPin* OldPin : OldPins)
	{
		if (IsSpawnVarPin(OldPin))
		{
			Pins.Remove(OldPin);
			OldClassPins.Add(OldPin);
		}
	}

	CachedNodeTitle.MarkDirty();

	TArray<UEdGraphPin*> NewClassPins;
	if (UClass* UseSpawnClass = GetClassToSpawn())
	{
		CreatePinsForClass(UseSpawnClass, &NewClassPins);
	}
	ShowExtendPins();

	RestoreSplitPins(OldPins);

	UEdGraphPin* ResultPin = GetResultPin();
	// Cache all the pin connections to the ResultPin, we will attempt to recreate them
	TArray<UEdGraphPin*> ResultPinConnectionList = ResultPin->LinkedTo;
	// Because the archetype has changed, we break the output link as the output pin type will change
	ResultPin->BreakAllPinLinks(true);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Recreate any pin links to the Result pin that are still valid
	for (UEdGraphPin* Connections : ResultPinConnectionList)
	{
		K2Schema->TryCreateConnection(ResultPin, Connections);
	}

	// Rewire the old pins to the new pins so connections are maintained if possible
	RewireOldPinsToNewPins(OldClassPins, Pins, nullptr);

	// Refresh the UI for the graph so the pin changes show up
	GetGraph()->NotifyGraphChanged();

	// Mark dirty
	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UBpNode_AD_CreateObjectBase::CreateResultPin()
{
	//调整节点顺序
	UObject* ClassType = GetResultPin()->PinType.PinSubCategoryObject.Get();
	RemovePin(GetThenPin());
	RemovePin(GetResultPin());
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, ClassType, UEdGraphSchema_K2::PN_ReturnValue);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

void UBpNode_AD_CreateObjectBase::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EEdGraphPinDirection::EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject)
		{
			if (Pin->DefaultValue.IsEmpty() && Pin->LinkedTo.Num() == 0)
			{
				if (UClass* ClassToSpawn = GetClassToSpawn())
				{
					UProperty* Property = FindField<UProperty>(ClassToSpawn, Pin->PinName);
					if (!Property->GetBoolMetaData(TEXT("AllowEmpty")))
					{
						CompilerContext.MessageLog.Error(*LOCTEXT("节点软引用为空", "节点 @@ 的引脚 @@ 必须存在连接。").ToString(), this, Pin);
					}
				}
			}
		}
	}
}

void UBpNode_AD_CreateObjectBase::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (Pin && (Pin->PinName == FBpNode_CreateActionFromClassHelper::ClassPinName))
	{
		OnClassPinChanged();
	}
}

void UBpNode_AD_CreateObjectBase::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	if (UEdGraphPin* ClassPin = GetClassPin())
	{
		SetPinToolTip(*ClassPin, LOCTEXT("ClassPinDescription", "创建的行为类型"));
	}
	if (UEdGraphPin* ResultPin = GetResultPin())
	{
		SetPinToolTip(*ResultPin, LOCTEXT("ResultPinDescription", "行为实例"));
	}
	if (UEdGraphPin* OuterPin = (UseOuter() ? GetOuterPin() : nullptr))
	{
		SetPinToolTip(*OuterPin, LOCTEXT("OuterPinDescription", "Owner of the constructed object"));
	}

	return Super::GetPinHoverText(Pin, HoverTextOut);
}

void UBpNode_AD_CreateObjectBase::PinDefaultValueChanged(UEdGraphPin* ChangedPin) 
{
	if (ChangedPin && (ChangedPin->PinName == FBpNode_CreateActionFromClassHelper::ClassPinName))
	{
		OnClassPinChanged();
		ReconstructNode();
	}
}

FText UBpNode_AD_CreateObjectBase::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip", "创建行为");
}

UEdGraphPin* UBpNode_AD_CreateObjectBase::GetThenPin()const
{
	UEdGraphPin* Pin = FindPinChecked(UEdGraphSchema_K2::PN_Then);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UBpNode_AD_CreateObjectBase::GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch /*= NULL*/) const
{
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* Pin = nullptr;
	for (UEdGraphPin* TestPin : *PinsToSearch)
	{
		if (TestPin && TestPin->PinName == FBpNode_CreateActionFromClassHelper::ClassPinName)
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UBpNode_AD_CreateObjectBase::GetWorldContextPin() const
{
	UEdGraphPin* Pin = FindPin(FBpNode_CreateActionFromClassHelper::WorldContextPinName);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UBpNode_AD_CreateObjectBase::GetResultPin() const
{
	UEdGraphPin* Pin = FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

FText UBpNode_AD_CreateObjectBase::GetBaseNodeTitle() const
{
	return LOCTEXT("ConstructObject_BaseTitle", "Construct Object from Class");
}

FText UBpNode_AD_CreateObjectBase::GetNodeTitleFormat() const
{
	return LOCTEXT("Construct", "Construct {ClassName}");
}

FText UBpNode_AD_CreateObjectBase::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return GetBaseNodeTitle();
	}
	else if (UClass* ClassToSpawn = GetClassToSpawn())
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ClassName"), ClassToSpawn->GetDisplayNameText());
			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(GetNodeTitleFormat(), Args), this);
		}
		return CachedNodeTitle;
	}
	return NSLOCTEXT("K2Node", "ConstructObject_Title_NONE", "Construct NONE");
}

bool UBpNode_AD_CreateObjectBase::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const 
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Super::IsCompatibleWithGraph(TargetGraph) && (!Blueprint || FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph);
}

void UBpNode_AD_CreateObjectBase::GetNodeAttributes( TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes ) const
{
	UClass* ClassToSpawn = GetClassToSpawn();
	const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT( "InvalidClass" );
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Type" ), TEXT( "ConstructObjectFromClass" ) ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Class" ), GetClass()->GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Name" ), GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "ObjectClass" ), ClassToSpawnStr ));
}

void UBpNode_AD_CreateObjectBase::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{

}

bool UBpNode_AD_CreateObjectBase::CanShowActionClass(bool ShowPluginNode, UXD_DispatchableActionBase* Action) const
{
	if (!ShowPluginNode && Action->bIsPluginAction)
	{
		return false;
	}
	return true;
}

FText UBpNode_AD_CreateObjectBase::GetMenuCategory() const
{
	return LOCTEXT("行为调度器", "行为调度器");
}

bool UBpNode_AD_CreateObjectBase::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	UClass* SourceClass = GetClassToSpawn();
	const UBlueprint* SourceBlueprint = GetBlueprint();
	const bool bResult = (SourceClass && (SourceClass->ClassGeneratedBy != SourceBlueprint));
	if (bResult && OptionalOutput)
	{
		OptionalOutput->AddUnique(SourceClass);
	}
	const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
	return bSuperResult || bResult;
}

FText UBpNode_AD_CreateObjectBase::GetKeywords() const
{
	return LOCTEXT("ConstructObjectKeywords", "Create New");
}

void UBpNode_CreateActionFromClassBase::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
}

void UBpNode_CreateActionFromClassBase::PostPlacedNewNode()
{
	GetClassPin()->DefaultObject = ActionClass;
	ReconstructNode();
}

void UBpNode_CreateActionFromClassBase::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	TSubclassOf<UXD_DispatchableActionBase> SpawnActionClass = GetClassPinBaseClass();
	if (SpawnActionClass == nullptr)
	{
		return;
	}

	const bool ShowPluginNode = GetDefault<UXD_ActionDispatcherSettings>()->bShowPluginNode;

	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		for (UClass* It : UXD_ObjectFunctionLibrary::GetAllSubclass(SpawnActionClass))
		{
			UXD_DispatchableActionBase* Action = It->GetDefaultObject<UXD_DispatchableActionBase>();
			if (!CanShowActionClass(ShowPluginNode, Action))
			{
				continue;
			}

			UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
			check(NodeSpawner != nullptr);

			NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda([=](UEdGraphNode* NewNode, bool bIsTemplateNode)
				{
					UBpNode_CreateActionFromClassBase* ExecuteActionNode = CastChecked<UBpNode_CreateActionFromClassBase>(NewNode);
					ExecuteActionNode->ActionClass = It;
				});
			ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
		}
	}
}

FText UBpNode_CreateActionFromClassBase::GetTooltipText() const
{
	return ActionClass ? ActionClass->GetToolTipText() : LOCTEXT("NodeTooltip", "创建行为");
}

void UBpNode_CreateActionFromClassBase::OnClassPinChanged()
{
	Super::OnClassPinChanged();
	ActionClass = GetClassToSpawn();
}

#undef LOCTEXT_NAMESPACE
