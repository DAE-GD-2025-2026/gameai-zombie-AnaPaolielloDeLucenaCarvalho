#include "UBTService_UpdateStatsCarvalhoAna.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "AIController.h"
#include "UStudentPerceptorCarvalhoAna.h"
#include "Common/HealthComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/Weapon.h"
#include "Items/Medkit.h"
#include "Items/Food.h"
#include "Common/StaminaComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Zombies/BaseZombie.h"
#include "Village/House/House.h"
#include "PurgeZones/PurgeZone.h"

// constants
static constexpr float ZombieForgetDistance = 800.f;
static constexpr float HouseScanRange = 1200.f; //999999.f;

UBTService_UpdateStatsCarvalhoAna::UBTService_UpdateStatsCarvalhoAna()
{
	NodeName = "Update Survivor Stats";
	bNotifyTick = true;
}

// helper - line to confirm the survivor can see target (no wall between them)
static bool HasLineOfSight(APawn* Pawn, AActor* Target)
{
	if (!Pawn || !Target) return false;

	UWorld* World = Pawn->GetWorld();
	if (!World) return false;

	FVector Start = Pawn->GetActorLocation() + FVector(0.f, 0.f, 60.f); // eye height
	FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 60.f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);
	Params.AddIgnoredActor(Target);

	FHitResult Hit;
	bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	return !bBlocked;
}

void UBTService_UpdateStatsCarvalhoAna::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float  DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// get Blackboard and AI Controller
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	AAIController* AICon  = OwnerComp.GetAIOwner();
	if (!BBComp || !AICon) return;

	// get survivor
	APawn* Pawn = AICon->GetPawn();
	if (!Pawn) return;

	FVector MyLoc = Pawn->GetActorLocation();

	// health
	float NewHealth = 0.f;
	float NewStamina = 0.f;

	if (UHealthComponent* HC = Pawn->FindComponentByClass<UHealthComponent>())
	{
		float OldHealth = BBComp->GetValueAsFloat(FName("CurrentHealth"));
		NewHealth = HC->GetHealth();

		if (NewHealth < OldHealth && OldHealth > 0.f)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("OUCH!"));

			TArray<AActor*> AllZombies;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseZombie::StaticClass(), AllZombies);

			AActor* Attacker = nullptr;
			float ClosestDist = 999999.f;
			for (AActor* Z : AllZombies)
			{
				float D = FVector::Dist(MyLoc, Z->GetActorLocation());
				if (D < ClosestDist) { ClosestDist = D; Attacker = Z; }
			}

			if (Attacker && ClosestDist < 400.f)
			{
				BBComp->SetValueAsObject(FName("NearestZombie"), Attacker);
				BBComp->SetValueAsBool(FName("IsHeavyZombie"), Attacker->GetName().Contains("Heavy"));
				BBComp->SetValueAsBool(FName("IsRunnerZombie"), Attacker->GetName().Contains("Runner"));
			}
		}

		BBComp->SetValueAsFloat(FName("CurrentHealth"), NewHealth);
	}

	// stamina
	if (UStaminaComponent* SC = Pawn->FindComponentByClass<UStaminaComponent>())
	{
		NewStamina = SC->GetCurrentStamina();
		BBComp->SetValueAsFloat(FName("CurrentStamina"), NewStamina);
	}

	// inventory
	bool bHasWeapon = false;
	bool bHasMedkit = false;
	bool bHasFood = false;
	int  EmptySlots = 0;

	if (UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>())
	{
		for (ABaseItem* Item : Inv->GetInventory())
		{
			if (!Item) EmptySlots++;
			if (Cast<AWeapon>(Item)) bHasWeapon = true;
			if (Cast<AMedkit>(Item)) bHasMedkit = true;
			if (Cast<AFood>(Item)) bHasFood = true;
		}
		BBComp->SetValueAsBool(FName("HasWeapon"), bHasWeapon);
		BBComp->SetValueAsBool(FName("HasMedkit"), bHasMedkit);
		BBComp->SetValueAsBool(FName("HasFood"), bHasFood);
	}
	
	UStudentPerceptorCarvalhoAna* Perceptor = AICon->FindComponentByClass<UStudentPerceptorCarvalhoAna>();
	if (!Perceptor) Perceptor = Pawn->FindComponentByClass<UStudentPerceptorCarvalhoAna>();

	// still in FLEE but zombie is far = forget zombie
	if (AActor* CurZombie = Cast<AActor>(BBComp->GetValueAsObject(FName("NearestZombie"))))
	{
		float ZombieDist = FVector::Dist2D(MyLoc, CurZombie->GetActorLocation());
		if (ZombieDist > ZombieForgetDistance)
		{
			BBComp->ClearValue(FName("NearestZombie"));
			BBComp->SetValueAsBool(FName("IsHeavyZombie"),false);
			BBComp->SetValueAsBool(FName("IsRunnerZombie"), false);
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("[ZOMBIE] Safe distance reached — resuming normal behaviour."));
		}
	}
	
	UObject* CurrentItem = BBComp->GetValueAsObject(FName("NearestItem"));
	UObject* CurrentHouse = BBComp->GetValueAsObject(FName("NearestHouse"));

	if (Perceptor && !CurrentItem)
	{
		Perceptor->KnownItems.RemoveAll([](ABaseItem* M){ return !IsValid(M); });

		ABaseItem* BestMemItem = nullptr;
		float BestDist = 999999.f;

		for (ABaseItem* MemItem : Perceptor->KnownItems)
		{
			if (!IsValid(MemItem)) continue;

			// only retrieve from memory when we actually need the item
			bool bNeedThis = false;
			if (Cast<AMedkit>(MemItem) && NewHealth  <= 5.f && !bHasMedkit) bNeedThis = true;
			if (Cast<AFood>(MemItem) && NewStamina <= 5.f && !bHasFood) bNeedThis = true;
			if (Cast<AWeapon>(MemItem) && !bHasWeapon) bNeedThis = true;
			// garbage always picked up
			if (MemItem->GetItemType() == EItemType::Garbage) bNeedThis = true;

			if (!bNeedThis) continue;

			float D = FVector::Dist(MyLoc, MemItem->GetActorLocation());
			if (D < BestDist) { BestDist = D; BestMemItem = MemItem; }
		}

		if (BestMemItem)
		{
			BBComp->SetValueAsObject(FName("NearestItem"), BestMemItem);
			CurrentItem = BestMemItem;
			GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan, FString::Printf(TEXT("[MEMORY->] Going back for: %s"), *BestMemItem->GetName()));
			Perceptor->KnownItems.Remove(BestMemItem);
		}
	}

	if (!CurrentItem && BBComp->GetValueAsBool(FName("IsInsideHouse")))
	{
		TArray<AActor*> AllItems;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseItem::StaticClass(), AllItems);

		float BestDist = 700.f;
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
			BBComp->SetValueAsObject(FName("NearestItem"), BestItem);
			CurrentItem = BestItem;
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("[INSIDE] Item: %s (%.0f units)"), *BestItem->GetName(), BestDist));
		}
	}


	// house scan
	ProximityScanTimer += DeltaSeconds;
	if (ProximityScanTimer >= 0.5f)
	{
		ProximityScanTimer = 0.f;

		// skip house scan while exiting - avoids re-setting NearestHouse and aborting EXIT HOUSE
		bool bCurrentlyInside = BBComp->GetValueAsBool(FName("IsInsideHouse"));
		if (!CurrentHouse && !bCurrentlyInside)
		{
			TArray<AActor*> AllHouses;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHouse::StaticClass(), AllHouses);

			float BestDist = HouseScanRange;
			AHouse* BestHouse = nullptr;

			for (AActor* HA : AllHouses)
			{
				AHouse* House = Cast<AHouse>(HA);
				if (!House || !IsValid(House)) continue;
				if (Perceptor && Perceptor->VisitedHouses.Contains(House)) continue;

				float D = FVector::Dist2D(MyLoc, House->GetActorLocation());
				if (D < BestDist) { BestDist = D; BestHouse = House; }
			}

			if (BestHouse)
			{
				BBComp->SetValueAsObject(FName("NearestHouse"), BestHouse);
				CurrentHouse = BestHouse;
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Silver, FString::Printf(TEXT("[SCAN] House nearby (%.0f units)"), BestDist));

				if (Perceptor)
				{
					bool bKnown = Perceptor->KnownHouses.ContainsByPredicate([&](FVector Loc)
					{
						return FVector::Dist2D(Loc, BestHouse->GetActorLocation()) < 200.f;
					});
					if (!bKnown) Perceptor->KnownHouses.AddUnique(BestHouse->GetActorLocation());
				}
			}

			if (!CurrentHouse && Perceptor)
			{
				// find closest known house that is still unvisited
				float MemBestDist = 999999.f;
				AHouse* MemBestHouse = nullptr;

				for (const FVector& KnownLoc : Perceptor->KnownHouses)
				{
					for (AActor* HA : AllHouses)
					{
						AHouse* House = Cast<AHouse>(HA);
						if (!House || !IsValid(House)) continue;
						if (Perceptor->VisitedHouses.Contains(House)) continue;
						if (FVector::Dist2D(House->GetActorLocation(), KnownLoc) > 200.f) continue;

						float D = FVector::Dist2D(MyLoc, House->GetActorLocation());
						if (D < MemBestDist) { MemBestDist = D; MemBestHouse = House; }
					}
				}

				if (MemBestHouse)
				{
					BBComp->SetValueAsObject(FName("NearestHouse"), MemBestHouse);
					CurrentHouse = MemBestHouse;
					GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Blue, FString::Printf(TEXT("[MEMORY->] Heading to remembered house (%.0f units)"), MemBestDist));
				}
			}
		}

		// stuck when leaving houses
		bool bIsInHouseFlag = BBComp->GetValueAsBool(FName("IsInsideHouse"));
		if (bIsInHouseFlag)
		{
			TArray<AActor*> AllHouses;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHouse::StaticClass(), AllHouses);

			bool bStillInside = false;
			for (AActor* HA : AllHouses)
			{
				if (!HA) continue;

				// Use actor bounds (all components, not just collision) for accurate size
				FVector BoundsOrigin, BoundsExtent;
				HA->GetActorBounds(false, BoundsOrigin, BoundsExtent);

				// Generous margin so doorway area never false-triggers
				const float Margin = 350.f;

				// XY only (Dist2D) — Z differences on sloped ground are irrelevant
				bool bInX = MyLoc.X > BoundsOrigin.X - BoundsExtent.X - Margin
				         && MyLoc.X < BoundsOrigin.X + BoundsExtent.X + Margin;
				bool bInY = MyLoc.Y > BoundsOrigin.Y - BoundsExtent.Y - Margin
				         && MyLoc.Y < BoundsOrigin.Y + BoundsExtent.Y + Margin;

				if (bInX && bInY) { bStillInside = true; break; }
			}

			if (!bStillInside)
			{
				// Safety net only — BlendedSteer is the primary authority
				BBComp->SetValueAsBool(FName("IsInsideHouse"), false);
				BBComp->ClearValue(FName("NearestHouse"));
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, TEXT("[HOUSE] Safety reset: pawn left all house bounds."));
			}
		}

		if (!BBComp->GetValueAsObject(FName("NearestZombie")))
		{
			TArray<AActor*> AllZombies;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseZombie::StaticClass(), AllZombies);

			float BestDist = 500.f; // sight-range cap
			ABaseZombie* BestZombie = nullptr;

			for (AActor* ZA : AllZombies)
			{
				ABaseZombie* Z = Cast<ABaseZombie>(ZA);
				if (!Z || !IsValid(Z)) continue;

				float D = FVector::Dist2D(MyLoc, Z->GetActorLocation());
				if (D >= BestDist) continue;

				// LINE-OF-SIGHT CHECK
				if (!HasLineOfSight(Pawn, Z)) continue;

				BestDist   = D;
				BestZombie = Z;
			}

			if (BestZombie)
			{
				BBComp->SetValueAsObject(FName("NearestZombie"), BestZombie);
				BBComp->SetValueAsBool(FName("IsHeavyZombie"),  BestZombie->GetName().Contains("Heavy"));
				BBComp->SetValueAsBool(FName("IsRunnerZombie"), BestZombie->GetName().Contains("Runner"));
			}
		}

		{
			TArray<AActor*> AllPurgeZones;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), APurgeZone::StaticClass(), AllPurgeZones);

			bool    bNowInPurge = false;
			FVector PurgeCentre = FVector::ZeroVector;

			for (AActor* PZA : AllPurgeZones)
			{
				if (!PZA || !IsValid(PZA)) continue;

				// Derive the zone radius from its actor bounds — public API, no protected access needed
				FVector BoundsOrigin, BoundsExtent;
				PZA->GetActorBounds(false, BoundsOrigin, BoundsExtent);
				float Radius = FMath::Max(BoundsExtent.X, BoundsExtent.Y); // XY extent = radius for circular zones

				float Dist = FVector::Dist2D(MyLoc, PZA->GetActorLocation());

				float FleeMargin = 800.f;

				if (Dist < Radius + FleeMargin)
				{
					bNowInPurge = true;
					PurgeCentre = PZA->GetActorLocation();
					break; // first match is enough
				}
			}

			BBComp->SetValueAsBool(FName("IsInPurgeZone"), bNowInPurge);
			if (bNowInPurge)
				BBComp->SetValueAsVector(FName("PurgeZoneCenter"), PurgeCentre);

			if (bNowInPurge)
				GEngine->AddOnScreenDebugMessage(-1, 0.6f, FColor::Orange, TEXT("[PURGE] Evading purge zone!"));
		}
	}

	// HUD STATE DISPLAY - This is an original idea from Damian's project that i take 0 credit for (only the code part, but the actual idea is not mine)
	bool bHasZombie = (BBComp->GetValueAsObject(FName("NearestZombie")) != nullptr);
	bool bHasItem = (CurrentItem  != nullptr);
	bool bHasHouse = (CurrentHouse != nullptr);
	bool bIsInPurge = BBComp->GetValueAsBool(FName("IsInPurgeZone"));
	bool bIsInHouse = BBComp->GetValueAsBool(FName("IsInsideHouse"));
	bool bIsHeavy = BBComp->GetValueAsBool(FName("IsHeavyZombie"));
	bool bIsRunner  = BBComp->GetValueAsBool(FName("IsRunnerZombie"));

	FColor  StateColor;
	FString StateText;

	if (bIsInPurge)
	{
		StateText = TEXT(">> FLEE PURGE ZONE [SPRINT] <<");
		StateColor = FColor::Orange;
	}
	else if (bHasZombie && bHasWeapon && !bIsHeavy)
	{
		StateText = TEXT(">> COMBAT - Shooting! <<");
		StateColor = FColor::Red;
	}
	else if (bHasZombie && bIsHeavy)
	{
		StateText = TEXT(">> FLEE - Heavy Zombie! <<");
		StateColor = FColor(255, 100, 0);
	}
	else if (bHasZombie)
	{
		StateText = bIsRunner ? TEXT(">> FLEE - Runner Zombie! [SPRINT] <<") : TEXT(">> FLEE - Zombie nearby <<");
		StateColor = FColor(255, 80, 80);
	}
	else if (NewHealth <= 5.f && bHasMedkit)
	{
		StateText = TEXT(">> HEAL - Using Medkit <<");
		StateColor = FColor::Green;
	}
	else if (NewStamina <= 5.f && bHasFood)
	{
		StateText = TEXT(">> EAT - Using Food <<");
		StateColor = FColor::Yellow;
	}
	else if (bHasItem)
	{
		AActor* TargetItem = Cast<AActor>(CurrentItem);
		FString ItemName  = TargetItem ? TargetItem->GetName() : TEXT("???");
		StateText  = FString::Printf(TEXT(">> SEEK ITEM: %s <<"), *ItemName);
		StateColor = FColor::Cyan;
	}
	else if (bIsInHouse)
	{
		StateText = TEXT(">> EXIT HOUSE <<");
		StateColor = FColor::Silver;
	}
	else if (bHasHouse)
	{
		StateText = TEXT(">> SCOUT HOUSE <<");
		StateColor = FColor(100, 200, 255);
	}
	else
	{
		StateText = TEXT(">> WANDER <<");
		StateColor = FColor::White;
	}

	LastStateText  = StateText;
	LastStateColor = StateColor;
	GEngine->AddOnScreenDebugMessage(50, 3.0f, StateColor, FString::Printf(TEXT("AI STATE: %s"), *StateText));

	// refresh stats
	GEngine->AddOnScreenDebugMessage(51, 2.0f, FColor::White, FString::Printf(TEXT("HP: %.0f  |  Stamina: %.1f  |  Weapon:%s  Medkit:%s  Food:%s"), NewHealth, NewStamina, bHasWeapon ? TEXT("YES") : TEXT("no"), bHasMedkit ? TEXT("YES") : TEXT("no"), bHasFood ? TEXT("YES") : TEXT("no")));
	GEngine->AddOnScreenDebugMessage(52, 2.0f, FColor::Silver, FString::Printf(TEXT("Zombie:%s  House:%s  Item:%s  InPurge:%s  InHouse:%s  EmptySlots:%d"), bHasZombie ? TEXT("YES") : TEXT("no"), bHasHouse ? TEXT("YES") : TEXT("no"), bHasItem ? TEXT("YES") : TEXT("no"), bIsInPurge ? TEXT("YES") : TEXT("no"), bIsInHouse ? TEXT("YES") : TEXT("no"), EmptySlots));

	// ammo count so the UI updates on every shot
	if (UInventoryComponent* InvDisplay = Pawn->FindComponentByClass<UInventoryComponent>())
	{
		int32 Ammo = -1;
		for (ABaseItem* Slot : InvDisplay->GetInventory())
		{
			if (Slot && Cast<AWeapon>(Slot)) { Ammo = Slot->GetValue(); break; }
		}
		FString AmmoStr = (Ammo >= 0) ? FString::Printf(TEXT("%d"), Ammo) : TEXT("---");
		GEngine->AddOnScreenDebugMessage(53, 2.0f, FColor::Yellow, FString::Printf(TEXT("Gun Ammo: %s"), *AmmoStr));
	}

	// current memory list
	if (Perceptor)
	{
		GEngine->AddOnScreenDebugMessage(54, 2.0f, FColor::Purple, FString::Printf(TEXT("Memory: %d item(s) | %d house(s)"), Perceptor->KnownItems.Num(), Perceptor->KnownHouses.Num()));
	}
}
