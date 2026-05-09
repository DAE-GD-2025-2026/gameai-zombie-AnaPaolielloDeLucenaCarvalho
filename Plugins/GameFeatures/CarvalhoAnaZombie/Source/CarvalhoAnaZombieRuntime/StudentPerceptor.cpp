// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptor.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

// these work bc we changed the dependencies in the .Build.cs file
#include "Zombies/BaseZombie.h"
#include "Items/BaseItem.h"


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
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Sensed Actor: %s"), *Actor->GetName()));

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
	

	// is actor Zombie?
	if (ABaseZombie* SeenZombie = Cast<ABaseZombie>(Actor))
	{
		BlackboardComp->SetValueAsObject(FName("NearestZombie"), SeenZombie);
		
		// DEBUG - we see a zombie
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("I see a Zombie!"));
	}
	// is actor Item
	else if (ABaseItem* SeenItem = Cast<ABaseItem>(Actor))
	{
		BlackboardComp->SetValueAsObject(FName("NearestItem"), SeenItem);
		
		// DEBUG - we see an item
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("I see an Item!"));
	}
	else
	{
		// DEBUG - we see something else
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("Saw something, but it's not a Zombie or Item."));
	}
}