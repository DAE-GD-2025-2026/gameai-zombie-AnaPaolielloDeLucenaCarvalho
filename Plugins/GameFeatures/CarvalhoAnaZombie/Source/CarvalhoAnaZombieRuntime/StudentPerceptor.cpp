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
	GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("Saw Something!")));
	
	if (!Actor || !Stimulus.WasSuccessfullySensed())
	{
		return;
	}
	
	// DEBUG - what object was seen?
	// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Sensed Actor: %s"), *Actor->GetName()));

	// get AI Controller
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (!AIController)
	{
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			AIController = Cast<AAIController>(OwnerPawn->GetController());
		}
	}
	
	// DEBUG - have we found the AI Controller
	if (!AIController)
	{
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
	

	// is a Zombie?
	if (ABaseZombie* SeenZombie = Cast<ABaseZombie>(Actor))
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
	// is House? (Fleeing logic)
	else if (AHouse* SeenHouse = Cast<AHouse>(Actor))
	{
		BlackboardComp->SetValueAsObject(FName("NearestHouse"), SeenHouse);
		
		// DEBUG - we see a house
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("I see a House!"));
	}
	// is a Weapon?
	else if (AWeapon* SeenWeapon = Cast<AWeapon>(Actor))
	{
		BlackboardComp->SetValueAsObject(FName("NearestWeapon"), SeenWeapon);		
		BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenWeapon); 
		
		// DEBUG - we see a weapon
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("I see a Weapon!"));
	}
	// is a Medkit?
	else if (AMedkit* SeenMedkit = Cast<AMedkit>(Actor))
	{
		BlackboardComp->SetValueAsObject(FName("NearestMedkit"), SeenMedkit);
		BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenMedkit); 
		
		// DEBUG - we see a medkit
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("I see a Medkit!"));
	}
	// is Food?
	else if (AFood* SeenFood = Cast<AFood>(Actor))
	{
		BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenFood);
		
		// DEBUG - we see food
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("I see Food!"));
	}
	// is a Purge Zone?
	else if (Actor->GetName().Contains("Purge"))
	{
		BlackboardComp->SetValueAsObject(FName("NearestPurgeZone"), Actor);
	}
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