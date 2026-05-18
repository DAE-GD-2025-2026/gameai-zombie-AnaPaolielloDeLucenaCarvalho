#include "UBTT_ShootWeapon.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"

UUBTT_ShootWeapon::UUBTT_ShootWeapon()
{
	NodeName = "Shoot Weapon";
}

EBTNodeResult::Type UUBTT_ShootWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComp) return EBTNodeResult::Failed;

	// open backpack
	TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();

	// look for weapon
	for (int i = 0; i < Backpack.Num(); ++i)
	{
		if (Backpack[i] && Cast<AWeapon>(Backpack[i]))
		{
			InventoryComp->UseItem(i); // pull trigger
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BANG BANG!"));
			
			return EBTNodeResult::Succeeded;
		}
	}
	return EBTNodeResult::Failed;
}