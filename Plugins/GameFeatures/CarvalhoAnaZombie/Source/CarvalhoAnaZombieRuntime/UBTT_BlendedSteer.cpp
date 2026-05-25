#include "UBTT_BlendedSteer.h"
#include "AIController.h"
#include "StudentPerceptor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/StaminaComponent.h"

UUBTT_BlendedSteer::UUBTT_BlendedSteer()
{
	NodeName = "Blended Steering (Lab Math)";
	bNotifyTick = true; // run every frame
}

EBTNodeResult::Type UUBTT_BlendedSteer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UUBTT_BlendedSteer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BBComp) return;

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return;

	UWorld* World = Pawn->GetWorld();
	if (!World) return;

	FVector FinalSteeringForce = FVector::ZeroVector;
	
// PATH FOLLOWING  (exit house door)
	FVector SeekForce = FVector::ZeroVector;

	if (BBComp->GetValueAsBool(FName("IsInsideHouse")))
	{
		FVector TargetLocation = BBComp->GetValueAsVector(FName("TargetLocation"));
		float DistanceToTarget = FVector::Dist2D(Pawn->GetActorLocation(), TargetLocation);

		if (DistanceToTarget < 400.0f)
		{
			BBComp->SetValueAsBool(FName("IsInsideHouse"), false);
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return; 
		}

		if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(Pawn))
		{
			TArray<FVector> Path = Survivor->CalculatePath(TargetLocation);

			if (Path.Num() > 1)
				SeekForce = Path[1] - Pawn->GetActorLocation();
			else
				SeekForce = TargetLocation - Pawn->GetActorLocation();

			SeekForce.Z = 0.0f;
			SeekForce.Normalize();
		}
	}

// WANDER (Lab: SteeringBehaviors.cpp)
	WanderAngle += FMath::RandRange(-WanderJitter, WanderJitter) * DeltaSeconds;

	FVector ForwardVec = Pawn->GetActorForwardVector();
	FVector RightVec   = Pawn->GetActorRightVector();

	FVector CircleCenter = ForwardVec * WanderDistance;

	FVector Displacement = (ForwardVec * FMath::Cos(WanderAngle)) + (RightVec * FMath::Sin(WanderAngle));
	Displacement *= WanderRadius;

	// local wander force
	FVector WanderForce = CircleCenter + Displacement;
	WanderForce.Z = 0.0f;
	WanderForce.Normalize();


// FLEE (Lab: SteeringBehaviors.cpp)
	FVector FleeForce = FVector::ZeroVector;
	AActor* NearestZombie = Cast<AActor>(BBComp->GetValueAsObject("NearestZombie"));

	if (NearestZombie)
	{
		FVector ToZombie    = NearestZombie->GetActorLocation() - Pawn->GetActorLocation();
		float   ZombieDist  = ToZombie.Size2D();

		FleeForce = -ToZombie;
		FleeForce.Z = 0.0f;
		FleeForce.Normalize();

		UStudentPerceptor* Perceptor = Pawn->FindComponentByClass<UStudentPerceptor>();
		bool bIsRunner = BBComp->GetValueAsBool(FName("IsRunnerZombie"));

		// only blend toward a house if it's not a runner and we know one
		if (!bIsRunner && Perceptor && Perceptor->KnownHouses.Num() > 0)
		{
			FVector BestHouse = FVector::ZeroVector;
			float   MinDist   = 999999.0f;

			for (FVector HouseLoc : Perceptor->KnownHouses)
			{
				float Dist = FVector::Dist(Pawn->GetActorLocation(), HouseLoc);
				if (Dist < MinDist) { MinDist = Dist; BestHouse = HouseLoc; }
			}

			if (MinDist < 4000.0f)
			{
				FVector SeekHouseForce = BestHouse - Pawn->GetActorLocation();
				SeekHouseForce.Z = 0.0f;
				SeekHouseForce.Normalize();

				float FleeWeight  = FMath::Lerp(0.4f, 0.85f, FMath::Clamp(1.0f - (ZombieDist / 600.0f), 0.0f, 1.0f));
				float HouseWeight = 1.0f - FleeWeight;

				FleeForce = (FleeForce * FleeWeight) + (SeekHouseForce * HouseWeight);
				FleeForce.Normalize();
			}
		}
	}

// OBSTACLE AVOIDANCE
	FVector AvoidanceForce = FVector::ZeroVector;
	
	FVector Start = Pawn->GetActorLocation();
	FVector Forward = Pawn->GetActorForwardVector();

	struct FWhisker { float Angle; float Distance; };
	TArray<FWhisker> Whiskers = 
	{
		{ -45.0f, 75.0f }, // Left
		{ 0.0f,   100.0f }, // Center 
		{ 45.0f,  75.0f }  // Right
	};

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);

	for (const FWhisker& Whisker : Whiskers)
	{
		FVector WhiskerDir = Forward.RotateAngleAxis(Whisker.Angle, FVector::UpVector);
		FVector End = Start + (WhiskerDir * Whisker.Distance);

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			float ProximityRatio = 1.0f - (Hit.Distance / Whisker.Distance);
			AvoidanceForce += Hit.ImpactNormal * ProximityRatio * 2.5f;
			DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Red,   false, -1.0f, 0, 2.0f); // FIX
		}
		else
		{
			DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, -1.0f, 0, 1.0f);
		}
	}
	
// BLENDED STEERING (Lab: CombinedSteeringBehaviors.cpp)	
	FVector DesiredDirection = FVector::ZeroVector;

	// what is the goal?
	if (!FleeForce.IsNearlyZero()) 
	{
		DesiredDirection = (FleeForce * 0.85f) + (WanderForce * 0.15f);
	}
	else if (!SeekForce.IsNearlyZero())
	{
		// inside house, heading for the door
		DesiredDirection = SeekForce;
	}
	else
	{
		DesiredDirection = WanderForce; // explore
	}

	DesiredDirection.Z = 0.0f;
	DesiredDirection.Normalize();

	// wall sliding
	if (!AvoidanceForce.IsNearlyZero())
	{
		FVector WallNormal     = AvoidanceForce.GetSafeNormal();
		FVector SlideDirection = FVector::VectorPlaneProject(DesiredDirection, WallNormal);

		if (SlideDirection.SizeSquared() < 0.1f)
			SlideDirection = FVector::CrossProduct(FVector::UpVector, WallNormal);

		FinalSteeringForce = SlideDirection + AvoidanceForce;
	}
	else
	{
		FinalSteeringForce = DesiredDirection;
	}

	FinalSteeringForce.Z = 0.0f;
	FinalSteeringForce.Normalize();


// SPRINTING  (stamina-aware)
	if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(Pawn))
	{
		bool bShouldSprint = false;

		// do we need to sprint? only sprint if fleeing
		if (!FleeForce.IsNearlyZero())
		{
			bool bIsHeavy = BBComp->GetValueAsBool(FName("IsHeavyZombie"));
			bool bIsRunner = BBComp->GetValueAsBool(FName("IsRunnerZombie"));
			bool bIsInPurgeZone = BBComp->GetValueAsBool(FName("IsInPurgeZone"));

			if (bIsHeavy || bIsRunner || bIsInPurgeZone)
				bShouldSprint = true;
		}

		if (UStaminaComponent* StaminaComp = Survivor->FindComponentByClass<UStaminaComponent>())
		{
			if (StaminaComp->GetCurrentStamina() <= 0.0f)
				bShouldSprint = false;
		}

		if (bShouldSprint) Survivor->StartRunning();
		else               Survivor->StopRunning();
	}

// APPLY MOVEMENT
	Pawn->AddMovementInput(FinalSteeringForce, 1.0f);

	if (!FinalSteeringForce.IsNearlyZero())
	{
		FRotator CurrentRot = Pawn->GetActorRotation();
		FRotator TargetRot  = FinalSteeringForce.Rotation();
		FRotator SmoothRot  = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, 8.0f);
		Pawn->SetActorRotation(SmoothRot);
	}
}