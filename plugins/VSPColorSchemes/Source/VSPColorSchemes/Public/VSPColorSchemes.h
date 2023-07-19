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
#include "Engine/DeveloperSettings.h"
#include "Modules/ModuleManager.h"
#include "VSPColorSchemes.generated.h"

namespace UVSPColorSchemesSettings
{
	static constexpr int32 ColorR { 5 };
	static constexpr int32 ColorG { 6 };
	static constexpr int32 ColorB { 7 };
	static constexpr int32 ColorA { 8 };
}

USTRUCT()
struct FSchemesForLevels
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<TSoftObjectPtr<UWorld>> Levels {};

	UPROPERTY(EditAnywhere)
	TArray<FLinearColor> Colors {};

	bool GetColorScheme(const TSoftObjectPtr<UWorld>& InPersistentLevel, TArray<FLinearColor>& InOutColors) const;
};

UCLASS(Config = VSPEditor, DefaultConfig, meta = (DisplayName = "Color Schemes"))
class VSPCOLORSCHEMES_API UVSPColorSchemes : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static UVSPColorSchemes* Get();

	TArray<FLinearColor> GetColorScheme(UObject* InRequestingObject) const;
	bool CheckObjectClass(const UObject* InObject, const FName& InClassName) const;

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Vehicles")
	TArray<FLinearColor> VehiclesDefaultColors {};

	UPROPERTY(EditDefaultsOnly, Config, Category = "Vehicles")
	TArray<FSchemesForLevels> VehiclesSchemesForLevels {};

	UPROPERTY(EditDefaultsOnly, Config, Category = "SimpleDestruction")
	TArray<FLinearColor> SimpleDestructionDefaultColors {};

	UPROPERTY(EditDefaultsOnly, Config, Category = "SimpleDestruction")
	TArray<FSchemesForLevels> SimpleDestructionSchemesForLevels {};

	UPROPERTY(EditDefaultsOnly, Config, Category = "MeshPainter")
	TArray<FLinearColor> PainterDefaultColors {};

	UPROPERTY(EditDefaultsOnly, Config, Category = "MeshPainter")
	TArray<FSchemesForLevels> PainterSchemesForLevels {};

	UPROPERTY(EditDefaultsOnly, Config, Category = "Category Classes")
	TMap<FName, FSoftClassPath> CategoryClasses {};

#if WITH_EDITORONLY_DATA

	UPROPERTY(EditDefaultsOnly, Config, Category = "Editor Category Classes")
	TMap<FName, FSoftClassPath> EditorCategoryClasses {};
#endif
};

class FVSPColorSchemesModule : public IModuleInterface
{
};
