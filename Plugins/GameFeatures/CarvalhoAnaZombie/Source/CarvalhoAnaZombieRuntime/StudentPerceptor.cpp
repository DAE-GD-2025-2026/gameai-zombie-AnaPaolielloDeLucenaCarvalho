// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptor.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

// these work bc we changed the dependencies in the .Build.cs file
#include "Zombies/BaseZombie.h"
#include "Items/BaseItem.h"
#include "Items/Medkit.h"
#include "Items/Weapon.h"
#include "Items/Food.h"
#include "Village/House/House.h"
#include "Common/InventoryComponent.h"


UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
}

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	//GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, FString::Printf(TEXT("Saw Something!")));
	
	if (!Actor || !Stimulus.WasSuccessfullySensed())
	{
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
		// took damage!
		if (ABaseZombie* Attacker = Cast<ABaseZombie>(Actor))
		{
			BlackboardComp->SetValueAsObject(FName("NearestZombie"), Attacker);
			
			// check for Heavy zombie
			if (Attacker->GetName().Contains("Heavy")) {
				BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"), true);
			} else {
				BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"), false);
			}

			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("OUCH! BEHIND USE!"));
			
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
	if (ABaseZombie* SeenZombie = Cast<ABaseZombie>(Actor)) // -> is a Zombie?
	{
		BlackboardComp->SetValueAsObject(FName("NearestZombie"), SeenZombie);		
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("I see a Zombie!"));
	}
	else if (AHouse* SeenHouse = Cast<AHouse>(Actor)) // -> is a House? (Seeking logic)
	{
		BlackboardComp->SetValueAsObject(FName("NearestHouse"), SeenHouse);
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("I see a House!"));
		
		// Add to memory (AddUnique = don't add same house twice)
		KnownHouses.AddUnique(SeenHouse->GetActorLocation()); 
	}
	else if (AWeapon* SeenWeapon = Cast<AWeapon>(Actor)) // -> is a Weapon? (Seeking logic)
	{
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("I see a Weapon!"));
		
		// do we have space + do we need it?
		if (EmptySlots > 0 && !bHasWeapon)
		{
			BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenWeapon);
		}
		else
		{
			// not needed, save to memory
			KnownItems.AddUnique(SeenWeapon);
		}
	}
	else if (ABaseItem* SeenItem = Cast<ABaseItem>(Actor)) // -> is a Medkit / Food / Garbage? (Seeking logic)
	{
		// garbage we always pickup since it wont take up inventory space
		if (SeenItem->GetItemType() == EItemType::Garbage)
		{
			BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenItem);
		}
		
		// do we have space for Food/Medkits?
		if (EmptySlots > 0)
		{
			// have space, grab it
			BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenItem);
		}
		else
		{
			// no space, save to memory
			KnownItems.AddUnique(SeenItem);
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