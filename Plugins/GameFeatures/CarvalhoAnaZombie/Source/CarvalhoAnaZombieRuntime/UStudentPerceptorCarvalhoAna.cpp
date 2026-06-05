// Fill out your copyright notice in the Description page of Project Settings.


#include "UStudentPerceptorCarvalhoAna.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

// these work bc we changed the dependencies in the .Build.cs file
#include "Zombies/BaseZombie.h"
#include "Items/BaseItem.h"
#include "Items/Weapon.h"
#include "Village/House/House.h"
#include "Common/InventoryComponent.h"


UStudentPerceptorCarvalhoAna::UStudentPerceptorCarvalhoAna()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptorCarvalhoAna::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptorCarvalhoAna::OnPerceptionUpdated);
	}
}

void UStudentPerceptorCarvalhoAna::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	//GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, FString::Printf(TEXT("Saw Something!")));
	if (!Actor || !Stimulus.WasSuccessfullySensed())
	{
		// when actor leaves perception, remove from sighting cache - next time it enters range the message fires once more.
		if (ABaseItem* LostItem = Cast<ABaseItem>(Actor))
			RecentlySightedItems.Remove(LostItem);
		return;
	}
	
	// get AI Controller
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (!AIController)
	{
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			AIController = Cast<AAIController>(OwnerPawn->GetController());
		}
	}
	if (!AIController)
	{
		// DEBUG - have we found the AI Controller
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("ERROR: Cannot find AI Controller!"));
		return;
	}
	
	// get Blackboard
	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		// DEBUG - have we found the Blackboard
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("ERROR: Blackboard is NULL! (Behavior Tree is probably not running yet)"));
		return;
	}
	
	// DAMAGE SENSE - 360 DEGREE VISION (feedback)
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
	{
		if (ABaseZombie* Attacker = Cast<ABaseZombie>(Actor))
		{
			BlackboardComp->SetValueAsObject(FName("NearestZombie"), Attacker);

			bool bIsHeavy  = Attacker->GetName().Contains("Heavy");
			bool bIsRunner = Attacker->GetName().Contains("Runner");
			BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"),  bIsHeavy);
			BlackboardComp->SetValueAsBool(FName("IsRunnerZombie"), bIsRunner);

			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("OUCH! Hit by %s (Heavy=%s)"), *Attacker->GetName(), bIsHeavy ? TEXT("YES") : TEXT("no")));

			return;
		}
	}
	
	APawn* Pawn = AIController->GetPawn();
	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
	
	int EmptySlots = 0;
	bool bHasWeapon = false;
	
	if (InventoryComp)
	{
		for (ABaseItem* Item : InventoryComp->GetInventory())
		{
			if (Item == nullptr) EmptySlots++;
			if (Item && Cast<AWeapon>(Item)) bHasWeapon = true;
		}
	}
	
	// SIGHT SENSE & MEMORY
	if (ABaseZombie* SeenZombie = Cast<ABaseZombie>(Actor)) // -> is a Zombie? (Fleeing logic)
	{
		BlackboardComp->SetValueAsObject(FName("NearestZombie"), SeenZombie);
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("I see a Zombie!"));

		// heavy zombie?
		if (SeenZombie->GetName().Contains("Heavy")) 
		{
			BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"), true);
		} else 
		{
			BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"), false);
		}

		// runner zombie?
		if (SeenZombie->GetName().Contains("Runner")) 
		{
			BlackboardComp->SetValueAsBool(FName("IsRunnerZombie"), true);
		} else 
		{
			BlackboardComp->SetValueAsBool(FName("IsRunnerZombie"), false);
		}
	}
	else if (AHouse* SeenHouse = Cast<AHouse>(Actor)) // -> is a House? (Seeking logic)
	{
		// THE MAP - remember where this house is for emergencies (hiding)
		bool bAlreadyKnown = KnownHouses.ContainsByPredicate([&](FVector Loc)
		{
			return FVector::Dist2D(Loc, SeenHouse->GetActorLocation()) < 200.f;
		});
		
		if (!bAlreadyKnown)
		{
			KnownHouses.AddUnique(SeenHouse->GetActorLocation()); 
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("[MEMORY +] Saved house location at (%.0f, %.0f)"), SeenHouse->GetActorLocation().X, SeenHouse->GetActorLocation().Y));
		}

		// do not set NearestHouse while inside
		if (BlackboardComp->GetValueAsBool(FName("IsInsideHouse"))) return;

		// LOOT - did we already looted this house?
		if (!VisitedHouses.Contains(SeenHouse))
		{
			// new house, loot it
			BlackboardComp->SetValueAsObject(FName("NearestHouse"), SeenHouse);
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("[SIGHT] New house spotted! Going to loot it.")));
		}
		else
		{
			// already looted -	not seting NearestHouse
		}
	}
	else if (AWeapon* SeenWeapon = Cast<AWeapon>(Actor)) // -> is a Weapon? (Seeking logic)
	{
		// do we have space + do we need it?
		if (EmptySlots > 0 && !bHasWeapon)
		{
			// only print + set once per sighting session
			if (!RecentlySightedItems.Contains(SeenWeapon))
			{
				RecentlySightedItems.Add(SeenWeapon);
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("[SIGHT] Weapon spotted! Going to pick up: %s"), *SeenWeapon->GetName()));
			}
			BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenWeapon);
		}
		else
		{
			// not needed, save to memory
			if (!KnownItems.Contains(SeenWeapon))
			{
				KnownItems.AddUnique(SeenWeapon);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, FString::Printf(TEXT("[MEMORY +] Saved weapon for later: %s at (%.0f, %.0f)"), *SeenWeapon->GetName(), SeenWeapon->GetActorLocation().X, SeenWeapon->GetActorLocation().Y));
			}
		}
	}
	else if (ABaseItem* SeenItem = Cast<ABaseItem>(Actor)) // -> is a Medkit / Food / Garbage? (Seeking logic)
	{
		// garbage we always pickup since it wont take up inventory space
		if (SeenItem->GetItemType() == EItemType::Garbage)
		{
			// only set the BB key once per sighting session
			if (!RecentlySightedItems.Contains(SeenItem))
			{
				RecentlySightedItems.Add(SeenItem);
				BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenItem);
			}
		}
		
		// do we have space for Food/Medkits?
		else if (EmptySlots > 0)
		{
			// only print + set once per sighting session
			if (!RecentlySightedItems.Contains(SeenItem))
			{
				RecentlySightedItems.Add(SeenItem);
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("[SIGHT] Item spotted! Going to pick up: %s"), *SeenItem->GetName()));
				BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenItem);
			}
		}
		else
		{
			// no space, save to memory
			if (!KnownItems.Contains(SeenItem))
			{
				KnownItems.AddUnique(SeenItem);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, FString::Printf(TEXT("[MEMORY +] Saved item for later: %s at (%.0f, %.0f)"), *SeenItem->GetName(), SeenItem->GetActorLocation().X, SeenItem->GetActorLocation().Y));
			}
		}
	}
	
	// Old code for generic item sensing
	{
		// is a generic Item?
		/*else if (ABaseItem* SeenItem = Cast<ABaseItem>(Actor))
		{
			BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenItem);
			
			// DEBUG - we see an item
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("I see an Item!"));
		}
		else
		{
			// DEBUG - we see something else
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("Saw something else"));
		}*/
	}
}