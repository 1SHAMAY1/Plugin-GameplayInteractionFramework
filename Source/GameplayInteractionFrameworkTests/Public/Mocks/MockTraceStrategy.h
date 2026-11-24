#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Interfaces/InteractionTraceStrategy.h"
#include "MockTraceStrategy.generated.h"

/** Mock trace strategy for unit testing pluggable trace provider interface */
UCLASS()
class UMockTraceStrategy : public UObject, public IInteractionTraceStrategy
{
	GENERATED_BODY()

public:
	mutable bool bWasExecuted = false;
	mutable float LastPassedDistance = 0.0f;

	virtual bool ExecuteTrace_Implementation(AActor* InstigatorActor, float MaxDistance, ECollisionChannel TraceChannel, TArray<FHitResult>& OutHits) const override
	{
		bWasExecuted = true;
		LastPassedDistance = MaxDistance;
		return true;
	}
};
