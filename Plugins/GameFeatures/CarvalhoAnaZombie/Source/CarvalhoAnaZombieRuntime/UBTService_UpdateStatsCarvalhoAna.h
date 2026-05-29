#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "UBTService_UpdateStatsCarvalhoAna.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UBTService_UpdateStatsCarvalhoAna : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateStatsCarvalhoAna();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};