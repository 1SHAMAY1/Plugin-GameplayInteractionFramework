#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/InteractionDetectorComponent.h"
#include "Animation/AnimNotify_InteractionEvent.h"
#include "Animation/AnimNotifyState_InteractionEvent.h"
#include "Types/InteractionNativeTags.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimNotifyInstantEventCommitTest,
	"GameplayInteractionFramework.Animation.InstantEventCommit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnimNotifyInstantEventCommitTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World should be created"), World);

	// Create Instigator Actor with SkeletalMesh and Detector
	AActor* InstigatorActor = World->SpawnActor<AActor>();
	USceneComponent* InstigatorRoot = NewObject<USceneComponent>(InstigatorActor);
	InstigatorActor->SetRootComponent(InstigatorRoot);
	InstigatorRoot->RegisterComponent();
	InstigatorActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
	InstigatorActor->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>(InstigatorActor);
	MeshComp->SetupAttachment(InstigatorRoot);
	MeshComp->RegisterComponent();

	UInteractionDetectorComponent* DetectorComp = NewObject<UInteractionDetectorComponent>(InstigatorActor);
	DetectorComp->MaxTraceDistance = 300.0f;
	DetectorComp->RegisterComponent();

	// Create Target Actor with Box Component and InteractableComponent
	AActor* TargetActor = World->SpawnActor<AActor>();
	UBoxComponent* TargetBox = NewObject<UBoxComponent>(TargetActor);
	TargetActor->SetRootComponent(TargetBox);
	TargetBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	TargetBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TargetBox->SetCollisionResponseToAllChannels(ECR_Block);
	TargetBox->RegisterComponent();
	TargetActor->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));

	UInteractableComponent* InteractableComp = NewObject<UInteractableComponent>(TargetActor);
	InteractableComp->InteractionModeTag = InteractionNativeTags::Mode_Instant.GetTag();
	InteractableComp->InteractionDistance = 200.0f;
	InteractableComp->RegisterComponent();

	// Set focus
	DetectorComp->PerformDetection();
	TestNotNull(TEXT("Detector should focus target after detection"), DetectorComp->GetFocusedTarget());

	// Create and fire Notify with Event_Commit
	UAnimNotify_InteractionEvent* Notify = NewObject<UAnimNotify_InteractionEvent>();
	Notify->EventTag = InteractionNativeTags::Event_Commit.GetTag();

	FAnimNotifyEventReference EventRef;
	Notify->Notify(MeshComp, nullptr, EventRef);

	// Verify Target received completion
	TestEqual(TEXT("Target state should be Completed"), InteractableComp->InteractionState.StateTag, InteractionNativeTags::State_Completed.GetTag());

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimNotifyInstantCustomEventTest,
	"GameplayInteractionFramework.Animation.InstantCustomEvent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnimNotifyInstantCustomEventTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World should be created"), World);

	AActor* TargetActor = World->SpawnActor<AActor>();
	USceneComponent* TargetRoot = NewObject<USceneComponent>(TargetActor);
	TargetActor->SetRootComponent(TargetRoot);
	TargetRoot->RegisterComponent();

	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>(TargetActor);
	MeshComp->SetupAttachment(TargetRoot);
	MeshComp->RegisterComponent();

	UInteractableComponent* InteractableComp = NewObject<UInteractableComponent>(TargetActor);
	InteractableComp->RegisterComponent();

	bool bCustomEventReceived = false;
	FGameplayTag ReceivedTag;
	InteractableComp->OnInteractionEvent.AddLambda([&bCustomEventReceived, &ReceivedTag](FGameplayTag Tag, AActor* Instigator)
	{
		bCustomEventReceived = true;
		ReceivedTag = Tag;
	});

	// Fire custom notify tag
	UAnimNotify_InteractionEvent* Notify = NewObject<UAnimNotify_InteractionEvent>();
	const FGameplayTag CustomTag = InteractionNativeTags::CancelReason_Interrupted.GetTag();
	Notify->EventTag = CustomTag;

	FAnimNotifyEventReference EventRef;
	Notify->Notify(MeshComp, nullptr, EventRef);

	TestTrue(TEXT("Custom event should be broadcast"), bCustomEventReceived);
	TestEqual(TEXT("Received tag should match custom tag"), ReceivedTag, CustomTag);

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimNotifyStateProgressWindowTest,
	"GameplayInteractionFramework.Animation.StateProgressWindow",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnimNotifyStateProgressWindowTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World should be created"), World);

	AActor* InstigatorActor = World->SpawnActor<AActor>();
	USceneComponent* InstigatorRoot = NewObject<USceneComponent>(InstigatorActor);
	InstigatorActor->SetRootComponent(InstigatorRoot);
	InstigatorRoot->RegisterComponent();
	InstigatorActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
	InstigatorActor->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>(InstigatorActor);
	MeshComp->SetupAttachment(InstigatorRoot);
	MeshComp->RegisterComponent();

	UInteractionDetectorComponent* DetectorComp = NewObject<UInteractionDetectorComponent>(InstigatorActor);
	DetectorComp->MaxTraceDistance = 300.0f;
	DetectorComp->RegisterComponent();

	AActor* TargetActor = World->SpawnActor<AActor>();
	UBoxComponent* TargetBox = NewObject<UBoxComponent>(TargetActor);
	TargetActor->SetRootComponent(TargetBox);
	TargetBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	TargetBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TargetBox->SetCollisionResponseToAllChannels(ECR_Block);
	TargetBox->RegisterComponent();
	TargetActor->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));

	UInteractableComponent* InteractableComp = NewObject<UInteractableComponent>(TargetActor);
	InteractableComp->InteractionModeTag = InteractionNativeTags::Mode_Hold.GetTag();
	InteractableComp->HoldDuration = 2.0f;
	InteractableComp->InteractionDistance = 200.0f;
	InteractableComp->RegisterComponent();

	// Focus target
	DetectorComp->PerformDetection();
	TestNotNull(TEXT("Detector should focus target"), DetectorComp->GetFocusedTarget());

	UAnimNotifyState_InteractionEvent* NotifyState = NewObject<UAnimNotifyState_InteractionEvent>();
	NotifyState->BeginEventTag = InteractionNativeTags::Event_Start.GetTag();
	NotifyState->EndEventTag = InteractionNativeTags::Event_Commit.GetTag();

	FAnimNotifyEventReference EventRef;

	// Begin window
	NotifyState->NotifyBegin(MeshComp, nullptr, 2.0f, EventRef);
	TestTrue(TEXT("Detector should have initiated interaction on NotifyBegin"), DetectorComp->IsInteracting());

	// End window
	NotifyState->NotifyEnd(MeshComp, nullptr, EventRef);
	TestEqual(TEXT("Target should be completed on NotifyEnd commit"), InteractableComp->InteractionState.StateTag, InteractionNativeTags::State_Completed.GetTag());

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimNotifyStateInterruptionTest,
	"GameplayInteractionFramework.Animation.StateInterruption",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAnimNotifyStateInterruptionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World should be created"), World);

	AActor* InstigatorActor = World->SpawnActor<AActor>();
	USceneComponent* InstigatorRoot = NewObject<USceneComponent>(InstigatorActor);
	InstigatorActor->SetRootComponent(InstigatorRoot);
	InstigatorRoot->RegisterComponent();
	InstigatorActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
	InstigatorActor->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>(InstigatorActor);
	MeshComp->SetupAttachment(InstigatorRoot);
	MeshComp->RegisterComponent();

	UInteractionDetectorComponent* DetectorComp = NewObject<UInteractionDetectorComponent>(InstigatorActor);
	DetectorComp->MaxTraceDistance = 300.0f;
	DetectorComp->RegisterComponent();

	AActor* TargetActor = World->SpawnActor<AActor>();
	UBoxComponent* TargetBox = NewObject<UBoxComponent>(TargetActor);
	TargetActor->SetRootComponent(TargetBox);
	TargetBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	TargetBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TargetBox->SetCollisionResponseToAllChannels(ECR_Block);
	TargetBox->RegisterComponent();
	TargetActor->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));

	UInteractableComponent* InteractableComp = NewObject<UInteractableComponent>(TargetActor);
	InteractableComp->InteractionModeTag = InteractionNativeTags::Mode_Hold.GetTag();
	InteractableComp->HoldDuration = 2.0f;
	InteractableComp->InteractionDistance = 200.0f;
	InteractableComp->RegisterComponent();

	// Focus target
	DetectorComp->PerformDetection();
	TestNotNull(TEXT("Detector should focus target"), DetectorComp->GetFocusedTarget());

	bool bCanceled = false;
	FGameplayTag CanceledReason;
	DetectorComp->OnInteractionCanceled.AddLambda([&bCanceled, &CanceledReason](FGameplayTag Reason, UObject* Target)
	{
		bCanceled = true;
		CanceledReason = Reason;
	});

	// Start interaction
	DetectorComp->StartInteract();
	TestTrue(TEXT("Detector should be interacting"), DetectorComp->IsInteracting());

	// Simulate montage interruption event dispatched through notify system
	DetectorComp->HandleInteractionEvent(InteractionNativeTags::CancelReason_MontageInterrupted.GetTag());

	TestFalse(TEXT("Detector should no longer be interacting after interruption"), DetectorComp->IsInteracting());
	TestTrue(TEXT("Cancel callback should be invoked"), bCanceled);
	TestEqual(TEXT("Cancel reason should be MontageInterrupted"), CanceledReason, InteractionNativeTags::CancelReason_MontageInterrupted.GetTag());

	World->DestroyWorld(false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
