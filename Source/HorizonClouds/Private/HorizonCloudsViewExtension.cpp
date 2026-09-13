#include "HorizonCloudsViewExtension.h"

#include "HorizonCloudsCVars.h"
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

TAutoConsoleVariable<float> CVarHorizonCloudsWindSpeed(
	TEXT("r.HorizonClouds.WindSpeed"),
	12.0f,
	TEXT("Wind speed — verified reference value (WindSpeed=12 from VC_Clouds)."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<float> CVarHorizonCloudsTimeScale(
	TEXT("r.HorizonClouds.TimeScale"),
	0.001f,
	TEXT("Time-to-UV conversion scalar applied on top of WindSpeed. Unverified — reference's exact Time semantics are unknown."),
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

	PassParameters->bHasBaseNoiseTexture = CachedBox.BaseNoiseTextureRHI.IsValid() ? 1u : 0u;
	PassParameters->BaseNoiseTexture = CachedBox.BaseNoiseTextureRHI.IsValid()
											? RegisterExternalTexture(GraphBuilder, CachedBox.BaseNoiseTextureRHI, TEXT("HorizonClouds.BaseNoiseTexture"))
											: GSystemTextures.GetVolumetricBlackDummy(GraphBuilder);
	PassParameters->BaseNoiseTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();

	PassParameters->bHasSmallNoiseTexture = CachedBox.SmallNoiseTextureRHI.IsValid() ? 1u : 0u;
	PassParameters->SmallNoiseTexture = CachedBox.SmallNoiseTextureRHI.IsValid()
											 ? RegisterExternalTexture(GraphBuilder, CachedBox.SmallNoiseTextureRHI, TEXT("HorizonClouds.SmallNoiseTexture"))
											 : GSystemTextures.GetVolumetricBlackDummy(GraphBuilder);
	PassParameters->SmallNoiseTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();

	PassParameters->bDebugSolid = bDebugSolid ? 1u : 0u;
	PassParameters->WindSpeed = CVarHorizonCloudsWindSpeed.GetValueOnRenderThread();
	PassParameters->TimeScale = CVarHorizonCloudsTimeScale.GetValueOnRenderThread();
	PassParameters->View = View.ViewUniformBuffer;
	PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

	TShaderMapRef<FScreenPassVS> VertexShader(GlobalShaderMap);
	FRHIBlendState *BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_Zero, BF_One>::GetRHI();
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
