#include "UBTT_PickupItem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

UUBTT_PickupItem::UUBTT_PickupItem()
{
	NodeName = "Pickup Nearest Item";
}

EBTNodeResult::Type UUBTT_PickupItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// get brain and blackboard
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BlackboardComp) return EBTNodeResult::Failed;

	// get survivor and inventory
	APawn* SurvivorPawn = AIController->GetPawn();
	if (!SurvivorPawn) return EBTNodeResult::Failed;

	UInventoryComponent* InventoryComp = SurvivorPawn->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComp) return EBTNodeResult::Failed;

	// get item
	UObject* ItemObject = BlackboardComp->GetValueAsObject(FName("NearestItem"));
	ABaseItem* ItemToPickup = Cast<ABaseItem>(ItemObject);

	if (ItemToPickup)
	{
		// add to backpack - forcing to slot 0 for now (CHANGE THIS)
		bool bSuccess = InventoryComp->GrabItem(0, ItemToPickup);

		if (bSuccess)
		{
			// clear memory
			BlackboardComp->ClearValue(FName("NearestItem"));
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}