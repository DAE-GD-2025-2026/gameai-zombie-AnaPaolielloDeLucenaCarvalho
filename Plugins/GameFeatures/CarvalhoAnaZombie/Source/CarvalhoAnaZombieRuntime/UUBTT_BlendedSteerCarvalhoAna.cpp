#include "UUBTT_BlendedSteerCarvalhoAna.h"
#include "AIController.h"
#include "UStudentPerceptorCarvalhoAna.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/StaminaComponent.h"
#include "Village/House/House.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

UUBTT_BlendedSteerCarvalhoAna::UUBTT_BlendedSteerCarvalhoAna()
{
	NodeName = "Blended Steering (Lab Math)";
	bNotifyTick = true; // run every frame
}

EBTNodeResult::Type UUBTT_BlendedSteerCarvalhoAna::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	WanderTimer = WanderUpdateInterval;
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController && AIController->GetPawn())
	{
		APawn* P = AIController->GetPawn();
		LastPosition = P->GetActorLocation();
		bIsStuck = false;
		StuckTimer = 0.0f;

		CurrentPath.Empty();
		CurrentPathIndex = 0;
		PathUpdateTimer = 999.f;
		LastSeekTarget = FVector::ZeroVector;
		LastSeekForce = FVector::ZeroVector;

		// force wander to pick new target
		LastWanderTarget = FVector::ZeroVector;
		TimeSinceWanderStart = 9999.f;
	}
	
	return EBTNodeResult::InProgress;
}

// helper - returns the direction to NEXT waypoint
FVector UUBTT_BlendedSteerCarvalhoAna::GetPathSeekForce(APawn* Pawn, const FVector& TargetLoc, float DeltaSeconds)
{
	float TargetDrift = FVector::Dist2D(TargetLoc, LastSeekTarget);
	PathUpdateTimer += DeltaSeconds;

	if (CurrentPath.Num() == 0 || PathUpdateTimer >= 2.0f || TargetDrift > 150.f)
	{
		if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(Pawn))
		{
			TArray<FVector> NewPath = Survivor->CalculatePath(TargetLoc);
			if (NewPath.Num() >= 2)
			{
				CurrentPath = NewPath;
				CurrentPathIndex = 1;
				PathUpdateTimer = 0.f;
				LastSeekTarget = TargetLoc;

				if (bDrawAimingDebug)
				{
					for (int32 i = 1; i < CurrentPath.Num(); ++i)
					{
						DrawDebugLine(Pawn->GetWorld(), CurrentPath[i - 1] + FVector(0, 0, 30), CurrentPath[i]     + FVector(0, 0, 30), FColor::Magenta, false, 2.0f, 0, 3.f);
					}
				}
			}
			else
			{
				// navMesh returned empty path
				CurrentPath.Empty();
			}
		}
	}

	// follow waypoints
	if (CurrentPath.Num() > 0 && CurrentPathIndex < CurrentPath.Num())
	{
		FVector WP = CurrentPath[CurrentPathIndex];
		WP.Z = Pawn->GetActorLocation().Z;

		float DistToWP = FVector::Dist2D(Pawn->GetActorLocation(), WP);
		if (DistToWP < 120.f) // advance to next waypoint when close
		{
			CurrentPathIndex++;
			if (CurrentPathIndex >= CurrentPath.Num())
			{
				// all waypoints done
				CurrentPath.Empty();
				return (TargetLoc - Pawn->GetActorLocation()).GetSafeNormal2D();
			}
			WP = CurrentPath[CurrentPathIndex];
			WP.Z = Pawn->GetActorLocation().Z;
		}

		FVector ToWP = (WP - Pawn->GetActorLocation()).GetSafeNormal2D();

		ToWP = FMath::VInterpTo(LastSeekForce, ToWP, DeltaSeconds, 6.f);
		LastSeekForce = ToWP;
		return ToWP;
	}

	return (TargetLoc - Pawn->GetActorLocation()).GetSafeNormal2D();
}

void UUBTT_BlendedSteerCarvalhoAna::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BBComp) return;

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return;

	UWorld* World = Pawn->GetWorld();
	if (!World) return;

	FVector FinalSteeringForce = FVector::ZeroVector;
	FVector SeekForce = FVector::ZeroVector;

// SEEK ITEM (path-following)
	AActor* NearestItem = Cast<AActor>(BBComp->GetValueAsObject("NearestItem"));
	if (NearestItem)
	{
		FVector ItemLoc = NearestItem->GetActorLocation();
		float DistToItem = FVector::Dist2D(Pawn->GetActorLocation(), ItemLoc);

		if (DistToItem < 100.0f)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		SeekForce = GetPathSeekForce(Pawn, ItemLoc, DeltaSeconds);
	}
	
// SEEK HOUSE (path-following)
	else if (!BBComp->GetValueAsBool(FName("IsInsideHouse")))
	{
		AActor* NearestHouse = Cast<AActor>(BBComp->GetValueAsObject("NearestHouse"));
		if (NearestHouse)
		{
			FVector HouseLoc = NearestHouse->GetActorLocation();
			float   DistToHouse = FVector::Dist2D(Pawn->GetActorLocation(), HouseLoc);

			if (DistToHouse < 80.0f)
			{
				UStudentPerceptorCarvalhoAna* Perceptor = AIController->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
				if (!Perceptor) Perceptor = Pawn->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
				AHouse* HousePtr = Cast<AHouse>(BBComp->GetValueAsObject(FName("NearestHouse")));
				if (Perceptor && HousePtr) Perceptor->VisitedHouses.AddUnique(HousePtr);
				FVector OutsideDir = (Pawn->GetActorLocation() - HouseLoc).GetSafeNormal2D();
				BBComp->SetValueAsVector(FName("DoorwayLocation"), Pawn->GetActorLocation() + OutsideDir * 300.f);
				BBComp->SetValueAsBool(FName("IsInsideHouse"), true);
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}

			SeekForce = GetPathSeekForce(Pawn, HouseLoc, DeltaSeconds);
		}
	}
	
// SEEK DOORWAY
	bool bIsInside = BBComp->GetValueAsBool(FName("IsInsideHouse"));
	FVector DoorwayLoc = BBComp->GetValueAsVector(FName("DoorwayLocation"));
	AActor* NearestZombie = Cast<AActor>(BBComp->GetValueAsObject("NearestZombie"));

	if (bIsInside && !NearestItem)
	{
		// safety check
		if (DoorwayLoc.IsNearlyZero())
		{
			BBComp->SetValueAsBool(FName("IsInsideHouse"), false);
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		float DistToDoor = FVector::Dist2D(Pawn->GetActorLocation(), DoorwayLoc);

		if (DistToDoor < 150.f)
		{
			BBComp->SetValueAsBool(FName("IsInsideHouse"), false);
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		SeekForce = GetPathSeekForce(Pawn, DoorwayLoc, DeltaSeconds);
	}

// WANDER (NavMesh)
	FVector WanderForce = FVector::ZeroVector;

	float DistToWanderTarget = FVector::Dist(Pawn->GetActorLocation(), LastWanderTarget);
	bool  bNeedNewTarget = DistToWanderTarget < WanderTargetReachedDistance || TimeSinceWanderStart > 8.0f || LastWanderTarget.IsNearlyZero();

	if (bNeedNewTarget)
	{
		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
		{
			FVector CurrentLoc = Pawn->GetActorLocation();
			bool bFoundTarget  = false;

			UStudentPerceptorCarvalhoAna* WPerceptor = nullptr;
			if (AAIController* AIC = OwnerComp.GetAIOwner())
			{
				WPerceptor = AIC->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
				if (!WPerceptor) WPerceptor = Pawn->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
			}

			if (WPerceptor && WPerceptor->KnownHouses.Num() > 0)
			{
				// find nearest UNVISITED known house and steer toward it
				FVector BestHouseDir = FVector::ZeroVector;
				float   BestHouseDist = 999999.f;
				for (const FVector& KH : WPerceptor->KnownHouses)
				{
					float D = FVector::Dist2D(CurrentLoc, KH);
					if (D > 300.f && D < BestHouseDist) // skip house we're standing in
						{ BestHouseDist = D; BestHouseDir = (KH - CurrentLoc).GetSafeNormal2D(); }
				}

				if (!BestHouseDir.IsNearlyZero())
				{
					float HouseAngle = FMath::Atan2(BestHouseDir.Y, BestHouseDir.X);
					HouseAngle += FMath::RandRange(-PI / 4.f, PI / 4.f);
					WanderAngle = HouseAngle;
				}
			}

			FNavLocation Projected;
			
			TArray<AActor*> AllHouses;
			UGameplayStatics::GetAllActorsOfClass(World, AHouse::StaticClass(), AllHouses);

			for (int32 Attempt = 0; Attempt < 5; ++Attempt)
			{
				float SearchRadius = FMath::RandRange(2000.f, 4000.f);
				float PreferredAngle = WanderAngle + FMath::RandRange(-PI / 4.f, PI / 4.f);
				if (Attempt > 2) PreferredAngle = FMath::RandRange(-PI, PI); // fully random if failing
				
				FVector PreferredDir = FVector(FMath::Cos(PreferredAngle), FMath::Sin(PreferredAngle), 0.f);
				FVector Candidate = CurrentLoc + PreferredDir * SearchRadius;

				if (NavSys->GetRandomReachablePointInRadius(Candidate, 600.f, Projected))
				{
					bool bInsideHouse = false;
					for (AActor* HA : AllHouses)
					{
						if (!HA) continue;
						FVector Origin, Extent;
						HA->GetActorBounds(false, Origin, Extent);
						const float Margin = 200.f; // Buffer to prevent pathing directly against outer walls
						
						bool bInX = Projected.Location.X > Origin.X - Extent.X - Margin && Projected.Location.X < Origin.X + Extent.X + Margin;
						bool bInY = Projected.Location.Y > Origin.Y - Extent.Y - Margin && Projected.Location.Y < Origin.Y + Extent.Y + Margin;
						
						if (bInX && bInY) { bInsideHouse = true; break; }
					}

					if (!bInsideHouse)
					{
						LastWanderTarget = Projected.Location;
						bFoundTarget = true;
						break;
					}
				}
			}

			if (!bFoundTarget)
			{
				// fallback - random reachable point from current position
				for (int32 FbAttempt = 0; FbAttempt < 3; ++FbAttempt)
				{
					if (NavSys->GetRandomReachablePointInRadius(CurrentLoc, 3000.f, Projected))
					{
						bool bInsideHouseFb = false;
						for (AActor* HA : AllHouses)
						{
							if (!HA) continue;
							FVector Origin, Extent;
							HA->GetActorBounds(false, Origin, Extent);
							const float Margin = 200.f;
							bool bInX = Projected.Location.X > Origin.X - Extent.X - Margin && Projected.Location.X < Origin.X + Extent.X + Margin;
							bool bInY = Projected.Location.Y > Origin.Y - Extent.Y - Margin && Projected.Location.Y < Origin.Y + Extent.Y + Margin;
							if (bInX && bInY) { bInsideHouseFb = true; break; }
						}
						if (!bInsideHouseFb)
						{
							LastWanderTarget = Projected.Location;
							bFoundTarget = true;
							break;
						}
					}
				}
				// if fallback fails - keep old target
			}

			if (bFoundTarget)
			{
				TimeSinceWanderStart = 0.f;
				FVector NewDir = (LastWanderTarget - CurrentLoc).GetSafeNormal2D();
				if (!NewDir.IsNearlyZero()) WanderAngle = FMath::Atan2(NewDir.Y, NewDir.X);
				if (bDrawAimingDebug) DrawDebugSphere(World, LastWanderTarget, 40.f, 8, FColor::Yellow, false, 3.f);
			}
		}
	}
	TimeSinceWanderStart += DeltaSeconds;

	FVector ToWanderTarget = (LastWanderTarget - Pawn->GetActorLocation());
	ToWanderTarget.Z = 0.f;
	WanderForce = ToWanderTarget.IsNearlyZero() ? FVector(FMath::Cos(WanderAngle), FMath::Sin(WanderAngle), 0.f) : ToWanderTarget.GetSafeNormal();

// FLEE ZOMBIE/PURGE
	FVector FleeForce = FVector::ZeroVector;
	bool bIsInPurge = BBComp->GetValueAsBool(FName("IsInPurgeZone"));

	if (NearestZombie)
	{
		FVector ToZombie = NearestZombie->GetActorLocation() - Pawn->GetActorLocation();
		float ZombieDist = ToZombie.Size2D();

		FleeForce = -ToZombie.GetSafeNormal2D();

		UStudentPerceptorCarvalhoAna* Perceptor = Pawn->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
		bool bIsRunner = BBComp->GetValueAsBool(FName("IsRunnerZombie"));

		if (!bIsRunner && Perceptor && Perceptor->KnownHouses.Num() > 0)
		{
			FVector BestHouse = FVector::ZeroVector;
			float MinDist = 999999.f;
			for (FVector HouseLoc : Perceptor->KnownHouses)
			{
				float D = FVector::Dist(Pawn->GetActorLocation(), HouseLoc);
				if (D < MinDist) { MinDist = D; BestHouse = HouseLoc; }
			}
			if (MinDist < 4000.f)
			{
				FVector SeekHouse = (BestHouse - Pawn->GetActorLocation()).GetSafeNormal2D();
				float FleeW = FMath::Lerp(0.4f, 0.85f, FMath::Clamp(1.f - ZombieDist / 600.f, 0.f, 1.f));
				FleeForce = (FleeForce * FleeW + SeekHouse * (1.f - FleeW)).GetSafeNormal();
			}
		}
	}
	else if (bIsInPurge)
	{
		FVector PurgeLoc = BBComp->GetValueAsVector(FName("PurgeZoneCenter"));
		if (!PurgeLoc.IsNearlyZero())
		{
			FVector ToPurge = PurgeLoc - Pawn->GetActorLocation();
			FleeForce = -ToPurge.GetSafeNormal2D(); // flee directly away from the zone center
		}
		else
		{
			// fallback - flee in the opposite of the current movement direction
			FleeForce = -Pawn->GetActorForwardVector().GetSafeNormal2D();
			if (FleeForce.IsNearlyZero()) FleeForce = FVector(1.f, 0.f, 0.f);
		}
	}

// STUCK DETECTION + 8-RAY ESCAPE
	LastStuckCheckTime += DeltaSeconds;
	float DistanceMoved = FVector::Dist(Pawn->GetActorLocation(), LastReportedPosition);

	if (LastStuckCheckTime >= StuckCheckInterval)
	{
		LastStuckCheckTime = 0.f;

		float EffectiveThreshold = bInHallway ? 15.f : StuckThreshold;
		float EffectiveTimeLimit = bInHallway ? 0.5f : StuckTimeLimit;

		if (DistanceMoved < EffectiveThreshold)
		{
			StuckTimer += StuckCheckInterval;
			if (StuckTimer >= EffectiveTimeLimit)
			{
				bIsStuck = true;

				// cast 8 rays
				float BestClearDist = -1.f;
				float BestAngle = WanderAngle + PI;

				FVector RayStart = Pawn->GetActorLocation();
				FCollisionQueryParams RayParams;
				RayParams.AddIgnoredActor(Pawn);

				for (int32 r = 0; r < 8; ++r)
				{
					float TestAngle = (r / 8.f) * 2.f * PI;
					FVector Dir = FVector(FMath::Cos(TestAngle), FMath::Sin(TestAngle), 0.f);
					FHitResult Hit;
					float ClearDist = 400.f;
					if (World->LineTraceSingleByChannel(Hit, RayStart, RayStart + Dir * 400.f, ECC_WorldStatic, RayParams)) ClearDist = Hit.Distance;
					if (ClearDist > BestClearDist) { BestClearDist = ClearDist; BestAngle = TestAngle; }
				}

				WanderAngle = BestAngle;
				LastWanderTarget = FVector::ZeroVector;
				TimeSinceWanderStart = 999.f;
				
				// stuck = current path is bad
				CurrentPath.Empty();
				PathUpdateTimer = 999.f;

				StuckTimer = 0.f;

				// purple stuck sphere
				PurpleDebugTimer += DeltaSeconds;
				if (bDrawAimingDebug && PurpleDebugTimer >= 0.5f)
				{
					PurpleDebugTimer = 0.f;
					DrawDebugSphere(World, Pawn->GetActorLocation() + FVector(0,0,100), 40.f, 8, FColor::Purple, false, 0.45f);
				}
			}
		}
		else
		{
			StuckTimer = 0.f;
			bIsStuck   = false;
		}
		LastReportedPosition = Pawn->GetActorLocation();
	}

// OBSTACLE AVOIDANCE
	FVector AvoidanceForce = FVector::ZeroVector;
	FVector Start = Pawn->GetActorLocation();
	FVector Forward = Pawn->GetActorForwardVector();

	bool bSeekingItem = (NearestItem != nullptr);
	bool bIsHallway = false;

	struct FWhisker { float Angle; float Distance; };

	const FWhisker WhiskersSmall[] = { {-45.f, 35.f}, {0.f, 50.f}, {45.f, 35.f} };
	const FWhisker WhiskersRegular[] = { {-45.f, 80.f}, {0.f,100.f}, {45.f, 80.f} };
	const FWhisker* WK = bSeekingItem ? WhiskersSmall : WhiskersRegular;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);

	int32 WhiskersHitting = 0;
	for (int32 i = 0; i < 3; ++i)
	{
		FVector Dir = Forward.RotateAngleAxis(WK[i].Angle, FVector::UpVector);
		FVector End = Start + Dir * WK[i].Distance;
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			WhiskersHitting++;
			float Proximity = 1.f - (Hit.Distance / WK[i].Distance);
			AvoidanceForce += Hit.ImpactNormal * Proximity * 3.f; // reduced multiplier
			if (WK[i].Angle == 0.f)	 AvoidanceForce += Pawn->GetActorRightVector() * Proximity * 2.f;
			if (bDrawAimingDebug) DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Red,  false, -1.f, 0, 2.f);
		}
		else if (bDrawAimingDebug) DrawDebugLine(World, Start, End, FColor::Green, false, -1.f, 0, 1.f);
	}

	if (WhiskersHitting >= 2 && !bSeekingItem)
	{
		bIsHallway = bInHallway = true;
		AvoidanceForce = Forward * 3.f;

		// yellow hallway sphere
		YellowDebugTimer += DeltaSeconds;
		if (bDrawAimingDebug && YellowDebugTimer >= 0.5f)
		{
			YellowDebugTimer = 0.f;
			DrawDebugSphere(World, Start + FVector(0,0,100), 50.f, 8, FColor::Yellow, false, 0.45f);
		}
	}
	else
	{
		bIsHallway = bInHallway = false;
	}

	if (WhiskersHitting >= 3)
	{
		AvoidanceForce = -Forward * 8.f;
		WanderAngle += PI;
		bIsStuck = true;
		LastWanderTarget = FVector::ZeroVector;
		TimeSinceWanderStart = 999.f;

		OrangeDebugTimer += DeltaSeconds;
		if (bDrawAimingDebug && OrangeDebugTimer >= 0.5f)
		{
			OrangeDebugTimer = 0.f;
			DrawDebugSphere(World, Start + FVector(0,0,120), 60.f, 8, FColor::Orange, false, 0.5f);
		}
	}
	else if (WhiskersHitting == 2)
	{
		AvoidanceForce *= 2.f;
		AvoidanceForce += Pawn->GetActorRightVector() * 2.f;
	}

// HOUSE CONTAINMENT (looting only - skip if fleeing or exiting)
	AActor* NearestHouseForContainment = Cast<AActor>(BBComp->GetValueAsObject("NearestHouse"));
	if (bIsInside && !NearestItem && FleeForce.IsNearlyZero() && NearestHouseForContainment)
	{
		if (AHouse* CH = Cast<AHouse>(NearestHouseForContainment))
		{
			FHouseBounds B = CH->GetBounds();
			FVector PL = Pawn->GetActorLocation();
			float M = 120.f;
			if (PL.X < B.Origin.X - B.Extent.X + M) AvoidanceForce.X += 1.f;
			if (PL.X > B.Origin.X + B.Extent.X - M) AvoidanceForce.X -= 1.f;
			if (PL.Y < B.Origin.Y - B.Extent.Y + M) AvoidanceForce.Y += 1.f;
			if (PL.Y > B.Origin.Y + B.Extent.Y - M) AvoidanceForce.Y -= 1.f;
		}
	}

// BLEND FORCES
	FVector DesiredDirection;

	if (!FleeForce.IsNearlyZero())
	{
		// inside + fleeing: pull toward doorway so the AI exits instead of being pushed deeper in
		if (bIsInside && !DoorwayLoc.IsNearlyZero())
		{
			FVector ToDoor = (DoorwayLoc - Pawn->GetActorLocation()).GetSafeNormal2D();
			DesiredDirection = (FleeForce * 0.5f) + (ToDoor * 0.5f);
		}
		else
		{
			DesiredDirection = (FleeForce * 0.85f) + (WanderForce * 0.15f);
		}
	}
	else if (!SeekForce.IsNearlyZero() && !bIsStuck)
	{
		DesiredDirection = SeekForce;
	}
	else
	{
		DesiredDirection = WanderForce;
	}

	DesiredDirection.Z = 0.f;
	if (!DesiredDirection.IsNearlyZero()) DesiredDirection.Normalize();

	FinalSteeringForce = !AvoidanceForce.IsNearlyZero() ? (DesiredDirection + AvoidanceForce) : DesiredDirection;

	FinalSteeringForce.Z = 0.f;
	if (!FinalSteeringForce.IsNearlyZero()) FinalSteeringForce.Normalize();

// SPRINTING
	if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(Pawn))
	{
		bool bShouldSprint = !FleeForce.IsNearlyZero() && ( BBComp->GetValueAsBool(FName("IsHeavyZombie")) || BBComp->GetValueAsBool(FName("IsRunnerZombie")) || BBComp->GetValueAsBool(FName("IsInPurgeZone")));

		if (UStaminaComponent* SC = Survivor->FindComponentByClass<UStaminaComponent>())if (SC->GetCurrentStamina() <= 0.f) bShouldSprint = false;

		bShouldSprint ? Survivor->StartRunning() : Survivor->StopRunning();
	}

// APPLY MOVEMENT
	Pawn->AddMovementInput(FinalSteeringForce, 1.0f);

	if (!FinalSteeringForce.IsNearlyZero())
	{
		FRotator NewRot = FMath::RInterpTo(Pawn->GetActorRotation(), FinalSteeringForce.Rotation(), DeltaSeconds, 8.f);
		Pawn->SetActorRotation(NewRot);
	}

// DEBUG
	if (bDrawAimingDebug)
	{
		FVector AimStart = Pawn->GetActorLocation() + FVector(0, 0, 80);
		DrawDebugLine(World, AimStart, AimStart + Pawn->GetActorForwardVector() * 200.f, FColor::Cyan, false, -1.f, 0, 3.f);
		if (!FinalSteeringForce.IsNearlyZero()) DrawDebugLine(World, AimStart, AimStart + FinalSteeringForce * 200.f, FColor::Yellow, false, -1.f, 0, 2.f);
		if (bIsStuck)
		{
			// purple stuck indicator
			PurpleDebugTimer += DeltaSeconds; // timer already incremented above when drawing on stuck event
			if (PurpleDebugTimer >= 0.5f)
			{
				PurpleDebugTimer = 0.f;
				DrawDebugSphere(World, AimStart, 35.f, 8, FColor::Purple, false, 0.45f);
			}
		}
		if (NearestZombie) DrawDebugLine(World, AimStart, NearestZombie->GetActorLocation(), FColor::Red, false, -1.f, 0, 1.5f);
		if (!LastWanderTarget.IsZero()) DrawDebugLine(World, Pawn->GetActorLocation(), LastWanderTarget, FColor::White, false, -1.f, 0, 1.f);
	}
}