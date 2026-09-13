#pragma once

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

// Step 2: ray-box intersection (IQ's box intersector) against the volume, no density/raymarch yet.
// Hit pixels are magenta, misses are black.
class HORIZONCLOUDSCORESHADERS_API FHorizonCloudsPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FHorizonCloudsPS);
	SHADER_USE_PARAMETER_STRUCT(FHorizonCloudsPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector3f, BoxPositionRelative)
		SHADER_PARAMETER(FVector3f, BoxPositionRelativeHi)
		SHADER_PARAMETER(FVector3f, BoxPositionRelativeLo)
		SHADER_PARAMETER(FVector3f, CloudsVolume)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WeatherTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, WeatherTextureSampler)
		SHADER_PARAMETER(uint32, bHasWeatherTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture3D, BaseNoiseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, BaseNoiseTextureSampler)
		SHADER_PARAMETER(uint32, bHasBaseNoiseTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture3D, SmallNoiseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SmallNoiseTextureSampler)
		SHADER_PARAMETER(uint32, bHasSmallNoiseTexture)
		SHADER_PARAMETER(uint32, bDebugSolid)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
