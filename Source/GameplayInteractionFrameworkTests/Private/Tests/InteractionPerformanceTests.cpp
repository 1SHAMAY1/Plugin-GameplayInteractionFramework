#include "Misc/AutomationTest.h"
#include "Subsystems/InteractionWorldSubsystem.h"
#include "Components/InteractableComponent.h"
#include "Components/InteractionDetectorComponent.h"
#include "FunctionLibraries/InteractionFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionMassCandidateStressBenchmark, "GameplayInteractionFramework.Performance.MassCandidateStress", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionMassCandidateStressBenchmark::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		UInteractionWorldSubsystem* Subsystem = World->GetSubsystem<UInteractionWorldSubsystem>();
		TestNotNull(TEXT("UInteractionWorldSubsystem must exist"), Subsystem);

		if (Subsystem)
		{
			TArray<AActor*> SpawnedActors;
			const int32 GridDim = 32; // 32x32 = 1,024 actors
			const float Spacing = 300.0f; // 300cm apart (~100m total grid extent)

			SpawnedActors.Reserve(GridDim * GridDim);

			// Spawn 1,024 actors across a 100m x 100m area
			for (int32 x = 0; x < GridDim; ++x)
			{
				for (int32 y = 0; y < GridDim; ++y)
				{
					AActor* Prop = World->SpawnActor<AActor>();
					USceneComponent* Root = NewObject<USceneComponent>(Prop);
					Prop->SetRootComponent(Root);
					Root->RegisterComponent();
					Prop->SetActorLocation(FVector(x * Spacing, y * Spacing, 0.0f));

					UInteractableComponent* Comp = NewObject<UInteractableComponent>(Prop);
					Comp->RegisterComponent();
					Subsystem->RegisterInteractable(Comp);

					SpawnedActors.Add(Prop);
				}
			}

			TestEqual(TEXT("Should have 1,024 interactables registered in subsystem"), Subsystem->GetRegisteredCount(), 1024);

			// Center point for queries
			const FVector QueryCenter(16.0f * Spacing, 16.0f * Spacing, 0.0f);
			const float QueryRadius = 1500.0f;

			// Warmup pass
			TArray<UInteractableComponent*> InRangeComps;
			Subsystem->GetInteractablesInRadius(QueryCenter, QueryRadius, InRangeComps);

			// Run 50 benchmark query & scoring iterations
			const int32 BenchmarkIterations = 50;
			const double StartTime = FPlatformTime::Seconds();

			for (int32 Iter = 0; Iter < BenchmarkIterations; ++Iter)
			{
				Subsystem->GetInteractablesInRadius(QueryCenter, QueryRadius, InRangeComps);

				// Score and rank each candidate
				TArray<FInteractionCandidate> Candidates;
				Candidates.Reserve(InRangeComps.Num());

				for (UInteractableComponent* Comp : InRangeComps)
				{
					if (Comp && Comp->GetOwner())
					{
						const float Dist = FVector::Dist(QueryCenter, Comp->GetOwner()->GetActorLocation());
						const float Score = UInteractionFunctionLibrary::CalculateDirectionalScore(Dist, QueryRadius, 1.0f, 0);

						FInteractionCandidate Cand;
						Cand.TargetObject = Comp;
						Cand.Distance = Dist;
						Cand.Score = Score;
						Candidates.Add(Cand);
					}
				}

				Candidates.Sort();
			}

			const double EndTime = FPlatformTime::Seconds();
			const double TotalElapsedMs = (EndTime - StartTime) * 1000.0;
			const double AvgMsPerPass = TotalElapsedMs / static_cast<double>(BenchmarkIterations);

			AddInfo(FString::Printf(TEXT("Mass Candidate Stress Benchmark (1,024 actors): %.4f ms average per spatial query + score pass"), AvgMsPerPass));

			// Performance Gate: must execute well within budget (< 0.50 ms for 1,024 actors)
			TestTrue(TEXT("Average spatial query + scoring time must be < 0.50 ms"), AvgMsPerPass < 0.50);

			for (AActor* Actor : SpawnedActors)
			{
				Actor->Destroy();
			}
		}

		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionThrottledTickBenchmark, "GameplayInteractionFramework.Performance.ThrottledTickSavings", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionThrottledTickBenchmark::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PlayerActor = World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(PlayerActor);
		PlayerActor->SetRootComponent(Root);
		Root->RegisterComponent();

		UInteractionDetectorComponent* Detector = NewObject<UInteractionDetectorComponent>(PlayerActor);
		Detector->QueryInterval = 0.0333f; // 30 Hz throttling
		Detector->RegisterComponent();

		// Simulate 1,000 engine frames at 120 FPS (0.008333s delta time per frame)
		const int32 TotalFrames = 1000;
		const float DeltaTime120Fps = 0.008333f;

		Detector->ResetDetectionPassCount();

		for (int32 Frame = 0; Frame < TotalFrames; ++Frame)
		{
			Detector->TickComponent(DeltaTime120Fps, LEVELTICK_All, nullptr);
		}

		const int32 ExecutedPasses = Detector->GetDetectionPassCount();
		const int32 SkippedFrames = TotalFrames - ExecutedPasses;
		const float SavingsRatio = static_cast<float>(SkippedFrames) / static_cast<float>(TotalFrames);

		AddInfo(FString::Printf(TEXT("Throttled Tick Benchmark: Out of %d frames @ 120 FPS, executed %d detection passes and skipped %d frames (%.1f%% trace savings)"),
			TotalFrames, ExecutedPasses, SkippedFrames, SavingsRatio * 100.0f));

		// At 30 Hz with 120 FPS frames, should skip ~75% of frames (>60% savings threshold)
		TestTrue(TEXT("Throttled ticking must skip >60% of engine frames"), SavingsRatio >= 0.60f);

		PlayerActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}
