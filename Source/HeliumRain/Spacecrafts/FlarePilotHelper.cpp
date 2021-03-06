#include "../Flare.h"
#include "../Game/FlareCompany.h"
#include "../Game/FlareSector.h"
#include "../Game/FlareGame.h"
#include "../Game/FlareCollider.h"
#include "FlareRCS.h"
#include "FlareOrbitalEngine.h"
#include "FlareWeapon.h"
#include "FlareShipPilot.h"
#include "FlarePilotHelper.h"


DECLARE_CYCLE_STAT(TEXT("PilotHelper CheckFriendlyFire"), STAT_PilotHelper_CheckFriendlyFire, STATGROUP_Flare);
DECLARE_CYCLE_STAT(TEXT("PilotHelper Anticollision"), STAT_PilotHelper_AnticollisionCorrection, STATGROUP_Flare);
DECLARE_CYCLE_STAT(TEXT("PilotHelper Anticollision Avoidance"), STAT_PilotHelper_AnticollisionCorrection_Avoidance, STATGROUP_Flare);
DECLARE_CYCLE_STAT(TEXT("PilotHelper GetBestTarget"), STAT_PilotHelper_GetBestTarget, STATGROUP_Flare);
DECLARE_CYCLE_STAT(TEXT("PilotHelper GetBestTargetComponent"), STAT_PilotHelper_GetBestTargetComponent, STATGROUP_Flare);
DECLARE_CYCLE_STAT(TEXT("PilotHelper CheckRelativeDangerosity"), STAT_PilotHelper_CheckRelativeDangerosity, STATGROUP_Flare);


bool PilotHelper::CheckFriendlyFire(UFlareSector* Sector, UFlareCompany* MyCompany, FVector FireBaseLocation, FVector FireBaseVelocity , float AmmoVelocity, FVector FireAxis, float MaxDelay, float AimRadius)
{
	SCOPE_CYCLE_COUNTER(STAT_PilotHelper_CheckFriendlyFire);

	//FLOG("CheckFriendlyFire");
	for (int32 SpacecraftIndex = 0; SpacecraftIndex < Sector->GetSpacecrafts().Num(); SpacecraftIndex++)
	{
		AFlareSpacecraft* SpacecraftCandidate = Sector->GetSpacecrafts()[SpacecraftIndex];

		if (SpacecraftCandidate)
		{
			if (MyCompany->GetWarState(SpacecraftCandidate->GetParent()->GetCompany()) == EFlareHostility::Hostile)
			{
				//FLOG("  hostile, skip");
				continue;
			}

			// Compute target Axis for each gun
			FVector AmmoIntersectionLocation;
			float AmmoIntersectionTime = SpacecraftHelper::GetIntersectionPosition(SpacecraftCandidate->GetActorLocation(), SpacecraftCandidate->Airframe->GetPhysicsLinearVelocity(), FireBaseLocation, FireBaseVelocity , AmmoVelocity, 0, &AmmoIntersectionLocation);
			if (AmmoIntersectionTime < 0 || AmmoIntersectionTime > MaxDelay)
			{
				// No ammo intersection, or too far, don't alert
				//FLOGV("  Too far (%f > %f)", AmmoIntersectionTime , MaxDelay);
				continue;
			}

			float TargetSize = SpacecraftCandidate->GetMeshScale() / 100.f + AimRadius * 2; // Radius in meters
			FVector DeltaLocation = (SpacecraftCandidate->GetActorLocation()-FireBaseLocation) / 100.f;
			float Distance = DeltaLocation.Size();

			FVector FireTargetAxis = (AmmoIntersectionLocation - FireBaseLocation - AmmoIntersectionTime * FireBaseVelocity).GetUnsafeNormal();

			float AngularPrecisionDot = FVector::DotProduct(FireTargetAxis, FireAxis);
			float AngularPrecision = FMath::Acos(AngularPrecisionDot);
			float AngularSize = FMath::Atan(TargetSize / Distance);



			/*FLOGV("  TargetSize %f", TargetSize);
			FLOGV("  Distance %f", Distance);
			FLOGV("  AngularPrecision %f", AngularPrecision);
			FLOGV("  AngularSize %f", AngularSize);*/
			if (AngularPrecision < AngularSize)
			{
				//FLOG("  Dangerous");
				// Dangerous !
				return true;
			}
		}
	}
	return false;

}

bool PilotHelper::FindMostDangerousCollision(AFlareSpacecraft* Ship, AFlareSpacecraft* SpacecraftToIgnore,
											 AActor** MostDangerousCandidateActor,
											 FVector* MostDangerousLocation,
											 float* MostDangerousHitTime,
											 float* MostDangerousInterCollisionTravelTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PilotHelper_AnticollisionCorrection);

	typedef TPair<AActor*, FVector> TFlareCollisionCandidate;

	UFlareSector* ActiveSector = Ship->GetGame()->GetActiveSector();
	TArray<TFlareCollisionCandidate> Candidates;
	TFlareCollisionCandidate Candidate;

	// Select dangerous ships
	for (auto SpacecraftCandidate : ActiveSector->GetSpacecrafts())
	{
		if (SpacecraftCandidate != Ship
		 && SpacecraftCandidate != SpacecraftToIgnore
		 && !Ship->GetDockingSystem()->IsGrantedShip(SpacecraftCandidate)
		 && !Ship->GetDockingSystem()->IsDockedShip(SpacecraftCandidate))
		{
			Candidate.Key = SpacecraftCandidate;
			Candidate.Value = SpacecraftCandidate->Airframe->GetPhysicsLinearVelocity();
			Candidates.Add(Candidate);
		}
	}

	// Select dangerous asteroids
	for (auto AsteroidCandidate : ActiveSector->GetAsteroids())
	{
		Candidate.Key = AsteroidCandidate;
		Candidate.Value = AsteroidCandidate->GetAsteroidComponent()->GetPhysicsLinearVelocity();
		Candidates.Add(Candidate);
	}

	// Select dangerous colliders
	TArray<AActor*> ColliderActorList;
	UGameplayStatics::GetAllActorsOfClass(Ship->GetWorld(), AFlareCollider::StaticClass(), ColliderActorList);
	for (auto ColliderCandidate : ColliderActorList)
	{
		Candidate.Key = Cast<AFlareCollider>(ColliderCandidate);
		Candidate.Value = FVector::ZeroVector;
		Candidates.Add(Candidate);
	}

	// No candidate found, return
	if (Candidates.Num() == 0)
	{
		return false;
	}

	// Input data for danger processing
	FBox ShipBox = Ship->GetComponentsBoundingBox();
	FVector CurrentVelocity = Ship->GetLinearVelocity() * 100;
	FVector CurrentLocation = (ShipBox.Max + ShipBox.Min) / 2.0;
	float CurrentSize = FMath::Max(ShipBox.GetExtent().Size(), 1.0f);
	float MaxRelevanceDistance = 200 * CurrentSize;

	// Output data
	*MostDangerousCandidateActor = NULL;
	*MostDangerousHitTime = 0;
	*MostDangerousInterCollisionTravelTime = 0;

	// Process all candidates
	for (auto SelectedCandidate : Candidates)
	{
		if ((SelectedCandidate.Key->GetActorLocation() - CurrentLocation).Size() < MaxRelevanceDistance)
		{
			CheckRelativeDangerosity(SelectedCandidate.Key, CurrentLocation, CurrentSize, SelectedCandidate.Value, CurrentVelocity,
				MostDangerousCandidateActor, MostDangerousLocation, MostDangerousHitTime, MostDangerousInterCollisionTravelTime);
		}
	}

	return *MostDangerousCandidateActor != NULL;
}

bool PilotHelper::IsAnticollisionImminent(AFlareSpacecraft* Ship, float PreventionDuration)
{
	// Output data
	AActor* MostDangerousCandidateActor;
	FVector MostDangerousLocation;
	float MostDangerousHitTime;
	float MostDangerousInterCollisionTravelTime;

	bool HaveCollision = FindMostDangerousCollision(Ship, NULL, &MostDangerousCandidateActor, &MostDangerousLocation, &MostDangerousHitTime, &MostDangerousInterCollisionTravelTime);

	if (HaveCollision && Ship->IsLoadedAndReady() && !Ship->GetNavigationSystem()->IsAutoPilot() && !Ship->GetNavigationSystem()->IsDocked())
	{
		if((MostDangerousHitTime - MostDangerousInterCollisionTravelTime) < PreventionDuration)
		{
			return true;
		}
	}

	return false;
}

FVector PilotHelper::AnticollisionCorrection(AFlareSpacecraft* Ship, FVector InitialVelocity, AFlareSpacecraft* SpacecraftToIgnore, float PreventionDuration)
{
	SCOPE_CYCLE_COUNTER(STAT_PilotHelper_AnticollisionCorrection);

	// Output data
	AActor* MostDangerousCandidateActor;
	FVector MostDangerousLocation;
	float MostDangerousHitTime;
	float MostDangerousInterCollisionTravelTime;

	bool HaveCollision = FindMostDangerousCollision(Ship, SpacecraftToIgnore,
		&MostDangerousCandidateActor, &MostDangerousLocation, &MostDangerousHitTime, &MostDangerousInterCollisionTravelTime);

	if(!HaveCollision)
	{
		return InitialVelocity;
	}

	// Avoid the most dangerous target
	if (MostDangerousCandidateActor)
	{
		SCOPE_CYCLE_COUNTER(STAT_PilotHelper_AnticollisionCorrection_Avoidance);

		FBox ShipBox = Ship->GetComponentsBoundingBox();
		FVector CurrentLocation = (ShipBox.Max + ShipBox.Min) / 2.0;

		if (MostDangerousHitTime > 0)
		{
			UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MostDangerousCandidateActor->GetRootComponent());
			

			FVector MinDistancePoint = CurrentLocation + Ship->Airframe->GetPhysicsLinearVelocity() * MostDangerousHitTime;
			FVector FutureTargetLocation =  MostDangerousLocation + StaticMeshComponent->GetPhysicsLinearVelocity() * MostDangerousHitTime;
			FVector AvoidanceVector = MinDistancePoint - FutureTargetLocation;
			FVector AvoidanceAxis;

			if (AvoidanceVector.IsNearlyZero())
			{
				AvoidanceAxis = FMath::VRand();
			}
			else
			{
				AvoidanceAxis = AvoidanceVector.GetUnsafeNormal();
			}

			/*DrawDebugLine(Ship->GetWorld(), CurrentLocation, MinDistancePoint , FColor::Yellow, true);
			DrawDebugLine(Ship->GetWorld(), CurrentLocation, CurrentLocation + Ship->Airframe->GetPhysicsLinearVelocity(), FColor::White, true);
			DrawDebugLine(Ship->GetWorld(), CurrentLocation, CurrentLocation + AvoidanceAxis *1000 , FColor::Green, true);
			DrawDebugLine(Ship->GetWorld(), CurrentLocation, CurrentLocation + InitialVelocity *10 , FColor::Blue, true);
			FLOGV("MostDangerousHitTime %f", MostDangerousHitTime);
			FLOGV("MostDangerousInterCollisionTravelTime %f", MostDangerousInterCollisionTravelTime);*/

			// Below few second begin avoidance maneuver
			float Alpha = 1 - FMath::Max(0.0f, MostDangerousHitTime - MostDangerousInterCollisionTravelTime)/PreventionDuration;
			FVector Temp = InitialVelocity * (1.f - Alpha) + Alpha * AvoidanceAxis * Ship->GetNavigationSystem()->GetLinearMaxVelocity();

			//DrawDebugLine(Ship->GetWorld(), CurrentLocation, CurrentLocation + Temp * 10, FColor::Magenta, true);

			return Temp;
		}
		else
		{
			FVector Temp = (CurrentLocation - MostDangerousLocation).GetUnsafeNormal() * Ship->GetNavigationSystem()->GetLinearMaxVelocity();

			//DrawDebugLine(Ship->GetWorld(), CurrentLocation, CurrentLocation + Temp * 10, FColor::Red, true);
			return Temp;
		}
	}

	return InitialVelocity;
}

bool PilotHelper::IsSectorExitImminent(AFlareSpacecraft* Ship, float PreventionDuration)
{
	float Distance = Ship->GetActorLocation().Size();
	float Limits = Ship->GetGame()->GetActiveSector()->GetSectorLimits();

	float MinDistance = Limits - Distance;
	bool ExitImminent = false;

	// Facing the center of the sector : no warning
	if (FVector::DotProduct(Ship->GetActorRotation().Vector(), -Ship->GetActorLocation()) > 0.5)
	{
		ExitImminent = false;
	}

	// Else, if close to limits : warning
	else if (MinDistance < 0.1 * Limits)
	{
		ExitImminent = true;
	}

	// Else if going to exit soon : warning
	else if (!Ship->GetLinearVelocity().IsNearlyZero())
	{
		// https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
		FVector ShipDirection = Ship->GetLinearVelocity().GetUnsafeNormal();
		FVector ShipLocation = Ship->GetActorLocation();
		float Dot = FVector::DotProduct(ShipDirection, ShipLocation);

		float DistanceBeforeExit = -Dot + FMath::Sqrt(Dot * Dot - Distance * Distance + Limits * Limits);
		float Velocity = Ship->GetLinearVelocity().Size() * 100;
		float DurationBeforeExit = DistanceBeforeExit / Velocity;

		if (DurationBeforeExit < PreventionDuration)
		{
			ExitImminent = true;
		}
	}

	return ExitImminent;
}

AFlareSpacecraft* PilotHelper::GetBestTarget(AFlareSpacecraft* Ship, struct TargetPreferences Preferences)
{
	SCOPE_CYCLE_COUNTER(STAT_PilotHelper_GetBestTarget);

	if (!Ship || !Ship->GetGame()->GetActiveSector())
	{
		return NULL;
	}

	AFlareSpacecraft* BestTarget = NULL;
	float BestScore = 0;

	//FLOGV("GetBestTarget for %s", *Ship->GetImmatriculation().ToString());

	for (int32 SpacecraftIndex = 0; SpacecraftIndex < Ship->GetGame()->GetActiveSector()->GetSpacecrafts().Num(); SpacecraftIndex++)
	{
		AFlareSpacecraft* ShipCandidate = Ship->GetGame()->GetActiveSector()->GetSpacecrafts()[SpacecraftIndex];

		if (Preferences.IgnoreList.Contains(ShipCandidate))
		{
			continue;
		}

		if (Ship->GetParent()->GetCompany()->GetWarState(ShipCandidate->GetCompany()) != EFlareHostility::Hostile)
		{
			// Ignore not hostile ships
			continue;
		}

		if (!ShipCandidate->GetParent()->GetDamageSystem()->IsAlive())
		{
			// Ignore destroyed ships
			continue;
		}

		if (ShipCandidate->GetActorLocation().Size() > ShipCandidate->GetGame()->GetActiveSector()->GetSectorLimits())
		{
			// Ignore out limit ships
			continue;
		}

		float Score;
		float StateScore;
		float AttackTargetScore;
		float DistanceScore;
		float AlignementScore;

		StateScore = Preferences.TargetStateWeight;

		if (ShipCandidate->GetParent()->GetSize() == EFlarePartSize::L)
		{
			StateScore *= Preferences.IsLarge;
		}

		if (ShipCandidate->GetParent()->GetSize() == EFlarePartSize::S)
		{
			StateScore *= Preferences.IsSmall;
		}

		if (ShipCandidate->GetParent()->IsStation())
		{
			StateScore *= Preferences.IsStation;
		}
		else
		{
			StateScore *= Preferences.IsNotStation;
		}

		if (ShipCandidate->GetParent()->IsMilitary())
		{
			StateScore *= Preferences.IsMilitary;
		}
		else
		{
			StateScore *= Preferences.IsNotMilitary;
		}

		if (IsShipDangerous(ShipCandidate))
		{
			StateScore *= Preferences.IsDangerous;
		}
		else
		{
			StateScore *= Preferences.IsNotDangerous;
		}

		if (ShipCandidate->GetParent()->GetDamageSystem()->IsStranded())
		{
			StateScore *= Preferences.IsStranded;
		}
		else
		{
			StateScore *= Preferences.IsNotStranded;
		}

		if (ShipCandidate->GetParent()->GetDamageSystem()->IsUncontrollable() && ShipCandidate->GetParent()->GetDamageSystem()->IsDisarmed())
		{
			if (ShipCandidate->IsMilitary())
			{
				StateScore *= Preferences.IsUncontrollableMilitary;
			}
			else
			{
				StateScore *= Preferences.IsUncontrollableCivil;
			}
		}
		else
		{
			StateScore *= Preferences.IsNotUncontrollable;
		}

		int BombCount = 0;
		// Divise by 25 the stateScore per current incoming missile
		for (AFlareBomb* Bomb : Ship->GetGame()->GetActiveSector()->GetBombs())
		{
			if (Bomb->GetTargetSpacecraft() == ShipCandidate && Bomb->IsActive())
			{
				BombCount++;
				StateScore /= 25;
			}
		}

		if (IsShipDangerous(ShipCandidate))
		{
			if(BombCount > 1)
			{
				continue;
			}
		}
		else
		{
			if(BombCount > 0)
			{
				continue;
			}
		}


		if(ShipCandidate->GetParent()->IsHarpooned()) {
			if(ShipCandidate->GetParent()->GetDamageSystem()->IsUncontrollable())
			{
				// Never target harponned uncontrollable ships
				continue;
			}
			StateScore *=  Preferences.IsHarpooned;
		}


		if(ShipCandidate == Preferences.LastTarget) {
			StateScore *=  Preferences.LastTargetWeight;
		}

		float Distance = (Preferences.BaseLocation - ShipCandidate->GetActorLocation()).Size();
		if (Distance >= Preferences.MaxDistance)
		{
			DistanceScore = 0.f;
		}
		else
		{
			DistanceScore = Preferences.DistanceWeight * (1.f - (Distance / Preferences.MaxDistance));
		}

		if (Preferences.AttackTarget && IsShipDangerous(ShipCandidate) && ShipCandidate->GetPilot()->GetTargetShip() == Preferences.AttackTarget)
		{
			AttackTargetScore = Preferences.AttackTargetWeight;
		}
		else
		{
			AttackTargetScore = 0.0f;
		}

		FVector Direction = (ShipCandidate->GetActorLocation() - Preferences.BaseLocation).GetUnsafeNormal();


		float Alignement = FVector::DotProduct(Preferences.PreferredDirection, Direction);

		if (Alignement > Preferences.MinAlignement)
		{
			AlignementScore = Preferences.AlignementWeight * ((Alignement - Preferences.MinAlignement) / (1 - Preferences.MinAlignement));
		}
		else
		{
			AlignementScore = 0;
		}

		Score = StateScore * (AttackTargetScore + DistanceScore + AlignementScore);


		/*FLOGV("  - %s: %f", *ShipCandidate->GetImmatriculation().ToString(), Score);
		FLOGV("        - StateScore=%f", StateScore);
		FLOGV("        - AttackTargetScore=%f", AttackTargetScore);
		FLOGV("        - DistanceScore=%f", DistanceScore);
		FLOGV("        - AlignementScore=%f", AlignementScore);*/

		if (Score > 0)
		{
			if (BestTarget == NULL || Score > BestScore)
			{
				BestTarget = ShipCandidate;
				BestScore = Score;
			}
		}
	}

	/*if(BestTarget)
	{
		FLOGV(" -> BestTarget %s with %f", *BestTarget->GetImmatriculation().ToString(), BestScore);
	}
	else
	{
		FLOG(" -> No target");
	}*/

	return BestTarget;
}


UFlareSpacecraftComponent* PilotHelper::GetBestTargetComponent(AFlareSpacecraft* TargetSpacecraft)
{
	SCOPE_CYCLE_COUNTER(STAT_PilotHelper_GetBestTargetComponent);

	// Is armed, target the gun
	// Else if not stranger target the orbital
	// else target the rsc

	float WeaponWeight = 1;
	float PodWeight = 1;
	float RCSWeight = 1;
	float InternalWeight = 1;

	if (!TargetSpacecraft->GetParent()->GetDamageSystem()->IsDisarmed())
	{
		WeaponWeight = 10;
		PodWeight = 4;
		RCSWeight = 1;
		InternalWeight = 1;
	}
	else if (!TargetSpacecraft->GetParent()->GetDamageSystem()->IsStranded())
	{
		PodWeight = 5;
		RCSWeight = 1;
		InternalWeight = 1;
	}
	else
	{
		RCSWeight = 1;
		InternalWeight = 1;
	}

	TArray<UFlareSpacecraftComponent*> ComponentSelection;

	TArray<UActorComponent*> Components = TargetSpacecraft->GetComponentsByClass(UFlareSpacecraftComponent::StaticClass());
	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
	{
		UFlareSpacecraftComponent* Component = Cast<UFlareSpacecraftComponent>(Components[ComponentIndex]);

		if (Component->GetDescription() && !Component->IsBroken() )
		{

			UFlareRCS* RCS = Cast<UFlareRCS>(Component);
			if (RCS)
			{
				for (int32 i = 0; i < RCSWeight; i++)
				{
					ComponentSelection.Add(Component);
				}
			}

			UFlareOrbitalEngine* OrbitalEngine = Cast<UFlareOrbitalEngine>(Component);
			if (OrbitalEngine)
			{
				for (int32 i = 0; i < PodWeight; i++)
				{
					ComponentSelection.Add(Component);
				}
			}

			UFlareWeapon* Weapon = Cast<UFlareWeapon>(Component);
			if (Weapon)
			{
				for (int32 i = 0; i < WeaponWeight; i++)
				{
					ComponentSelection.Add(Component);
				}
			}

			if (Component->GetDescription()->Type == EFlarePartType::InternalComponent)
			{
				for (int32 i = 0; i < InternalWeight; i++)
				{
					ComponentSelection.Add(Component);
				}
			}
		}
	}

	if (ComponentSelection.Num() == 0)
	{
		return NULL;
	}

	int32 ComponentIndex = FMath::RandRange(0, ComponentSelection.Num() - 1);
	return ComponentSelection[ComponentIndex];
}

bool PilotHelper::CheckRelativeDangerosity(AActor* CandidateActor, FVector CurrentLocation, float CurrentSize, FVector TargetVelocity, FVector CurrentVelocity, AActor** MostDangerousCandidateActor, FVector*MostDangerousLocation, float* MostDangerousHitTime, float* MostDangerousInterCollisionTravelTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PilotHelper_CheckRelativeDangerosity);
	//FLOGV("PilotHelper::CheckRelativeDangerosity for %s, ship size %f", *CandidateActor->GetName(), CurrentSize);

	FVector CandidateLocation = CandidateActor->GetActorLocation();
	FVector DeltaVelocity = TargetVelocity - CurrentVelocity;
	FVector DeltaLocation = CandidateLocation - CurrentLocation;

	// Eliminate obvious not-dangerous candidates based on velocity
	if (DeltaVelocity.IsNearlyZero() || (FVector::DotProduct(DeltaLocation.GetUnsafeNormal(), DeltaVelocity) > -200))
	{
		return false;
	}
	
	// Get the object size & location
	float CandidateSize;
	if (CandidateActor->IsA(AFlareCollider::StaticClass()) || CandidateActor->IsA(AFlareAsteroid::StaticClass()))
	{
		CandidateSize = Cast<UStaticMeshComponent>(CandidateActor->GetRootComponent())->Bounds.SphereRadius;
	}
	else
	{
		FBox CandidateBox = CandidateActor->GetComponentsBoundingBox();
		CandidateSize = FMath::Max(CandidateBox.GetExtent().Size(), 1.0f);
		CandidateLocation = (CandidateBox.Max + CandidateBox.Min) / 2.0;
	}

	// Minimum distance highter than object size sum : not dangerous
	float MinDistance = FVector::CrossProduct(DeltaLocation, -DeltaVelocity).Size() / DeltaVelocity.Size();
	float SizeSum = CurrentSize + CandidateSize;
	if (SizeSum < MinDistance)
	{
		return false;
	}

	// Already intersecting ?
	if (DeltaLocation.Size() < SizeSum)
	{
		float IntersectDeep = SizeSum - DeltaLocation.Size();
		if (!*MostDangerousCandidateActor || *MostDangerousHitTime > -IntersectDeep)
		{
			*MostDangerousCandidateActor = CandidateActor;
			*MostDangerousHitTime = -IntersectDeep;
			*MostDangerousInterCollisionTravelTime = 0;
			*MostDangerousLocation = CandidateLocation;
		}
	}
	
	// There will be a hit
	float DistanceToMinDistancePoint = FMath::Sqrt(DeltaLocation.SizeSquared() - FMath::Square(MinDistance));
	float TimeToMinDistance = DistanceToMinDistancePoint / DeltaVelocity.Size();
	float InterCollisionTravelTime = SizeSum / DeltaVelocity.Size();

	// Time to minimum distance is high : not dangerous
	if (TimeToMinDistance > 5.f + InterCollisionTravelTime)
	{
		return false;
	}

	//FLOGV("DistanceToMinDistancePoint %f", DistanceToMinDistancePoint)
	//FLOGV("TimeToMinDistance %f", TimeToMinDistance)	
	//FLOGV("InterCollisionTravelTime %f", InterCollisionTravelTime)

	//DrawDebugLine(Ship->GetWorld(), CandidateLocation, Box.Max , FColor::Yellow, true);
	//DrawDebugLine(Ship->GetWorld(), CandidateLocation, Box.Min , FColor::Green, true);
	//DrawDebugSphere(Ship->GetWorld(), CandidateLocation, 10, 12, FColor::White, true);
	
	//DrawDebugBox(Ship->GetWorld(), BoxLocation, BoxSize, FQuat(FRotator::ZeroRotator), FColor::Blue, 1.f);
	//DrawDebugBox(Ship->GetWorld(), Box2Location, Box2Size, FQuat(FRotator::ZeroRotator), FColor::Cyan, 1.f);

	//DrawDebugSphere(Ship->GetWorld(), CandidateLocation, CandidateSize, 12, FColor::Red, true);
	//DrawDebugSphere(Ship->GetWorld(), CurrentLocation, Ship->GetMeshScale(), 12, FColor::Green, true);
	//DrawDebugSphere(Ship->GetWorld(), Box2Location, CandidateSize * 2.f, 12, FColor::Magenta, true);
	
	// Keep only most imminent hit
	if (!*MostDangerousCandidateActor || *MostDangerousHitTime > TimeToMinDistance)
	{
		*MostDangerousCandidateActor = CandidateActor;
		*MostDangerousHitTime = TimeToMinDistance;
		*MostDangerousInterCollisionTravelTime = InterCollisionTravelTime;
		*MostDangerousLocation = CandidateLocation;
	}
	return true;
}

bool PilotHelper::IsShipDangerous(AFlareSpacecraft* ShipCandidate)
{
	return ShipCandidate->IsMilitary() && !ShipCandidate->GetParent()->GetDamageSystem()->IsDisarmed();
}
