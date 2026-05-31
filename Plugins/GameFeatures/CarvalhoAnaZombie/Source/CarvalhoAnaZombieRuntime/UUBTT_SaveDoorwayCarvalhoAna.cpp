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
	APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!BBComp || !Pawn) return EBTNodeResult::Failed;
 
	UStudentPerceptorCarvalhoAna* Perceptor = Pawn->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
	AHouse* HouseToEnter = Cast<AHouse>(BBComp->GetValueAsObject(FName("NearestHouse")));
	if (Perceptor && HouseToEnter)
	{
		Perceptor->VisitedHouses.AddUnique(HouseToEnter);
	}
 
	return EBTNodeResult::Succeeded;

}