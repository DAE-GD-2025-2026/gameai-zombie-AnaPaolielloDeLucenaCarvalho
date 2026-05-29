#include "UBTT_ClearMemory.h"
#include "BehaviorTree/BlackboardComponent.h"

UUBTT_ClearMemoryCarvalhoAna::UUBTT_ClearMemoryCarvalhoAna()
{
	NodeName = "Clear Memory (Dynamic)";
}

EBTNodeResult::Type UUBTT_ClearMemoryCarvalhoAna::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp) return EBTNodeResult::Failed;

	BBComp->ClearValue(KeyToClear.SelectedKeyName);

	return EBTNodeResult::Succeeded;
}