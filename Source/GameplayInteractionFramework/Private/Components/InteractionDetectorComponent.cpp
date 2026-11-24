#include "Components/InteractionDetectorComponent.h"
#include "Components/InteractableComponent.h"
#include "Interfaces/InteractableTarget.h"
#include "Interfaces/InteractionInstigator.h"
#include "Settings/GameplayInteractionSettings.h"
#include "Types/InteractionNativeTags.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Subsystems/InteractionLocalPlayerSubsystem.h"
#include "Engine/LocalPlayer.h"

UInteractionDetectorComponent::UInteractionDetectorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);

	TraceShapeTag = InteractionNativeTags::TraceShape_CameraLine.GetTag();
	TraceChannel = ECC_Visibility;
	MaxTraceDistance = 0.0f;
	SweepRadius = 40.0f;
	BoxSweepExtents = FVector(50.0f, 50.0f, 50.0f);
	QueryInterval = 0.0333f;

	WeightFacingAngle = 0.5f;
	WeightDistance = 0.3f;
	WeightPriority = 0.2f;

	QueryTimeAccumulator = 0.0f;
	bIsInteracting = false;
	CurrentHoldTime = 0.0f;
	CurrentTapCount = 0;
	TimeSinceLastTap = 0.0f;
	CurrentProgressRatio = 0.0f;
}

void UInteractionDetectorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Load defaults from Project Settings if not explicitly overridden on component
	if (const UGameplayInteractionSettings* Settings = UGameplayInteractionSettings::Get())
	{
		if (MaxTraceDistance <= 0.0f)
		{
			MaxTraceDistance = Settings->DefaultInteractionDistance;
		}
		if (QueryInterval <= 0.0f)
		{
			QueryInterval = Settings->DefaultQueryInterval;
		}
		if (!TraceShapeTag.IsValid())
		{
			TraceShapeTag = Settings->DefaultTraceShapeTag;
		}
	}
}

void UInteractionDetectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Throttled detection queries to maintain low CPU footprint
	QueryTimeAccumulator += DeltaTime;
	if (QueryInterval <= 0.0f || QueryTimeAccumulator >= QueryInterval)
	{
		QueryTimeAccumulator = 0.0f;
		PerformDetection();
	}

	// Update active interaction state machine
	if (bIsInteracting && FocusedCandidate.TargetObject.IsValid())
	{
		UObject* TargetObj = FocusedCandidate.TargetObject.Get();
		UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObj);
		const FGameplayTag ModeTag = InteractableComp ? InteractableComp->InteractionModeTag : InteractionNativeTags::Mode_Instant.GetTag();

		if (ModeTag == InteractionNativeTags::Mode_Hold.GetTag())
		{
			const float TargetHoldDuration = InteractableComp ? FMath::Max(0.01f, InteractableComp->HoldDuration) : 1.0f;
			CurrentHoldTime += DeltaTime;
			CurrentProgressRatio = FMath::Clamp(CurrentHoldTime / TargetHoldDuration, 0.0f, 1.0f);

			FInteractionFrameworkContext Context;
			Context.InstigatorActor = GetOwner();
			Context.TargetObject = TargetObj;
			Context.SelectedPointIndex = FocusedCandidate.SelectedPointIndex;

			if (TargetObj->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
			{
				IInteractableTarget::Execute_OnInteractionProgress(TargetObj, Context, CurrentProgressRatio);
			}

			OnInteractionProgressBP.Broadcast(CurrentProgressRatio, ModeTag, TargetObj);
			if (UInteractionLocalPlayerSubsystem* Subsystem = GetLocalPlayerSubsystem())
			{
				Subsystem->NotifyInteractionProgress(CurrentProgressRatio, ModeTag, TargetObj);
			}

			if (CurrentHoldTime >= TargetHoldDuration)
			{
				CompleteInteraction();
			}
		}
		else if (ModeTag == InteractionNativeTags::Mode_RapidTap.GetTag())
		{
			TimeSinceLastTap += DeltaTime;
			const float DecayRate = InteractableComp ? InteractableComp->TapDecayRate : 1.0f;
			const float DecayInterval = (DecayRate > 0.0f) ? (1.0f / DecayRate) : 1.0f;
			if (DecayRate > 0.0f && TimeSinceLastTap >= DecayInterval && CurrentTapCount > 0)
			{
				while (TimeSinceLastTap >= DecayInterval && CurrentTapCount > 0)
				{
					TimeSinceLastTap -= DecayInterval;
					CurrentTapCount = FMath::Max(0, CurrentTapCount - 1);
				}

				const int32 TapsNeeded = InteractableComp ? FMath::Max(1, InteractableComp->RapidTapsRequired) : 5;
				CurrentProgressRatio = FMath::Clamp(static_cast<float>(CurrentTapCount) / static_cast<float>(TapsNeeded), 0.0f, 1.0f);
				OnInteractionProgressBP.Broadcast(CurrentProgressRatio, ModeTag, TargetObj);
			if (UInteractionLocalPlayerSubsystem* Subsystem = GetLocalPlayerSubsystem())
			{
				Subsystem->NotifyInteractionProgress(CurrentProgressRatio, ModeTag, TargetObj);
			}

				if (CurrentTapCount == 0)
				{
					bIsInteracting = false;
				}
			}
		}
	}
}

void UInteractionDetectorComponent::GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		OutLocation = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
		return;
	}

	// 1. Check if owner implements IInteractionInstigator
	if (OwnerActor->GetClass()->ImplementsInterface(UInteractionInstigator::StaticClass()))
	{
		IInteractionInstigator::Execute_GetInteractionViewPoint(OwnerActor, OutLocation, OutRotation);
		return;
	}

	// 2. Check PlayerController
	if (APlayerController* PC = Cast<APlayerController>(OwnerActor))
	{
		PC->GetPlayerViewPoint(OutLocation, OutRotation);
		return;
	}

	// 3. Check Pawn controller or camera
	if (APawn* Pawn = Cast<APawn>(OwnerActor))
	{
		if (APlayerController* PawnPC = Cast<APlayerController>(Pawn->GetController()))
		{
			PawnPC->GetPlayerViewPoint(OutLocation, OutRotation);
			return;
		}

		if (UCameraComponent* Camera = Pawn->FindComponentByClass<UCameraComponent>())
		{
			OutLocation = Camera->GetComponentLocation();
			OutRotation = Camera->GetComponentRotation();
			return;
		}

		OutLocation = Pawn->GetPawnViewLocation();
		OutRotation = Pawn->GetBaseAimRotation();
		return;
	}

	// Fallback: Owner Actor Transform
	OutLocation = OwnerActor->GetActorLocation();
	OutRotation = OwnerActor->GetActorRotation();
}

bool UInteractionDetectorComponent::RunDefaultTrace(const FVector& ViewLocation, const FVector& ViewDirection, float Distance, TArray<FHitResult>& OutHits) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractionTrace), false, GetOwner());
	const FVector TraceEnd = ViewLocation + (ViewDirection * Distance);

	if (TraceShapeTag == InteractionNativeTags::TraceShape_CameraLine.GetTag())
	{
		return World->LineTraceMultiByChannel(OutHits, ViewLocation, TraceEnd, TraceChannel, QueryParams);
	}
	else if (TraceShapeTag == InteractionNativeTags::TraceShape_SphereSweep.GetTag() || TraceShapeTag == InteractionNativeTags::TraceShape_ForwardCone.GetTag())
	{
		const FCollisionShape SphereShape = FCollisionShape::MakeSphere(SweepRadius);
		return World->SweepMultiByChannel(OutHits, ViewLocation, TraceEnd, FQuat::Identity, TraceChannel, SphereShape, QueryParams);
	}
	else if (TraceShapeTag == InteractionNativeTags::TraceShape_BoxSweep.GetTag())
	{
		const FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxSweepExtents);
		return World->SweepMultiByChannel(OutHits, ViewLocation, TraceEnd, ViewDirection.Rotation().Quaternion(), TraceChannel, BoxShape, QueryParams);
	}

	// Default fallback: line trace
	return World->LineTraceMultiByChannel(OutHits, ViewLocation, TraceEnd, TraceChannel, QueryParams);
}

float UInteractionDetectorComponent::CalculateCandidateScore(float Distance, float MaxDistance, float DotAngle, int32 Priority) const
{
	const float DistFactor = FMath::Clamp(1.0f - (Distance / FMath::Max(1.0f, MaxDistance)), 0.0f, 1.0f);
	const float AngleFactor = FMath::Clamp((DotAngle + 1.0f) * 0.5f, 0.0f, 1.0f);
	const float PriorityFactor = FMath::Clamp(static_cast<float>(Priority) / 100.0f, 0.0f, 1.0f);

	return (WeightFacingAngle * AngleFactor) + (WeightDistance * DistFactor) + (WeightPriority * PriorityFactor);
}

void UInteractionDetectorComponent::PerformDetection()
{
	DetectionPassCount++;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDirection = ViewRotation.Vector();

	const float EffectiveDistance = MaxTraceDistance > 0.0f ? MaxTraceDistance : 250.0f;

	TArray<FHitResult> Hits;
	if (CustomTraceStrategy.GetObject())
	{
		IInteractionTraceStrategy::Execute_ExecuteTrace(CustomTraceStrategy.GetObject(), GetOwner(), EffectiveDistance, TraceChannel, Hits);
	}
	else
	{
		RunDefaultTrace(ViewLocation, ViewDirection, EffectiveDistance, Hits);
	}

	TArray<FInteractionCandidate> EvaluatedCandidates;
	TSet<UObject*> EvaluatedTargets;

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == GetOwner())
		{
			continue;
		}

		UObject* TargetObject = nullptr;
		if (Hit.Component.IsValid() && Hit.Component->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
		{
			TargetObject = Hit.Component.Get();
		}
		else if (UInteractableComponent* Comp = HitActor->FindComponentByClass<UInteractableComponent>())
		{
			TargetObject = Comp;
		}
		else if (HitActor->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
		{
			TargetObject = HitActor;
		}

		if (!TargetObject || EvaluatedTargets.Contains(TargetObject))
		{
			continue;
		}
		EvaluatedTargets.Add(TargetObject);

		int32 PointIndex = 0;
		FTransform PointTransform = HitActor->GetActorTransform();
		int32 TargetPriority = 0;

		if (UInteractableComponent* Comp = Cast<UInteractableComponent>(TargetObject))
		{
			PointIndex = Comp->FindClosestSmartPointIndex(ViewLocation);
			PointTransform = Comp->GetSmartPointWorldTransform(PointIndex);
			TargetPriority = Comp->Priority;
		}

		FInteractionFrameworkContext Context;
		Context.InstigatorActor = GetOwner();
		Context.TargetObject = TargetObject;
		Context.SelectedPointIndex = PointIndex;
		Context.HitLocation = Hit.ImpactPoint;
		Context.HitNormal = Hit.ImpactNormal;

		FGameplayTagContainer FailureTags;
		if (IInteractableTarget::Execute_CanInteract(TargetObject, Context, FailureTags))
		{
			const FVector TargetLocation = PointTransform.GetLocation();
			const float Distance = FVector::Dist(ViewLocation, TargetLocation);
			const FVector DirToTarget = (TargetLocation - ViewLocation).GetSafeNormal();
			const float DotAngle = FVector::DotProduct(ViewDirection, DirToTarget);

			FInteractionCandidate Candidate;
			Candidate.TargetObject = TargetObject;
			Candidate.SelectedPointIndex = PointIndex;
			Candidate.WorldLocation = TargetLocation;
			Candidate.Distance = Distance;
			Candidate.DotAngle = DotAngle;
			Candidate.Priority = TargetPriority;
			Candidate.Score = CalculateCandidateScore(Distance, EffectiveDistance, DotAngle, TargetPriority);

			EvaluatedCandidates.Add(Candidate);
		}
	}

	// Sort candidates (higher scores sort first)
	EvaluatedCandidates.Sort();

	UpdateFocusedCandidate(EvaluatedCandidates);
}

void UInteractionDetectorComponent::UpdateFocusedCandidate(const TArray<FInteractionCandidate>& NewCandidates)
{
	CurrentCandidates = NewCandidates;

	UObject* OldFocused = FocusedCandidate.TargetObject.Get();
	UObject* NewFocused = NewCandidates.Num() > 0 ? NewCandidates[0].TargetObject.Get() : nullptr;

	if (OldFocused != NewFocused)
	{
		// Target lost focus
		if (OldFocused && OldFocused->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
		{
			FInteractionFrameworkContext LostContext;
			LostContext.InstigatorActor = GetOwner();
			LostContext.TargetObject = OldFocused;
			LostContext.SelectedPointIndex = FocusedCandidate.SelectedPointIndex;
			IInteractableTarget::Execute_OnFocusLost(OldFocused, LostContext);
		}

		if (NewCandidates.Num() > 0)
		{
			FocusedCandidate = NewCandidates[0];
		}
		else
		{
			FocusedCandidate = FInteractionCandidate();
		}

		// New target gained focus
		if (NewFocused && NewFocused->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
		{
			FInteractionFrameworkContext GainedContext;
			GainedContext.InstigatorActor = GetOwner();
			GainedContext.TargetObject = NewFocused;
			GainedContext.SelectedPointIndex = FocusedCandidate.SelectedPointIndex;
			IInteractableTarget::Execute_OnFocusGained(NewFocused, GainedContext);

			const FInteractionPromptData Prompt = IInteractableTarget::Execute_GetPromptData(NewFocused, FocusedCandidate.SelectedPointIndex);
			OnFocusedTargetChangedBP.Broadcast(NewFocused, Prompt);

			if (UInteractionLocalPlayerSubsystem* Subsystem = GetLocalPlayerSubsystem())
			{
				Subsystem->NotifyFocusedTargetChanged(NewFocused, Prompt);
			}
		}
		else
		{
			OnFocusedTargetChangedBP.Broadcast(nullptr, FInteractionPromptData());

			if (UInteractionLocalPlayerSubsystem* Subsystem = GetLocalPlayerSubsystem())
			{
				Subsystem->NotifyFocusedTargetChanged(nullptr, FInteractionPromptData());
			}
		}
	}

	OnCandidatesUpdatedBP.Broadcast(CurrentCandidates);
}

UInteractionLocalPlayerSubsystem* UInteractionDetectorComponent::GetLocalPlayerSubsystem() const
{
	if (AActor* OwnerActor = GetOwner())
	{
		APlayerController* PC = Cast<APlayerController>(OwnerActor);
		if (!PC)
		{
			if (APawn* Pawn = Cast<APawn>(OwnerActor))
			{
				PC = Cast<APlayerController>(Pawn->GetController());
			}
		}

		if (PC && PC->IsLocalController())
		{
			if (ULocalPlayer* LP = PC->GetLocalPlayer())
			{
				return LP->GetSubsystem<UInteractionLocalPlayerSubsystem>();
			}
		}
	}
	return nullptr;
}

UObject* UInteractionDetectorComponent::GetFocusedTarget() const
{
	return FocusedCandidate.TargetObject.Get();
}

int32 UInteractionDetectorComponent::GetFocusedPointIndex() const
{
	return FocusedCandidate.SelectedPointIndex;
}

void UInteractionDetectorComponent::StartInteract()
{
	if (!FocusedCandidate.TargetObject.IsValid())
	{
		return;
	}

	UObject* TargetObj = FocusedCandidate.TargetObject.Get();
	const int32 PointIndex = FocusedCandidate.SelectedPointIndex;

	AActor* OwnerActor = GetOwner();
	const bool bIsAutonomousProxy = OwnerActor && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy;

	CurrentTransactionId = FGuid::NewGuid();

	if (!bIsAutonomousProxy)
	{
		ExecuteServerStartInteract(TargetObj, PointIndex, CurrentTransactionId);
	}
	else
	{
		bIsInteracting = true;
		CurrentHoldTime = 0.0f;
		CurrentProgressRatio = 0.0f;

		FInteractionFrameworkContext Context;
		Context.InstigatorActor = OwnerActor;
		Context.TargetObject = TargetObj;
		Context.SelectedPointIndex = PointIndex;
		Context.TransactionId = CurrentTransactionId;

		if (TargetObj->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
		{
			IInteractableTarget::Execute_OnInteractionStarted(TargetObj, Context);
		}

		UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObj);
		const FGameplayTag ModeTag = InteractableComp ? InteractableComp->InteractionModeTag : InteractionNativeTags::Mode_Instant.GetTag();
		OnInteractionStartedBP.Broadcast(ModeTag, TargetObj);

		Server_StartInteract(TargetObj, PointIndex, CurrentTransactionId);
	}
}

void UInteractionDetectorComponent::StopInteract()
{
	if (bIsInteracting)
	{
		if (FocusedCandidate.TargetObject.IsValid())
		{
			UObject* TargetObj = FocusedCandidate.TargetObject.Get();
			UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObj);
			const FGameplayTag ModeTag = InteractableComp ? InteractableComp->InteractionModeTag : InteractionNativeTags::Mode_Instant.GetTag();

			if (ModeTag == InteractionNativeTags::Mode_Hold.GetTag())
			{
				const float TargetHoldDuration = InteractableComp ? InteractableComp->HoldDuration : 1.0f;
				if (CurrentHoldTime < TargetHoldDuration)
				{
					CancelInteract(InteractionNativeTags::CancelReason_KeyReleased.GetTag());
					return;
				}
			}
		}

		bIsInteracting = false;
	}
}

void UInteractionDetectorComponent::TapInteract()
{
	if (!FocusedCandidate.TargetObject.IsValid())
	{
		return;
	}

	bIsInteracting = true;

	UObject* TargetObj = FocusedCandidate.TargetObject.Get();
	UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObj);
	const int32 RequiredTaps = InteractableComp ? FMath::Max(1, InteractableComp->RapidTapsRequired) : 5;

	CurrentTapCount++;
	TimeSinceLastTap = 0.0f;

	CurrentProgressRatio = FMath::Clamp(static_cast<float>(CurrentTapCount) / static_cast<float>(RequiredTaps), 0.0f, 1.0f);
	OnInteractionProgressBP.Broadcast(CurrentProgressRatio, InteractionNativeTags::Mode_RapidTap.GetTag(), TargetObj);

	if (UInteractionLocalPlayerSubsystem* Subsystem = GetLocalPlayerSubsystem())
	{
		Subsystem->NotifyInteractionProgress(CurrentProgressRatio, InteractionNativeTags::Mode_RapidTap.GetTag(), TargetObj);
	}

	if (CurrentTapCount >= RequiredTaps)
	{
		CompleteInteraction();
	}
}

void UInteractionDetectorComponent::CancelInteract(FGameplayTag ReasonTag)
{
	if (bIsInteracting && FocusedCandidate.TargetObject.IsValid())
	{
		UObject* TargetObj = FocusedCandidate.TargetObject.Get();
		AActor* OwnerActor = GetOwner();
		FInteractionFrameworkContext Context;
		Context.InstigatorActor = OwnerActor;
		Context.TargetObject = TargetObj;
		Context.SelectedPointIndex = FocusedCandidate.SelectedPointIndex;

		if (TargetObj->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
		{
			IInteractableTarget::Execute_OnInteractionCanceled(TargetObj, Context, ReasonTag);
		}

		OnInteractionCanceledBP.Broadcast(ReasonTag, TargetObj);
		OnInteractionCanceled.Broadcast(ReasonTag, TargetObj);

		if (UInteractionLocalPlayerSubsystem* Subsystem = GetLocalPlayerSubsystem())
		{
			Subsystem->NotifyInteractionCanceled(ReasonTag, TargetObj);
		}

		if (OwnerActor && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_CancelInteract(TargetObj, ReasonTag, CurrentTransactionId);
		}
		else if (UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObj))
		{
			InteractableComp->ClearServerHoldStart(OwnerActor);
			if (InteractableComp->InteractionState.Instigator == OwnerActor)
			{
				InteractableComp->SetInteractionState(InteractionNativeTags::State_Available.GetTag(), nullptr);
			}
		}
	}

	CurrentTransactionId.Invalidate();
	bIsInteracting = false;
	CurrentHoldTime = 0.0f;
	CurrentTapCount = 0;
	CurrentProgressRatio = 0.0f;
}

void UInteractionDetectorComponent::HandleInteractionEvent(FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	if (EventTag == InteractionNativeTags::Event_Start.GetTag())
	{
		StartInteract();
	}
	else if (EventTag == InteractionNativeTags::Event_Commit.GetTag())
	{
		if (bIsInteracting)
		{
			CompleteInteraction();
		}
		else
		{
			// If not already interacting, trigger instant interaction directly
			StartInteract();
		}
	}
	else if (EventTag == InteractionNativeTags::Event_Cancel.GetTag() || EventTag == InteractionNativeTags::CancelReason_MontageInterrupted.GetTag())
	{
		CancelInteract(EventTag);
	}
	else
	{
		// Forward generic or custom payload event to focused target if valid
		if (FocusedCandidate.TargetObject.IsValid())
		{
			if (UInteractableComponent* TargetComp = Cast<UInteractableComponent>(FocusedCandidate.TargetObject.Get()))
			{
				TargetComp->HandleInteractionEvent(EventTag);
			}
		}
	}
}

void UInteractionDetectorComponent::CompleteInteraction()
{
	bIsInteracting = false;
	CurrentHoldTime = 0.0f;
	CurrentTapCount = 0;
	CurrentProgressRatio = 1.0f;

	if (FocusedCandidate.TargetObject.IsValid())
	{
		UObject* TargetObj = FocusedCandidate.TargetObject.Get();
		AActor* OwnerActor = GetOwner();
		const int32 PointIndex = FocusedCandidate.SelectedPointIndex;

		UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObj);
		const FGameplayTag ModeTag = InteractableComp ? InteractableComp->InteractionModeTag : InteractionNativeTags::Mode_Instant.GetTag();

		if (ModeTag == InteractionNativeTags::Mode_Hold.GetTag())
		{
			if (!OwnerActor || OwnerActor->GetLocalRole() != ROLE_AutonomousProxy)
			{
				ExecuteServerCompleteHoldInteract(TargetObj, PointIndex, CurrentTransactionId);
			}
			else
			{
				Server_CompleteHoldInteract(TargetObj, PointIndex, CurrentTransactionId);
			}
		}
		else
		{
			FInteractionFrameworkContext Context;
			Context.InstigatorActor = OwnerActor;
			Context.TargetObject = TargetObj;
			Context.SelectedPointIndex = PointIndex;
			Context.TransactionId = CurrentTransactionId;

			if (InteractableComp)
			{
				InteractableComp->SetInteractionState(InteractionNativeTags::State_Completed.GetTag(), OwnerActor);
			}

			if (TargetObj->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
			{
				IInteractableTarget::Execute_OnInteractionCompleted(TargetObj, Context);
			}
		}

		if (UInteractionLocalPlayerSubsystem* Subsystem = GetLocalPlayerSubsystem())
		{
			Subsystem->NotifyInteractionCompleted(TargetObj);
		}

		OnInteractionCompletedBP.Broadcast(TargetObj);
	}

	CurrentTransactionId.Invalidate();
}

void UInteractionDetectorComponent::ExecuteServerStartInteract(UObject* TargetObject, int32 PointIndex, FGuid TransactionId)
{
	if (!TargetObject)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	FInteractionFrameworkContext Context;
	Context.InstigatorActor = OwnerActor;
	Context.TargetObject = TargetObject;
	Context.SelectedPointIndex = PointIndex;
	Context.TransactionId = TransactionId;

	UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObject);
	if (InteractableComp)
	{
		FGameplayTagContainer FailureTags;
		const EInteractionValidationResult Result = InteractableComp->EvaluateEligibility(Context, FailureTags);
		if (Result != EInteractionValidationResult::Valid)
		{
			const FGameplayTag ReasonTag = FailureTags.First();
			const FGameplayTag FinalReason = ReasonTag.IsValid() ? ReasonTag : InteractionNativeTags::CancelReason_ServerRejected.GetTag();

			if (OwnerActor && OwnerActor->GetNetMode() == NM_DedicatedServer)
			{
				Client_RejectInteraction(TargetObject, FinalReason);
			}
			else
			{
				CancelInteract(FinalReason);
			}
			return;
		}

		const FGameplayTag ModeTag = InteractableComp->InteractionModeTag;
		if (ModeTag == InteractionNativeTags::Mode_Instant.GetTag())
		{
			IInteractableTarget::Execute_OnInteractionCompleted(TargetObject, Context);
		}
		else if (ModeTag == InteractionNativeTags::Mode_Hold.GetTag())
		{
			bIsInteracting = true;
			CurrentHoldTime = 0.0f;
			CurrentProgressRatio = 0.0f;

			InteractableComp->RecordServerHoldStart(OwnerActor);
			InteractableComp->SetInteractionState(InteractionNativeTags::State_Active.GetTag(), OwnerActor);
			IInteractableTarget::Execute_OnInteractionStarted(TargetObject, Context);
			OnInteractionStartedBP.Broadcast(ModeTag, TargetObject);
		}
		else if (ModeTag == InteractionNativeTags::Mode_RapidTap.GetTag())
		{
			TapInteract();
		}
		else if (ModeTag == InteractionNativeTags::Mode_Toggle.GetTag())
		{
			IInteractableTarget::Execute_OnInteractionCompleted(TargetObject, Context);
		}
	}
	else if (TargetObject->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
	{
		FGameplayTagContainer FailureTags;
		if (!IInteractableTarget::Execute_CanInteract(TargetObject, Context, FailureTags))
		{
			return;
		}
		IInteractableTarget::Execute_OnInteractionCompleted(TargetObject, Context);
	}
}

void UInteractionDetectorComponent::ExecuteServerCompleteHoldInteract(UObject* TargetObject, int32 PointIndex, FGuid TransactionId)
{
	if (!TargetObject)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	FInteractionFrameworkContext Context;
	Context.InstigatorActor = OwnerActor;
	Context.TargetObject = TargetObject;
	Context.SelectedPointIndex = PointIndex;
	Context.TransactionId = TransactionId;

	UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObject);
	if (InteractableComp)
	{
		FGameplayTagContainer FailureTags;
		const EInteractionValidationResult Result = InteractableComp->EvaluateEligibility(Context, FailureTags);
		if (Result != EInteractionValidationResult::Valid)
		{
			if (!(Result == EInteractionValidationResult::TargetBusy && InteractableComp->InteractionState.Instigator == OwnerActor))
			{
				InteractableComp->ClearServerHoldStart(OwnerActor);
				const FGameplayTag FinalReason = FailureTags.First().IsValid() ? FailureTags.First() : InteractionNativeTags::CancelReason_ServerRejected.GetTag();
				if (OwnerActor && OwnerActor->GetNetMode() == NM_DedicatedServer)
				{
					Client_RejectInteraction(TargetObject, FinalReason);
				}
				else
				{
					CancelInteract(FinalReason);
				}
				return;
			}
		}

		const ENetMode NetMode = OwnerActor ? OwnerActor->GetNetMode() : NM_Standalone;
		if (NetMode == NM_DedicatedServer || NetMode == NM_ListenServer)
		{
			float ElapsedTime = 0.0f;
			if (!InteractableComp->ValidateServerHoldTiming(OwnerActor, InteractableComp->HoldDuration, ElapsedTime))
			{
				InteractableComp->ClearServerHoldStart(OwnerActor);
				InteractableComp->SetInteractionState(InteractionNativeTags::State_Available.GetTag(), nullptr);
				if (NetMode == NM_DedicatedServer)
				{
					Client_RejectInteraction(TargetObject, InteractionNativeTags::CancelReason_TimingViolation.GetTag());
				}
				else
				{
					CancelInteract(InteractionNativeTags::CancelReason_TimingViolation.GetTag());
				}
				return;
			}
		}

		InteractableComp->ClearServerHoldStart(OwnerActor);
		IInteractableTarget::Execute_OnInteractionCompleted(TargetObject, Context);
	}
	else if (TargetObject->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
	{
		IInteractableTarget::Execute_OnInteractionCompleted(TargetObject, Context);
	}
}

bool UInteractionDetectorComponent::Server_StartInteract_Validate(UObject* TargetObject, int32 PointIndex, FGuid TransactionId)
{
	return PointIndex >= 0 && PointIndex < 1000;
}

void UInteractionDetectorComponent::Server_StartInteract_Implementation(UObject* TargetObject, int32 PointIndex, FGuid TransactionId)
{
	ExecuteServerStartInteract(TargetObject, PointIndex, TransactionId);
}

bool UInteractionDetectorComponent::Server_CompleteHoldInteract_Validate(UObject* TargetObject, int32 PointIndex, FGuid TransactionId)
{
	return PointIndex >= 0 && PointIndex < 1000;
}

void UInteractionDetectorComponent::Server_CompleteHoldInteract_Implementation(UObject* TargetObject, int32 PointIndex, FGuid TransactionId)
{
	ExecuteServerCompleteHoldInteract(TargetObject, PointIndex, TransactionId);
}

bool UInteractionDetectorComponent::Server_CancelInteract_Validate(UObject* TargetObject, FGameplayTag ReasonTag, FGuid TransactionId)
{
	return true;
}

void UInteractionDetectorComponent::Server_CancelInteract_Implementation(UObject* TargetObject, FGameplayTag ReasonTag, FGuid TransactionId)
{
	AActor* OwnerActor = GetOwner();
	if (UInteractableComponent* InteractableComp = Cast<UInteractableComponent>(TargetObject))
	{
		InteractableComp->ClearServerHoldStart(OwnerActor);
		if (InteractableComp->InteractionState.Instigator == OwnerActor)
		{
			InteractableComp->SetInteractionState(InteractionNativeTags::State_Available.GetTag(), nullptr);
		}
	}

	if (TargetObject && TargetObject->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
	{
		FInteractionFrameworkContext Context;
		Context.InstigatorActor = OwnerActor;
		Context.TargetObject = TargetObject;
		Context.TransactionId = TransactionId;
		IInteractableTarget::Execute_OnInteractionCanceled(TargetObject, Context, ReasonTag);
	}
}

void UInteractionDetectorComponent::Client_RejectInteraction_Implementation(UObject* TargetObject, FGameplayTag ReasonTag)
{
	bIsInteracting = false;
	CurrentHoldTime = 0.0f;
	CurrentTapCount = 0;
	CurrentProgressRatio = 0.0f;

	FInteractionFrameworkContext Context;
	Context.InstigatorActor = GetOwner();
	Context.TargetObject = TargetObject;
	Context.SelectedPointIndex = FocusedCandidate.SelectedPointIndex;

	if (TargetObject && TargetObject->GetClass()->ImplementsInterface(UInteractableTarget::StaticClass()))
	{
		IInteractableTarget::Execute_OnInteractionCanceled(TargetObject, Context, ReasonTag);
	}

	OnInteractionCanceledBP.Broadcast(ReasonTag, TargetObject);

	if (UInteractionLocalPlayerSubsystem* Subsystem = GetLocalPlayerSubsystem())
	{
		Subsystem->NotifyInteractionCanceled(ReasonTag, TargetObject);
	}
}
