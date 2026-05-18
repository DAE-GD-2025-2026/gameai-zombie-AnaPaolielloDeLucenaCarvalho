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

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	float WanderAngle = 0.0f;
	float WanderDistance = 500.0f;
	float WanderRadius = 300.0f;
	float WanderJitter = 50.0f; 
};