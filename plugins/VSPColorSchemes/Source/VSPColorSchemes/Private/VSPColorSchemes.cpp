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

#include "VSPColorSchemes.h"
#include "VSPCheck.h"

#define LOCTEXT_NAMESPACE "FVSPColorSchemesModule"

namespace UVSPColorSchemeLocal
{
	const FName VSPEditor(TEXT("VSP Editor"));
	const FName AssetRegisterSettings(TEXT("ColorSchemes"));
	const FName SimpleDestruction { TEXT("SimpleDestruction") };
	const FName Vehicles { TEXT("Vehicles") };
	const FName MeshPainter { TEXT("MeshPainter") };
	const FName DestructibleActor { TEXT("DestructibleActor") };
}

bool FSchemesForLevels::GetColorScheme(
	const TSoftObjectPtr<UWorld>& InPersistentLevel,
	TArray<FLinearColor>& InOutColors) const
{
	auto LevelMatch = [InPersistentLevel](const TSoftObjectPtr<UWorld>& InWorld)
	{
		return InPersistentLevel == InWorld;
	};

	if (Levels.ContainsByPredicate(LevelMatch) && Colors.Num() > 0)
	{
		InOutColors = Colors;
		return true;
	}

	return false;
}

UVSPColorSchemes* UVSPColorSchemes::Get()
{
	return GetMutableDefault<UVSPColorSchemes>();
}

TArray<FLinearColor> UVSPColorSchemes::GetColorScheme(UObject* InRequestingObject) const
{
	TArray<FLinearColor> ColorsResult;

	if (!InRequestingObject)
	{
		return ColorsResult;
	}

	const TSoftObjectPtr<UWorld> PersistentLevelSoftPtr { InRequestingObject->GetWorld() };

	TArray<FSchemesForLevels> ArrayForSearch {};

	if (CheckObjectClass(InRequestingObject, UVSPColorSchemeLocal::SimpleDestruction))
	{
		ArrayForSearch = SimpleDestructionSchemesForLevels;
		ColorsResult = SimpleDestructionDefaultColors;
	}

	else if (CheckObjectClass(InRequestingObject, UVSPColorSchemeLocal::Vehicles))
	{
		ArrayForSearch = VehiclesSchemesForLevels;
		ColorsResult = VehiclesDefaultColors;
	}

	else if (CheckObjectClass(InRequestingObject, UVSPColorSchemeLocal::MeshPainter))
	{
		ArrayForSearch = PainterSchemesForLevels;
		ColorsResult = PainterDefaultColors;
	}

	else if (CheckObjectClass(InRequestingObject, UVSPColorSchemeLocal::DestructibleActor))
	{
		ArrayForSearch = PainterSchemesForLevels;
		ColorsResult = PainterDefaultColors;
	}

	for (const FSchemesForLevels& CurrentScheme : ArrayForSearch)
	{
		if (CurrentScheme.GetColorScheme(PersistentLevelSoftPtr, ColorsResult))
		{
			break;
		}
	}

	return ColorsResult;
}

bool UVSPColorSchemes::CheckObjectClass(const UObject* InObject, const FName& InClassName) const
{
	FSoftClassPath FoundClass = CategoryClasses.FindRef(InClassName);

	if (UClass* LoadedClass = Cast<UClass>(FoundClass.TryLoad()))
	{
		return InObject->IsA(LoadedClass);
	}

#if WITH_EDITOR
	FoundClass = EditorCategoryClasses.FindRef(InClassName);

	if (UClass* LoadedClass = Cast<UClass>(FoundClass.TryLoad()))
	{
		return InObject->IsA(LoadedClass);
	}
#endif

	return false;
}

FName UVSPColorSchemes::GetCategoryName() const
{
	return UVSPColorSchemeLocal::VSPEditor;
}

FName UVSPColorSchemes::GetSectionName() const
{
	return UVSPColorSchemeLocal::AssetRegisterSettings;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVSPColorSchemesModule, VSPColorSchemes)
