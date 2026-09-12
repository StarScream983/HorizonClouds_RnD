#pragma once

#include "CoreMinimal.h"
#include "HorizonCloudsRenderTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "HorizonCloudsSubsystem.generated.h"

class UHorizonCloudsComponent;
class FHorizonCloudsViewExtension;

UCLASS()
class HORIZONCLOUDS_API UHorizonCloudsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	void RegisterBox(UHorizonCloudsComponent* Component);
	void UnregisterBox(UHorizonCloudsComponent* Component);

	bool GetBoxRenderData(FHorizonCloudsBoxRenderData& OutBoxData) const;
	static bool FindBoxRenderData(const UWorld* World, FHorizonCloudsBoxRenderData& OutBoxData);

private:
	TWeakObjectPtr<UHorizonCloudsComponent> RegisteredBox;
	TSharedPtr<FHorizonCloudsViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
