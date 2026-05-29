#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UUBTT_ClearMemoryCarvalhoAna.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_ClearMemoryCarvalhoAna : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_ClearMemoryCarvalhoAna();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector KeyToClear;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};