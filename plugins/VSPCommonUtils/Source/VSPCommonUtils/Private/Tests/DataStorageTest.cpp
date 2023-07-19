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

#include "DataStorage/VSPCollection.h"
#include "DataStorage/VSPDataStorage.h"
#include "DataStorage/VSPEntity.h"
#include "DataStorageTestClasses.h"
#include "VSPTests.h"

static constexpr int TestsFlags = EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter;



VSP_TEST(DataStorageTests, EntityLifecycle, TestsFlags)
{
	const auto DS = NewObject<UVSPDataStorage>();
	const auto EntityA = DS->CreateEntity();
	const auto EntityB = DS->CreateEntity();

	VSP_EXPECT_TRUE(EntityA != nullptr);
	VSP_EXPECT_TRUE(EntityB != nullptr);
	VSP_EXPECT_TRUE(EntityA->IsValid());
	VSP_EXPECT_TRUE(EntityB->IsValid());

	VSP_EXPECT_TRUE(EntityA != EntityB);

	const auto EntityA_ID = EntityA->GetID();
	const auto EntityB_ID = EntityB->GetID();

	VSP_EXPECT_TRUE(EntityA_ID != EntityB_ID);

	const auto SameEntityA = DS->GetEntity(EntityA_ID);
	const auto SameEntityB = DS->GetEntity(EntityB_ID);

	VSP_EXPECT_TRUE(SameEntityA != nullptr);
	VSP_EXPECT_TRUE(SameEntityB != nullptr);

	VSP_EXPECT_EQ(EntityA, SameEntityA);
	VSP_EXPECT_EQ(EntityB, SameEntityB);

	DS->RemoveEntity(EntityA);
	VSP_EXPECT_TRUE(!EntityA->IsValid());
	VSP_EXPECT_TRUE(!SameEntityA->IsValid());

	const auto SameEntityARemoved = DS->GetEntity(EntityA_ID);
	VSP_EXPECT_TRUE(SameEntityARemoved == nullptr);

	return true;
}

VSP_TEST(DataStorageTests, ComponentData, TestsFlags)
{
	constexpr int32 TestValue = 10;

	const auto DS = NewObject<UVSPDataStorage>();
	const auto Component = NewObject<UComponentInt>();
	Component->Value = TestValue;

	const auto Entity = DS->CreateEntity()->AddComponent(Component);
	VSP_EXPECT_TRUE(Entity != nullptr);

	const auto SameComponent = Entity->GetComponent<UComponentInt>();
	VSP_EXPECT_TRUE(SameComponent != nullptr);
	VSP_EXPECT_EQ(SameComponent->Value, TestValue);

	return true;
}

VSP_TEST(DataStorageTests, ComponentRemove, TestsFlags)
{
	const auto DS = NewObject<UVSPDataStorage>();
	const auto Component = NewObject<UComponentInt>();
	const auto Entity = DS->CreateEntity()->AddComponent(Component);
	const auto Collection = DS->GetCollection<UComponentInt>();

	VSP_EXPECT_EQ(Collection->Num(), 1);
	Entity->RemoveComponent<UComponentInt>();

	VSP_EXPECT_EQ(Entity->GetComponent<UComponentInt>(), nullptr);
	VSP_EXPECT_EQ(Collection->Num(), 0);

	return true;
}

VSP_TEST(DataStorageTests, CollectionSize, TestsFlags)
{
	const auto DS = NewObject<UVSPDataStorage>();
	const auto ComponentA = NewObject<UComponentInt>();
	const auto ComponentB = NewObject<UComponentInt>();
	const auto ComponentC = NewObject<UComponentFloat>();
	DS->CreateEntity()->AddComponent(ComponentA);
	DS->CreateEntity()->AddComponent(ComponentB);
	DS->CreateEntity()->AddComponent(ComponentC);

	const auto ComponentD = NewObject<UComponentInt>();
	const auto ComponentE = NewObject<UComponentFloat>();
	DS->CreateEntity()->AddComponent(ComponentD)->AddComponent(ComponentE);

	VSP_EXPECT_EQ(DS->GetCollection<UComponentInt>()->Num(), 3);
	VSP_EXPECT_EQ(DS->GetCollection<UComponentFloat>()->Num(), 2);

	return true;
}

VSP_TEST(DataStorageTests, CollectionClear, TestsFlags)
{
	const auto DS = NewObject<UVSPDataStorage>();
	const auto Component = NewObject<UComponentInt>();
	const auto Entity = DS->CreateEntity()->AddComponent(Component);

	DS->ClearCollection<UComponentInt>();

	VSP_EXPECT_EQ(DS->GetCollection<UComponentInt>()->Num(), 0);
	VSP_EXPECT_TRUE(!Entity->IsValid());

	return true;
}


VSP_TEST(DataStorageTests, Callbacks, TestsFlags)
{

	const auto DS = NewObject<UVSPDataStorage>();
	const auto ComponentA = NewObject<UComponentInt>();
	const auto ComponentB = NewObject<UComponentFloat>();

	const auto CollectionA = DS->GetCollection<UComponentInt>();
	const auto CollectionB = DS->GetCollection<UComponentFloat>();

	const auto Handler = NewObject<UCollectionCallbackHandler>();
	CollectionA->OnAdded.AddDynamic(Handler, &UCollectionCallbackHandler::OnAdded);
	CollectionB->OnAdded.AddDynamic(Handler, &UCollectionCallbackHandler::OnAdded);
	CollectionA->OnRemoved.AddDynamic(Handler, &UCollectionCallbackHandler::OnRemoved);

	DS->CreateEntity()->AddComponent(ComponentA)->AddComponent(ComponentB)->RemoveComponent(ComponentA);

	const int32 Counter = Handler->Counter;
	VSP_EXPECT_TRUE(Counter == 3);

	return true;
}
