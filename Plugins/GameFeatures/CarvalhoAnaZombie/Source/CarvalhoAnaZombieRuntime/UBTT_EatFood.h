#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_EatFood.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_EatFood : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_EatFood();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};