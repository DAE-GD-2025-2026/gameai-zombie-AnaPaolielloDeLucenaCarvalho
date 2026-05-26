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
	WanderTimer = WanderUpdateInterval;
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

// EXIT HOUSE
	FVector SeekForce = FVector::ZeroVector;

	if (BBComp->GetValueAsBool(FName("IsInsideHouse")))
	{
		FVector Doorway = BBComp->GetValueAsVector(FName("DoorwayLocation"));
		float   DistToDoorway = FVector::Dist2D(Pawn->GetActorLocation(), Doorway);

		if (DistToDoorway < 400.0f)
		{
			BBComp->SetValueAsBool(FName("IsInsideHouse"), false);
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(Pawn))
		{
			TArray<FVector> Path = Survivor->CalculatePath(Doorway);
			SeekForce = (Path.Num() > 1) ? Path[1] - Pawn->GetActorLocation()
			                              : Doorway  - Pawn->GetActorLocation();
			SeekForce.Z = 0.0f;
			SeekForce.Normalize();
		}
	}

// WANDER  (Lab: SteeringBehaviors.cpp)
	WanderTimer += DeltaSeconds;
	if (WanderTimer >= WanderUpdateInterval)
	{
		WanderTimer        = 0.0f;
		WanderTargetAngle  = FMath::RandRange(-PI, PI); // pick a new direction
	}

	// rotate toward angle
	WanderAngle = FMath::FInterpTo(WanderAngle, WanderTargetAngle, DeltaSeconds, 2.5f);

	FVector ForwardVec   = Pawn->GetActorForwardVector();
	FVector RightVec     = Pawn->GetActorRightVector();
	FVector CircleCenter = ForwardVec * WanderDistance;
	FVector Displacement = (RightVec * FMath::Cos(WanderAngle) + ForwardVec * FMath::Sin(WanderAngle)) * WanderRadius;

	FVector WanderForce = (CircleCenter + Displacement);
	WanderForce.Z = 0.0f;
	WanderForce.Normalize();

// FLEE  (Lab: SteeringBehaviors.cpp)
	FVector FleeForce    = FVector::ZeroVector;
	AActor* NearestZombie = Cast<AActor>(BBComp->GetValueAsObject("NearestZombie"));

	if (NearestZombie)
	{
		FVector ToZombie   = NearestZombie->GetActorLocation() - Pawn->GetActorLocation();
		float   ZombieDist = ToZombie.Size2D();

		FleeForce   = -ToZombie;
		FleeForce.Z = 0.0f;
		FleeForce.Normalize();

		UStudentPerceptor* Perceptor = Pawn->FindComponentByClass<UStudentPerceptor>();
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

// OBSTACLE AVOIDANCE  (whisker raycasts)
	FVector AvoidanceForce = FVector::ZeroVector;
	FVector Start          = Pawn->GetActorLocation();
	FVector Forward        = Pawn->GetActorForwardVector();

	struct FWhisker { float Angle; float Distance; };
	const FWhisker Whiskers[] =
	{
		{ -45.0f,  75.0f }, // Left
		{   0.0f, 100.0f }, // Centre
		{  45.0f,  75.0f } // Right
	};

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);

	for (const FWhisker& W : Whiskers)
	{
		FVector Dir = Forward.RotateAngleAxis(W.Angle, FVector::UpVector);
		FVector End = Start + Dir * W.Distance;

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			float Proximity = 1.0f - Hit.Distance / W.Distance;
			AvoidanceForce += Hit.ImpactNormal * Proximity * 2.5f;
			DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Red, false, -1.0f, 0, 2.0f);
		}
		else
		{
			DrawDebugLine(World, Start, End, FColor::Green, false, -1.0f, 0, 1.0f);
		}
	}

// BLEND FORCES  (Lab: CombinedSteeringBehaviors.cpp)
	FVector DesiredDirection;

	if (!FleeForce.IsNearlyZero())
		DesiredDirection = (FleeForce * 0.85f) + (WanderForce * 0.15f); // flee with slight wander so it doesn't run in a straight line
	else if (!SeekForce.IsNearlyZero())
		DesiredDirection = SeekForce; // heading for doorway
	else
		DesiredDirection = WanderForce; // free explore

	DesiredDirection.Z = 0.0f;
	DesiredDirection.Normalize();

	if (!AvoidanceForce.IsNearlyZero())
	{
		FVector WallNormal     = AvoidanceForce.GetSafeNormal();
		FVector SlideDirection = FVector::VectorPlaneProject(DesiredDirection, WallNormal);

		if (SlideDirection.SizeSquared() < 0.1f)
			SlideDirection = FVector::CrossProduct(FVector::UpVector, WallNormal);

		FinalSteeringForce = (SlideDirection + AvoidanceForce);
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
}