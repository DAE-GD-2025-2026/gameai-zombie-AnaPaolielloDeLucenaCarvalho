#include "UUBTT_SaveDoorwayCarvalhoAna.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "UStudentPerceptorCarvalhoAna.h"
#include "Village/House/House.h"

UUBTT_SaveDoorwayCarvalhoAna::UUBTT_SaveDoorwayCarvalhoAna() 
{ 
	NodeName = "Blacklist House"; 
}

EBTNodeResult::Type UUBTT_SaveDoorwayCarvalhoAna::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!BBComp || !AIController) return EBTNodeResult::Failed;
 
	UStudentPerceptorCarvalhoAna* Perceptor = AIController->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
	if (!Perceptor && Pawn) Perceptor = Pawn->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
	AHouse* HouseToEnter = Cast<AHouse>(BBComp->GetValueAsObject(FName("NearestHouse")));
	if (Perceptor && HouseToEnter)
	{
		Perceptor->VisitedHouses.AddUnique(HouseToEnter);
		// clear the key so SCOUT HOUSE decorator stops aborting lower-priority branches
		BBComp->ClearValue(FName("NearestHouse"));
	}
 
	return EBTNodeResult::Succeeded;

}