#include "HorizonCloudsSubsystem.h"

#include "HorizonCloudsComponent.h"
#include "HorizonCloudsImGui.h"
#include "HorizonCloudsViewExtension.h"
#include "EngineUtils.h"

void UHorizonCloudsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ViewExtension = FSceneViewExtensions::NewExtension<FHorizonCloudsViewExtension>(GetWorld());
}

void UHorizonCloudsSubsystem::Deinitialize()
{
	ViewExtension.Reset();
	Super::Deinitialize();
}

void UHorizonCloudsSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	HorizonCloudsImGui::Draw();
}

TStatId UHorizonCloudsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UHorizonCloudsSubsystem, STATGROUP_Tickables);
}

void UHorizonCloudsSubsystem::RegisterBox(UHorizonCloudsComponent* Component)
{
	if (!IsValid(Component))
	{
		return;
	}

	RegisteredBox = Component;
}

void UHorizonCloudsSubsystem::UnregisterBox(UHorizonCloudsComponent* Component)
{
	if (RegisteredBox.Get() == Component)
	{
		RegisteredBox.Reset();
	}
}

bool UHorizonCloudsSubsystem::GetBoxRenderData(FHorizonCloudsBoxRenderData& OutBoxData) const
{
	if (const UHorizonCloudsComponent* Component = RegisteredBox.Get())
	{
		OutBoxData = Component->BuildBoxRenderData();
		return true;
	}

	return false;
}

bool UHorizonCloudsSubsystem::FindBoxRenderData(const UWorld* World, FHorizonCloudsBoxRenderData& OutBoxData)
{
	if (!World)
	{
		return false;
	}

	if (UHorizonCloudsSubsystem* Subsystem = World->GetSubsystem<UHorizonCloudsSubsystem>())
	{
		if (Subsystem->GetBoxRenderData(OutBoxData))
		{
			return true;
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (UHorizonCloudsComponent* Component = It->FindComponentByClass<UHorizonCloudsComponent>())
			{
				Subsystem->RegisterBox(Component);
				OutBoxData = Component->BuildBoxRenderData();
				return true;
			}
		}
	}

	return false;
}
