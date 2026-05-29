#include "UBTT_Heal.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/Medkit.h"

UUBTT_HealCarvalhoAna::UUBTT_HealCarvalhoAna()
{
	NodeName = "Use Medkit";
}

EBTNodeResult::Type UUBTT_HealCarvalhoAna::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;
 
	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
	UHealthComponent*    HealthComp    = Pawn->FindComponentByClass<UHealthComponent>();
	if (!InventoryComp) return EBTNodeResult::Failed;
 
	bool bHealed = false;
 
	// loop through all medkits to keep healing until health is full/no more medkit
	TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();
	for (int i = 0; i < Backpack.Num(); ++i)
	{
		AMedkit* Medkit = Cast<AMedkit>(Backpack[i]);
		if (!Medkit) continue;
 
		while (Medkit && Medkit->GetValue() > 0)
		{
			if (HealthComp && HealthComp->GetHealth() >= HealthComp->GetMaxHealth())
				break;
 
			InventoryComp->UseItem(i);
			bHealed = true;
 
			if (Medkit->GetValue() <= 0)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Medkit empty — discarding."));
				InventoryComp->RemoveItem(i);
				break;
			}
		}
 
		if (HealthComp && HealthComp->GetHealth() >= HealthComp->GetMaxHealth())
			break;
	}
 
	return bHealed ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}