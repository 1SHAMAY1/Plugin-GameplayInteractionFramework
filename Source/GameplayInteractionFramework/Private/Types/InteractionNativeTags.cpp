#include "Types/InteractionNativeTags.h"

namespace InteractionNativeTags
{
	// --- Interaction Modes ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mode_Instant, "Interaction.Mode.Instant", "Single button press triggers immediate execution.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mode_Hold, "Interaction.Mode.Hold", "Requires holding the button for a defined duration.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mode_RapidTap, "Interaction.Mode.RapidTap", "Requires repeatedly tapping the button within a decay window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mode_Toggle, "Interaction.Mode.Toggle", "Alternates between on and off states on successive presses.");

	// --- Trace Shapes ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TraceShape_CameraLine, "Interaction.TraceShape.CameraLine", "Center-screen raycast from player camera view.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TraceShape_ForwardCone, "Interaction.TraceShape.ForwardCone", "Forward cone or sphere sweep from character facing direction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TraceShape_SphereSweep, "Interaction.TraceShape.SphereSweep", "Radius sphere overlap query scored by distance and facing angle.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TraceShape_BoxSweep, "Interaction.TraceShape.BoxSweep", "Directional box sweep for 2.5D or platformer mechanics.");

	// --- Cancellation & Rejection Reasons ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_None, "Interaction.CancelReason.None", "No cancellation occurred.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_OutOfRange, "Interaction.CancelReason.OutOfRange", "Target moved outside interactable radius.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_LineOfSightLost, "Interaction.CancelReason.LineOfSightLost", "Line of sight between interactor and target was obstructed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_KeyReleased, "Interaction.CancelReason.KeyReleased", "Input key was released before hold completed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_Interrupted, "Interaction.CancelReason.Interrupted", "External gameplay interruption (e.g., stunned, damaged, knocked back).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_ServerRejected, "Interaction.CancelReason.ServerRejected", "Server authority rejected interaction attempt.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_AlreadyInteracted, "Interaction.CancelReason.AlreadyInteracted", "Another player already claimed or completed this interaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_InstigatorInvalid, "Interaction.CancelReason.InstigatorInvalid", "Instigator actor is invalid or cannot initiate interactions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_TargetBusy, "Interaction.CancelReason.TargetBusy", "Target is actively being interacted with by another player.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_TimingViolation, "Interaction.CancelReason.TimingViolation", "Server rejected completion because hold duration did not meet required duration (anti-speedhack).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_TimedOut, "Interaction.CancelReason.TimedOut", "Interaction lease expired or client heartbeat timed out.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_TransactionFailed, "Interaction.CancelReason.TransactionFailed", "Asynchronous transaction failed or rolled back.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CancelReason_MontageInterrupted, "Interaction.CancelReason.MontageInterrupted", "Montage was interrupted or blended out before interaction window completed.");

	// --- Animation Notify & Signal Events ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Start, "Interaction.Event.Start", "Signal initiating interaction start.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Commit, "Interaction.Event.Commit", "Signal committing interaction completion on exact visual contact.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Cancel, "Interaction.Event.Cancel", "Signal canceling active interaction.");

	// --- Replicated Interaction States ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Available, "Interaction.State.Available", "Interactable is idle and ready to receive interaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Active, "Interaction.State.Active", "Interaction is actively in progress.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PendingCommit, "Interaction.State.PendingCommit", "Interaction is awaiting asynchronous server validation, database transaction, or external system commit.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Completed, "Interaction.State.Completed", "Interaction has successfully completed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Cooldown, "Interaction.State.Cooldown", "Interactable is temporarily on cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Locked, "Interaction.State.Locked", "Interactable is locked by gameplay tags or puzzle requirements.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Disabled, "Interaction.State.Disabled", "Interactable is completely disabled.");
}
