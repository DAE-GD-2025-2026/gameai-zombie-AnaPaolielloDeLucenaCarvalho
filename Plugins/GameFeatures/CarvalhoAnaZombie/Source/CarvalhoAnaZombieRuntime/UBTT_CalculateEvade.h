#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_CalculateEvade.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_CalculateEvade : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_CalculateEvade();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};