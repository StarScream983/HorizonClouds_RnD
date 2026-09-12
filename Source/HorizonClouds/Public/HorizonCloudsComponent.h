#pragma once

#include "CoreMinimal.h"
#include "HorizonCloudsRenderTypes.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture2D.h"
#include "HorizonCloudsComponent.generated.h"

// No mesh — the volume is pure raymarch data. Component location is the CENTER of the volume.
UCLASS(ClassGroup = (HorizonClouds), meta = (BlueprintSpawnableComponent, DisplayName = "Horizon Clouds Component"))
class HORIZONCLOUDS_API UHorizonCloudsComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UHorizonCloudsComponent();

	// Full world-space size of the volume in UU (not a mesh scale factor). Default 40km x 40km x 2km.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HorizonClouds|Box")
	FVector CloudsVolume = FVector(4000000.0, 4000000.0, 200000.0);

	// Reference plugin's WeatherTex — a flat 2D coverage map sampled by world XY (RayPos.xy/WeatherTexTile).
	// Copied from the reference project's own demo content (T_WeatherMap/T_WeatherMap_2), not generated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HorizonClouds|Weather")
	TObjectPtr<UTexture2D> WeatherTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HorizonClouds|Weather")
	TObjectPtr<UTexture2D> WeatherTexture2 = nullptr;

	// UU that one full tile of WeatherTexture covers in world XY — matches the reference's WeatherTexTile.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HorizonClouds|Weather", meta = (ClampMin = "1.0"))
	float WeatherTexTile = 4000000.0;

	FHorizonCloudsBoxRenderData BuildBoxRenderData() const;
	void NotifyChanged();

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;

private:
	void UpdateSubsystemRegistration();
};
