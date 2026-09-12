#pragma once

#include "Modules/ModuleManager.h"

class FHorizonCloudsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
