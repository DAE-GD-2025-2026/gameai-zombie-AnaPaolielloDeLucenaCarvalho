#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UUBTT_BlendedSteerCarvalhoAna.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_BlendedSteerCarvalhoAna : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UUBTT_BlendedSteerCarvalhoAna();
	
	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderUpdateInterval = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderDistance = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderJitter = 1.0f;  

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	// Wander tracking (navigation-based)
	float WanderAngle = 0.0f;
	float WanderTimer = 0.0f;
	FVector LastWanderTarget = FVector::ZeroVector;
	float WanderTargetReachedDistance = 200.0f;
	float TimeSinceWanderStart = 0.0f;
	
	// Stuck detection (improved)
	FVector LastPosition = FVector::ZeroVector;
	FVector LastReportedPosition = FVector::ZeroVector;
	float StuckTimer = 0.0f;
	bool bIsStuck = false;
	float StuckThreshold = 25.0f;  // Tighter threshold
	float StuckTimeLimit = 0.8f;   // Faster trigger
	float StuckCheckInterval = 0.25f; // More frequent checks
	float LastStuckCheckTime = 0.0f;
	
	// Hallway detection
	int32 HallwayWhiskerCount = 0;
	float HallwayEscapeTimer = 0.0f;
	bool bInHallway = false;
	
	// House tracking with position verification
	bool bPreviouslyInside = false;
	FVector SavedDoorwayLocation = FVector::ZeroVector;
	float HouseExitThreshold = 200.0f;
	
	// Aiming debug
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawAimingDebug = true;
};