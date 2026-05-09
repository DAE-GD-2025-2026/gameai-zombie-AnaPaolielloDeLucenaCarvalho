#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "UBTService_UpdateStats.generated.h"

UCLASS()
class CARVALHOANAZOMBIERUNTIME_API UBTService_UpdateStats : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateStats();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};