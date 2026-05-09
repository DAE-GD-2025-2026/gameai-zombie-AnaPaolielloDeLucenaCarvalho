#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_PickupItem.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_PickupItem : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UUBTT_PickupItem();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};