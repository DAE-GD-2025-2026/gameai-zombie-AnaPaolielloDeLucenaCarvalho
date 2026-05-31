#include "UBTService_UpdateStatsCarvalhoAna.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "AIController.h"
#include "UStudentPerceptorCarvalhoAna.h"
#include "Common/HealthComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"
#include "Items/Medkit.h"
#include "Items/Food.h"
#include "Common/StaminaComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Zombies/BaseZombie.h"
#include "Village/House/House.h"

UBTService_UpdateStatsCarvalhoAna::UBTService_UpdateStatsCarvalhoAna()
{
	NodeName = "Update Survivor Stats";
	bNotifyTick = true;
}

void UBTService_UpdateStatsCarvalhoAna::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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
	float NewHealth = 0.f;
	float NewStamina = 0.f;

	if (UHealthComponent* HealthComp = SurvivorPawn->FindComponentByClass<UHealthComponent>())
	{
		// get old health
		float OldHealth = BlackboardComp->GetValueAsFloat(FName("CurrentHealth"));
		NewHealth = HealthComp->GetHealth(); 
        
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
		NewStamina = StaminaComp->GetCurrentStamina();
		BlackboardComp->SetValueAsFloat(FName("CurrentStamina"), NewStamina);
	}

	// monitor inventory
	bool bHasWeapon = false;
	bool bHasMedkit = false;
	bool bHasFood   = false;

	if (UInventoryComponent* InventoryComp = SurvivorPawn->FindComponentByClass<UInventoryComponent>())
	{
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
	
	// memory retrival
	UStudentPerceptorCarvalhoAna* Perceptor = SurvivorPawn->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
	UObject* CurrentTargetItem  = BlackboardComp->GetValueAsObject(FName("NearestItem"));
	UObject* CurrentTargetHouse = BlackboardComp->GetValueAsObject(FName("NearestHouse"));

	if (Perceptor)
	{
		// clean memory
		Perceptor->KnownItems.RemoveAll([](ABaseItem* MemItem) { return !IsValid(MemItem); });

		if (!CurrentTargetItem)
		{
			ABaseItem* BestMemoryItem = nullptr;
			float ClosestDist = 999999.0f;

			for (ABaseItem* MemItem : Perceptor->KnownItems)
			{
				if (!IsValid(MemItem)) continue;
                
				bool bNeedThis = false;
                
				if (Cast<AMedkit>(MemItem) && NewHealth  <= 5.0f && !bHasMedkit) bNeedThis = true;
				if (Cast<AFood>(MemItem)   && NewStamina <= 5.0f && !bHasFood)   bNeedThis = true;
				if (Cast<AWeapon>(MemItem) && !bHasWeapon)                       bNeedThis = true;

				if (bNeedThis)
				{
					float Dist = FVector::Dist(SurvivorPawn->GetActorLocation(), MemItem->GetActorLocation());
					if (Dist < ClosestDist)
					{
						ClosestDist = Dist;
						BestMemoryItem = MemItem;
					}
				}
			}

			if (BestMemoryItem)
			{
				BlackboardComp->SetValueAsObject(FName("NearestItem"), BestMemoryItem);
				GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan,
					FString::Printf(TEXT("[MEMORY ->] I need a %s! Going back to get it!"), *BestMemoryItem->GetName()));

				Perceptor->KnownItems.Remove(BestMemoryItem);
			}
		}

		if (!CurrentTargetHouse)
		{
			FVector BestHouseLoc = FVector::ZeroVector;
			float ClosestDist = 999999.0f;
			bool bFoundHouse = false;

			for (FVector HouseLoc : Perceptor->KnownHouses)
			{
				// skip if this is already a visited house (check by proximity)
				bool bAlreadyVisited = Perceptor->VisitedHouses.ContainsByPredicate([&](AHouse* VH)
				{
					return IsValid(VH) && FVector::Dist2D(VH->GetActorLocation(), HouseLoc) < 200.f;
				});
				if (bAlreadyVisited) continue;

				float Dist = FVector::Dist(SurvivorPawn->GetActorLocation(), HouseLoc);
				if (Dist < ClosestDist)
				{
					ClosestDist = Dist;
					BestHouseLoc = HouseLoc;
					bFoundHouse = true;
				}
			}

			if (bFoundHouse)
			{
				// We can only set a vector key, not find the AHouse* from just a location.
				// Set a dedicated memory key so the BT/BlendedSteer can use it.
				BlackboardComp->SetValueAsVector(FName("MemoryHouseLocation"), BestHouseLoc);
				GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan,
					FString::Printf(TEXT("[MEMORY ->] Going back to known house at (%.0f, %.0f)"),
						BestHouseLoc.X, BestHouseLoc.Y));
			}
			else
			{
				// No unvisited house in memory - clear the key
				BlackboardComp->SetValueAsVector(FName("MemoryHouseLocation"), FAISystem::InvalidLocation);
			}
		}
	}

	// Damians idea - see what state is happening

	// Determine current AI state for display
	bool bHasZombie    = (BlackboardComp->GetValueAsObject(FName("NearestZombie")) != nullptr);
	bool bHasItem      = (BlackboardComp->GetValueAsObject(FName("NearestItem"))   != nullptr);
	bool bHasHouse     = (BlackboardComp->GetValueAsObject(FName("NearestHouse"))  != nullptr);
	bool bIsInPurge    = BlackboardComp->GetValueAsBool(FName("IsInPurgeZone"));
	bool bIsInHouse    = BlackboardComp->GetValueAsBool(FName("IsInsideHouse"));
	bool bIsHeavy      = BlackboardComp->GetValueAsBool(FName("IsHeavyZombie"));
	bool bIsRunner     = BlackboardComp->GetValueAsBool(FName("IsRunnerZombie"));

	FColor StateColor = FColor::White;
	FString StateText;

	if (bIsInPurge)
	{
		StateText  = TEXT(">> FLEE PURGE ZONE [SPRINT] <<");
		StateColor = FColor::Orange;
	}
	else if (bHasZombie && bHasWeapon && !bIsHeavy)
	{
		StateText  = TEXT(">> COMBAT - Shooting! <<");
		StateColor = FColor::Red;
	}
	else if (bHasZombie && bIsHeavy)
	{
		StateText  = TEXT(">> FLEE - Heavy Zombie! <<");
		StateColor = FColor(255, 100, 0); // orange-red
	}
	else if (bHasZombie)
	{
		StateText  = bIsRunner
			? TEXT(">> FLEE - Runner Zombie! [SPRINT] <<")
			: TEXT(">> FLEE - Zombie nearby <<");
		StateColor = FColor(255, 80, 80);
	}
	else if (NewHealth <= 5.f && bHasMedkit)
	{
		StateText  = TEXT(">> HEAL - Using Medkit <<");
		StateColor = FColor::Green;
	}
	else if (NewStamina <= 5.f && bHasFood)
	{
		StateText  = TEXT(">> EAT - Using Food <<");
		StateColor = FColor::Yellow;
	}
	else if (bHasItem)
	{
		AActor* TargetItem = Cast<AActor>(BlackboardComp->GetValueAsObject(FName("NearestItem")));
		FString ItemName = TargetItem ? TargetItem->GetName() : TEXT("???");
		StateText  = FString::Printf(TEXT(">> SEEK ITEM: %s <<"), *ItemName);
		StateColor = FColor::Cyan;
	}
	else if (bIsInHouse)
	{
		StateText  = TEXT(">> EXIT HOUSE <<");
		StateColor = FColor::Silver;
	}
	else if (bHasHouse)
	{
		StateText  = TEXT(">> SCOUT HOUSE <<");
		StateColor = FColor(100, 200, 255);
	}
	else
	{
		StateText  = TEXT(">> WANDER <<");
		StateColor = FColor::White;
	}

	GEngine->AddOnScreenDebugMessage(50, 0.0f, StateColor,
		FString::Printf(TEXT("AI STATE: %s"), *StateText));

	GEngine->AddOnScreenDebugMessage(51, 0.0f, FColor::White,
		FString::Printf(TEXT("HP: %.0f  |  Stamina: %.1f  |  Weapon:%s  Medkit:%s  Food:%s"),
			NewHealth, NewStamina,
			bHasWeapon ? TEXT("YES") : TEXT("no"),
			bHasMedkit ? TEXT("YES") : TEXT("no"),
			bHasFood   ? TEXT("YES") : TEXT("no")));

	GEngine->AddOnScreenDebugMessage(52, 0.0f, FColor::Silver,
		FString::Printf(TEXT("Zombie:%s  House:%s  Item:%s  InPurge:%s  InHouse:%s"),
			bHasZombie  ? TEXT("YES") : TEXT("no"),
			bHasHouse   ? TEXT("YES") : TEXT("no"),
			bHasItem    ? TEXT("YES") : TEXT("no"),
			bIsInPurge  ? TEXT("YES") : TEXT("no"),
			bIsInHouse  ? TEXT("YES") : TEXT("no")));
}