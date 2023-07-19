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

#include "HydraulicErosionComponent.h"
#include "LayerStackBase.h"

#include "HydraulicErosion.generated.h"

class ALandscape;
class UHydraulicErosionComponent;
class USceneCaptureComponent2D;

USTRUCT(BlueprintType)
struct FRenderTargetChannels
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* RenderTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName R = TEXT("Sand");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName G = TEXT("Grass");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName B = TEXT("Gravel");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName A = TEXT("Mountain");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ResolutionDivisor = 1;
};

USTRUCT(BlueprintType)
struct FErosionWeightmapEffects
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WeightmapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DepositionStrength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ErosionStrength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VelocityStrength = 0.f;
};

UENUM(BlueprintType)
enum ESectionSizeOptions
{
	ESS_127x127_Quads = 0,
	ESS_255x255_Quads = 1,
};

ENUM_CLASS_FLAGS(ESectionSizeOptions);

UCLASS()
class TERRAINGENERATOR_API AHydraulicErosion : public ALayerStackBase
{
	GENERATED_BODY()

public:
	AHydraulicErosion();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
	UHydraulicErosionComponent* HydraulicErosion { nullptr };

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
	UInstancedStaticMeshComponent* TerrainISMC { nullptr };

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
	UInstancedStaticMeshComponent* ParticlesScatterISMC { nullptr };

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
	USceneCaptureComponent2D* ScatterSceneCapture;

	UFUNCTION(CallInEditor, Category = "Simulation", meta = (DisplayPriority = 1))
	void StarSimulation();

	UFUNCTION(CallInEditor, Category = "Simulation", meta = (DisplayPriority = 2))
	void StopSimulation();

	UFUNCTION(CallInEditor, Category = "Simulation", meta = (DisplayPriority = 3))
	void ResetSimulation();

	UFUNCTION(CallInEditor, Category = "Simulation", meta = (DisplayPriority = 4))
	void PerformIterations() const;

	UPROPERTY(VisibleInstanceOnly, Category = "Simulation", meta = (DisplayPriority = 1))
	int32 Iterations = 0;

	// Per Tick Iterations
	UPROPERTY(EditInstanceOnly, Category = "Simulation", meta = (DisplayPriority = 1))
	int32 PerTickIterations = 8;

	// When above 0, limits iterations. Useful for comparison various settings with the same number of iterations.
	UPROPERTY(EditInstanceOnly, Category = "Simulation", meta = (DisplayPriority = 1))
	int32 MaxIterations = 0;

	// Toggles the optional display of debug water on the Mesh Preview Material. If the preview material is overridden, the material must include the necessary water preview functions.
	UPROPERTY(EditInstanceOnly, Category = "Simulation", meta = (DisplayPriority = 1))
	bool bShowWater = true;

	// When the above 'Show Water' flag is enabled, this value allows tweaking of the displayed velocity brightness. It is purely for debug visualization feature.
	UPROPERTY(EditInstanceOnly, Category = "Simulation", meta = (DisplayPriority = 1))
	float VelocityDebugBrightness = 0;

	// How many iterations to perform when pressing the 'Perform Iterations' button.
	UPROPERTY(EditInstanceOnly, Category = "Simulation", meta = (DisplayPriority = 1))
	int32 ButtonPressIterations = 1;

	// Generation

	UPROPERTY(EditInstanceOnly, Category = "Generation", meta = (DisplayPriority = 2))
	bool ShowPreviewMesh = true;

	// Adjusts Tiling of the noise in the Seed Material.
	UPROPERTY(EditInstanceOnly, Category = "Generation", meta = (DisplayPriority = 2))
	float Tiling = 2.f;

	// This changes the random noise seed.
	UPROPERTY(EditInstanceOnly, Category = "Generation", meta = (DisplayPriority = 2))
	float Seed = 0.f;

	// The noise will start with a range around 0-1 and this multiplier effectively sets your desired Max Elevation
	UPROPERTY(EditInstanceOnly, Category = "Generation", meta = (DisplayPriority = 2))
	float ElevationScale = 1000.f;

	// This subtracts a fixed amount from the starting noise value (in 0-1 space) and then clamps to 0. Useful for setting a 'floor' in a base noise, for water or valleys.
	UPROPERTY(EditInstanceOnly, Category = "Generation", meta = (DisplayPriority = 2))
	float ElevationPreOffset = 0.f;

	// Inverts the Noise using 1-x when it is in the 0-1 space, before elevation offset and scaling.
	UPROPERTY(EditInstanceOnly, Category = "Generation", meta = (DisplayPriority = 2))
	bool bInvertNoise = true;

	// The number of noise octaves to use. More octaves give more detail, but add cost to the GPU for re-rendering the stack.
	UPROPERTY(EditInstanceOnly, Category = "Generation", meta = (DisplayPriority = 2))
	int32 Octaves = 5;

	// Persistence is how much the noise amplitude is scaled down for each new octave. The default is 0.5 which means each octave gets half as intense. Lower values will give smoother terrain.
	UPROPERTY(EditInstanceOnly, Category = "Generation", meta = (DisplayPriority = 2))
	float Persistence = 0.35f;

	UPROPERTY(EditInstanceOnly, AdvancedDisplay, Category = "Generation")
	float Ramp = 0.f;

	UPROPERTY(EditInstanceOnly, AdvancedDisplay, Category = "Generation")
	float Plateau = 0.f;

	UPROPERTY(EditInstanceOnly, AdvancedDisplay, Category = "Generation")
	float Channel = 6000.f;

	UPROPERTY(EditInstanceOnly, AdvancedDisplay, Category = "Generation")
	float EdgeLip = 0.f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialInterface* ComputeNormalMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialInterface* MeshPreviewMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialInterface* TerrainSeedMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialInterface* UnpackRG8HeightMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialInterface* DebugDrawingMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialInterface* NormalizeHeightMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialInterface* PrepHMforLS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialInterface* PrepWMforLS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Materials")
	UMaterialParameterCollection* MPC_Erosion;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|MIDs")
	UMaterialInstanceDynamic* MeshPreviewMID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|MIDs")
	UMaterialInstanceDynamic* ComputeNormalMID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|MIDs")
	UMaterialInstanceDynamic* NormalizeHeightMID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|MIDs")
	UMaterialInstanceDynamic* SeedNoiseMID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|MIDs")
	UMaterialInstanceDynamic* PrepRTforLSMID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|MIDs")
	UMaterialInstanceDynamic* CombineLandscapeNoiseWaterMID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|MIDs")
	UMaterialInstanceDynamic* DebugDrawMID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|MIDs")
	UMaterialInstanceDynamic* PrepWeightmapforLSMID;


	// Stores the height data.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* HeightRTA;

	// Stores the height data.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* HeightRTB;

	// This stores a cached copy of the noise generation seed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* NoiseSeedRT;

	// Landscape Data is meant to store data copied from Landscape.
	// It is reserved from being used as a scratch layer for any procedural process so that we don't have to re-copy the landscape data to re-render the stack.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* LandscapeHeightRT;

	// Used to capture mesh depth data for Brush blueprints. Also reusable for other passes as meshes always recapture depth.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* MeshDepthRT;

	// Used for Jump Flooding for Brushes and Velocity in fluid simulation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* JumpFloodA;

	// // Used for Jump Flooding for Brushes and Velocity in fluid simulation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* JumpFloodB;

	// The computed normal map of the heightmap. RG is the XY of the terrain normal and BA is the XY of the fluid normal (combined with terrain normal anywhere fluid is absent).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* NormalRT;

	// Particle Positions for Scatter Particle simulation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* PositionA;

	// Particle Positions for Scatter Particle simulation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Generation|RenderTargets")
	UTextureRenderTarget2D* PositionB;


	// Erosion. These settings control the basic behavior behind erosion.
	//
	// Hydraulic Erosion Mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|SimulationSettings")
	TEnumAsByte<EHydraulicErosionMode> HydraulicErosionMode = ShallowWater;

	// When > 0, causes the fluid simulation to reset every N iterations.
	// Useful if you prefer to leave 'Rain Amount' to 0 and see whole 'rain events' complete before re-starting. It will re-use the 'Start Water' value for each reset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|SimulationSettings")
	int32 ResetEveryNInterations = 512;

	// The amount of time per frame. Can be thought of as 1 / fps of sim. The default of 0.016 is set for 60fps
	// (ie enter 1/60 to get that value). Larger values can lead to instability. The Free Particles method is the most flexible with larger deltaT.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|SimulationSettings")
	float DeltaT = 0.01f;

	// The amount of water height to add to each terrain vertex every iteration.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|SimulationSettings")
	float RainAmount = 0.f;

	// The Solubility is the amount soil that can be dissolved into the water, per unit volume. Typically small values < 0.1 should be used.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|SimulationSettings")
	float Solubility = 0.f;

	// The amount of water volume per cell that is lost to evaporation at the end of each iteration.
	// Since soil does not evaporate, this leads to supersaturated and then deposition (ie, water evaporates, then extra soil falls back to terrain below).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|SimulationSettings")
	float Evaporation = 0.f;

	// The amount of water to start with per cell. This amount is measured in depth, not volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|SimulationSettings")
	float StartWater = 50.f;

	// If true, water will be levelled already instead of evenly distributed along the terrain.
	// The Start Water value will be a water elevation instead of depth. Does not apply to Free Particles Mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|SimulationSettings")
	bool bWaterIsLevel = false;

	// These advanced effects also take place during erosion simulation. They can occur in addition to, or instead of standard erosion.
	//
	// This method uses the fluid velocity to directly advect the terrain soil. This is a simplified version of erosion that does not use Solubility or Evaporation settings.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|AdvancedEffects")
	float VelocityAdvectionErosion = 0.f;

	// Normal advection performs Advection of the Heightmap using its own Normal map. This tends to exaggerate and extend existing slopes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|AdvancedEffects")
	float NormalAdvectionErosion = 0.f; // private

	// When > 0, acts as a filter for smoothing neighboring Heightmap texels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|AdvancedEffects")
	float MaxAdvectionSlope = 0.5f;

	// When > 0, acts as a filter for smoothing neighboring Heightmap texels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|AdvancedEffects")
	float SmoothSlopesGreaterThan = 0.f;

	// When > 0, acts as a filter for smoothing neighboring Heightmap texels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|AdvancedEffects")
	float SmoothSlopesLessThan = 0.f;

	// This is an Exponential Damping term that is used to prevent fluid velocities from ever growing.
	// It is applied exponentially so that the damping result is time step independent.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|AdvancedEffects")
	float VelocityDamping = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion|AdvancedEffects")
	TArray<FErosionWeightmapEffects> ErosionWeightmapEffects = {
		{ "Biomes.Sand", 0.f, 0.5f, 0.000f },
		{ "Biomes.Grass", 1.f, 0.f, 0.000f },
		{ "Biomes.Gravel", 0.f, 0.f, 1.00f },
		{ "Biomes.Mountain", 0.f, 0.f, 0.f }
	};

	// Specifying a Landscape actor gives the ability to read from an existing Landscape as Seed, as well as preview using the actual Landscape.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	TSoftObjectPtr<ALandscape> Landscape { nullptr };

	// The number of quads in each section. Should match the settings from the referenced Landscape above, if any.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	TEnumAsByte<ESectionSizeOptions> ComponentSize = ESS_255x255_Quads;

	// Number of sections in the X direction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	int32 SectionsX = 4;

	// Number of sections in the Y direction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	int32 SectionsY = 4;

	// Overall resolution calculated based on Section size and Section Counts. This gives the option to match existing landscapes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	FVector2D OverallResolution = { 1024.f, 1024.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	FName LayerWeightName; //private

	//
	// When specifying a Landscape above, this flag allows all Landmass modifications to be Toggled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	bool bShowProceduralImpact = true;

	// By default, the max Z range is 0.65536 kilometers or 65536 centimeters.
	// Since height is stored as an integer with a range from 0 to 65536, this allows one centimeter precision.
	// Higher ranges are possible by sacraficing some precision. Note that the range is split in half so the default 65536 cm is split into -32k to +32k range.
	// This means a 'flat' default landscape starts at the halfway point so if you intend to paint very steep mountains, you may want to offset downwards towards the bottom floor first.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	float TotalZRangeKilometers = 0.65536f;

	// This is the Z scale a Landscape actor needs to be set to in order to have a matching Z range.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	float LandscapeActorZScale = 100.f;

	// If non-zero, this setting will determine the world size of the landscape.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Landscape")
	int32 OverrideSize = 0;

	// Forcing the LOD to a higher value can gain back some performance if your PC is having trouble rendering a very dense mesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Landscape")
	int32 ForcedLOD = 0;

	// Useful when using this to create textures as displacements. This will round up to the next size power of 2, enabling static textures to be created.
	// Ie 2041 becomes 2048. Some minor preview precision will be lost since the grid will not be adjusted to account for this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Landscape")
	bool bForcePowerOfTwo = true;

	// This lets you add Weightmaps that can be written to by brushes. Each weightmap will be RGBA and include 4 channels. Be sure to name them and place TextureSampleParameter nodes in your Preview material with matching names.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape|WeightmapSetup")
	TMap<FName, FRenderTargetChannels> Weightmaps = { { "Biomes",
														{ nullptr, "Sand", "Grass", "Gravel", "Mountain", 1 } } };

	// If set >= 2, can be used to force a manual render target resolution. Can break parity with supported Landscape sizes but often useful when power of two is needed for testing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	FIntPoint OverrideResolution = FIntPoint::ZeroValue;

	// Internal only. Transient.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Debug")
	bool bTickingErosion = false;

	// Draws an optional white outline on component edges.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bHighlightComponents = true;

	// Calculated size per quad.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float SizePerQuad = 100.f;

	// The total world size of the grid. This is the number of quads * the SizePerQuad.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FVector2D TotalSize = FVector2D::ZeroVector;

	// This is the raw Z scale applied to the height data.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float RawZScale = 1.f;

	UFUNCTION(CallInEditor, Category = "Landscape")
	void WriteLandscapeWeightmap() const;

	UFUNCTION(CallInEditor, Category = "Landscape")
	void ExportHeightToLandscape();

	UFUNCTION(CallInEditor, Category = "Landscape")
	void ReadLandscapeHeight();

	UFUNCTION(CallInEditor, Category = "Landscape")
	void CopyLandscapeRange();

	UFUNCTION(CallInEditor, Category = "Landscape")
	void ExportWeightmapsToLandscape();

	void UpdateNormalAndPreviewRTAssignments(UTextureRenderTarget2D* InVelocityRT);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateBrushesAndNormals();
	void UpdateHeight();
	void SpawnInstanceMeshes(UInstancedStaticMeshComponent* ISMC);
	void ConfigResolution();
	void AllocateRenderTargets();
	void CreateMIDs();
	void SetMPCParams();
	void LoadMaterials();
	bool CheckSizeParamsChanged() const;
	bool CheckSeedParamsChanged() const;
	void RenderSeed();
	void CacheSizeParams();
	void CacheSeedParams();
	FVector GetLandscapeCenter() const;

	UTextureRenderTarget2D* HeightPingPong_Source() const;
	UTextureRenderTarget2D* HeightPingPong_Target() const;

	UPROPERTY()
	UStaticMesh* PreviewMesh128Q;

	UPROPERTY()
	UStaticMesh* PreviewMesh256Q;

	UPROPERTY()
	UStaticMesh* Scatter127Q;

	UPROPERTY()
	UStaticMesh* Scatter255Q;

	// CachedValues
	FVector2D CachedOverallResolution;
	int32 StoredInstances;
	TEnumAsByte<EHydraulicErosionMode> CachedErosionMode;
	float CachedElevationScale;
	float CachedElevationPreOffset;
	float CachedRamp;
	float CachedPlateau;
	float CachedChannel;
	float CachedSeed;
	float CachedTiling;
	int32 CachedOverrideSize;
	float CachedStartWater;
	float CachedPersistence;
	float CachedZRangeKilometers;
	bool bCachedInvert;
	int32 CachedOctaves;
	FVector CachedPosition;
};
