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

#include "SplineComponentVisualizer.h"
#include "TerraformerSplineComponent.h"

/** Base class for clickable spline editing proxies */
struct HTerraformerSplineVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HTerraformerSplineVisProxy(const UActorComponent* InComponent) : HComponentVisProxy(InComponent, HPP_Wireframe)
	{
	}

	virtual EMouseCursor::Type GetMouseCursor() override
	{
		return EMouseCursor::CardinalCross;
	}
};

/** Proxy for a spline key */
struct HTerraformerSplineKeyProxy : public HTerraformerSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HTerraformerSplineKeyProxy(const UActorComponent* InComponent, int32 InKeyIndex)
		: HTerraformerSplineVisProxy(InComponent)
		, KeyIndex(InKeyIndex)
	{
	}

	int32 KeyIndex;

	virtual EMouseCursor::Type GetMouseCursor() override
	{
		return EMouseCursor::CardinalCross;
	}
};

/** Proxy for a spline segment */
struct HTerraformerSplineSegmentProxy : public HTerraformerSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HTerraformerSplineSegmentProxy(const UActorComponent* InComponent, int32 InSegmentIndex)
		: HTerraformerSplineVisProxy(InComponent)
		, SegmentIndex(InSegmentIndex)
	{
	}

	int32 SegmentIndex;

	virtual EMouseCursor::Type GetMouseCursor() override
	{
		return EMouseCursor::CardinalCross;
	}
};

/** Proxy for a tangent handle */
struct HTerraformerSplineTangentHandleProxy : public HTerraformerSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HTerraformerSplineTangentHandleProxy(const UActorComponent* InComponent, int32 InKeyIndex, bool bInArriveTangent)
		: HTerraformerSplineVisProxy(InComponent)
		, KeyIndex(InKeyIndex)
		, bArriveTangent(bInArriveTangent)
	{
	}

	int32 KeyIndex;
	bool bArriveTangent;

	virtual EMouseCursor::Type GetMouseCursor() override
	{
		return EMouseCursor::CardinalCross;
	}
};

class TERRAFORMER_API FTerraformerSplineVisualizer : public FSplineComponentVisualizer
{
public:
	virtual void DrawVisualization(
		const UActorComponent* Component,
		const FSceneView* View,
		FPrimitiveDrawInterface* PDI) override;
	virtual bool VisProxyHandleClick(
		FEditorViewportClient* InViewportClient,
		HComponentVisProxy* VisProxy,
		const FViewportClick& Click) override;
	UTerraformerSplineComponent* GetEditedTerraformerSplineComponent() const;
};
