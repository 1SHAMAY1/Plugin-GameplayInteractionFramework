#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Interfaces/InteractableTarget.h"
#include "Types/GameplayInteractionTypes.h"
#include "InteractableComponent.generated.h"

/**
 * UInteractableComponent
 * 
 * Modular component placed on any Actor (doors, chests, NPCs, pickups, switches) to make it interactable.
 * Implements IInteractableTarget. Fully replicated with zero multicasts (RepNotify-first architecture),
 * supporting smart sockets, GameplayTag conditions, and custom input modes (Instant, Hold, RapidTap, Toggle).
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent, DisplayName = "Interactable Component"))
class GAMEPLAYINTERACTIONFRAMEWORK_API UInteractableComponent : public UActorComponent, public IInteractableTarget
{
	GENERATED_BODY()

public:
	UInteractableComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Configuration: Prompt & Smart Points ---

	/** Default UI prompt data displayed when player focuses this interactable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Prompt")
	FInteractionPromptData PromptData;

	/** Multi-point interaction sockets / offsets (e.g., driver door, trunk, engine) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SmartPoints")
	TArray<FSmartInteractionPoint> SmartPoints;

	// --- Configuration: Interaction Mode & Timings ---

	/** Tag defining interaction mode (e.g. Interaction.Mode.Instant, Mode.Hold, Mode.RapidTap, Mode.Toggle) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mode", meta = (Categories = "Interaction.Mode"))
	FGameplayTag InteractionModeTag;

	/** Duration in seconds required to complete Hold interactions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mode", meta = (ClampMin = "0.0", EditCondition = "InteractionModeTag.MatchesTag(InteractionNativeTags::Mode_Hold)"))
	float HoldDuration = 1.5f;

	/** Total number of button taps required to complete RapidTap interactions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mode", meta = (ClampMin = "1", EditCondition = "InteractionModeTag.MatchesTag(InteractionNativeTags::Mode_RapidTap)"))
	int32 RapidTapsRequired = 5;

	/** Rate at which accumulated taps decay per second in RapidTap mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mode", meta = (ClampMin = "0.0", EditCondition = "InteractionModeTag.MatchesTag(InteractionNativeTags::Mode_RapidTap)"))
	float TapDecayRate = 1.0f;

	/** Maximum distance in cm allowed for interaction. If <= 0, uses project default */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Distance", meta = (ClampMin = "0.0"))
	float InteractionDistance = 0.0f;

	/** Required facing alignment dot product (e.g., 0.5 for 60-degree cone; 0.0 disables) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Distance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FacingAngleDotThreshold = 0.0f;

	/** Priority weight used to resolve overlapping targets (higher priority takes precedence) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Priority")
	int32 Priority = 0;

	/** Cooldown duration in seconds after interaction before it can be used again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Cooldown", meta = (ClampMin = "0.0"))
	float CooldownDuration = 0.0f;

	/** If true, this component automatically disables itself upon successful completion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Lifecycle")
	bool bIsSingleUse = false;

	// --- Multiplayer Networking & Validation Policies ---

	/** Whether multiple players can interact with this target at the same time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Networking")
	bool bAllowConcurrentInteractions = false;

	/** If true, server and local evaluation perform line-of-sight obstruction traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Validation")
	bool bRequireLineOfSight = true;

	/** Collision channel used for line-of-sight occlusion traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Validation")
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

	/** Maximum time in seconds an interaction lock can be held before automatically expiring (TTL anti-starvation). If <= 0, defaults to HoldDuration + 1.0s or 5.0s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Networking", meta = (ClampMin = "0.0"))
	float InteractionLeaseDuration = 0.0f;

	// --- Cross-Level & Metroidvania Persistence ---

	/** Scope of persistence: shared for entire world or personal to each player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Persistence")
	EInteractionPersistenceScope PersistenceScope = EInteractionPersistenceScope::WorldShared;

	/** Persistent gameplay tag identifying this interactable in the cross-level ledger */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Persistence", meta = (Categories = "Interaction.Persistent"))
	FGameplayTag PersistentStateTag = FGameplayTag::EmptyTag;

	// --- Gameplay Tag Gating ---

	/** Required tags that the instigating actor must satisfy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tags")
	FGameplayTagQuery RequiredInstigatorTags;

	/** Tags on the instigator that will block this interaction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tags")
	FGameplayTagContainer BlockedInstigatorTags;

	/** Dynamic tags currently carried by this interactable component / prop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tags")
	FGameplayTagContainer TargetTags;

	/** Tags on this target that block interaction (e.g., Interaction.State.Locked) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Tags", meta = (Categories = "Interaction.State"))
	FGameplayTagContainer BlockedTargetTags;

	// --- Replication State (RepNotify-First, Zero Multicasts) ---

	/** Whether this component is enabled and ready to interact */
	UPROPERTY(ReplicatedUsing = OnRep_IsInteractable, EditAnywhere, BlueprintReadWrite, Category = "Interaction|State")
	bool bIsInteractable = true;

	/** Authoritative replicated state struct synchronizing active state and instigator */
	UPROPERTY(ReplicatedUsing = OnRep_InteractionState, BlueprintReadOnly, Category = "Interaction|State")
	FInteractionReplicatedState InteractionState;

	// Blueprint Assignable Delegates
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractionStateChangedBP OnInteractionStateChangedBP;

	/** Delegate broadcast when a generic interaction event or AnimNotify triggers */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractionCanceledBP OnInteractionEventBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractionProgressBP OnInteractionProgressBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractionCanceledBP OnInteractionCanceledBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractionFocusChangedBP OnFocusGainedBP;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractionFocusChangedBP OnFocusLostBP;

	// Native C++ Non-Dynamic Delegates (Support AddLambda / Native Callbacks)
	FOnInteractionEvent OnInteractionEvent;
	FOnInteractionCanceled OnInteractionCanceled;

	// --- API & Helpers ---

	/** Gets the effective interaction distance (resolves project default if <= 0) */
	UFUNCTION(BlueprintPure, Category = "Interaction|Distance")
	float GetEffectiveInteractionDistance() const;

	/** Gets the world transform of a smart point by index */
	UFUNCTION(BlueprintPure, Category = "Interaction|SmartPoints")
	FTransform GetSmartPointWorldTransform(int32 PointIndex) const;

	/** Gets the total number of smart interaction points (at least 1 is guaranteed) */
	UFUNCTION(BlueprintPure, Category = "Interaction|SmartPoints")
	int32 GetSmartPointCount() const;

	/** Finds the best/closest smart point index to the given world location */
	UFUNCTION(BlueprintPure, Category = "Interaction|SmartPoints")
	int32 FindClosestSmartPointIndex(const FVector& InteractorLocation) const;

	/** Dispatches an animation or signal event tag into this interactable */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Events")
	void HandleInteractionEvent(FGameplayTag EventTag);

	/** Checks eligibility and returns detailed validation result code */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Validation")
	EInteractionValidationResult EvaluateEligibility(const FInteractionFrameworkContext& Context, FGameplayTagContainer& OutFailureTags) const;

	/** Sets interactable availability (Server authoritative) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|State")
	void SetIsInteractable(bool bNewInteractable);

	/** Sets active interaction state and synchronizes via RepNotify (Server authoritative) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|State")
	void SetInteractionState(FGameplayTag NewStateTag, AActor* Instigator);

	/** Records start time on server for anti-speedhack hold validation */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Networking")
	void RecordServerHoldStart(AActor* Instigator, double StartTimeOverride = -999999.0);

	/** Validates whether client hold elapsed time meets server anti-speedhack requirements */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Networking")
	bool ValidateServerHoldTiming(AActor* Instigator, float RequiredDuration, float& OutElapsedTime) const;

	/** Clears server hold timing record for an instigator */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Networking")
	void ClearServerHoldStart(AActor* Instigator);

	// --- 3-Phase Async Commit API ---

	/** Gets active transaction GUID if in progress */
	UFUNCTION(BlueprintPure, Category = "Interaction|Async")
	FGuid GetActiveTransactionId() const { return ActiveTransactionId; }

	/** Begins asynchronous interaction, entering PendingCommit state and setting lock */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Async")
	bool BeginAsyncInteraction(FGuid TransactionId, AActor* Instigator);

	/** Commits asynchronous interaction on success, transitioning to Completed state */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Async")
	bool CommitAsyncInteraction(FGuid TransactionId, AActor* Instigator);

	/** Rolls back asynchronous interaction on failure, resetting to Available and notifying client */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Async")
	bool RollbackAsyncInteraction(FGuid TransactionId, FGameplayTag ReasonTag, AActor* Instigator);

	/** Handles or forces lease expiration (TTL anti-starvation) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Networking")
	void HandleLeaseExpired();

	/** Handler for instigator actor destruction mid-interaction (crash / disconnect / Alt+F4) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Networking")
	void HandleInstigatorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	/** RepNotify for bIsInteractable */
	UFUNCTION()
	virtual void OnRep_IsInteractable();

	/** RepNotify for InteractionState (Zero Multicasts) */
	UFUNCTION()
	virtual void OnRep_InteractionState(const FInteractionReplicatedState& PreviousState);

	// --- IInteractableTarget Interface Implementation ---

	virtual bool CanInteract_Implementation(const FInteractionFrameworkContext& Context, FGameplayTagContainer& OutFailureTags) const override;
	virtual FInteractionPromptData GetPromptData_Implementation(int32 PointIndex) const override;
	virtual TArray<FSmartInteractionPoint> GetSmartPoints_Implementation() const override;
	virtual void OnInteractionStarted_Implementation(const FInteractionFrameworkContext& Context) override;
	virtual void OnInteractionProgress_Implementation(const FInteractionFrameworkContext& Context, float ProgressRatio) override;
	virtual void OnInteractionCompleted_Implementation(const FInteractionFrameworkContext& Context) override;
	virtual void OnInteractionCanceled_Implementation(const FInteractionFrameworkContext& Context, FGameplayTag ReasonTag) override;
	virtual void OnFocusGained_Implementation(const FInteractionFrameworkContext& Context) override;
	virtual void OnFocusLost_Implementation(const FInteractionFrameworkContext& Context) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Internal handler for cooldown expiration */
	void HandleCooldownExpired();

private:
	FTimerHandle CooldownTimerHandle;
	FTimerHandle LeaseTimerHandle;

	/** Active transaction GUID for async 3-phase commits */
	UPROPERTY(Transient)
	FGuid ActiveTransactionId;

	/** Weak reference to currently monitored instigator for OnEndPlay hook */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentMonitoredInstigator = nullptr;

	/** Server timestamps per instigator for hold timing validation */
	TMap<TWeakObjectPtr<AActor>, double> ServerHoldStartTimes;
};
