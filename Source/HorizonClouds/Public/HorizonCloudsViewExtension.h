#pragma once

#include "HorizonCloudsRenderTypes.h"
#include "SceneViewExtension.h"

class FHorizonCloudsViewExtension : public FWorldSceneViewExtension
{
public:
	FHorizonCloudsViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld);

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PrePostProcessPass_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessingInputs& Inputs) override;

private:
	uint32 CachedFrameNumber = 0;
	bool bHasCachedBox = false;
	FHorizonCloudsBoxRenderData CachedBox;
};
