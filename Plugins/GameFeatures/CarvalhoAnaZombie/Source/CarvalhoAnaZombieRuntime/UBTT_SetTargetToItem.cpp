#include "UBTT_SetTargetToItem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UUBTT_SetTargetToItem::UUBTT_SetTargetToItem() { NodeName = "Set Target To Nearest Item"; }

EBTNodeResult::Type UUBTT_SetTargetToItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	AActor* Item = Cast<AActor>(BB->GetValueAsObject(FName("NearestItem")));
    
	if (Item)
	{
		BB->SetValueAsVector(FName("TargetLocation"), Item->GetActorLocation());
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}