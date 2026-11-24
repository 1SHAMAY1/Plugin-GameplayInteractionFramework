#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/HitResult.h"
#include "Engine/EngineTypes.h"
#include "InteractionTraceStrategy.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractionTraceStrategy : public UInterface
{
	GENERATED_BODY()
};

/**
 * Strategy interface allowing developers to supply custom trace algorithms and origins.
 * Provides the framework foundation for developer-controlled tracing (Camera, Bones, VR Hand, Mouse Cursor).
 */
class GAMEPLAYINTERACTIONFRAMEWORK_API IInteractionTraceStrategy
{
	GENERATED_BODY()

public:
	/**
	 * Executes a trace to discover candidate interactable targets.
	 * @param InstigatorActor The actor performing the trace (character, pawn, controller, vehicle).
	 * @param MaxDistance Maximum trace distance.
	 * @param TraceChannel Collision channel to query against.
	 * @param OutHits Array of hit results returned by the trace query.
	 * @return True if one or more potential hit targets were detected.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Trace")
	bool ExecuteTrace(AActor* InstigatorActor, float MaxDistance, ECollisionChannel TraceChannel, TArray<FHitResult>& OutHits) const;
};
