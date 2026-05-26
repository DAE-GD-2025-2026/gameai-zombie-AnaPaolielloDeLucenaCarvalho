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
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!BBComp || !Pawn) return EBTNodeResult::Failed;
 
	UStudentPerceptor* Perceptor    = Pawn->FindComponentByClass<UStudentPerceptor>();
	AHouse*            HouseToEnter = Cast<AHouse>(BBComp->GetValueAsObject(FName("NearestHouse")));
	if (Perceptor && HouseToEnter)
		Perceptor->VisitedHouses.AddUnique(HouseToEnter);
 
	BBComp->SetValueAsVector(FName("DoorwayLocation"), Pawn->GetActorLocation());
	BBComp->SetValueAsBool(FName("IsInsideHouse"), true);
 
	return EBTNodeResult::Succeeded;
}