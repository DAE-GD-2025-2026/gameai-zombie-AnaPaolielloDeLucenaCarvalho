#include "UUBTT_BlendedSteerCarvalhoAna.h"
#include "AIController.h"
#include "UStudentPerceptorCarvalhoAna.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/StaminaComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Village/House/House.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshPath.h"

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
		LastPosition = AIController->GetPawn()->GetActorLocation();
		bIsStuck = false;
		StuckTimer = 0.0f;
	}
	
	return EBTNodeResult::InProgress;
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

// SEEK ITEM
	AActor* NearestItem = Cast<AActor>(BBComp->GetValueAsObject("NearestItem"));
	if (NearestItem)
	{
		FVector ToItem = NearestItem->GetActorLocation() - Pawn->GetActorLocation();
    
		// close enough to item = pickup
		if (ToItem.Size2D() < 100.0f) 
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		SeekForce = ToItem;
		SeekForce.Z = 0.0f;
		SeekForce.Normalize();
	}
	
// SEEK HOUSE
	else if (!BBComp->GetValueAsBool(FName("IsInsideHouse")))
	{
		AActor* NearestHouse = Cast<AActor>(BBComp->GetValueAsObject("NearestHouse"));
		if (NearestHouse)
		{
			FVector ToHouse = NearestHouse->GetActorLocation() - Pawn->GetActorLocation();
			float DistToHouse = ToHouse.Size2D();
        
			// close enough to house = enter it
			if (DistToHouse < 150.0f) 
			{
				BBComp->SetValueAsBool(FName("IsInsideHouse"), true);
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}
        
			SeekForce = ToHouse;
			SeekForce.Z = 0.0f;
			SeekForce.Normalize();
		}
	}
	
// SEEK DOORWAY
	bool bIsInside = BBComp->GetValueAsBool(FName("IsInsideHouse"));
	FVector DoorwayLoc = BBComp->GetValueAsVector(FName("DoorwayLocation"));
	AActor* NearestZombie = Cast<AActor>(BBComp->GetValueAsObject("NearestZombie"));
	AActor* CurrentHouseActor = Cast<AActor>(BBComp->GetValueAsObject("NearestHouse"));

	if (bIsInside && !NearestItem) 
	{
		FVector ToDoor = DoorwayLoc - Pawn->GetActorLocation();
		float DistToDoor = ToDoor.Size2D();
        
		// check if outside the house bounds
		bool bActuallyOutside = false;
		if (CurrentHouseActor)
		{
			AHouse* House = Cast<AHouse>(CurrentHouseActor);
			if (House)
			{
				FHouseBounds Bounds = House->GetBounds();
				FVector PawnLoc = Pawn->GetActorLocation();
				
				float Margin = 150.0f;
				bool bOutsideX = (PawnLoc.X < Bounds.Origin.X - Bounds.Extent.X - Margin) || 
				                 (PawnLoc.X > Bounds.Origin.X + Bounds.Extent.X + Margin);
				bool bOutsideY = (PawnLoc.Y < Bounds.Origin.Y - Bounds.Extent.Y - Margin) || 
				                 (PawnLoc.Y > Bounds.Origin.Y + Bounds.Extent.Y + Margin);
				
				bActuallyOutside = bOutsideX || bOutsideY;
			}
		}
		
		// outside only if verified OR far from door
		if (bActuallyOutside || DistToDoor > HouseExitThreshold)
		{
			BBComp->SetValueAsBool(FName("IsInsideHouse"), false);
			bIsInside = false;
			
			// Debug - Show exit point
			if (bDrawAimingDebug)
			{
				DrawDebugSphere(World, Pawn->GetActorLocation(), 100.0f, 16, FColor::Green, false, 1.0f);
			}
		}
		
		if (bIsInside)  // still inside = seek the door
		{
			if (DistToDoor < 50.0f)  // close to door = exit succeeded
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}

			SeekForce = ToDoor;
			SeekForce.Z = 0.0f;
			SeekForce.Normalize();
		}
	}

// WANDER
	FVector WanderForce = FVector::ZeroVector;
	
	//  reached current wander target
	float DistToWanderTarget = FVector::Dist(Pawn->GetActorLocation(), LastWanderTarget);
	if (DistToWanderTarget < WanderTargetReachedDistance || TimeSinceWanderStart > 10.0f)
	{
		// get new location on navmesh
		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
		{
			FVector CurrentLoc = Pawn->GetActorLocation();
			FVector RandomOffset = FVector(
				FMath::RandRange(-2000.0f, 2000.0f),
				FMath::RandRange(-2000.0f, 2000.0f),
				0.0f
			);
			FVector CandidateLocation = CurrentLoc + RandomOffset;
			
			FNavLocation ProjectedLocation;
			if (NavSys->ProjectPointToNavigation(CandidateLocation, ProjectedLocation))
			{
				LastWanderTarget = ProjectedLocation.Location;
				TimeSinceWanderStart = 0.0f;
				WanderAngle = FMath::RandRange(-PI, PI);
			}
		}
	}
	
	TimeSinceWanderStart += DeltaSeconds;
	
	// wander force toward target
	FVector ToWanderTarget = (LastWanderTarget - Pawn->GetActorLocation());
	ToWanderTarget.Z = 0.0f;
	
	if (!ToWanderTarget.IsNearlyZero())
	{
		ToWanderTarget.Normalize();
		
		WanderAngle += FMath::RandRange(-WanderJitter * 0.3f, WanderJitter * 0.3f) * DeltaSeconds;
		
		FVector AngleVariation = FVector(FMath::Cos(WanderAngle), FMath::Sin(WanderAngle), 0.0f);
		WanderForce = (ToWanderTarget * 0.85f + AngleVariation * 0.15f);
		WanderForce.Normalize();
	}
	else
	{
		// Fallback - small circle if target not set
		WanderAngle += FMath::RandRange(-WanderJitter * 0.2f, WanderJitter * 0.2f) * DeltaSeconds;
		WanderForce = FVector(FMath::Cos(WanderAngle), FMath::Sin(WanderAngle), 0.0f);
	}

// FLEE  (Lab: SteeringBehaviors.cpp)
	FVector FleeForce    = FVector::ZeroVector;

	if (NearestZombie)
	{
		FVector ToZombie   = NearestZombie->GetActorLocation() - Pawn->GetActorLocation();
		float   ZombieDist = ToZombie.Size2D();

		FleeForce   = -ToZombie;
		FleeForce.Z = 0.0f;
		FleeForce.Normalize();

		UStudentPerceptorCarvalhoAna* Perceptor = Pawn->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
		bool bIsRunner = BBComp->GetValueAsBool(FName("IsRunnerZombie"));

		// not a runner zombie - flee + toward the nearest known house
		if (!bIsRunner && Perceptor && Perceptor->KnownHouses.Num() > 0)
		{
			FVector BestHouse = FVector::ZeroVector;
			float   MinDist   = 999999.0f;
			for (FVector HouseLoc : Perceptor->KnownHouses)
			{
				float D = FVector::Dist(Pawn->GetActorLocation(), HouseLoc);
				if (D < MinDist) { MinDist = D; BestHouse = HouseLoc; }
			}

			if (MinDist < 4000.0f)
			{
				FVector SeekHouse = (BestHouse - Pawn->GetActorLocation()).GetSafeNormal2D();

				float FleeW = FMath::Lerp(0.4f, 0.85f, FMath::Clamp(1.0f - ZombieDist / 600.0f, 0.0f, 1.0f));
				FleeForce = (FleeForce * FleeW + SeekHouse * (1.0f - FleeW)).GetSafeNormal();
			}
		}
	}

// STUCK DETECTION
	LastStuckCheckTime += DeltaSeconds;
	float DistanceMoved = FVector::Dist(Pawn->GetActorLocation(), LastReportedPosition);
	
	if (LastStuckCheckTime >= StuckCheckInterval)
	{
		LastStuckCheckTime = 0.0f;
		
		// hallway?
		float EffectiveThreshold = bInHallway ? 15.0f : StuckThreshold;
		float EffectiveTimeLimit = bInHallway ? 0.5f : StuckTimeLimit;
		
		if (DistanceMoved < EffectiveThreshold)
		{
			StuckTimer += StuckCheckInterval;
			if (StuckTimer >= EffectiveTimeLimit)
			{
				bIsStuck = true;
				
				// in hallway = forward + rotate heavily
				if (bInHallway)
				{
					float EscapeRotation = FMath::RandRange(PI * 0.3f, PI * 1.2f);
					WanderAngle += EscapeRotation;
				}
				else
				{
					// open = rotate 90-180 degrees
					float EscapeRotation = FMath::RandRange(PI * 0.5f, PI);
					WanderAngle += EscapeRotation + FMath::RandRange(-0.3f, 0.3f);
				}
				
				StuckTimer = 0.0f;
				
				// Debug - Show escape attempt
				if (bDrawAimingDebug)
				{
					FVector DebugPos = Pawn->GetActorLocation() + FVector(0, 0, 100);
					FColor DebugColor = bInHallway ? FColor::Magenta : FColor::Purple;
					DrawDebugSphere(World, DebugPos, 50.0f, 16, DebugColor, false, 0.5f);
				}
			}
		}
		else
		{
			StuckTimer = 0.0f;
			bIsStuck = false;
		}
		
		LastReportedPosition = Pawn->GetActorLocation();
	}
	
	// track for corner escaping
	LastPosition = Pawn->GetActorLocation();

// OBSTACLE AVOIDANCE 
	FVector AvoidanceForce = FVector::ZeroVector;
	FVector Start = Pawn->GetActorLocation();
	FVector Forward = Pawn->GetActorForwardVector();

	bool bSeekingItem = (NearestItem != nullptr);
	bool bIsHallway = false;
	
	struct FWhisker { float Angle; float Distance; };
	
	const FWhisker WhiskersSmall[] =
	{
		{ -45.0f,  35.0f }, // Left (was 75cm)
		{   0.0f,  50.0f }, // Centre (was 100cm)
		{  45.0f,  35.0f }  // Right (was 75cm)
	};
	
	// Regular whiskers for general movement
	const FWhisker WhiskersRegular[] =
	{
		{ -45.0f,  50.0f }, // Left
		{   0.0f,  70.0f }, // Centre
		{  45.0f,  50.0f }  // Right
	};

	const FWhisker* WhiskersToUse = bSeekingItem ? WhiskersSmall : WhiskersRegular;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);

	int32 WhiskersHitting = 0;
	
	for (int32 i = 0; i < 3; ++i)
	{
		const FWhisker& W = WhiskersToUse[i];
		FVector Dir = Forward.RotateAngleAxis(W.Angle, FVector::UpVector);
		FVector End = Start + Dir * W.Distance;

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			WhiskersHitting++;
			float Proximity = 1.0f - (Hit.Distance / W.Distance);
			
			AvoidanceForce += Hit.ImpactNormal * Proximity * 5.0f;

			if (W.Angle == 0.0f)
			{
				AvoidanceForce += Pawn->GetActorRightVector() * Proximity * 4.0f;
			}

			DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Red, false, -1.0f, 0, 2.0f);
		}
		else
		{
			DrawDebugLine(World, Start, End, FColor::Green, false, -1.0f, 0, 1.0f);
		}
	}
	
	// HALLWAY DETECTION
	if (WhiskersHitting >= 2 && !bSeekingItem)
	{
		bIsHallway = true;
		HallwayEscapeTimer += DeltaSeconds;
		
		AvoidanceForce = Forward * 3.0f;
		
		if (bDrawAimingDebug)
		{
			DrawDebugSphere(World, Start + FVector(0, 0, 100), 80.0f, 12, FColor::Yellow, false, -1.0f);
		}
	}
	else
	{
		bIsHallway = false;
		HallwayEscapeTimer = 0.0f;
	}
	
	// all whiskers hit = trapped
	if (WhiskersHitting >= 3)
	{
		AvoidanceForce = -Forward * 8.0f;
		WanderAngle += PI;		
		//WanderAngle += FMath::RandRange(-PI * 0.9f, PI * 0.9f);
		bIsStuck = true;
		
		if (bDrawAimingDebug)
		{
			DrawDebugSphere(World, Start + FVector(0, 0, 120), 100.0f, 16, FColor::Orange, false, 0.5f);
		}
	}
	else if (WhiskersHitting == 2)
	{
		AvoidanceForce *= 2.0f;
		AvoidanceForce += Pawn->GetActorRightVector() * 3.0f; 
		WanderAngle += FMath::RandRange(PI * 0.4f, PI * 0.8f);
		//WanderAngle += FMath::RandRange(-PI * 0.4f, PI * 0.4f);
	}

// HOUSE CONTAINMENT
	bool bAvoidWalls = true;
	
	if (bIsInside && !NearestItem)
	{
		bAvoidWalls = false;
	}

	if (bIsInside && bAvoidWalls)
	{
		AHouse* CurrentHouse = Cast<AHouse>(BBComp->GetValueAsObject("NearestHouse"));
		if (CurrentHouse)
		{
			FHouseBounds Bounds = CurrentHouse->GetBounds();
			FVector PawnLoc = Pawn->GetActorLocation();

			// get walls
			float MinX = Bounds.Origin.X - Bounds.Extent.X;
			float MaxX = Bounds.Origin.X + Bounds.Extent.X;
			float MinY = Bounds.Origin.Y - Bounds.Extent.Y;
			float MaxY = Bounds.Origin.Y + Bounds.Extent.Y;

			float Margin = 120.0f;

			// force if too close
			if (PawnLoc.X < MinX + Margin) AvoidanceForce.X += 1.0f;
			if (PawnLoc.X > MaxX - Margin) AvoidanceForce.X -= 1.0f;
			if (PawnLoc.Y < MinY + Margin) AvoidanceForce.Y += 1.0f;
			if (PawnLoc.Y > MaxY - Margin) AvoidanceForce.Y -= 1.0f;
		}
	}
	
// BLEND FORCES  (Lab: CombinedSteeringBehaviors.cpp)
	FVector DesiredDirection;

	if (!FleeForce.IsNearlyZero())
	{
		DesiredDirection = (FleeForce * 0.85f) + (WanderForce * 0.15f); 
	}
	else if (!SeekForce.IsNearlyZero() && !bIsStuck)
	{
		DesiredDirection = SeekForce; 
	}
	else
	{
		DesiredDirection = WanderForce; 
	}

	DesiredDirection.Z = 0.0f;
	DesiredDirection.Normalize();

	if (!AvoidanceForce.IsNearlyZero())
	{
		FinalSteeringForce = DesiredDirection + AvoidanceForce;
	}
	else
	{
		FinalSteeringForce = DesiredDirection;
	}

	FinalSteeringForce.Z = 0.0f;
	FinalSteeringForce.Normalize();

// SPRINTING  (only when fleeing + stamina available)
	if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(Pawn))
	{
		bool bShouldSprint = false;

		if (!FleeForce.IsNearlyZero())
		{
			bool bIsHeavy = BBComp->GetValueAsBool(FName("IsHeavyZombie"));
			bool bIsRunner = BBComp->GetValueAsBool(FName("IsRunnerZombie"));
			bool bIsInPurge = BBComp->GetValueAsBool(FName("IsInPurgeZone"));
			bShouldSprint = bIsHeavy || bIsRunner || bIsInPurge;
		}

		if (UStaminaComponent* StaminaComp = Survivor->FindComponentByClass<UStaminaComponent>())
		{
			if (StaminaComp->GetCurrentStamina() <= 0.0f)
				bShouldSprint = false;
		}

		bShouldSprint ? Survivor->StartRunning() : Survivor->StopRunning();
	}

// APPLY MOVEMENT
	Pawn->AddMovementInput(FinalSteeringForce, 1.0f);

	if (!FinalSteeringForce.IsNearlyZero())
	{
		FRotator NewRot = FMath::RInterpTo(Pawn->GetActorRotation(),
		                                   FinalSteeringForce.Rotation(),
		                                   DeltaSeconds, 8.0f);
		Pawn->SetActorRotation(NewRot);
	}

// DEBUG VISUALIZATION
	if (bDrawAimingDebug)
	{
		// Draw aiming direction (forward vector)
		FVector AimStart = Pawn->GetActorLocation() + FVector(0, 0, 80);
		FVector AimEnd = AimStart + (Pawn->GetActorForwardVector() * 200.0f);
		DrawDebugLine(World, AimStart, AimEnd, FColor::Cyan, false, -1.0f, 0, 3.0f);
		
		// Draw steering force direction (in yellow)
		if (!FinalSteeringForce.IsNearlyZero())
		{
			FVector SteerEnd = AimStart + (FinalSteeringForce * 200.0f);
			DrawDebugLine(World, AimStart, SteerEnd, FColor::Yellow, false, -1.0f, 0, 2.0f);
		}
		
		// Draw "stuck" indicator (purple sphere)
		if (bIsStuck)
		{
			DrawDebugSphere(World, AimStart, 50.0f, 12, FColor::Purple, false, -1.0f);
		}
		
		// Draw nearest threat (zombie) in red
		if (NearestZombie)
		{
			FVector ThreatPos = NearestZombie->GetActorLocation();
			DrawDebugLine(World, AimStart, ThreatPos, FColor::Red, false, -1.0f, 0, 1.5f);
		}
	}
}