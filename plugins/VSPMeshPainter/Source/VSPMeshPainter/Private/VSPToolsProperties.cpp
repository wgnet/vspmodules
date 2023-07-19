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
#include "VSPToolsProperties.h"

#include "Engine/Texture2DArray.h"
#include "LevelEditorMenuContext.h"
#include "VSPColorSchemes.h"
#include "VSPMeshPainterSettings.h"

void UVSPBrushToolProperties::SetupFor(UMeshComponent* MeshComponent)
{
	if (MeshComponent)
	{
		TextureToModify = UVSPMeshPainterSettings::Get()->GetToPaintTextureFor(MeshComponent);
		MaterialToModify = UVSPMeshPainterSettings::Get()->GetSupportedMaterialFor(MeshComponent);
		BrushMaterial = UVSPMeshPainterSettings::Get()->GetBrushFor(MeshComponent);
		AccumulatorMaterial = UVSPMeshPainterSettings::Get()->GetAccumulatorFor(MeshComponent);
		UnwrapMaterial = UVSPMeshPainterSettings::Get()->GetUnwrapMaterialFor(MeshComponent);
		TextureArray = UVSPMeshPainterSettings::Get()->GetMaterialArrayFor(MeshComponent);
		LayerToTextureIdx = UVSPMeshPainterSettings::Get()->GetLayerToTextureMapFor(MaterialToModify);
		AlphaDataToTexture = UVSPMeshPainterSettings::Get()->GetAlphaToTextureMapFor(MaterialToModify);
		MaterialLayersCount = LayerToTextureIdx.Num();
		AlphaLayersCount = AlphaDataToTexture.Num();

		TintsArray = UVSPColorSchemes::Get()->GetColorScheme(this);
		auto StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent);
		if (StaticMeshComponent && StaticMeshComponent->GetStaticMesh())
		{
			UVChannel = StaticMeshComponent->GetStaticMesh()->GetLightMapCoordinateIndex();
		}

		bSpecifyRadius = true;
	}
	else
	{
		Reset();
	}

	if (OnLayerCountChanged.IsBound())
		OnLayerCountChanged.Broadcast(MaterialLayersCount, AlphaLayersCount);
}

void UVSPBrushToolProperties::Reset()
{
	MaterialToModify = nullptr;
	TextureToModify = nullptr;
	BrushMaterial = nullptr;
	TextureArray = nullptr;
	LayerToTextureIdx.Reset();
	MaterialLayersCount = 0;
	AlphaLayersCount = 0;

	if (OnLayerCountChanged.IsBound())
		OnLayerCountChanged.Broadcast(MaterialLayersCount, AlphaLayersCount);
}

void UVSPBrushToolProperties::SelectTile(int32 TileIndex, EMaterialLayerType MaterialLayerType)
{
	SelectedTile = TileIndex;
	SelectedLayerType = MaterialLayerType;
	OnSelectedTileChanged.Broadcast();
}

int32 UVSPBrushToolProperties::SetTextureIdxForLayer(
	int32 LayerIdx,
	int32 TextureIdx,
	EMaterialLayerType MaterialLayerType)
{
	if (TextureArray)
	{
		TextureIdx = FMath::Clamp(TextureIdx, 0, TextureArray->SourceTextures.Num() - 1);
		if (MaterialLayerType == EMaterialLayerType::MaterialLayer)
		{
			if (LayerToTextureIdx.IsValidIndex(LayerIdx))
			{
				LayerToTextureIdx[LayerIdx] = TextureIdx;
			}
		}
		else
		{
			if (AlphaDataToTexture.IsValidIndex(LayerIdx))
			{
				AlphaDataToTexture[LayerIdx].TextureIndex = TextureIdx;
			}
		}
	}


	OnModified.Broadcast(this, nullptr);
	OnLayerUpdate.Broadcast();

	return TextureIdx;
}

FLinearColor UVSPBrushToolProperties::ShiftLayerTintColor(int32 LayerIdx, int32 Offset)
{

	int32 CurrentTintIndex = -1;
	if (TintsArray.Find(AlphaDataToTexture[LayerIdx].TintColor, CurrentTintIndex))
	{
		AlphaDataToTexture[LayerIdx].TintColor =
			TintsArray[FMath::Clamp(CurrentTintIndex + Offset, 0, TintsArray.Num() - 1)];
	}
	else
	{
		AlphaDataToTexture[LayerIdx].TintColor = TintsArray[0];
	}

	OnModified.Broadcast(this, nullptr);
	OnLayerUpdate.Broadcast();

	return AlphaDataToTexture[LayerIdx].TintColor;
}

FLinearColor UVSPBrushToolProperties::SetLayerTextureChannel(int32 LayerIdx, FLinearColor TextureChannel)
{
	AlphaDataToTexture[LayerIdx].TextureChannel = TextureChannel;

	OnModified.Broadcast(this, nullptr);
	OnLayerUpdate.Broadcast();

	return TextureChannel;
}

float UVSPBrushToolProperties::SetLayerRoughness(int32 LayerIdx, float Roughness)
{
	AlphaDataToTexture[LayerIdx].Roughness = Roughness;

	OnModified.Broadcast(this, nullptr);
	OnLayerUpdate.Broadcast();

	return Roughness;
}

void UVSPBrushToolProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UVSPBrushToolProperties, UVChannel))
	{
		OnUVLevelUpdate.Broadcast();
	}
}

void UVSPBrushToolProperties::ShiftRadius(float Value)
{
	BrushRadius = FMath::Clamp(BrushRadius + Value, 0.f, 5000.f);

	if (OnBrushRadiusChanged.IsBound())
		OnBrushRadiusChanged.Broadcast(Value);
}

void UVSPBrushToolProperties::ShiftFalloff(float Value)
{
	BrushFalloffAmount = FMath::Clamp(BrushFalloffAmount + Value, 0.f, 1.f);
}

void UVSPBrushToolProperties::ShiftStrength(float Value)
{
	BrushStrength = FMath::Clamp(BrushStrength + Value, 0.f, 1.f);

	if (OnBrushStrengthChanged.IsBound())
		OnBrushStrengthChanged.Broadcast(Value);
}

void UVSPBrushToolProperties::ShiftRotation(float Value)
{
	Rotation = FMath::Clamp(Rotation + Value, 0.f, 1.f);
}

int32 UVSPBrushToolProperties::GetSelectedTileIndex()
{
	return SelectedTile;
}

EMaterialLayerType UVSPBrushToolProperties::GetSelectedLayerType()
{
	return SelectedLayerType;
}

int32 UVSPBrushToolProperties::GetMaterialLayersCount()
{
	return MaterialLayersCount;
}

int32 UVSPBrushToolProperties::GetAlphaLayersCount()
{
	return AlphaLayersCount;
}

int32 UVSPBrushToolProperties::GetSelectedLayerID()
{
	if (SelectedLayerType == EMaterialLayerType::MaterialLayer)
		return SelectedTile;
	else
		return SelectedTile + MaterialLayersCount;
}

bool UVSPBrushToolProperties::InputKey(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	FKey Key,
	EInputEvent Event)
{
	bool bHandled = false;
	if (Key == EKeys::LeftAlt)
	{
		static FVector2D PreviousMouseLocation = FVector2D::ZeroVector;
		const FVector2D MouseLocation = FVector2D(Viewport->GetMouseX(), Viewport->GetMouseY());

		if (Event == IE_Pressed)
			PreviousMouseLocation = MouseLocation;

		const FVector2D MouseDelta = MouseLocation - PreviousMouseLocation;
		int MouseKeyIndex = Viewport->KeyState(EKeys::RightMouseButton) ? 1
			: Viewport->KeyState(EKeys::LeftMouseButton)				? 0
																		: -1;
		bHandled |= HandleMouseShortcut(MouseKeyIndex, MouseDelta);
		PreviousMouseLocation = MouseLocation;
	}

	bHandled |= HandleKeyShortcut(Key);

	return bHandled;
}

bool UVSPBrushToolProperties::HandleMouseShortcut(int MouseKeyIndex, const FVector2D& MouseDelta)
{
	const float RadiusChangeSpeed = UVSPMeshPainterSettings::Get()->RadiusChangeSpeed;
	const float FalloffChangeSpeed = UVSPMeshPainterSettings::Get()->FalloffChangeSpeed;
	const float StrengthChangeSpeed = UVSPMeshPainterSettings::Get()->StrengthChangeSpeed;
	const float RotationChangeSpeed = UVSPMeshPainterSettings::Get()->RotationChangeSpeed;


	if (MouseKeyIndex == 1)
	{
		if (FMath::Abs(MouseDelta.X) > FMath::Abs(MouseDelta.Y))
		{
			ShiftRadius(MouseDelta.X * RadiusChangeSpeed);
		}
		else
		{
			ShiftFalloff(MouseDelta.Y * FalloffChangeSpeed);
		}
	}

	if (MouseKeyIndex == 0)
	{
		if (FMath::Abs(MouseDelta.X) > FMath::Abs(MouseDelta.Y))
		{
			ShiftStrength(MouseDelta.X * StrengthChangeSpeed);
		}
		else
		{
			ShiftRotation(MouseDelta.Y * RotationChangeSpeed);
		}
	}

	return true;
}

bool UVSPBrushToolProperties::HandleKeyShortcut(
	/*FEditorViewportClient* ViewportClient, FViewport* Viewport,*/ FKey Key /*, EInputEvent Event*/)
{
	const float RadiusChangeDelta = UVSPMeshPainterSettings::Get()->RadiusChangeDelta;

	bool bHandled = false;
	if (Key == EKeys::LeftBracket)
	{
		ShiftRadius(-RadiusChangeDelta);
		bHandled |= true;
	}

	if (Key == EKeys::RightBracket)
	{
		ShiftRadius(RadiusChangeDelta);
		bHandled |= true;
	}

	if (Key == EKeys::One)
	{
		Function = EBrushFunction::Add;
		bHandled |= true;
	}

	if (Key == EKeys::Two)
	{
		Function = EBrushFunction::Sub;
		bHandled |= true;
	}

	if (Key == EKeys::Three)
	{
		Function = EBrushFunction::Max;
		bHandled |= true;
	}

	return bHandled;
}

bool UVSPBrushToolProperties::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale)
{
	return InViewport->KeyState(EKeys::LeftAlt);
}

void UMaskBakeProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	TargetLayer = FMath::Clamp(TargetLayer, 0, LayersCount - 1);
	OnUpdate.Execute();
}

void UMaskBakeProperties::SetupForMeshComp(UMeshComponent* MeshComp)
{
	if (MeshComp)
	{
		TextureToModify = UVSPMeshPainterSettings::Get()->GetToPaintTextureFor(MeshComp);
		MaterialToModify = UVSPMeshPainterSettings::Get()->GetSupportedMaterialFor(MeshComp);
		AccumulateMat = UVSPMeshPainterSettings::Get()->GetAccumulatorFor(MeshComp);
		LayersCount = UVSPMeshPainterSettings::Get()->GetLayerToTextureMapFor(MaterialToModify).Num();
	}
}

FBrushSpreadVariation::FBrushSpreadVariation(UVSPBrushToolProperties* InBrushProperties)
{
	BrushProperties = InBrushProperties;
	if (BrushProperties)
	{
		Translation = FMath::RandPointInCircle(BrushProperties->TranslationSpread * BrushProperties->BrushRadius);
		Rotation = FMath::RandRange(-BrushProperties->RotationSpread, BrushProperties->RotationSpread);
		Scale = FMath::RandRange(1 - BrushProperties->ScaleSpread, 1 + BrushProperties->ScaleSpread);
		NextPaintDistance = FMath::RandRange(
								BrushProperties->Step - BrushProperties->StepSpread,
								BrushProperties->Step + BrushProperties->StepSpread)
			* BrushProperties->BrushRadius;
	}
}

FSmearProperties FBrushSpreadVariation::GetSmearProperties(const FHitResult& Hit) const
{
	FSmearProperties Out;
	if (BrushProperties)
	{
		FVector BestAxisX, BestAxisY;
		Hit.Normal.FindBestAxisVectors(BestAxisX, BestAxisY);
		Out.Location = Hit.Location + BestAxisX * Translation.X + BestAxisY * Translation.Y;

		Out.Normal = Hit.Normal;
		Out.Size = BrushProperties->BrushRadius * Scale;
		Out.Rotation = BrushProperties->Rotation + Rotation;
	}

	return Out;
}
