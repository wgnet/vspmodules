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
#include "MeshPaintUtilities.h"

#include "MeshPaintingToolset/Public/MeshTexturePaintingTool.h"
#include "VSPMeshPainterSettings.h"

#define LOCTEXT_NAMESPACE "VSPMeshPainter"

void UMeshPaintUtilities::CopyTextureToRT(UTexture* Texture, UTextureRenderTarget2D* RT)
{
	if (Texture && RT)
	{
		UTexturePaintToolset::CopyTextureToRenderTargetTexture(
			Texture,
			RT,
			VSPMeshPainterExpressions::SupportedFeatureLevel);
	}
}

void UMeshPaintUtilities::WriteRTDataToTexture(UTextureRenderTarget2D* RT, UTexture2D* Texture)
{
	//create copy of RT with size of Texture
	FIntPoint TextureSize = FIntPoint(Texture->Source.GetSizeX(), Texture->Source.GetSizeY());

	UTextureRenderTarget2D* TempTargetFullSize =
		UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize, TA_Clamp, TA_Clamp);
	UMeshPaintUtilities::ClearRT(TempTargetFullSize);
	CopyTextureToRT(RT, TempTargetFullSize);
	RT = TempTargetFullSize;
	FlushRenderingCommands();

	TArray<FColor> Data;
	Data.AddUninitialized(RT->SizeX * RT->SizeY);
	FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();
	check(RTResource != nullptr);
	RTResource->ReadPixels(Data);

	{
		FScopedTransaction Transaction(LOCTEXT("MeshPaintMode_TexturePaint_Transaction", "Texture Paint"));

		Texture->SetFlags(RF_Transactional);
		Texture->Modify();

		FColor* Colors = (FColor*)Texture->Source.LockMip(0);
		check(Texture->Source.CalcMipSize(0) == Data.Num() * sizeof(FColor));
		FMemory::Memcpy(Colors, Data.GetData(), Data.Num() * sizeof(FColor));
		Texture->Source.UnlockMip(0);

		Texture->SRGB = false;
		Texture->bHasBeenPaintedInEditor = true;

		Texture->PostEditChange();
	}
}

UTextureRenderTarget2D* UMeshPaintUtilities::CreateInternalRenderTarget(
	const FIntPoint& Size,
	TextureAddress TextureAddressX,
	TextureAddress TextureAddressY)
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	RT->ClearColor = FLinearColor::Black;
	RT->bNeedsTwoCopies = true;
	RT->InitCustomFormat(Size.X, Size.Y, PF_R8G8B8A8, true);
	RT->UpdateResourceImmediate();
	RT->AddressX = TextureAddressX;
	RT->AddressY = TextureAddressY;

	return RT;
}

void UMeshPaintUtilities::ClearRT(UTextureRenderTarget2D* RT, const FLinearColor& ClearColor)
{
	if (!RT)
		return;

	FCanvas BrushPaintCanvas(
		RT->GameThread_GetRenderTargetResource(),
		nullptr,
		0,
		0,
		0,
		VSPMeshPainterExpressions::SupportedFeatureLevel);
	BrushPaintCanvas.Clear(ClearColor);

	ENQUEUE_RENDER_COMMAND(UpdateMeshPaintRTCommand1)
	(
		[=](FRHICommandListImmediate& RHICmdList)
		{
			FTextureRenderTargetResource* RenderTargetResource = RT->GetRenderTargetResource();
			RHICmdList.CopyToResolveTarget(
				RenderTargetResource->GetRenderTargetTexture(),
				RenderTargetResource->TextureRHI,
				FResolveParams());
		});
}

UStaticMeshComponent* UMeshPaintUtilities::GetSingleSelectedStaticMeshComp(const FToolBuilderState& SceneState)
{
	if (SceneState.SelectedActors.Num() == 1)
	{
		TArray<UStaticMeshComponent*> MeshComponents;
		SceneState.SelectedActors[0]->GetComponents(MeshComponents);

		return MeshComponents.Num() == 1 ? MeshComponents[0] : nullptr;
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
