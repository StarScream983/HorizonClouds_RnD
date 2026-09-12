#include "HorizonCloudsImGui.h"

#if WITH_EDITOR
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

	ImGui::TextUnformatted("hello");

	ImGui::End();
#endif
}
