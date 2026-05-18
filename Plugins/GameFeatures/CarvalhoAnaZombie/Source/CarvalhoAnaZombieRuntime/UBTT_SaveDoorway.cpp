#include "UBTT_SaveDoorway.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "StudentPerceptor.h"
#include "Village/House/House.h"

UUBTT_SaveDoorway::UUBTT_SaveDoorway() 
{ 
	NodeName = "Blacklist House"; 
}

EBTNodeResult::Type UUBTT_SaveDoorway::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !BBComp) return EBTNodeResult::Failed;

	// Blacklist the house
	UStudentPerceptor* Perceptor = Pawn->FindComponentByClass<UStudentPerceptor>();
	UObject* HouseObj = BBComp->GetValueAsObject(FName("NearestHouse"));
	if (Perceptor && HouseObj) {
		Perceptor->VisitedHouses.AddUnique(Cast<AHouse>(HouseObj));
	}

	BBComp->SetValueAsVector(FName("TargetLocation"), Pawn->GetActorLocation());
	BBComp->SetValueAsBool(FName("IsInsideHouse"), true);

	return EBTNodeResult::Succeeded;
}