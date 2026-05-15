#include "UBTT_EatFood.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/Food.h"

UUBTT_EatFood::UUBTT_EatFood() { NodeName = "Eat Food"; }

EBTNodeResult::Type UUBTT_EatFood::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComp) return EBTNodeResult::Failed;

	TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();
	for (int i = 0; i < Backpack.Num(); ++i)
	{
		if (Backpack[i] && Cast<AFood>(Backpack[i]))
		{
			InventoryComp->UseItem(i);
			//InventoryComp->RemoveItem(i); // eat and throw away
			return EBTNodeResult::Succeeded;
		}
	}
	return EBTNodeResult::Failed;
}