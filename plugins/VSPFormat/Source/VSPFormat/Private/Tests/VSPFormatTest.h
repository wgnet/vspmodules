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

#include "VSPFormat.h"

#include "CoreMinimal.h"
#include "CoreUObject/Public/UObject/Object.h"

#include "VSPFormatTest.generated.h"

UCLASS()
class UDeriveUObjectWithFormatter : public UObject
{
	GENERATED_BODY()

public:
	int SomeField = 15;
};

UCLASS()
class UDeriveUObjectWithoutFormatter : public UObject
{
	GENERATED_BODY()

public:
	bool SomeField = false;
};
