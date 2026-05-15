#include "UBTT_ClearMemory.h"
#include "BehaviorTree/BlackboardComponent.h"

UUBTT_ClearMemory::UUBTT_ClearMemory()
{
	NodeName = "Clear Memory (Dynamic)";
}

EBTNodeResult::Type UUBTT_ClearMemory::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp) return EBTNodeResult::Failed;

	BBComp->ClearValue(KeyToClear.SelectedKeyName);

	return EBTNodeResult::Succeeded;
}