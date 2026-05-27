#include "UBTT_BlendedSteer.h"
#include "AIController.h"
#include "StudentPerceptor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/StaminaComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Village/House/House.h"

UUBTT_BlendedSteer::UUBTT_BlendedSteer()
{
	NodeName = "Blended Steering (Lab Math)";
	bNotifyTick = true; // run every frame
}

EBTNodeResult::Type UUBTT_BlendedSteer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	WanderTimer = WanderUpdateInterval;
	return EBTNodeResult::InProgress;
}

void UUBTT_BlendedSteer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BBComp) return;

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return;

	UWorld* World = Pawn->GetWorld();
	if (!World) return;

	FVector FinalSteeringForce = FVector::ZeroVector;
	FVector SeekForce = FVector::ZeroVector;

// SEEK ITEM
	AActor* NearestItem = Cast<AActor>(BBComp->GetValueAsObject("NearestItem"));
	if (NearestItem)
	{
		FVector ToItem = NearestItem->GetActorLocation() - Pawn->GetActorLocation();
    
		// close enough to the item = pickup
		if (ToItem.Size2D() < 100.0f) 
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		SeekForce = ToItem;
		SeekForce.Z = 0.0f;
		SeekForce.Normalize();
	}
// SEEK HOUSE
	else if (!BBComp->GetValueAsBool(FName("IsInsideHouse")))
	{
		AActor* NearestHouse = Cast<AActor>(BBComp->GetValueAsObject("NearestHouse"));
		if (NearestHouse)
		{
			FVector ToHouse = NearestHouse->GetActorLocation() - Pawn->GetActorLocation();
        
			// close enough to the house = SaveDoorway
			if (ToHouse.Size2D() < 150.0f) 
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}
        
			SeekForce = ToHouse;
			SeekForce.Z = 0.0f;
			SeekForce.Normalize();
		}
	}
// SEEK DOORWAY
	bool bIsInside = BBComp->GetValueAsBool(FName("IsInsideHouse"));
	FVector DoorwayLoc = BBComp->GetValueAsVector(FName("DoorwayLocation"));
	AActor* NearestZombie = Cast<AActor>(BBComp->GetValueAsObject("NearestZombie"));

	if (bIsInside && !NearestItem) 
	{
		FVector ToDoor = DoorwayLoc - Pawn->GetActorLocation();
        
		if (ToDoor.Size2D() < 120.0f)
		{
			BBComp->SetValueAsBool(FName("IsInsideHouse"), false);
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		SeekForce = ToDoor;
		SeekForce.Z = 0.0f;
		SeekForce.Normalize();
	}

// WANDER  (Lab: SteeringBehaviors.cpp)
	WanderTimer += DeltaSeconds;
	
	if (WanderTimer >= 0.5f)
	{
		WanderTimer        = 0.0f;
		WanderTargetAngle  = FMath::RandRange(-PI, PI); // pick a new direction
	}

	WanderAngle = FMath::FInterpTo(WanderAngle, WanderTargetAngle, DeltaSeconds, 1.0f);

	FVector ForwardVec   = Pawn->GetActorForwardVector();
	FVector RightVec     = Pawn->GetActorRightVector();
	
	float SafeWanderDistance = 400.0f; 
	float SafeWanderRadius = 150.0f;   

	FVector CircleCenter = ForwardVec * SafeWanderDistance;
	FVector Displacement = (RightVec * FMath::Cos(WanderAngle) + ForwardVec * FMath::Sin(WanderAngle)) * SafeWanderRadius;

	FVector WanderForce = (CircleCenter + Displacement);
	WanderForce.Z = 0.0f;
	WanderForce.Normalize();

// FLEE  (Lab: SteeringBehaviors.cpp)
	FVector FleeForce    = FVector::ZeroVector;

	if (NearestZombie)
	{
		FVector ToZombie   = NearestZombie->GetActorLocation() - Pawn->GetActorLocation();
		float   ZombieDist = ToZombie.Size2D();

		FleeForce   = -ToZombie;
		FleeForce.Z = 0.0f;
		FleeForce.Normalize();

		UStudentPerceptor* Perceptor = Pawn->FindComponentByClass<UStudentPerceptor>();
		bool bIsRunner = BBComp->GetValueAsBool(FName("IsRunnerZombie"));

		// not a runner zombie - flee + toward the nearest known house
		if (!bIsRunner && Perceptor && Perceptor->KnownHouses.Num() > 0)
		{
			FVector BestHouse = FVector::ZeroVector;
			float   MinDist   = 999999.0f;
			for (FVector HouseLoc : Perceptor->KnownHouses)
			{
				float D = FVector::Dist(Pawn->GetActorLocation(), HouseLoc);
				if (D < MinDist) { MinDist = D; BestHouse = HouseLoc; }
			}

			if (MinDist < 4000.0f)
			{
				FVector SeekHouse = (BestHouse - Pawn->GetActorLocation()).GetSafeNormal2D();

				float FleeW = FMath::Lerp(0.4f, 0.85f, FMath::Clamp(1.0f - ZombieDist / 600.0f, 0.0f, 1.0f));
				FleeForce = (FleeForce * FleeW + SeekHouse * (1.0f - FleeW)).GetSafeNormal();
			}
		}
	}

// OBSTACLE AVOIDANCE  (whisker raycasts)
	FVector AvoidanceForce = FVector::ZeroVector;
	FVector Start = Pawn->GetActorLocation();
	FVector Forward = Pawn->GetActorForwardVector();

	bool bSeekingItem = (NearestItem != nullptr);
	float WhiskerScale = (bSeekingItem && bIsInside) ? 1.2f : 1.75f;

	struct FWhisker { float Angle; float Distance; };
	const FWhisker Whiskers[] =
	{
		{ -45.0f,  75.0f * WhiskerScale }, // Left
		{   0.0f,  100.0f  * WhiskerScale }, // Centre
		{  45.0f,  75.0f  * WhiskerScale }  // Right
	};

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);

	for (const FWhisker& W : Whiskers)
	{
		FVector Dir = Forward.RotateAngleAxis(W.Angle, FVector::UpVector);
		FVector End = Start + Dir * W.Distance;

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			float Proximity = 1.0f - (Hit.Distance / W.Distance);
			
			AvoidanceForce += Hit.ImpactNormal * Proximity * 5.0f;

			if (W.Angle == 0.0f)
			{
				AvoidanceForce += Pawn->GetActorRightVector() * Proximity * 4.0f;
			}

			DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Red, false, -1.0f, 0, 2.0f);
		}
		else
		{
			DrawDebugLine(World, Start, End, FColor::Green, false, -1.0f, 0, 1.0f);
		}
	}

// HOUSE CONTAINMENT (Inside Only)
	bool bAvoidWalls = true;
	
	if (bIsInside && !NearestItem)
	{
		bAvoidWalls = false;
	}

	if (bIsInside && bAvoidWalls)
	{
		AHouse* CurrentHouse = Cast<AHouse>(BBComp->GetValueAsObject("NearestHouse"));
		if (CurrentHouse)
		{
			FHouseBounds Bounds = CurrentHouse->GetBounds();
			FVector PawnLoc = Pawn->GetActorLocation();

			// get walls
			float MinX = Bounds.Origin.X - Bounds.Extent.X;
			float MaxX = Bounds.Origin.X + Bounds.Extent.X;
			float MinY = Bounds.Origin.Y - Bounds.Extent.Y;
			float MaxY = Bounds.Origin.Y + Bounds.Extent.Y;

			float Margin = 120.0f;

			// force if too close
			if (PawnLoc.X < MinX + Margin) AvoidanceForce.X += 1.0f;
			if (PawnLoc.X > MaxX - Margin) AvoidanceForce.X -= 1.0f;
			if (PawnLoc.Y < MinY + Margin) AvoidanceForce.Y += 1.0f;
			if (PawnLoc.Y > MaxY - Margin) AvoidanceForce.Y -= 1.0f;
		}
	}
	
// BLEND FORCES  (Lab: CombinedSteeringBehaviors.cpp)
	FVector DesiredDirection;

	if (!FleeForce.IsNearlyZero())
		DesiredDirection = (FleeForce * 0.85f) + (WanderForce * 0.15f); 
	else if (!SeekForce.IsNearlyZero())
		DesiredDirection = SeekForce; 
	else
		DesiredDirection = WanderForce; 

	DesiredDirection.Z = 0.0f;
	DesiredDirection.Normalize();

	if (!AvoidanceForce.IsNearlyZero())
	{
		FVector WallNormal     = AvoidanceForce.GetSafeNormal();
		FVector SlideDirection = FVector::VectorPlaneProject(DesiredDirection, WallNormal);

		if (SlideDirection.SizeSquared() < 0.1f)
			SlideDirection = FVector::CrossProduct(FVector::UpVector, WallNormal);

		SlideDirection.Normalize();

		FinalSteeringForce = (SlideDirection * 0.7f) + (WallNormal * 0.3f);
	}
	else
	{
		FinalSteeringForce = DesiredDirection;
	}

	FinalSteeringForce.Z = 0.0f;
	FinalSteeringForce.Normalize();

// SPRINTING  (only when fleeing + stamina available)
	if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(Pawn))
	{
		bool bShouldSprint = false;

		if (!FleeForce.IsNearlyZero())
		{
			bool bIsHeavy = BBComp->GetValueAsBool(FName("IsHeavyZombie"));
			bool bIsRunner = BBComp->GetValueAsBool(FName("IsRunnerZombie"));
			bool bIsInPurge = BBComp->GetValueAsBool(FName("IsInPurgeZone"));
			bShouldSprint = bIsHeavy || bIsRunner || bIsInPurge;
		}

		if (UStaminaComponent* StaminaComp = Survivor->FindComponentByClass<UStaminaComponent>())
		{
			if (StaminaComp->GetCurrentStamina() <= 0.0f)
				bShouldSprint = false;
		}

		bShouldSprint ? Survivor->StartRunning() : Survivor->StopRunning();
	}

// APPLY MOVEMENT
	Pawn->AddMovementInput(FinalSteeringForce, 1.0f);

	if (!FinalSteeringForce.IsNearlyZero())
	{
		FRotator NewRot = FMath::RInterpTo(Pawn->GetActorRotation(),
		                                   FinalSteeringForce.Rotation(),
		                                   DeltaSeconds, 8.0f);
		Pawn->SetActorRotation(NewRot);
	}
}