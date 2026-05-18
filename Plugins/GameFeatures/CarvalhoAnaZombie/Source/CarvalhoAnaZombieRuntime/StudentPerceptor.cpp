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

			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("OUCH! Bit from behind!"));
			
			return; 
		}
	}
	
	// SIGHT SENSE
	if (ABaseZombie* SeenZombie = Cast<ABaseZombie>(Actor)) // is a Zombie?
	{
		BlackboardComp->SetValueAsObject(FName("NearestZombie"), SeenZombie);

		// is heavy zombie?
		if (SeenZombie->GetName().Contains("Heavy"))
		{
			BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"), true);
		}
		else
		{
			BlackboardComp->SetValueAsBool(FName("IsHeavyZombie"), false);
		}
	}
	else if (AHouse* SeenHouse = Cast<AHouse>(Actor)) // is House? (Fleeing logic)
	{
		BlackboardComp->SetValueAsObject(FName("NearestHouse"), SeenHouse);
		
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("I see a House!"));
	}
	else if (AWeapon* SeenWeapon = Cast<AWeapon>(Actor)) // is a Weapon?
	{
		BlackboardComp->SetValueAsObject(FName("NearestWeapon"), SeenWeapon);		
		BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenWeapon); 
		
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("I see a Weapon!"));
	}
	else if (AMedkit* SeenMedkit = Cast<AMedkit>(Actor)) // is a Medkit?
	{
		BlackboardComp->SetValueAsObject(FName("NearestMedkit"), SeenMedkit);
		BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenMedkit); 
		
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("I see a Medkit!"));
	}
	else if (AFood* SeenFood = Cast<AFood>(Actor)) // is Food?
	{
		BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenFood);
		
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("I see Food!"));
	}
	else if (Actor->GetName().Contains("Purge")) // is a Purge Zone? (Fleeing logic)
	{
		BlackboardComp->SetValueAsObject(FName("NearestPurgeZone"), Actor);
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