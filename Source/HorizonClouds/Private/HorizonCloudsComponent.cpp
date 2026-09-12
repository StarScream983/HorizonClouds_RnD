#include "HorizonCloudsComponent.h"

#include "HorizonCloudsSubsystem.h"

UHorizonCloudsComponent::UHorizonCloudsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FHorizonCloudsBoxRenderData UHorizonCloudsComponent::BuildBoxRenderData() const
{
	FHorizonCloudsBoxRenderData Data;
	Data.BoxPosition = GetComponentLocation();
	Data.CloudsVolume = CloudsVolume;
	if (WeatherTexture && WeatherTexture->GetResource())
	{
		Data.WeatherTextureRHI = WeatherTexture->GetResource()->TextureRHI;
	}
	if (WeatherTexture2 && WeatherTexture2->GetResource())
	{
		Data.WeatherTexture2RHI = WeatherTexture2->GetResource()->TextureRHI;
	}
	Data.WeatherTexTile = WeatherTexTile;
	return Data;
}

void UHorizonCloudsComponent::OnRegister()
{
	Super::OnRegister();
	UpdateSubsystemRegistration();
}

void UHorizonCloudsComponent::OnUnregister()
{
	if (UWorld* World = GetWorld())
	{
		if (UHorizonCloudsSubsystem* Subsystem = World->GetSubsystem<UHorizonCloudsSubsystem>())
		{
			Subsystem->UnregisterBox(this);
		}
	}

	Super::OnUnregister();
}

void UHorizonCloudsComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateSubsystemRegistration();
}

void UHorizonCloudsComponent::NotifyChanged()
{
	UpdateSubsystemRegistration();
}

void UHorizonCloudsComponent::UpdateSubsystemRegistration()
{
	if (UWorld* World = GetWorld())
	{
		if (UHorizonCloudsSubsystem* Subsystem = World->GetSubsystem<UHorizonCloudsSubsystem>())
		{
			Subsystem->RegisterBox(this);
		}
	}
}
