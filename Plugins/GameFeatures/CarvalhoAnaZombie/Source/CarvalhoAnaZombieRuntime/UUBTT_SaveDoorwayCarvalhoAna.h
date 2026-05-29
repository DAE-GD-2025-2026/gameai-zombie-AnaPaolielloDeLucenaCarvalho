#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_SaveDoorway.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_SaveDoorwayCarvalhoAna : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_SaveDoorwayCarvalhoAna();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};