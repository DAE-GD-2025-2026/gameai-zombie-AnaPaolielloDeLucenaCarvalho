#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_CalculateWander.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_CalculateWander : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_CalculateWander();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};