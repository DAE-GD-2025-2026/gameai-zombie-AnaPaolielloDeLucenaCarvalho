#include "UBTT_ClearMemory.h"
#include "BehaviorTree/BlackboardComponent.h"

UUBTT_ClearMemory::UUBTT_ClearMemory()
{
	NodeName = "Clear Memory (Reset)";
}

EBTNodeResult::Type UUBTT_ClearMemory::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp) return EBTNodeResult::Failed;

	// delete memory so player can restart logic
	BBComp->ClearValue(FName("NearestHouse"));
	BBComp->ClearValue(FName("NearestZombie"));

	return EBTNodeResult::Succeeded;
}