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


// BLENDED STEERING (Lab: CombinedSteeringBehaviors.cpp)
	if (!FleeForce.IsNearlyZero())
	{
		// if being chased (Flee = 80%, Wander = 20%)
		FinalSteeringForce = (FleeForce * 0.8f) + (WanderForce * 0.2f);
	}
	else
	{
		// if safe (Wander = 100%)
		FinalSteeringForce = WanderForce;
	}

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