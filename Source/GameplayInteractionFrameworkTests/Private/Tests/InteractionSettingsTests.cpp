#include "Misc/AutomationTest.h"
#include "Settings/GameplayInteractionSettings.h"
#include "Types/InteractionNativeTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionSettingsValidationTest, "GameplayInteractionFramework.Settings.Validation", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionSettingsValidationTest::RunTest(const FString& Parameters)
{
	const UGameplayInteractionSettings* Settings = UGameplayInteractionSettings::Get();
	TestNotNull(TEXT("GameplayInteractionSettings must be instantiated"), Settings);

	if (Settings)
	{
		TestTrue(TEXT("DefaultInteractionDistance must be greater than 0"), Settings->DefaultInteractionDistance > 0.0f);
		TestTrue(TEXT("DefaultQueryInterval must be greater than 0"), Settings->DefaultQueryInterval > 0.0f);
		TestTrue(TEXT("NetworkDistanceTolerance must be greater than 0"), Settings->NetworkDistanceTolerance > 0.0f);
		TestTrue(TEXT("NetworkHoldTimeTolerance must be greater than 0"), Settings->NetworkHoldTimeTolerance > 0.0f);

		TestTrue(TEXT("WeightFacingAngle must be non-negative"), Settings->WeightFacingAngle >= 0.0f);
		TestTrue(TEXT("WeightDistance must be non-negative"), Settings->WeightDistance >= 0.0f);
		TestTrue(TEXT("WeightPriority must be non-negative"), Settings->WeightPriority >= 0.0f);

		const float TotalWeight = Settings->WeightFacingAngle + Settings->WeightDistance + Settings->WeightPriority;
		TestNearlyEqual(TEXT("Candidate scoring weights should sum to 1.0"), TotalWeight, 1.0f, 0.01f);

		TestTrue(TEXT("DefaultInteractionModeTag must be valid"), Settings->DefaultInteractionModeTag.IsValid());
		TestTrue(TEXT("DefaultTraceShapeTag must be valid"), Settings->DefaultTraceShapeTag.IsValid());
	}

	return true;
}
