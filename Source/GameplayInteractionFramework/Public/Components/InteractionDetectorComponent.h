#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "Types/GameplayInteractionTypes.h"
#include "Interfaces/InteractionTraceStrategy.h"
#include "InteractionDetectorComponent.generated.h"

class UInteractableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFocusedTargetChangedBP, UObject*, NewTarget, const FInteractionPromptData&, PromptData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDetectorInteractionStartedBP, FGameplayTag, ModeTag, UObject*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDetectorInteractionProgressBP, float, ProgressRatio, FGameplayTag, ModeTag, UObject*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDetectorInteractionCompletedBP, UObject*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDetectorInteractionCanceledBP, FGameplayTag, ReasonTag, UObject*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDetectorCandidatesUpdatedBP, const TArray<FInteractionCandidate>&, Candidates);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDetectorInteractionStarted, FGameplayTag /*ModeTag*/, UObject* /*Target*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDetectorInteractionProgress, float /*ProgressRatio*/, FGameplayTag /*ModeTag*/, UObject* /*Target*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDetectorInteractionCompleted, UObject* /*Target*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDetectorInteractionCanceled, FGameplayTag /*ReasonTag*/, UObject* /*Target*/);

/**
 * UInteractionDetectorComponent
 *
 * Component placed on Character or PlayerController to detect nearby interactables and process input.
 * Fully modular and extensible:
 * - Pluggable trace strategies (CameraLine, ForwardCone, SphereSweep, BoxSweep, or Custom IInteractionTraceStrategy)
 * - Directional candidate scoring with configurable weights (Angle Dot, Distance, Priority)
 * - CPU budget throttling (e.g. 30 Hz query interval)
 * - Input state machine: Instant, Hold, RapidTap, Toggle with client-side prediction
 * - Direct Enhanced Input integration or standalone C++/BP invocation
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent, DisplayName = "Interaction Detector Component"))
class GAMEPLAYINTERACTIONFRAMEWORK_API UInteractionDetectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionDetectorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Configuration: Tracing & Strategy ---

	/** Trace shape tag governing default detection raycasts / sweeps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace", meta = (Categories = "Interaction.TraceShape"))
	FGameplayTag TraceShapeTag;

	/** Collision channel used for interaction discovery */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Maximum search distance in centimeters. If <= 0, uses project default */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace", meta = (ClampMin = "0.0"))
	float MaxTraceDistance = 0.0f;

	/** Radius used for sphere sweeps or forward cone apex radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace", meta = (ClampMin = "0.0"))
	float SweepRadius = 40.0f;

	/** Extents used for box sweeps (2.5D or platformer mechanics) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace")
	FVector BoxSweepExtents = FVector(50.0f, 50.0f, 50.0f);

	/** Throttled query interval in seconds (e.g. 0.0333s for ~30 Hz). 0.0f runs every frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Performance", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float QueryInterval = 0.0333f;

	/** Optional custom trace strategy implementing IInteractionTraceStrategy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace")
	TScriptInterface<IInteractionTraceStrategy> CustomTraceStrategy;

	// --- Configuration: Scoring Formula Weights ---

	/** Weight applied to facing alignment dot product (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeightFacingAngle = 0.5f;

	/** Weight applied to proximity / distance (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeightDistance = 0.3f;

	/** Weight applied to target priority (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeightPriority = 0.2f;

	// --- Blueprint Assignable Delegates ---

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnFocusedTargetChangedBP OnFocusedTargetChangedBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnDetectorInteractionStartedBP OnInteractionStartedBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnDetectorInteractionProgressBP OnInteractionProgressBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnDetectorInteractionCompletedBP OnInteractionCompletedBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnDetectorInteractionCanceledBP OnInteractionCanceledBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnDetectorCandidatesUpdatedBP OnCandidatesUpdatedBP;

	// Native C++ Non-Dynamic Delegates (Support AddLambda / Native Callbacks)
	FOnDetectorInteractionStarted OnInteractionStarted;
	FOnDetectorInteractionProgress OnInteractionProgress;
	FOnDetectorInteractionCompleted OnInteractionCompleted;
	FOnDetectorInteractionCanceled OnInteractionCanceled;

	// --- Public API ---

	/** Performs the candidate discovery trace and scores all candidate targets */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Detection")
	void PerformDetection();

	/** Gets the current primary focused interactable target */
	UFUNCTION(BlueprintPure, Category = "Interaction|Detection")
	UObject* GetFocusedTarget() const;

	/** Gets the active smart point index of the current focused candidate */
	UFUNCTION(BlueprintPure, Category = "Interaction|Detection")
	int32 GetFocusedPointIndex() const;

	/** Gets all evaluated candidate targets from the last detection pass */
	UFUNCTION(BlueprintPure, Category = "Interaction|Detection")
	const TArray<FInteractionCandidate>& GetCandidates() const { return CurrentCandidates; }

	/** Calculates composite score for a candidate target */
	UFUNCTION(BlueprintPure, Category = "Interaction|Scoring")
	float CalculateCandidateScore(float Distance, float MaxDistance, float DotAngle, int32 Priority) const;

	// --- Input Execution API (Works standalone or via Enhanced Input) ---

	/** Triggered when the interact button is pressed down */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
	void StartInteract();

	/** Triggered when the interact button is released */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
	void StopInteract();

	/** Triggered on discrete button tap (for RapidTap mode) */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
	void TapInteract();

	/** Cancels the active interaction */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
	void CancelInteract(FGameplayTag ReasonTag);

	/** Handles incoming animation notify or external signal event tag */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
	void HandleInteractionEvent(FGameplayTag EventTag);

	/** Gets current normalized progress (0.0 - 1.0) for active continuous interactions */
	UFUNCTION(BlueprintPure, Category = "Interaction|Input")
	float GetCurrentProgressRatio() const { return CurrentProgressRatio; }

	/** Checks whether an interaction is actively in progress */
	UFUNCTION(BlueprintPure, Category = "Interaction|Input")
	bool IsInteracting() const { return bIsInteracting; }

	/** Gets total detection passes performed (for profiling, debugging, and benchmarks) */
	UFUNCTION(BlueprintPure, Category = "Interaction|Profiling")
	int32 GetDetectionPassCount() const { return DetectionPassCount; }

	/** Resets detection pass counter */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Profiling")
	void ResetDetectionPassCount() { DetectionPassCount = 0; }

	/** Gets active transaction GUID if in progress */
	UFUNCTION(BlueprintPure, Category = "Interaction|Input")
	FGuid GetCurrentTransactionId() const { return CurrentTransactionId; }

	// --- Authoritative Networking RPCs ---

	/** Requests server to initiate interaction with the given target */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StartInteract(UObject* TargetObject, int32 PointIndex, FGuid TransactionId);

	/** Requests server to commit hold interaction completion */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CompleteHoldInteract(UObject* TargetObject, int32 PointIndex, FGuid TransactionId);

	/** Notifies server that interaction was canceled by client */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CancelInteract(UObject* TargetObject, FGameplayTag ReasonTag, FGuid TransactionId);

	/** Client RPC: Server rejected interaction attempt with specific gameplay failure reason */
	UFUNCTION(Client, Reliable)
	void Client_RejectInteraction(UObject* TargetObject, FGameplayTag ReasonTag);

	/** Helper to retrieve the owning local player subsystem for decoupled UI dispatch */
	UFUNCTION(BlueprintPure, Category = "Interaction|UI")
	class UInteractionLocalPlayerSubsystem* GetLocalPlayerSubsystem() const;

protected:
	virtual void BeginPlay() override;

	/** Direct server-authoritative execution without RPC overhead (used on Server / Standalone) */
	void ExecuteServerStartInteract(UObject* TargetObject, int32 PointIndex, FGuid TransactionId);
	void ExecuteServerCompleteHoldInteract(UObject* TargetObject, int32 PointIndex, FGuid TransactionId);

	/** Obtains viewpoint origin and aim rotation (from camera or instigator) */
	void GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	/** Executes default shape traces */
	bool RunDefaultTrace(const FVector& ViewLocation, const FVector& ViewDirection, float Distance, TArray<FHitResult>& OutHits) const;

	/** Processes candidate targets and updates focused target */
	void UpdateFocusedCandidate(const TArray<FInteractionCandidate>& NewCandidates);

	/** Completes the interaction and commits state */
	void CompleteInteraction();

private:
	/** Time accumulated toward next query interval */
	float QueryTimeAccumulator = 0.0f;

	/** List of evaluated candidates from the most recent detection */
	UPROPERTY(Transient)
	TArray<FInteractionCandidate> CurrentCandidates;

	/** The primary nominated target */
	UPROPERTY(Transient)
	FInteractionCandidate FocusedCandidate;

	/** Whether button is currently held */
	bool bIsInteracting = false;

	/** Local hold accumulator in seconds */
	float CurrentHoldTime = 0.0f;

	/** Local accumulated rapid tap count */
	int32 CurrentTapCount = 0;

	/** Time since last tap for decay calculations */
	float TimeSinceLastTap = 0.0f;

	/** Normalized progress ratio (0.0 - 1.0) */
	float CurrentProgressRatio = 0.0f;

	/** Total count of detection passes executed */
	int32 DetectionPassCount = 0;

	/** Active transaction GUID for current interaction session */
	FGuid CurrentTransactionId;
};
