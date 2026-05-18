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
		// if garbage, destroy to clean world and free memory
		if (ItemToPickup->GetItemType() == EItemType::Garbage)
		{
			// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Stomped on Garbage!"));
			ItemToPickup->Destroy();
			
			return EBTNodeResult::Succeeded;
		}

		bool bSuccess = false;
        
		// open the backpack
		TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();

		// check each slot
		for (int i = 0; i < Backpack.Num(); ++i) 
		{
			// grab item if slot is empty
			if (Backpack[i] == nullptr)
			{
				if (InventoryComp->GrabItem(i, ItemToPickup))
				{
					// hide item in game
					ItemToPickup->SetActorHiddenInGame(true);
					ItemToPickup->SetActorEnableCollision(false);

					bSuccess = true;
					break;
				}
			}
		}

		if (bSuccess)
		{
			// clear memory
			BlackboardComp->ClearValue(FName("NearestItem"));
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}