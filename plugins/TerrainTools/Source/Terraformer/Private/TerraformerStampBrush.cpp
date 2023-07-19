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

#include "TerraformerStampBrush.h"

#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "StampBlend.h"
#include "TerraformerEditorSettings.h"
#include "TerrainGeneratorUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

UStampBoxComponent::UStampBoxComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	LineThickness = GetDefault<UTerraformerEditorSettings>()->GetStampLineThickness();
}

ATerraformerStampBrush::ATerraformerStampBrush()
{
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> SampleTexture;
		FConstructorStatics() : SampleTexture(TEXT("/TerrainTools/Terraformer/Textures/T_SampleHills.T_SampleHills"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;
	DefaultTexture = ConstructorStatics.SampleTexture.Get();
	BrushTexture = DefaultTexture;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	// Add box for visualization of bounds
	Box = CreateEditorOnlyDefaultSubobject<UStampBoxComponent>(TEXT("Bounds"));
	Box->MarkAsEditorOnlySubobject();
	Box->ShapeColor = FColor::Cyan;
	Box->SetBoxExtent(FVector(50, 50, 50), false);
	Box->bDrawOnlyIfSelected = false;
	Box->SetIsVisualizationComponent(true);
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetCanEverAffectNavigation(false);
	Box->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Box->SetGenerateOverlapEvents(false);
	Box->SetupAttachment(RootComponent);

	BlendMode = ETFStampBrushBlendMode::Add;
}

void ATerraformerStampBrush::Render(FTerraformerBrushRenderData& Data)
{
	const FVector TerrainOrigin =
		FVector(Data.TerrainOrigin.X, Data.TerrainOrigin.Y, Data.TerrainTransform.GetLocation().Z);
	const FVector BrushPosition = Data.TerrainTransform.InverseTransformVectorNoScale(GetActorLocation());
	const float BrushRotationAngle = UKismetMathLibrary::ClampAxis(GetActorRotation().Yaw);

	if (!BrushTexture.IsValid())
	{
		BrushTexture.LoadSynchronous();
	}

	if (!BrushTexture.IsValid())
	{
		UE_LOG(LogTerraformerBrush, Error, TEXT("Invalid BrushTexture Parameter for %s"), *GetName());
		return;
	}
	const FVector Location = GetActorLocation();
	const FVector Scale = GetActorScale3D();
	const FVector Extent(
		BrushTexture.Get()->GetSizeX() * Scale.X,
		BrushTexture.Get()->GetSizeY() * Scale.Y,
		(Location.Z * 0.5f) / Scale.Z);

	FBrushStampData BrushStampData {
		BrushTexture.Get(),
		Data.SourceHeightRT,
		TerrainOrigin,
		Data.TerrainExtent,
		Data.TerrainTransform.GetScale3D(),
		BrushPosition,
		Extent,
		Scale,
		FMath::DegreesToRadians(BrushRotationAngle),
		(uint8)BlendMode
	};

	ENQUEUE_RENDER_COMMAND(DrawStampBrush)
	(
		[BrushStampData, Data](FRHICommandListImmediate& RHICmdList)
		{
			FBrushStampPassProcessor::BlendBrushStamp_RenderThread(RHICmdList, BrushStampData, Data.TargetHeightRT);
			RHICmdList.BlockUntilGPUIdle();
		});
}

void ATerraformerStampBrush::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, BrushTexture))
	{
		if (BrushTexture->CompressionSettings != TextureCompressionSettings::TC_Grayscale)
		{
			BrushTexture = DefaultTexture;
			UE_LOG(LogTerraformerBrush, Warning, TEXT("Unsuported Texture Fomat"));
		}
	}
	UpdateBrushManager();
}

void ATerraformerStampBrush::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!BrushTexture.IsValid())
	{
		BrushTexture.LoadSynchronous();
	}

	if (BrushTexture.IsValid())
	{
		const FVector BoxExtent(
			BrushTexture.Get()->GetSizeX(),
			BrushTexture.Get()->GetSizeY(),
			(Transform.GetLocation().Z * 0.5f) / Transform.GetScale3D().Z);
		Box->SetBoxExtent(BoxExtent, false);

		const FVector BoxOffset(0, 0, -Box->GetUnscaledBoxExtent().Z);
		Box->SetRelativeLocation(BoxOffset);
	}
}

void ATerraformerStampBrush::EditorApplyRotation(
	const FRotator& DeltaRotation,
	bool bAltDown,
	bool bShiftDown,
	bool bCtrlDown)
{
	const FRotator LockYaw = FRotator(0.f, DeltaRotation.Yaw, 0.f);
	Super::EditorApplyRotation(LockYaw, bAltDown, bShiftDown, bCtrlDown);
}

void ATerraformerStampBrush::EditorApplyScale(
	const FVector& DeltaScale,
	const FVector* PivotLocation,
	bool bAltDown,
	bool bShiftDown,
	bool bCtrlDown)
{
	const FVector CurrentScale = GetRootComponent()->GetComponentScale();
	const float DeltaX = (CurrentScale + DeltaScale).X >= 1 ? DeltaScale.X : 0;
	const float DeltaY = (CurrentScale + DeltaScale).Y >= 1 ? DeltaScale.Y : 0;
	const float DeltaZ = (CurrentScale + DeltaScale).Z >= 1 ? DeltaScale.Z : 0;
	Super::EditorApplyScale(FVector(DeltaX, DeltaY, DeltaZ), PivotLocation, bAltDown, bShiftDown, bCtrlDown);
}
