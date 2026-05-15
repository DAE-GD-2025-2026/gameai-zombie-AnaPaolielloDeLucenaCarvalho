#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_ClearMemory.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_ClearMemory : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UUBTT_ClearMemory();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};