#include "UBTService_UpdateStats.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "AIController.h"
#include "Common/HealthComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"
#include "Items/Medkit.h"
#include "Items/Food.h"
#include "Common/StaminaComponent.h"

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
	
	// Monitor Stamina
	if (UStaminaComponent* StaminaComp = SurvivorPawn->FindComponentByClass<UStaminaComponent>())
	{
		float CurrentStamina = StaminaComp->GetCurrentStamina();
		BlackboardComp->SetValueAsFloat(FName("CurrentStamina"), CurrentStamina);
	}

	// monitor inventory
	if (UInventoryComponent* InventoryComp = SurvivorPawn->FindComponentByClass<UInventoryComponent>())
	{
		bool bHasWeapon = false;
		bool bHasMedkit = false;
		bool bHasFood = false;

		TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();
		
		for (ABaseItem* Item : Backpack)
		{
			if (Item && Cast<AWeapon>(Item)) bHasWeapon = true;
			if (Item && Cast<AMedkit>(Item)) bHasMedkit = true;
			if (Item && Cast<AFood>(Item)) bHasFood = true;
		}

		BlackboardComp->SetValueAsBool(FName("HasWeapon"), bHasWeapon);
		BlackboardComp->SetValueAsBool(FName("HasMedkit"), bHasMedkit);
		BlackboardComp->SetValueAsBool(FName("HasFood"), bHasFood);
	}
}