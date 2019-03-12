// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ActionDispatcherFactory.generated.h"

/**
 * 
 */
UCLASS()
class UActionDispatcherFactory : public UFactory
{
	GENERATED_BODY()
	
public:
	UActionDispatcherFactory();
	/**
	* Create a new object by class.
	*
	* @param InClass
	* @param InParent
	* @param InName
	* @param Flags
	* @param Context
	* @param Warn
	* @return The new object.
	*/
	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	FText GetDisplayName() const override;
	uint32 GetMenuCategories() const override;
	
};
