#pragma once

#include "CoreMinimal.h"

// BoxPosition is the BOTTOM-MID of the volume (centered in X/Y, base in Z), not its geometric center.
// CloudsVolume is the volume's full world-space size in UU (not a mesh scale factor — no mesh at all).
struct FHorizonCloudsBoxRenderData
{
	FVector BoxPosition = FVector::ZeroVector;
	FVector CloudsVolume = FVector(4000000.0, 4000000.0, 200000.0);
};
