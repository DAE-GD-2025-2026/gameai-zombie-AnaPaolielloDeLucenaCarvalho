#include "UBTT_CalculateEvade.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UUBTT_CalculateEvade::UUBTT_CalculateEvade()
{
	NodeName = "Calculate Evade Point";
}

// Evade enemies
EBTNodeResult::Type UUBTT_CalculateEvade::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BBComp) return EBTNodeResult::Failed;

	APawn* Pawn = AIController->GetPawn();
	AActor* Zombie = Cast<AActor>(BBComp->GetValueAsObject(FName("NearestZombie")));
	
	if (!Pawn || !Zombie) return EBTNodeResult::Failed;

	float EvadeDistance = 800.f; // how far

	// vector FROM Zombie TO Player
	FVector FleeDirection = Pawn->GetActorLocation() - Zombie->GetActorLocation();
	FleeDirection.Z = 0.f;
	FleeDirection.Normalize();

	// find safe spot
	FVector SafeLocation = Pawn->GetActorLocation() + (FleeDirection * EvadeDistance);

	// save it
	BBComp->SetValueAsVector(FName("TargetLocation"), SafeLocation);

	return EBTNodeResult::Succeeded;
}