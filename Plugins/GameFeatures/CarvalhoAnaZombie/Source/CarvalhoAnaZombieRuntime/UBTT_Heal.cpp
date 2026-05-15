#include "UBTT_Heal.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/Medkit.h"

UUBTT_Heal::UUBTT_Heal()
{
	NodeName = "Use Medkit";
}

EBTNodeResult::Type UUBTT_Heal::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComp) return EBTNodeResult::Failed;

	TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();

	for (int i = 0; i < Backpack.Num(); ++i)
	{
		if (Backpack[i] && Cast<AMedkit>(Backpack[i]))
		{
			InventoryComp->UseItem(i);
			//InventoryComp->RemoveItem(i);
			return EBTNodeResult::Succeeded;
		}
	}
	return EBTNodeResult::Failed;
}