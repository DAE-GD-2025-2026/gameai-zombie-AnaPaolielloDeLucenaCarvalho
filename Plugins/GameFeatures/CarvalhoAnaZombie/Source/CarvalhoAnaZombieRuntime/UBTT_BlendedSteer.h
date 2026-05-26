#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_BlendedSteer.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_BlendedSteer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UUBTT_BlendedSteer();
	
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
	float WanderAngle = 0.0f;
	float WanderTargetAngle = 0.0f;
	float WanderTimer = 0.0f;
};