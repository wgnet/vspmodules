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
#include "SHitboxPreviewWidget.h"

#include "HitboxViewer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SlateMaterialBrush.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"

SHitboxPreviewWidget::SHitboxPreviewWidget()
{
	PreviewMaterialPath = TEXT("Material'/HitboxTool/M_Widget.M_Widget'");
	PreviewMID = nullptr;
}

SHitboxPreviewWidget::~SHitboxPreviewWidget()
{
	if (PreviewBrush.IsValid())
	{
		PreviewBrush->SetResourceObject(nullptr);
	}
}

void SHitboxPreviewWidget::Construct(const FArguments& InArgs)
{
	UMaterialInterface* PreviewMaterial = Cast<UMaterialInterface>(PreviewMaterialPath.TryLoad());
	if (PreviewMaterial)
	{
		PreviewMID = UMaterialInstanceDynamic::Create(PreviewMaterial, GetTransientPackage());
		PreviewBrush = MakeShareable(new FSlateMaterialBrush(*PreviewMID, FVector2D(512.f, 512.f)));
	}

	this->ChildSlot
	[
		SNew( SScaleBox )
		.Stretch( EStretch::ScaleToFit )
		[
			SNew( SImage )
			.Image( PreviewBrush.Get() )
		]
	];
}

void SHitboxPreviewWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (PreviewMID)
	{
		Collector.AddReferencedObject(PreviewMID);
	}
}
