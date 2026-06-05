#include "UUBTT_PickupItemCarvalhoAna.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/Weapon.h"
#include "Items/Medkit.h"
#include "Items/Food.h"
#include "Kismet/GameplayStatics.h"

UUBTT_PickupItemCarvalhoAna::UUBTT_PickupItemCarvalhoAna()
{
	NodeName = "Pickup Nearest Item";
}

EBTNodeResult::Type UUBTT_PickupItemCarvalhoAna::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// get brain and blackboard
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BlackboardComp) return EBTNodeResult::Failed;

	// get survivor and inventory
	APawn* SurvivorPawn = AIController->GetPawn();
	if (!SurvivorPawn) return EBTNodeResult::Failed;

	UInventoryComponent* InventoryComp = SurvivorPawn->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComp) return EBTNodeResult::Failed;

	// get item
	UObject* ItemObject = BlackboardComp->GetValueAsObject(FName("NearestItem"));
	ABaseItem* ItemToPickup = Cast<ABaseItem>(ItemObject);

	if (ItemToPickup)
	{
		// if garbage, destroy to clean world and free memory
		if (ItemToPickup->GetItemType() == EItemType::Garbage)
		{
			// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Stomped on Garbage!"));
			ItemToPickup->Destroy();
			
			BlackboardComp->ClearValue(FName("NearestItem"));

			if (BlackboardComp->GetValueAsBool(FName("IsInsideHouse"))) ReScanRoomForNextItem(InventoryComp, BlackboardComp, SurvivorPawn);

			return EBTNodeResult::Succeeded;
		}

		bool bSuccess = false;
        
		// open the backpack
		TArray<ABaseItem*> Backpack = InventoryComp->GetInventory();

		// check each slot
		for (int i = 0; i < Backpack.Num(); ++i) 
		{
			// grab item if slot is empty
			if (Backpack[i] == nullptr)
			{
				if (InventoryComp->GrabItem(i, ItemToPickup))
				{
					// hide item in game
					ItemToPickup->SetActorHiddenInGame(true);
					ItemToPickup->SetActorEnableCollision(false);

					bSuccess = true;
					break;
				}
			}
		}

		if (bSuccess)
		{
			// clear memory
			BlackboardComp->ClearValue(FName("NearestItem"));

			if (BlackboardComp->GetValueAsBool(FName("IsInsideHouse")))
				ReScanRoomForNextItem(InventoryComp, BlackboardComp, SurvivorPawn);

			return EBTNodeResult::Succeeded;
		}

		BlackboardComp->ClearValue(FName("NearestItem"));
	}

	return EBTNodeResult::Failed;
}

// helper - synchronous room re-scan
void UUBTT_PickupItemCarvalhoAna::ReScanRoomForNextItem(UInventoryComponent* Inv, UBlackboardComponent* BB, APawn* Pawn)
{
	int  EmptySlots = 0;
	bool bHasWeapon = false;
	for (ABaseItem* Slot : Inv->GetInventory())
	{
		if (!Slot) EmptySlots++;
		if (Cast<AWeapon>(Slot)) bHasWeapon = true;
	}

	FVector MyLoc = Pawn->GetActorLocation();

	TArray<AActor*> AllItems;
	UGameplayStatics::GetAllActorsOfClass(Pawn->GetWorld(), ABaseItem::StaticClass(), AllItems);

	constexpr float RoomScanRadius = 600.f; // tighter than UpdateStats
	float BestDist = RoomScanRadius;
	ABaseItem* BestItem = nullptr;

	for (AActor* IA : AllItems)
	{
		ABaseItem* Item = Cast<ABaseItem>(IA);
		if (!Item || !IsValid(Item) || Item->IsHidden()) continue;

		bool bWant = false;
		if (Item->GetItemType() == EItemType::Garbage) bWant = true;
		else if (Cast<AWeapon>(Item) && !bHasWeapon && EmptySlots > 0) bWant = true;
		else if (Cast<AMedkit>(Item) && EmptySlots > 0) bWant = true;
		else if (Cast<AFood>(Item) && EmptySlots > 0) bWant = true;

		if (!bWant) continue;

		float D = FVector::Dist2D(MyLoc, Item->GetActorLocation());
		if (D < BestDist) { BestDist = D; BestItem = Item; }
	}

	if (BestItem)
	{
		BB->SetValueAsObject(FName("NearestItem"), BestItem);
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("[SCAVENGE] Next item in room: %s (%.0f units)"), *BestItem->GetName(), BestDist));
	}
}