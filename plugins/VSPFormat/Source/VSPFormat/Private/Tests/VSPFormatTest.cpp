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
#include "VSPFormatTest.h"

#include "VSPFormatTestUEnumDeclaration.h"

#include "VSPFormat.h"
#include "VSPFormat/DeclareFormatterForEnum.h"
#include "VSPFormat/FmtAddressOf.h"
#include "VSPFormat/FormatRanges.h"
#include "VSPTests.h"

#include "CoreUObject/Public/UObject/Object.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogVSPFormatTest, Log, All);

//------------------------------------------------------------------------------------------------
enum class ETestRegularEnum : uint8
{
	One,
	Two,
	Three
};
//------------------------------------------------------------------------------------------------

template<>
struct fmt::formatter<UDeriveUObjectWithFormatter, TCHAR> : public VSP::EmptyFmtParse
{
	template<typename FormatContext>
	auto format(const UDeriveUObjectWithFormatter& Object, FormatContext& Ctx) const
	{
		return fmt::format_to(Ctx.out(), TEXT("{} some field: {}"), Object.GetName(), Object.SomeField);
	}
};

template<>
struct fmt::formatter<ETestRegularEnum, TCHAR> : public VSP::EmptyFmtParse
{
	template<typename FormatContext>
	auto format(ETestRegularEnum Enum, FormatContext& Ctx) const
	{
		fmt::basic_string_view<TCHAR> Res { TEXT("Undefined") };

		switch (Enum)
		{
		case ETestRegularEnum::One:
			Res = TEXT("One");
			break;
		case ETestRegularEnum::Two:
			Res = TEXT("Two");
			break;
		case ETestRegularEnum::Three:
			Res = TEXT("Three");
			break;
		}

		return fmt::format_to(Ctx.out(), TEXT("{}"), Res);
	}
};



//------------------------------------------------------------------------------------------------
void UsageForLogsExample()
{
	UE_LOG(LogVSPFormatTest, Log, TEXT("%s"), *VSPFormat("Number is {} another number is {}", 3, 15));
	UE_LOG(LogVSPFormatTest, Log, TEXT("%s"), *VSPFormat("Is online {}", true));
	UE_LOG(LogVSPFormatTest, Log, TEXT("%s"), *VSPFormat("Enum is {}", ETestRegularEnum::Two));

	{
		const FString OwnerName { TEXT("SomeUObjectInstance_0") };
		const ETestUEnum ConnectionState = ETestUEnum::Open;
		const FVector DronePosition { 15.f, -0.852f, 60.f };

		// formatting custom types
		UE_LOG(
			LogVSPFormatTest,
			Log,
			TEXT("%s"),
			*VSPFormat("Owner {} with state {} has drone with position {}", OwnerName, ConnectionState, DronePosition));
	}

	{
		TArray<FVector> PlayerMovementPath {};
		PlayerMovementPath.Emplace(0.f, 0.f, 0.f);
		PlayerMovementPath.Emplace(0.f, 1.f, 0.f);
		PlayerMovementPath.Emplace(0.f, 0.f, 1.f);

		// formatting TArray
		UE_LOG(LogVSPFormatTest, Log, TEXT("%s"), *VSPFormat("Player movement path are: {}", PlayerMovementPath));
	}

	{
		using PlayerId = uint64;

		TMap<PlayerId, FString> OnlinePlayers;
		OnlinePlayers.Emplace(123, TEXT("Nagibator228"));
		OnlinePlayers.Emplace(71245, TEXT("VitalyaTheHunter"));
		OnlinePlayers.Emplace(995478, TEXT("GOAT"));
		OnlinePlayers.Emplace(6362626, TEXT("Punisher"));

		// formatting TMap
		UE_LOG(LogVSPFormatTest, Log, TEXT("%s"), *VSPFormat("All online players: {}", OnlinePlayers));

		for (const auto& PlayerEntry : OnlinePlayers)
		{
			if (PlayerEntry.Key == 123)
			{
				// formatting pair
				UE_LOG(LogVSPFormatTest, Error, TEXT("%s"), *VSPFormat("Player {} has invalid id!", PlayerEntry));
				break;
			}
		}
	}

	{
		// formatting pointers

		bool bSomeCondition = 2 + 2 == 5;
		UDeriveUObjectWithFormatter* ObjectPtr = bSomeCondition ? NewObject<UDeriveUObjectWithFormatter>() : nullptr;

		// if pointer is valid, it will be dereferenced and passed to his type formatter
		// if pointer is nullptr - it will resolves in "nullptr" string
		UE_LOG(LogVSPFormatTest, Log, TEXT("%s"), *VSPFormat("Object is: {}", ObjectPtr));

		// prints address of object, located in memory
		UE_LOG(LogVSPFormatTest, Log, TEXT("%s"), *VSPFormat("Object address is: {}", VSP::FmtAddressOf(ObjectPtr)));
	}
}
//------------------------------------------------------------------------------------------------

VSP_TEST(VSPFormatTests, BasicUsageTest, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
{
	VSP_EXPECT_EQ(VSPFormat("Number is {} another number is {}", 3, 15), TEXT("Number is 3 another number is 15"));
	VSP_EXPECT_EQ(VSPFormat("Is online {}", true), TEXT("Is online true"));
	VSP_EXPECT_EQ(VSPFormat("Enum is {}", ETestRegularEnum::Two), TEXT("Enum is Two"));

	return true;
}

VSP_TEST(VSPFormatTests, CommonFormattersTest, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
{
	const FString VectorStr = VSPFormat("My position is {}", FVector { 0.f, 0.f, 0.f });
	const FString FStr = VSPFormat("Str is {}", FString { TEXT("beautiful") });
	const FString PairStr = VSPFormat("Pair is {}", TPair<int, bool> { 15, false });
	const FString UenumStr = VSPFormat("Uenum is {}", ETestUEnum::Open);

	VSP_EXPECT_EQ(VectorStr, TEXT("My position is (0,0,0)"));
	VSP_EXPECT_EQ(FStr, TEXT("Str is beautiful"));
	VSP_EXPECT_EQ(PairStr, TEXT("Pair is (15, false)"));
	VSP_EXPECT_EQ(UenumStr, TEXT("Uenum is ETestUEnum::Open"));

	const FString StringExample { TEXT("Some String In Variant") };

	TVariant<int, FString, FVector> Variant;
	Variant.Set<FString>(StringExample);

	VSP_EXPECT_EQ(VSPFormat("{}", Variant), VSPFormat("{}", StringExample));

	return true;
}

VSP_TEST(VSPFormatTests, RangesTest, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
{
	const TArray<int> SimpleArray { 1, 2, 3 };

	TArray<FVector> Points {};
	Points.Emplace(0.f, 0.f, 0.f);
	Points.Emplace(0.f, 1.f, 0.f);
	Points.Emplace(0.f, 0.f, 1.f);

	TMap<FString, int> Map;
	Map.Emplace(TEXT("one"), 1);
	Map.Emplace(TEXT("two"), 2);
	Map.Emplace(TEXT("three"), 3);

	const FString SimpleArrayStr = VSPFormat("Simple array: {}", SimpleArray);
	const FString CustomTypeArrayStr = VSPFormat("Array of custom type are: {}", Points);
	const FString MapStr = VSPFormat("Map are: {}", Map);

	VSP_EXPECT_EQ(SimpleArrayStr, TEXT("Simple array: [1, 2, 3]"));
	VSP_EXPECT_EQ(CustomTypeArrayStr, TEXT("Array of custom type are: [(0,0,0), (0,1,0), (0,0,1)]"));
	VSP_EXPECT_EQ(MapStr, TEXT("Map are: [(one, 1), (two, 2), (three, 3)]"));

	return true;
}

VSP_TEST(VSPFormatTests, UObjectTest, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
{
	UDeriveUObjectWithFormatter* DerivedWithDefinedFormatter = NewObject<UDeriveUObjectWithFormatter>();
	UDeriveUObjectWithoutFormatter* DerivedWithoutFormatter = NewObject<UDeriveUObjectWithoutFormatter>();

	const FString DerivedWithFormatterStr =
		VSPFormat("Derived object with formatter is {}", *DerivedWithDefinedFormatter);
	const FString DerivedWithoutFormatterStr =
		VSPFormat("Derived object without formatter is {}", *DerivedWithoutFormatter);

	VSP_EXPECT_EQ(
		DerivedWithFormatterStr,
		TEXT("Derived object with formatter is DeriveUObjectWithFormatter_0 some field: 15"));
	VSP_EXPECT_EQ(
		DerivedWithoutFormatterStr,
		TEXT("Derived object without formatter is DeriveUObjectWithoutFormatter_0"));

	return true;
}

UObject* GetObjPtr()
{
	return nullptr;
}

const UObject* GetConstObjPtr()
{
	return nullptr;
}

const UObject* const GetConstObjConstPtr()
{
	return nullptr;
}

VSP_TEST(VSPFormatTests, PointerFormattingTest, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
{
	FString Object { TEXT("SomeTestString") };
	FString* ObjectPtr = &Object;

	UObject* ObjNull = nullptr;

	const int* Number = new int(15);
	const int* NullNumber = nullptr;

	VSP_EXPECT_EQ(VSPFormat("{}", ObjectPtr), VSPFormat("{}", *ObjectPtr));
	VSP_EXPECT_EQ(VSPFormat("{}", ObjNull), TEXT("nullptr"));

	VSP_EXPECT_EQ(VSPFormat("{}", Number), TEXT("15"));
	VSP_EXPECT_EQ(VSPFormat("{}", NullNumber), TEXT("nullptr"));

	VSP_EXPECT_EQ(VSPFormat("{}", GetObjPtr()), TEXT("nullptr"));
	VSP_EXPECT_EQ(VSPFormat("{}", GetConstObjPtr()), TEXT("nullptr"));
	VSP_EXPECT_EQ(VSPFormat("{}", GetConstObjConstPtr()), TEXT("nullptr"));

	delete Number;

	return true;
}

VSP_TEST(
	VSPFormatTests,
	StringLiteralsFormattingTest,
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
{
	static constexpr const TCHAR* TCharPointer = TEXT("TCHAR Pointer");
	static constexpr const TCHAR TCharArray[] = TEXT("TCHAR Array");
	static const FString String = TEXT("Unreal String");
	static const FName Name = TEXT("Unreal Name");
	static const FText Text = FText::FromString({ TEXT("Unreal Text") });

	VSP_EXPECT_EQ(VSPFormat("{}", TCharPointer), TEXT("TCHAR Pointer"));
	VSP_EXPECT_EQ(VSPFormat("{}", TCharArray), TEXT("TCHAR Array"));
	VSP_EXPECT_EQ(VSPFormat("{}", String), TEXT("Unreal String"));
	VSP_EXPECT_EQ(VSPFormat("{}", Name), TEXT("Unreal Name"));
	VSP_EXPECT_EQ(VSPFormat("{}", Text), TEXT("Unreal Text"));

	return true;
}
