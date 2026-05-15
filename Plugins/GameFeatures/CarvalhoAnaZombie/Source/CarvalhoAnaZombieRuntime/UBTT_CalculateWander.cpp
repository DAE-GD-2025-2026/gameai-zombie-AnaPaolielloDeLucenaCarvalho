#include "UBTT_CalculateWander.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UUBTT_CalculateWander::UUBTT_CalculateWander()
{
	NodeName = "Calculate Wander Point";
}

EBTNodeResult::Type UUBTT_CalculateWander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	float WanderOffset = 500.f; // distance in front of the agent
	float WanderRadius = 300.f; // size of radius

	// center of circle in front of agent
	FVector ForwardVec = Pawn->GetActorForwardVector();
	FVector CircleCenter = Pawn->GetActorLocation() + (ForwardVec * WanderOffset);

	// random angle
	float RandomAngle = FMath::RandRange(0.f, 360.f);
	
	// get a point
	FVector TargetPoint;
	TargetPoint.X = CircleCenter.X + (FMath::Cos(RandomAngle) * WanderRadius);
	TargetPoint.Y = CircleCenter.Y + (FMath::Sin(RandomAngle) * WanderRadius);
	TargetPoint.Z = Pawn->GetActorLocation().Z;

	// save it
	OwnerComp.GetBlackboardComponent()->SetValueAsVector(FName("TargetLocation"), TargetPoint);

	return EBTNodeResult::Succeeded;
}