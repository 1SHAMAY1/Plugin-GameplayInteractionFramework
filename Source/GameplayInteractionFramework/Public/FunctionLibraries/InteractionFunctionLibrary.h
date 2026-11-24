#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Types/GameplayInteractionTypes.h"
#include "InteractionFunctionLibrary.generated.h"

/**
 * UInteractionFunctionLibrary
 *
 * Blueprint function library offering pure mathematical and utility helpers
 * for scoring equations, spatial closest-point lookups, and gameplay tag validation.
 */
UCLASS()
class GAMEPLAYINTERACTIONFRAMEWORK_API UInteractionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Calculates directional candidate score using facing dot, distance, and priority weights */
	UFUNCTION(BlueprintPure, Category = "Interaction|Math")
	static float CalculateDirectionalScore(float Distance, float MaxDistance, float DotAngle, int32 Priority, float WeightDot = 0.5f, float WeightDist = 0.3f, float WeightPriority = 0.2f);

	/** Evaluates whether a given actor satisfies required and blocked tag criteria */
	UFUNCTION(BlueprintPure, Category = "Interaction|Tags")
	static bool EvaluateActorTags(const AActor* Actor, const FGameplayTagQuery& RequiredTags, const FGameplayTagContainer& BlockedTags);

	/** Finds the index of the closest point in an array to a query location */
	UFUNCTION(BlueprintPure, Category = "Interaction|Math")
	static int32 FindClosestPointIndex(const TArray<FVector>& Points, const FVector& QueryLocation);

	/** Formats a validation failure result into a user-facing or debug localized text */
	UFUNCTION(BlueprintPure, Category = "Interaction|Validation")
	static FText GetValidationResultDescription(EInteractionValidationResult Result);
};
