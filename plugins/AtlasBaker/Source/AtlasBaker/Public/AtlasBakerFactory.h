/*
* Copyright 2023 Wargaming.net Limited
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* https://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/ 

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AtlasBakerFactory.generated.h"

UCLASS()
class ATLASBAKER_API UAtlasBakerFactory : public UFactory
{
	GENERATED_BODY()
public:
	UAtlasBakerFactory();

#if WITH_EDITOR
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn,
		FName CallingContext) override;
#endif
};

UCLASS()
class ATLASBAKER_API USlateAtlasTileFactory : public UFactory
{
	GENERATED_BODY()
public:
	USlateAtlasTileFactory();

	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn,
		FName CallingContext) override;
};