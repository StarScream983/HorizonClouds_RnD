#include "HorizonCloudsImGui.h"

#if WITH_EDITOR
#include "HorizonCloudsCVars.h"
#include <imgui.h>
#endif

void HorizonCloudsImGui::Register()
{
}

void HorizonCloudsImGui::Unregister()
{
}

void HorizonCloudsImGui::Draw()
{
#if WITH_EDITOR
	if (!ImGui::Begin("HORIZON CLOUDS"))
	{
		ImGui::End();
		return;
	}

	float WindSpeed = CVarHorizonCloudsWindSpeed.GetValueOnGameThread();
	if (ImGui::SliderFloat("Wind Speed", &WindSpeed, 0.0f, 20.0f, "%.3f"))
	{
		CVarHorizonCloudsWindSpeed->Set(WindSpeed, ECVF_SetByConsole);
	}

	float TimeScale = CVarHorizonCloudsTimeScale.GetValueOnGameThread();
	if (ImGui::SliderFloat("Time Scale", &TimeScale, 0.0f, 0.01f, "%.5f"))
	{
		CVarHorizonCloudsTimeScale->Set(TimeScale, ECVF_SetByConsole);
	}

	ImGui::End();
#endif
}
