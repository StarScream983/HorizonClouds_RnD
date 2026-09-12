#pragma once

#include "CoreMinimal.h"
#include "RHIResources.h"

// BoxPosition is the volume's geometric CENTER. CloudsVolume is the volume's full world-space size in
// UU (not a mesh scale factor — no mesh at all).
struct FHorizonCloudsBoxRenderData
{
	FVector BoxPosition = FVector::ZeroVector;
	FVector CloudsVolume = FVector(4000000.0, 4000000.0, 200000.0);

	FTextureRHIRef WeatherTextureRHI;
	FTextureRHIRef WeatherTexture2RHI;
	float WeatherTexTile = 4000000.0f;
};
