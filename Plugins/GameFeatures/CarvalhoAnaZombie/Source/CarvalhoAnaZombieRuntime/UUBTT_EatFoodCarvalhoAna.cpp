#include "UUBTT_EatFoodCarvalhoAna.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/Food.h"

UUBTT_EatFoodCarvalhoAna::UUBTT_EatFoodCarvalhoAna() { NodeName = "Eat Food"; }

EBTNodeResult::Type UUBTT_EatFoodCarvalhoAna::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;
 
	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
	UStaminaComponent*   StaminaComp   = Pawn->FindComponentByClass<UStaminaComponent>();
	if (!InventoryComp) return EBTNodeResult::Failed;
 
	bool bAtedSomething = false;
 
	// loop through all food and keep eating until stamina is full/no more food
	TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();
	for (int i = 0; i < Backpack.Num(); ++i)
	{
		AFood* Food = Cast<AFood>(Backpack[i]);
		if (!Food) continue;
 
		while (Food && Food->GetValue() > 0)
		{
			if (StaminaComp && StaminaComp->GetCurrentStamina() >= StaminaComp->GetMaxStamina())
				break;
 
			InventoryComp->UseItem(i);
			bAtedSomething = true;
 
			if (Food->GetValue() <= 0)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Food empty — discarding."));
				InventoryComp->RemoveItem(i);
				break;
			}
		}
 
		if (StaminaComp && StaminaComp->GetCurrentStamina() >= StaminaComp->GetMaxStamina())
			break;
	}
 
	return bAtedSomething ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}