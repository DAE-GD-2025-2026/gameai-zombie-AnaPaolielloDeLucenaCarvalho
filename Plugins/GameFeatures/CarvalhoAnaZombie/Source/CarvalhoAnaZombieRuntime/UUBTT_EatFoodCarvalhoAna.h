#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UUBTT_EatFoodCarvalhoAna.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_EatFoodCarvalhoAna : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_EatFoodCarvalhoAna();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};