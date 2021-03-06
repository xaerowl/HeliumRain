#pragma once

#include "../Game/FlareSimulatedSector.h"
#include "FlareOrbitalMap.generated.h"


UCLASS()
class HELIUMRAIN_API UFlareOrbitalMap : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:

	/*----------------------------------------------------
		Public data
	----------------------------------------------------*/

	/** Sectors data */
	UPROPERTY(EditAnywhere, Category = Content)
	TArray<FFlareSectorCelestialBodyDescription> OrbitalBodies;

};
