#include "HorizonClouds.h"

#include "HorizonCloudsImGui.h"

void FHorizonCloudsModule::StartupModule()
{
	HorizonCloudsImGui::Register();
}

void FHorizonCloudsModule::ShutdownModule()
{
	HorizonCloudsImGui::Unregister();
}

IMPLEMENT_MODULE(FHorizonCloudsModule, HorizonClouds)
