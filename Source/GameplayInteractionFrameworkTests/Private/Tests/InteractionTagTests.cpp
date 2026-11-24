#include "Misc/AutomationTest.h"
#include "Types/InteractionNativeTags.h"
#include "GameplayTagsManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionTagRegistrationTest, "GameplayInteractionFramework.Tags.Registration", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionTagRegistrationTest::RunTest(const FString& Parameters)
{
	// Validate Interaction Modes
	TestTrue(TEXT("Mode_Instant must be valid"), InteractionNativeTags::Mode_Instant.GetTag().IsValid());
	TestTrue(TEXT("Mode_Hold must be valid"), InteractionNativeTags::Mode_Hold.GetTag().IsValid());
	TestTrue(TEXT("Mode_RapidTap must be valid"), InteractionNativeTags::Mode_RapidTap.GetTag().IsValid());
	TestTrue(TEXT("Mode_Toggle must be valid"), InteractionNativeTags::Mode_Toggle.GetTag().IsValid());

	// Validate Trace Shapes
	TestTrue(TEXT("TraceShape_CameraLine must be valid"), InteractionNativeTags::TraceShape_CameraLine.GetTag().IsValid());
	TestTrue(TEXT("TraceShape_ForwardCone must be valid"), InteractionNativeTags::TraceShape_ForwardCone.GetTag().IsValid());
	TestTrue(TEXT("TraceShape_SphereSweep must be valid"), InteractionNativeTags::TraceShape_SphereSweep.GetTag().IsValid());
	TestTrue(TEXT("TraceShape_BoxSweep must be valid"), InteractionNativeTags::TraceShape_BoxSweep.GetTag().IsValid());

	// Validate Cancellation & Rejection Reasons
	TestTrue(TEXT("CancelReason_None must be valid"), InteractionNativeTags::CancelReason_None.GetTag().IsValid());
	TestTrue(TEXT("CancelReason_OutOfRange must be valid"), InteractionNativeTags::CancelReason_OutOfRange.GetTag().IsValid());
	TestTrue(TEXT("CancelReason_LineOfSightLost must be valid"), InteractionNativeTags::CancelReason_LineOfSightLost.GetTag().IsValid());
	TestTrue(TEXT("CancelReason_KeyReleased must be valid"), InteractionNativeTags::CancelReason_KeyReleased.GetTag().IsValid());
	TestTrue(TEXT("CancelReason_Interrupted must be valid"), InteractionNativeTags::CancelReason_Interrupted.GetTag().IsValid());
	TestTrue(TEXT("CancelReason_ServerRejected must be valid"), InteractionNativeTags::CancelReason_ServerRejected.GetTag().IsValid());
	TestTrue(TEXT("CancelReason_AlreadyInteracted must be valid"), InteractionNativeTags::CancelReason_AlreadyInteracted.GetTag().IsValid());

	// Validate Replicated States
	TestTrue(TEXT("State_Available must be valid"), InteractionNativeTags::State_Available.GetTag().IsValid());
	TestTrue(TEXT("State_Active must be valid"), InteractionNativeTags::State_Active.GetTag().IsValid());
	TestTrue(TEXT("State_Completed must be valid"), InteractionNativeTags::State_Completed.GetTag().IsValid());
	TestTrue(TEXT("State_Cooldown must be valid"), InteractionNativeTags::State_Cooldown.GetTag().IsValid());
	TestTrue(TEXT("State_Locked must be valid"), InteractionNativeTags::State_Locked.GetTag().IsValid());
	TestTrue(TEXT("State_Disabled must be valid"), InteractionNativeTags::State_Disabled.GetTag().IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionTagHierarchyTest, "GameplayInteractionFramework.Tags.Hierarchy", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionTagHierarchyTest::RunTest(const FString& Parameters)
{
	const FGameplayTag ModeRootTag = FGameplayTag::RequestGameplayTag(FName("Interaction.Mode"));
	TestTrue(TEXT("ModeRootTag must be requested successfully"), ModeRootTag.IsValid());

	TestTrue(TEXT("Mode_Instant must match parent Interaction.Mode"), InteractionNativeTags::Mode_Instant.GetTag().MatchesTag(ModeRootTag));
	TestTrue(TEXT("Mode_Hold must match parent Interaction.Mode"), InteractionNativeTags::Mode_Hold.GetTag().MatchesTag(ModeRootTag));
	TestTrue(TEXT("Mode_RapidTap must match parent Interaction.Mode"), InteractionNativeTags::Mode_RapidTap.GetTag().MatchesTag(ModeRootTag));
	TestTrue(TEXT("Mode_Toggle must match parent Interaction.Mode"), InteractionNativeTags::Mode_Toggle.GetTag().MatchesTag(ModeRootTag));

	const FGameplayTag StateRootTag = FGameplayTag::RequestGameplayTag(FName("Interaction.State"));
	TestTrue(TEXT("StateRootTag must be requested successfully"), StateRootTag.IsValid());

	TestTrue(TEXT("State_Available must match parent Interaction.State"), InteractionNativeTags::State_Available.GetTag().MatchesTag(StateRootTag));
	TestTrue(TEXT("State_Active must match parent Interaction.State"), InteractionNativeTags::State_Active.GetTag().MatchesTag(StateRootTag));
	TestTrue(TEXT("State_Completed must match parent Interaction.State"), InteractionNativeTags::State_Completed.GetTag().MatchesTag(StateRootTag));

	return true;
}
