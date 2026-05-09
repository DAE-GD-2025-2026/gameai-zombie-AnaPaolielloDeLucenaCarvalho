#include "UBTService_UpdateStats.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "AIController.h"
#include "Common/HealthComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"

UBTService_UpdateStats::UBTService_UpdateStats()
{
	NodeName = "Update Survivor Stats";
	bNotifyTick = true;
}

void UBTService_UpdateStats::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// get Blackboard and AI Controller
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!BlackboardComp || !AIController) return;

	// get survivor
	APawn* SurvivorPawn = AIController->GetPawn();
	if (!SurvivorPawn) return;

	// monitor health
	if (UHealthComponent* HealthComp = SurvivorPawn->FindComponentByClass<UHealthComponent>())
	{
		float CurrentHealth = HealthComp->GetHealth(); 
        
		// write to memory
		BlackboardComp->SetValueAsFloat(FName("CurrentHealth"), CurrentHealth);
	}

	// monitor inventory
	if (UInventoryComponent* InventoryComp = SurvivorPawn->FindComponentByClass<UInventoryComponent>())
	{
		bool bHasWeapon = false; // don't have a weapon by default

		// get backpack items
		TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();

		for (ABaseItem* Item : Backpack)
		{
			if (Item && Cast<AWeapon>(Item))
			{
				bHasWeapon = true;
				break;
			}
		}

		// write to memory
		BlackboardComp->SetValueAsBool(FName("HasWeapon"), bHasWeapon);
	}
}