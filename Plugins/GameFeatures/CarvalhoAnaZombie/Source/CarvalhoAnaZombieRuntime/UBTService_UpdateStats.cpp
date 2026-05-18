#include "UBTService_UpdateStats.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "AIController.h"
#include "Common/HealthComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"
#include "Items/Medkit.h"
#include "Items/Food.h"
#include "Common/StaminaComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Zombies/BaseZombie.h"

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
		// get old health
		float OldHealth = BlackboardComp->GetValueAsFloat(FName("CurrentHealth"));
		float NewHealth = HealthComp->GetHealth(); 
        
		// have we taken damage since the last tick (since DAMAGE SENSE in student perceptor didnt work)
		if (NewHealth < OldHealth && OldHealth > 0.0f)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("OUCH!"));

			// search for all zombies
			TArray<AActor*> FoundZombies;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseZombie::StaticClass(), FoundZombies);

			AActor* ClosestZombie = nullptr;
			float ClosestDistance = 999999.0f;

			// find the one biting
			for (AActor* Zombie : FoundZombies)
			{
				float Distance = FVector::Dist(SurvivorPawn->GetActorLocation(), Zombie->GetActorLocation());
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					ClosestZombie = Zombie;
				}
			}

			// get zombie in memory
			if (ClosestZombie)
			{
				BlackboardComp->SetValueAsObject(FName("NearestZombie"), ClosestZombie);

				// check for Heavy zombie
				if (ClosestZombie->GetName().Contains("Heavy")) 
				{
					BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"), true);
				} else 
				{
					BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"), false);
				}
			}
		}

		// write new health
		BlackboardComp->SetValueAsFloat(FName("CurrentHealth"), NewHealth);
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