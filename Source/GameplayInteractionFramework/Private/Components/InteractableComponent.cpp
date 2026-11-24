#include "Components/InteractableComponent.h"
#include "Settings/GameplayInteractionSettings.h"
#include "Types/InteractionNativeTags.h"
#include "Interfaces/InteractionInstigator.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Subsystems/InteractionWorldSubsystem.h"
#include "Subsystems/InteractionPersistenceSubsystem.h"
#include "Engine/GameInstance.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	InteractionModeTag = InteractionNativeTags::Mode_Instant;
	InteractionState.StateTag = InteractionNativeTags::State_Available;
	InteractionState.ChangeCounter = 0;
	InteractionState.ServerStartTime = 0.0f;
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInteractableComponent, bIsInteractable);
	DOREPLIFETIME(UInteractableComponent, InteractionState);
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize default state
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InteractionState.StateTag = bIsInteractable ? InteractionNativeTags::State_Available : InteractionNativeTags::State_Disabled;

		// Stream-in hydration: if persistent state tag is valid, restore state from persistent ledger
		if (PersistentStateTag.IsValid())
		{
			if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
			{
				if (UInteractionPersistenceSubsystem* PersistenceSubsystem = GI->GetSubsystem<UInteractionPersistenceSubsystem>())
				{
					FGameplayTag SavedState;
					if (PersistenceSubsystem->GetPersistentState(PersistentStateTag, SavedState, TEXT(""), PersistenceScope))
					{
						if (SavedState == InteractionNativeTags::State_Completed.GetTag())
						{
							if (bIsSingleUse)
							{
								bIsInteractable = false;
								InteractionState.StateTag = InteractionNativeTags::State_Disabled.GetTag();
							}
							else
							{
								InteractionState.StateTag = InteractionNativeTags::State_Completed.GetTag();
							}
						}
					}
				}
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (UInteractionWorldSubsystem* Subsystem = World->GetSubsystem<UInteractionWorldSubsystem>())
		{
			Subsystem->RegisterInteractable(this);
		}
	}
}

void UInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CooldownTimerHandle);
		World->GetTimerManager().ClearTimer(LeaseTimerHandle);

		if (UInteractionWorldSubsystem* Subsystem = World->GetSubsystem<UInteractionWorldSubsystem>())
		{
			Subsystem->UnregisterInteractable(this);
		}
	}

	if (CurrentMonitoredInstigator.IsValid())
	{
		CurrentMonitoredInstigator->OnEndPlay.RemoveDynamic(this, &UInteractableComponent::HandleInstigatorEndPlay);
		CurrentMonitoredInstigator = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

float UInteractableComponent::GetEffectiveInteractionDistance() const
{
	if (InteractionDistance > 0.0f)
	{
		return InteractionDistance;
	}

	if (const UGameplayInteractionSettings* Settings = UGameplayInteractionSettings::Get())
	{
		return Settings->DefaultInteractionDistance;
	}

	return 250.0f;
}

int32 UInteractableComponent::GetSmartPointCount() const
{
	return SmartPoints.Num() > 0 ? SmartPoints.Num() : 1;
}

FTransform UInteractableComponent::GetSmartPointWorldTransform(int32 PointIndex) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return FTransform::Identity;
	}

	if (SmartPoints.IsValidIndex(PointIndex))
	{
		const FSmartInteractionPoint& Point = SmartPoints[PointIndex];
		if (Point.SocketOrBoneName != NAME_None)
		{
			// Try finding the socket on the root or child scene components
			if (USceneComponent* RootComp = OwnerActor->GetRootComponent())
			{
				if (RootComp->DoesSocketExist(Point.SocketOrBoneName))
				{
					FTransform SocketTransform = RootComp->GetSocketTransform(Point.SocketOrBoneName);
					if (!Point.RelativeOffset.IsZero())
					{
						SocketTransform.AddToTranslation(SocketTransform.TransformVector(Point.RelativeOffset));
					}
					return SocketTransform;
				}
			}

			// Search all scene components on the owner
			TArray<USceneComponent*> SceneComps;
			OwnerActor->GetComponents(SceneComps);
			for (USceneComponent* Comp : SceneComps)
			{
				if (Comp && Comp->DoesSocketExist(Point.SocketOrBoneName))
				{
					FTransform SocketTransform = Comp->GetSocketTransform(Point.SocketOrBoneName);
					if (!Point.RelativeOffset.IsZero())
					{
						SocketTransform.AddToTranslation(SocketTransform.TransformVector(Point.RelativeOffset));
					}
					return SocketTransform;
				}
			}
		}

		// Fallback: actor transform with relative offset
		FTransform ActorTransform = OwnerActor->GetActorTransform();
		if (!Point.RelativeOffset.IsZero())
		{
			ActorTransform.AddToTranslation(ActorTransform.TransformVector(Point.RelativeOffset));
		}
		return ActorTransform;
	}

	return OwnerActor->GetActorTransform();
}

int32 UInteractableComponent::FindClosestSmartPointIndex(const FVector& InteractorLocation) const
{
	if (SmartPoints.Num() <= 1)
	{
		return 0;
	}

	int32 BestIndex = 0;
	float BestDistSq = MAX_flt;

	for (int32 i = 0; i < SmartPoints.Num(); ++i)
	{
		const FTransform PointTransform = GetSmartPointWorldTransform(i);
		const float DistSq = FVector::DistSquared(PointTransform.GetLocation(), InteractorLocation);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}

	return BestIndex;
}

EInteractionValidationResult UInteractableComponent::EvaluateEligibility(const FInteractionFrameworkContext& Context, FGameplayTagContainer& OutFailureTags) const
{
	if (!bIsInteractable)
	{
		OutFailureTags.AddTag(InteractionNativeTags::State_Disabled);
		return EInteractionValidationResult::BlockedByTags;
	}

	if (InteractionState.StateTag == InteractionNativeTags::State_Cooldown)
	{
		OutFailureTags.AddTag(InteractionNativeTags::State_Cooldown);
		return EInteractionValidationResult::OnCooldown;
	}

	if (InteractionState.StateTag == InteractionNativeTags::State_Locked)
	{
		OutFailureTags.AddTag(InteractionNativeTags::State_Locked);
		return EInteractionValidationResult::Locked;
	}

	if (bIsSingleUse && InteractionState.StateTag == InteractionNativeTags::State_Completed)
	{
		OutFailureTags.AddTag(InteractionNativeTags::CancelReason_AlreadyInteracted);
		return EInteractionValidationResult::AlreadyInteracted;
	}

	// Contention check: if not concurrent, ensure another player is not actively holding / interacting
	if (!bAllowConcurrentInteractions && (InteractionState.StateTag == InteractionNativeTags::State_Active.GetTag() || InteractionState.StateTag == InteractionNativeTags::State_PendingCommit.GetTag()))
	{
		if (Context.InstigatorActor.IsValid() && InteractionState.Instigator.IsValid() && InteractionState.Instigator != Context.InstigatorActor)
		{
			OutFailureTags.AddTag(InteractionNativeTags::CancelReason_TargetBusy);
			return EInteractionValidationResult::TargetBusy;
		}
	}

	// Target-level blocked tags check
	if (!BlockedTargetTags.IsEmpty() && TargetTags.HasAny(BlockedTargetTags))
	{
		OutFailureTags.AppendTags(BlockedTargetTags);
		return EInteractionValidationResult::BlockedByTags;
	}

	// Instigator-level tags check
	if (Context.InstigatorActor.IsValid())
	{
		FGameplayTagContainer InstigatorTags;
		if (Context.InstigatorActor->Implements<UInteractionInstigator>())
		{
			InstigatorTags = IInteractionInstigator::Execute_GetInstigatorTags(Context.InstigatorActor.Get());
		}

		// Blocked tags on instigator
		if (!BlockedInstigatorTags.IsEmpty() && InstigatorTags.HasAny(BlockedInstigatorTags))
		{
			OutFailureTags.AppendTags(BlockedInstigatorTags);
			return EInteractionValidationResult::BlockedByTags;
		}

		// Required tags on instigator
		if (!RequiredInstigatorTags.IsEmpty() && !RequiredInstigatorTags.Matches(InstigatorTags))
		{
			OutFailureTags.AddTag(InteractionNativeTags::CancelReason_Interrupted);
			return EInteractionValidationResult::MissingRequiredTags;
		}

		// Distance check
		const FTransform TargetTransform = GetSmartPointWorldTransform(Context.SelectedPointIndex);
		const float Dist = FVector::Dist(TargetTransform.GetLocation(), Context.InstigatorActor->GetActorLocation());

		float MaxDist = GetEffectiveInteractionDistance();
		if (SmartPoints.IsValidIndex(Context.SelectedPointIndex) && SmartPoints[Context.SelectedPointIndex].DistanceOverride > 0.0f)
		{
			MaxDist = SmartPoints[Context.SelectedPointIndex].DistanceOverride;
		}

		// Add network tolerance buffer if running authoritative server check
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			if (const UGameplayInteractionSettings* Settings = UGameplayInteractionSettings::Get())
			{
				MaxDist += Settings->NetworkDistanceTolerance;
			}
		}

		if (Dist > MaxDist)
		{
			OutFailureTags.AddTag(InteractionNativeTags::CancelReason_OutOfRange);
			return EInteractionValidationResult::OutOfRange;
		}

		// Line of Sight Occlusion Check
		if (bRequireLineOfSight)
		{
			FVector TraceStart = Context.InstigatorActor->GetActorLocation();
			if (Context.InstigatorActor->Implements<UInteractionInstigator>())
			{
				FRotator UnusedRot;
				IInteractionInstigator::Execute_GetInteractionViewPoint(Context.InstigatorActor.Get(), TraceStart, UnusedRot);
			}

			const FVector TraceEnd = TargetTransform.GetLocation();
			FHitResult HitResult;
			FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(InteractionOcclusionTrace), false, Context.InstigatorActor.Get());
			if (GetOwner())
			{
				TraceParams.AddIgnoredActor(GetOwner());
			}

			if (UWorld* World = GetWorld())
			{
				const bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, OcclusionTraceChannel, TraceParams);
				if (bHit && HitResult.bBlockingHit && HitResult.GetActor() != GetOwner())
				{
					OutFailureTags.AddTag(InteractionNativeTags::CancelReason_LineOfSightLost);
					return EInteractionValidationResult::ObstructedLineOfSight;
				}
			}
		}
	}

	return EInteractionValidationResult::Valid;
}

void UInteractableComponent::RecordServerHoldStart(AActor* Instigator, double StartTimeOverride)
{
	if (!Instigator || !GetWorld())
	{
		return;
	}

	const double StartTime = (StartTimeOverride > -999998.0) ? StartTimeOverride : GetWorld()->GetTimeSeconds();
	ServerHoldStartTimes.Add(Instigator, StartTime);
}

bool UInteractableComponent::ValidateServerHoldTiming(AActor* Instigator, float RequiredDuration, float& OutElapsedTime) const
{
	OutElapsedTime = 0.0f;
	if (!Instigator || !GetWorld())
	{
		return false;
	}

	const double* FoundStartTime = ServerHoldStartTimes.Find(Instigator);
	if (!FoundStartTime)
	{
		return false;
	}

	OutElapsedTime = static_cast<float>(GetWorld()->GetTimeSeconds() - *FoundStartTime);

	float PingSeconds = 0.0f;
	if (APlayerController* PC = Cast<APlayerController>(Instigator))
	{
		if (PC->PlayerState)
		{
			PingSeconds = PC->PlayerState->GetPingInMilliseconds() * 0.001f;
		}
	}
	else if (APawn* Pawn = Cast<APawn>(Instigator))
	{
		if (APlayerController* PawnPC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (PawnPC->PlayerState)
			{
				PingSeconds = PawnPC->PlayerState->GetPingInMilliseconds() * 0.001f;
			}
		}
	}

	float Tolerance = 0.15f;
	if (const UGameplayInteractionSettings* Settings = UGameplayInteractionSettings::Get())
	{
		Tolerance = Settings->NetworkHoldTimeTolerance;
	}

	const float MinimumAllowedTime = FMath::Max(0.0f, RequiredDuration - (PingSeconds + Tolerance));
	return OutElapsedTime >= MinimumAllowedTime;
}

void UInteractableComponent::ClearServerHoldStart(AActor* Instigator)
{
	if (Instigator)
	{
		ServerHoldStartTimes.Remove(Instigator);
	}
}

void UInteractableComponent::SetIsInteractable(bool bNewInteractable)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (bIsInteractable != bNewInteractable)
	{
		bIsInteractable = bNewInteractable;
		if (!bIsInteractable)
		{
			SetInteractionState(InteractionNativeTags::State_Disabled, nullptr);
		}
		else
		{
			SetInteractionState(InteractionNativeTags::State_Available, nullptr);
		}
	}
}

void UInteractableComponent::SetInteractionState(FGameplayTag NewStateTag, AActor* Instigator)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const FInteractionReplicatedState PreviousState = InteractionState;

	InteractionState.StateTag = NewStateTag;
	InteractionState.Instigator = Instigator;
	InteractionState.ChangeCounter++;
	InteractionState.ServerStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// On Listen Server or Standalone, RepNotify does not execute automatically for host
	const ENetMode NetMode = GetOwner()->GetNetMode();
	if (NetMode == NM_Standalone || NetMode == NM_ListenServer)
	{
		OnRep_InteractionState(PreviousState);
	}
}

void UInteractableComponent::OnRep_IsInteractable()
{
	if (!bIsInteractable)
	{
		OnInteractionCanceledBP.Broadcast(InteractionNativeTags::CancelReason_Interrupted, nullptr);
	}
}

void UInteractableComponent::OnRep_InteractionState(const FInteractionReplicatedState& PreviousState)
{
	AActor* InstigatorActor = InteractionState.Instigator.Get();
	OnInteractionStateChangedBP.Broadcast(InteractionState.StateTag, InstigatorActor);
}

void UInteractableComponent::HandleCooldownExpired()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (bIsInteractable)
		{
			SetInteractionState(InteractionNativeTags::State_Available, nullptr);
		}
	}
}

// --- IInteractableTarget Interface Implementation ---

bool UInteractableComponent::CanInteract_Implementation(const FInteractionFrameworkContext& Context, FGameplayTagContainer& OutFailureTags) const
{
	return EvaluateEligibility(Context, OutFailureTags) == EInteractionValidationResult::Valid;
}

FInteractionPromptData UInteractableComponent::GetPromptData_Implementation(int32 PointIndex) const
{
	if (SmartPoints.IsValidIndex(PointIndex))
	{
		const FSmartInteractionPoint& Point = SmartPoints[PointIndex];
		if (!Point.PromptOverride.DisplayTitle.IsEmpty())
		{
			return Point.PromptOverride;
		}
	}

	return PromptData;
}

TArray<FSmartInteractionPoint> UInteractableComponent::GetSmartPoints_Implementation() const
{
	return SmartPoints;
}

void UInteractableComponent::OnInteractionStarted_Implementation(const FInteractionFrameworkContext& Context)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetInteractionState(InteractionNativeTags::State_Active, Context.InstigatorActor.Get());

		// Start lease timer (TTL anti-starvation)
		if (GetWorld())
		{
			const float EffectiveLease = InteractionLeaseDuration > 0.0f ? InteractionLeaseDuration : (HoldDuration > 0.0f ? (HoldDuration + 1.0f) : 5.0f);
			GetWorld()->GetTimerManager().SetTimer(LeaseTimerHandle, this, &UInteractableComponent::HandleLeaseExpired, EffectiveLease, false);
		}

		// Hook instigator destruction (crash / disconnect / Alt+F4)
		if (Context.InstigatorActor.IsValid())
		{
			if (CurrentMonitoredInstigator.IsValid() && CurrentMonitoredInstigator != Context.InstigatorActor)
			{
				CurrentMonitoredInstigator->OnEndPlay.RemoveDynamic(this, &UInteractableComponent::HandleInstigatorEndPlay);
			}
			CurrentMonitoredInstigator = Context.InstigatorActor;
			CurrentMonitoredInstigator->OnEndPlay.AddDynamic(this, &UInteractableComponent::HandleInstigatorEndPlay);
		}
	}
}

void UInteractableComponent::OnInteractionProgress_Implementation(const FInteractionFrameworkContext& Context, float ProgressRatio)
{
	OnInteractionProgressBP.Broadcast(ProgressRatio, InteractionModeTag, Context.InstigatorActor.Get());
}

void UInteractableComponent::OnInteractionCompleted_Implementation(const FInteractionFrameworkContext& Context)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LeaseTimerHandle);
	}

	if (CurrentMonitoredInstigator.IsValid())
	{
		CurrentMonitoredInstigator->OnEndPlay.RemoveDynamic(this, &UInteractableComponent::HandleInstigatorEndPlay);
		CurrentMonitoredInstigator = nullptr;
	}

	ActiveTransactionId.Invalidate();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// Record completion in persistent cross-level ledger
		if (PersistentStateTag.IsValid())
		{
			if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
			{
				if (UInteractionPersistenceSubsystem* PersistenceSubsystem = GI->GetSubsystem<UInteractionPersistenceSubsystem>())
				{
					FString InstigatorId = TEXT("");
					if (Context.InstigatorActor.IsValid())
					{
						InstigatorId = Context.InstigatorActor->GetName();
					}
					PersistenceSubsystem->SetPersistentState(PersistentStateTag, InteractionNativeTags::State_Completed.GetTag(), InstigatorId, PersistenceScope);
				}
			}
		}

		if (bIsSingleUse)
		{
			SetIsInteractable(false);
		}
		else if (CooldownDuration > 0.0f && GetWorld())
		{
			SetInteractionState(InteractionNativeTags::State_Cooldown, Context.InstigatorActor.Get());
			GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &UInteractableComponent::HandleCooldownExpired, CooldownDuration, false);
		}
		else
		{
			SetInteractionState(InteractionNativeTags::State_Completed, Context.InstigatorActor.Get());
		}
	}
}

void UInteractableComponent::OnInteractionCanceled_Implementation(const FInteractionFrameworkContext& Context, FGameplayTag ReasonTag)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LeaseTimerHandle);
	}

	if (CurrentMonitoredInstigator.IsValid())
	{
		CurrentMonitoredInstigator->OnEndPlay.RemoveDynamic(this, &UInteractableComponent::HandleInstigatorEndPlay);
		CurrentMonitoredInstigator = nullptr;
	}

	ActiveTransactionId.Invalidate();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (bIsInteractable)
		{
			SetInteractionState(InteractionNativeTags::State_Available, Context.InstigatorActor.Get());
		}
	}

	OnInteractionCanceledBP.Broadcast(ReasonTag, Context.InstigatorActor.Get());
}

void UInteractableComponent::OnFocusGained_Implementation(const FInteractionFrameworkContext& Context)
{
	OnFocusGainedBP.Broadcast(Context.InstigatorActor.Get());
}

void UInteractableComponent::OnFocusLost_Implementation(const FInteractionFrameworkContext& Context)
{
	OnFocusLostBP.Broadcast(Context.InstigatorActor.Get());
}

void UInteractableComponent::HandleLeaseExpired()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		AActor* StaleInstigator = CurrentMonitoredInstigator.Get();
		if (CurrentMonitoredInstigator.IsValid())
		{
			CurrentMonitoredInstigator->OnEndPlay.RemoveDynamic(this, &UInteractableComponent::HandleInstigatorEndPlay);
			CurrentMonitoredInstigator = nullptr;
		}

		ActiveTransactionId.Invalidate();

		if (bIsInteractable)
		{
			SetInteractionState(InteractionNativeTags::State_Available, StaleInstigator);
		}

		OnInteractionCanceledBP.Broadcast(InteractionNativeTags::CancelReason_TimedOut, StaleInstigator);
	}
}

void UInteractableComponent::HandleInstigatorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LeaseTimerHandle);
	}

	CurrentMonitoredInstigator = nullptr;
	ActiveTransactionId.Invalidate();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (bIsInteractable)
		{
			SetInteractionState(InteractionNativeTags::State_Available, Actor);
		}

		OnInteractionCanceledBP.Broadcast(InteractionNativeTags::CancelReason_InstigatorInvalid, Actor);
	}
}

bool UInteractableComponent::BeginAsyncInteraction(FGuid TransactionId, AActor* Instigator)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!bIsInteractable || (InteractionState.StateTag != InteractionNativeTags::State_Available && !bAllowConcurrentInteractions))
	{
		return false;
	}

	ActiveTransactionId = TransactionId.IsValid() ? TransactionId : FGuid::NewGuid();
	SetInteractionState(InteractionNativeTags::State_PendingCommit, Instigator);

	if (GetWorld())
	{
		const float EffectiveLease = InteractionLeaseDuration > 0.0f ? InteractionLeaseDuration : 5.0f;
		GetWorld()->GetTimerManager().SetTimer(LeaseTimerHandle, this, &UInteractableComponent::HandleLeaseExpired, EffectiveLease, false);
	}

	if (Instigator)
	{
		if (CurrentMonitoredInstigator.IsValid() && CurrentMonitoredInstigator != Instigator)
		{
			CurrentMonitoredInstigator->OnEndPlay.RemoveDynamic(this, &UInteractableComponent::HandleInstigatorEndPlay);
		}
		CurrentMonitoredInstigator = Instigator;
		CurrentMonitoredInstigator->OnEndPlay.AddDynamic(this, &UInteractableComponent::HandleInstigatorEndPlay);
	}

	return true;
}

bool UInteractableComponent::CommitAsyncInteraction(FGuid TransactionId, AActor* Instigator)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (ActiveTransactionId.IsValid() && TransactionId.IsValid() && ActiveTransactionId != TransactionId)
	{
		return false;
	}

	FInteractionFrameworkContext Context;
	Context.InstigatorActor = Instigator;
	Context.TargetObject = this;
	Context.TransactionId = TransactionId;

	Execute_OnInteractionCompleted(this, Context);
	return true;
}

bool UInteractableComponent::RollbackAsyncInteraction(FGuid TransactionId, FGameplayTag ReasonTag, AActor* Instigator)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (ActiveTransactionId.IsValid() && TransactionId.IsValid() && ActiveTransactionId != TransactionId)
	{
		return false;
	}

	const FGameplayTag EffectiveReason = ReasonTag.IsValid() ? ReasonTag : InteractionNativeTags::CancelReason_TransactionFailed;

	FInteractionFrameworkContext Context;
	Context.InstigatorActor = Instigator;
	Context.TargetObject = this;
	Context.TransactionId = TransactionId;

	Execute_OnInteractionCanceled(this, Context, EffectiveReason);
	return true;
}

void UInteractableComponent::HandleInteractionEvent(FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	OnInteractionEventBP.Broadcast(EventTag, InteractionState.Instigator.Get());
	OnInteractionEvent.Broadcast(EventTag, InteractionState.Instigator.Get());

	if (EventTag == InteractionNativeTags::Event_Commit.GetTag())
	{
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			FInteractionFrameworkContext Context;
			Context.InstigatorActor = InteractionState.Instigator.Get();
			Context.TargetObject = this;
			Context.TransactionId = ActiveTransactionId;

			Execute_OnInteractionCompleted(this, Context);
		}
	}
	else if (EventTag == InteractionNativeTags::Event_Cancel.GetTag())
	{
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			FInteractionFrameworkContext Context;
			Context.InstigatorActor = InteractionState.Instigator.Get();
			Context.TargetObject = this;
			Context.TransactionId = ActiveTransactionId;

			Execute_OnInteractionCanceled(this, Context, InteractionNativeTags::CancelReason_Interrupted.GetTag());
		}
	}
}
