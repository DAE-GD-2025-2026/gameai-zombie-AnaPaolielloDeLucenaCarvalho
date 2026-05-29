#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_Heal.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_HealCarvalhoAna : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_HealCarvalhoAna();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};