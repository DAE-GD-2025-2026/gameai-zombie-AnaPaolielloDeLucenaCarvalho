#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTT_ShootWeapon.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_ShootWeapon : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_ShootWeapon();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};