#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UUBTT_PickupItemCarvalhoAna.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_PickupItemCarvalhoAna : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UUBTT_PickupItemCarvalhoAna();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	void ReScanRoomForNextItem(class UInventoryComponent* Inv, class UBlackboardComponent* BB, APawn* Pawn);
};