#include "UBTT_BlendedSteer.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/PawnMovementComponent.h"

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

	FVector FinalSteeringForce = FVector::ZeroVector;
	
// SEEK BEHAVIOR
	FVector SeekForce = FVector::ZeroVector;

	if (BBComp->GetValueAsBool(FName("IsInsideHouse")))
	{
		FVector TargetLocation = BBComp->GetValueAsVector(FName("TargetLocation"));
		float DistanceToTarget = FVector::Dist2D(Pawn->GetActorLocation(), TargetLocation);

		if (DistanceToTarget < 150.0f)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return; 
		}

		SeekForce = TargetLocation - Pawn->GetActorLocation();
		SeekForce.Z = 0.0f;
		SeekForce.Normalize();
	}

// WANDER (Lab: SteeringBehaviors.cpp)
	WanderAngle += FMath::RandRange(-WanderJitter, WanderJitter) * DeltaSeconds;

	// find center of the circle in front of player
	FVector ForwardVec = Pawn->GetActorForwardVector();
	FVector CircleCenter = ForwardVec * WanderDistance;

	// find displacement point on the circle (Cos/Sin)
	FVector Displacement(FMath::Cos(WanderAngle), FMath::Sin(WanderAngle), 0.0f);
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
		// Agent Position - Target Position
		FleeForce = Pawn->GetActorLocation() - NearestZombie->GetActorLocation();
		FleeForce.Z = 0.0f;
		FleeForce.Normalize();
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
		if (Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			float ProximityRatio = 1.0f - (Hit.Distance / Whisker.Distance);
			AvoidanceForce += Hit.ImpactNormal * ProximityRatio * 2.5f;
			DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Red, false, -1.0f, 0, 2.0f);
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
		DesiredDirection = (FleeForce * 0.8f) + (WanderForce * 0.2f);
	} 
	else if (!SeekForce.IsNearlyZero()) 
	{
		DesiredDirection = SeekForce; // door
	} 
	else 
	{
		DesiredDirection = WanderForce; // explore
	}

	FinalSteeringForce = DesiredDirection + AvoidanceForce;
	
	FinalSteeringForce.Z = 0.0f; // Keep on ground
	FinalSteeringForce.Normalize();

// APPLY TO UNREAL - Now (after feedback) instead of using MoveTo in the BT, we add the steering into pawn
	Pawn->AddMovementInput(FinalSteeringForce, 1.0f);

	// rotate player to face where its going
	if (!FinalSteeringForce.IsNearlyZero())
	{
		FRotator CurrentRot = Pawn->GetActorRotation();
		FRotator TargetRot = FinalSteeringForce.Rotation();
		FRotator SmoothRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, 8.0f);
		Pawn->SetActorRotation(SmoothRot);
	}
}