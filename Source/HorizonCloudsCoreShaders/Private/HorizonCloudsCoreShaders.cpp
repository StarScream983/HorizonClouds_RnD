#include "HorizonCloudsCoreShaders.h"

#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

namespace
{
	const TCHAR* PluginName = TEXT("HorizonClouds");
	const TCHAR* ShaderVirtualPath = TEXT("/Plugin/HorizonClouds");
}

void FHorizonCloudsCoreShadersModule::StartupModule()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
	checkf(Plugin.IsValid(), TEXT("Failed to find plugin '%s' for shader directory mapping."), PluginName);

	const FString PluginShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"), TEXT("Private"));
	AddShaderSourceDirectoryMapping(ShaderVirtualPath, PluginShaderDir);
}

void FHorizonCloudsCoreShadersModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FHorizonCloudsCoreShadersModule, HorizonCloudsCoreShaders)
