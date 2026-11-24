#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "GameplayInteractionSettings.generated.h"

/**
 * Global project settings for Gameplay Interaction Framework.
 * Configurable via Project Settings -> Game -> Gameplay Interaction Framework.
 * Ensures zero hard-coding of default values across the framework.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Gameplay Interaction Framework"))
class GAMEPLAYINTERACTIONFRAMEWORK_API UGameplayInteractionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGameplayInteractionSettings();

	/** Gets the static settings instance */
	static const UGameplayInteractionSettings* Get();

	// --- Trace & Collision Defaults ---

	/** Default collision channel used for interaction traces */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Tracing")
	TEnumAsByte<ECollisionChannel> DefaultTraceChannel = ECC_Visibility;

	/** Default query interval for detector components in seconds (e.g. 0.0333s for ~30 Hz) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Tracing", meta = (ClampMin = "0.001", ClampMax = "0.5"))
	float DefaultQueryInterval = 0.0333f;

	/** Default maximum interaction distance in centimeters */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Tracing", meta = (ClampMin = "10.0", ClampMax = "10000.0"))
	float DefaultInteractionDistance = 250.0f;

	// --- Candidate Scoring Formula Weights ---

	/** Scoring weight for facing alignment / crosshair dot product (0.0 - 1.0) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeightFacingAngle = 0.5f;

	/** Scoring weight for proximity / distance to target (0.0 - 1.0) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeightDistance = 0.3f;

	/** Scoring weight for designer-specified priority integer (0.0 - 1.0) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeightPriority = 0.2f;

	// --- Multiplayer Authoritative Network Tolerances ---

	/** Movement buffer in centimeters added to server distance checks to account for network ping */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Multiplayer Tolerances", meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float NetworkDistanceTolerance = 50.0f;

	/** Timing buffer in seconds subtracted from required hold duration on server to account for ping */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Multiplayer Tolerances", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float NetworkHoldTimeTolerance = 0.15f;

	// --- Default Gameplay Tags ---

	/** Default interaction mode tag */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Tags", meta = (Categories = "Interaction.Mode"))
	FGameplayTag DefaultInteractionModeTag;

	/** Default trace shape tag */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Tags", meta = (Categories = "Interaction.TraceShape"))
	FGameplayTag DefaultTraceShapeTag;

	// --- Debugging ---

	/** Enables visual debug rays for interaction traces */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebugTraces = false;

	/** Enables candidate score annotations in world */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebugCandidates = false;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
