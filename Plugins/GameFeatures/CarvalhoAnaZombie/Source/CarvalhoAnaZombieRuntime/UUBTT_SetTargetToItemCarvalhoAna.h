#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_SetTargetToItem.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_SetTargetToItemCarvalhoAna : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_SetTargetToItemCarvalhoAna();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
