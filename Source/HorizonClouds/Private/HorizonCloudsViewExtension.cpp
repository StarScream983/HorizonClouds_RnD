#include "HorizonCloudsViewExtension.h"

#include "HorizonCloudsShader.h"
#include "HorizonCloudsSubsystem.h"
#include "FXRenderingUtils.h"
#include "Math/DoubleFloat.h"
#include "PostProcess/PostProcessInputs.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"
#include "SceneView.h"
#include "ScreenPass.h"
#include "SystemTextures.h"

static TAutoConsoleVariable<int32> CVarHorizonCloudsDebugSolid(
	TEXT("r.HorizonClouds.DebugSolid"),
	0,
	TEXT("1 = force fullscreen magenta regardless of the box intersection test. 0 = actual ray-box test."),
	ECVF_RenderThreadSafe);

FHorizonCloudsViewExtension::FHorizonCloudsViewExtension(const FAutoRegister &AutoRegister, UWorld *InWorld)
	: FWorldSceneViewExtension(AutoRegister, InWorld)
{
}

void FHorizonCloudsViewExtension::BeginRenderViewFamily(FSceneViewFamily &InViewFamily)
{
	if (InViewFamily.FrameNumber != CachedFrameNumber)
	{
		bHasCachedBox = false;
		CachedFrameNumber = InViewFamily.FrameNumber;
	}

	if (const FSceneInterface *Scene = InViewFamily.Scene)
	{
		if (UWorld *ViewWorld = Scene->GetWorld())
		{
			bHasCachedBox = UHorizonCloudsSubsystem::FindBoxRenderData(ViewWorld, CachedBox);
		}
	}
}

void FHorizonCloudsViewExtension::PrePostProcessPass_RenderThread(
	FRDGBuilder &GraphBuilder,
	const FSceneView &View,
	const FPostProcessingInputs &Inputs)
{
	const bool bDebugSolid = CVarHorizonCloudsDebugSolid.GetValueOnRenderThread() != 0;

	if (!bHasCachedBox || !View.Family)
	{
		return;
	}

	Inputs.Validate();
	if (!Inputs.SceneTextures)
	{
		return;
	}

	const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
	FScreenPassTexture SceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);
	if (!SceneColor.IsValid())
	{
		return;
	}

	FGlobalShaderMap *GlobalShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	TShaderMapRef<FHorizonCloudsPS> PixelShader(GlobalShaderMap);
	if (!PixelShader.IsValid())
	{
		return;
	}

	FScreenPassRenderTarget Output(SceneColor, ERenderTargetLoadAction::ELoad);
	const FScreenPassTextureViewport OutputViewport(Output);

	const FVector ViewOrigin = View.ViewMatrices.GetViewOrigin();
	const FVector BoxPositionRelative = CachedBox.BoxPosition - ViewOrigin;
	const FDFVector3 BoxPositionRelativeDF(BoxPositionRelative);

	FHorizonCloudsPS::FParameters *PassParameters = GraphBuilder.AllocParameters<FHorizonCloudsPS::FParameters>();
	PassParameters->BoxPositionRelative = FVector3f(BoxPositionRelative);
	PassParameters->BoxPositionRelativeHi = BoxPositionRelativeDF.High;
	PassParameters->BoxPositionRelativeLo = BoxPositionRelativeDF.Low;
	PassParameters->CloudsVolume = FVector3f(CachedBox.CloudsVolume);

	PassParameters->bHasWeatherTexture = CachedBox.WeatherTextureRHI.IsValid() ? 1u : 0u;
	PassParameters->WeatherTexture = CachedBox.WeatherTextureRHI.IsValid()
										  ? RegisterExternalTexture(GraphBuilder, CachedBox.WeatherTextureRHI, TEXT("HorizonClouds.WeatherTexture"))
										  : GSystemTextures.GetBlackDummy(GraphBuilder);
	PassParameters->WeatherTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();

	PassParameters->bHasWeatherTexture2 = CachedBox.WeatherTexture2RHI.IsValid() ? 1u : 0u;
	PassParameters->WeatherTexture2 = CachedBox.WeatherTexture2RHI.IsValid()
										   ? RegisterExternalTexture(GraphBuilder, CachedBox.WeatherTexture2RHI, TEXT("HorizonClouds.WeatherTexture2"))
										   : GSystemTextures.GetBlackDummy(GraphBuilder);
	PassParameters->WeatherTexture2Sampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();

	PassParameters->WeatherTexTile = CachedBox.WeatherTexTile;

	PassParameters->bDebugSolid = bDebugSolid ? 1u : 0u;
	PassParameters->View = View.ViewUniformBuffer;
	PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

	TShaderMapRef<FScreenPassVS> VertexShader(GlobalShaderMap);
	FRHIBlendState *BlendState = FScreenPassPipelineState::FDefaultBlendState::GetRHI();
	FRHIDepthStencilState *DepthStencilState = FScreenPassPipelineState::FDefaultDepthStencilState::GetRHI();

	AddDrawScreenPass(
		GraphBuilder,
		RDG_EVENT_NAME("HorizonClouds"),
		View,
		OutputViewport,
		OutputViewport,
		VertexShader,
		PixelShader,
		BlendState,
		DepthStencilState,
		PassParameters);
}
