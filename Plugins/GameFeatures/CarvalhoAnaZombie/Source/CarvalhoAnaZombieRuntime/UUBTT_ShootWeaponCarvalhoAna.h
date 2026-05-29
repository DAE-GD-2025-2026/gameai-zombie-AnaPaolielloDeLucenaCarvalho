#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UUBTT_ShootWeaponCarvalhoAna.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UUBTT_ShootWeaponCarvalhoAna : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTT_ShootWeaponCarvalhoAna();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};