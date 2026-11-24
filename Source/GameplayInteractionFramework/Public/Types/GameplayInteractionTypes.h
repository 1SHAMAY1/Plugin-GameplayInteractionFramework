#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameplayInteractionTypes.generated.h"

/** Validation result for interaction feasibility checks */
UENUM(BlueprintType)
enum class EInteractionValidationResult : uint8
{
	Valid UMETA(DisplayName = "Valid"),
	OutOfRange UMETA(DisplayName = "Out Of Range"),
	ObstructedLineOfSight UMETA(DisplayName = "Obstructed Line Of Sight"),
	MissingRequiredTags UMETA(DisplayName = "Missing Required Tags"),
	BlockedByTags UMETA(DisplayName = "Blocked By Tags"),
	OnCooldown UMETA(DisplayName = "On Cooldown"),
	Locked UMETA(DisplayName = "Locked"),
	AlreadyInteracted UMETA(DisplayName = "Already Interacted"),
	TargetBusy UMETA(DisplayName = "Target Busy"),
	TimingViolation UMETA(DisplayName = "Timing Violation"),
	PendingAsync UMETA(DisplayName = "Pending Async"),
	TimedOut UMETA(DisplayName = "Timed Out"),
	CustomPolicyFailed UMETA(DisplayName = "Custom Policy Failed")
};

/** UI display information for an interactable target or point */
USTRUCT(BlueprintType)
struct GAMEPLAYINTERACTIONFRAMEWORK_API FInteractionPromptData
{
	GENERATED_BODY()

	/** Primary action verb (e.g. "Open", "Talk", "Pick up", "Hack") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Prompt")
	FText ActionVerb = FText::GetEmpty();

	/** Display title or name of the target (e.g. "Reinforced Door", "Vendor NPC", "Terminal") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Prompt")
	FText DisplayTitle = FText::GetEmpty();

	/** Secondary context or status hint (e.g. "Requires Level 2 Security Pass", "Hold [E]") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Prompt")
	FText SecondaryDescription = FText::GetEmpty();

	/** Optional icon texture for the prompt */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Prompt")
	TSoftObjectPtr<UTexture2D> PromptIcon = nullptr;

	/** Optional gameplay tag for custom UI styling / icon resolution */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Prompt", meta = (Categories = "Interaction.Prompt"))
	FGameplayTag CustomPromptTag = FGameplayTag::EmptyTag;

	FInteractionPromptData()
		: ActionVerb(NSLOCTEXT("Interaction", "DefaultVerb", "Interact"))
		, DisplayTitle(NSLOCTEXT("Interaction", "DefaultTitle", "Object"))
	{
	}
};

/** Defines a smart interaction point on a multi-point prop (e.g., driver door vs trunk) */
USTRUCT(BlueprintType)
struct GAMEPLAYINTERACTIONFRAMEWORK_API FSmartInteractionPoint
{
	GENERATED_BODY()

	/** Socket or bone name to attach to. If None, RelativeOffset is used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SmartPoint")
	FName SocketOrBoneName = NAME_None;

	/** Local space offset if socket is None, or relative to the socket */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SmartPoint")
	FVector RelativeOffset = FVector::ZeroVector;

	/** Identification tag for this point (e.g., Interaction.Point.DriverDoor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SmartPoint", meta = (Categories = "Interaction.Point"))
	FGameplayTag PointTag = FGameplayTag::EmptyTag;

	/** Custom prompt data override for this specific point. If empty, uses parent prompt */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SmartPoint")
	FInteractionPromptData PromptOverride;

	/** Override interaction distance for this point. <= 0.0f uses parent component distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SmartPoint", meta = (ClampMin = "0.0"))
	float DistanceOverride = 0.0f;

	/** Optional dot product angle requirement for this point. 0.0f disables angle check */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SmartPoint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FacingDotThreshold = 0.0f;
};

/** Context bundle passed through interaction calls, validation, and delegates */
USTRUCT(BlueprintType)
struct GAMEPLAYINTERACTIONFRAMEWORK_API FInteractionFrameworkContext
{
	GENERATED_BODY()

	/** Instigating actor (character, pawn, or vehicle) */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction|Context")
	TWeakObjectPtr<AActor> InstigatorActor = nullptr;

	/** Controller of the instigating actor */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction|Context")
	TWeakObjectPtr<AController> InstigatorController = nullptr;

	/** Target object implementing IInteractableTarget */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction|Context")
	TWeakObjectPtr<UObject> TargetObject = nullptr;

	/** Selected smart interaction point index (0 is default/primary) */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction|Context")
	int32 SelectedPointIndex = 0;

	/** Impact point in world space from trace */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction|Context")
	FVector HitLocation = FVector::ZeroVector;

	/** Surface normal at impact point */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction|Context")
	FVector HitNormal = FVector::UpVector;

	/** Client timestamp when interaction was initiated */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction|Context")
	float ClientTimestamp = 0.0f;

	/** Unique transaction identifier for idempotent async RPC processing */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction|Context")
	FGuid TransactionId = FGuid();
};

/** Evaluated candidate interactable target scored by distance, angle, and priority */
USTRUCT(BlueprintType)
struct GAMEPLAYINTERACTIONFRAMEWORK_API FInteractionCandidate
{
	GENERATED_BODY()

	/** Target object */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Candidate")
	TWeakObjectPtr<UObject> TargetObject = nullptr;

	/** Selected smart point index */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Candidate")
	int32 SelectedPointIndex = 0;

	/** World location of the evaluated point */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Candidate")
	FVector WorldLocation = FVector::ZeroVector;

	/** Distance from interactor viewpoint to target point */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Candidate")
	float Distance = 0.0f;

	/** Dot product between interactor aim/facing vector and direction to target */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Candidate")
	float DotAngle = 0.0f;

	/** Priority weight of the target */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Candidate")
	int32 Priority = 0;

	/** Computed composite score (higher score is better) */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Candidate")
	float Score = 0.0f;

	bool operator<(const FInteractionCandidate& Other) const
	{
		return Score > Other.Score; // Higher score sorts first
	}
};

/** Replicated state struct driven via RepNotify with change counter (Zero Multicasts) */
USTRUCT(BlueprintType)
struct GAMEPLAYINTERACTIONFRAMEWORK_API FInteractionReplicatedState
{
	GENERATED_BODY()

	/** Active state tag (e.g. Interaction.State.Available, Active, Completed, Cooldown) */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	FGameplayTag StateTag = FGameplayTag::EmptyTag;

	/** Current or most recent instigator actor */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	TWeakObjectPtr<AActor> Instigator = nullptr;

	/** Incremented on each state transition to ensure OnRep executes even on repeated states */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	uint8 ChangeCounter = 0;

	/** Server world timestamp when current state was committed */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	float ServerStartTime = 0.0f;
};

/** Persistence scope for cross-level and Metroidvania state synchronization */
UENUM(BlueprintType)
enum class EInteractionPersistenceScope : uint8
{
	WorldShared UMETA(DisplayName = "World Shared"),
	InstigatorScoped UMETA(DisplayName = "Instigator Scoped")
};

/** Record stored in persistent cross-level ledger */
USTRUCT(BlueprintType)
struct GAMEPLAYINTERACTIONFRAMEWORK_API FInteractionPersistentRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Persistence")
	FGameplayTag StateTag = FGameplayTag::EmptyTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Persistence")
	FString CompletedByPlayerId = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Persistence")
	double CompletionTimestamp = 0.0;
};

/** Wrapper struct to allow nested TMaps in Unreal reflection */
USTRUCT(BlueprintType)
struct GAMEPLAYINTERACTIONFRAMEWORK_API FInteractionPersistentRecordMap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Persistence")
	TMap<FGameplayTag, FInteractionPersistentRecord> Records;
};

// Blueprint Delegate Signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionStateChangedBP, FGameplayTag, NewState, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInteractionProgressBP, float, ProgressRatio, FGameplayTag, ModeTag, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionCanceledBP, FGameplayTag, ReasonTag, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionFocusChangedBP, AActor*, Interactor);

// Native C++ Multicast Delegate Signatures
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInteractionEvent, FGameplayTag /*EventTag*/, AActor* /*Instigator*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInteractionCanceled, FGameplayTag /*ReasonTag*/, AActor* /*Instigator*/);
