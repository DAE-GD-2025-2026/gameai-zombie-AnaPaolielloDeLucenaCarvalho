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
	FVector GetPathSeekForce(APawn* Pawn, const FVector& TargetLoc, float DeltaSeconds);

private:
	// wander tracking (navigation-based)
	float WanderAngle = 0.0f;
	float WanderTimer = 0.0f;
	FVector LastWanderTarget = FVector::ZeroVector;
	float WanderTargetReachedDistance = 200.0f;
	float TimeSinceWanderStart = 0.0f;

	TArray<FVector> CurrentPath;
	int32 CurrentPathIndex = 0;
	float PathUpdateTimer  = 999.f;
	FVector LastSeekTarget = FVector::ZeroVector;
	
	// stuck detection
	FVector LastPosition = FVector::ZeroVector;
	FVector LastReportedPosition = FVector::ZeroVector;
	float StuckTimer = 0.0f;
	bool bIsStuck = false;
	float StuckThreshold = 25.0f; // distance moved threshold to consider stuck
	float StuckTimeLimit = 0.8f;
	float StuckCheckInterval = 0.25f; // frequent checks
	float LastStuckCheckTime = 0.0f;
	
	// hallway detection
	int32 HallwayWhiskerCount = 0;
	float HallwayEscapeTimer = 0.0f;
	bool bInHallway = false;
	
	// house tracking
	bool bPreviouslyInside = false;
	FVector SavedDoorwayLocation = FVector::ZeroVector;
	float HouseExitThreshold = 200.0f;
	
	// seek force
	FVector LastSeekForce = FVector::ZeroVector;

	// aiming debug
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawAimingDebug = true;
};