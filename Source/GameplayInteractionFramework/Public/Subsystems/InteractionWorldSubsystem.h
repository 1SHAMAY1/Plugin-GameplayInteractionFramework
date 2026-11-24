#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "InteractionWorldSubsystem.generated.h"

class UInteractableComponent;

/**
 * UInteractionWorldSubsystem
 *
 * World subsystem that tracks all active UInteractableComponent instances in the world.
 * Provides high-performance spatial radius querying and interaction debugging.
 */
UCLASS()
class GAMEPLAYINTERACTIONFRAMEWORK_API UInteractionWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Registers an interactable component with the world spatial registry */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Subsystem")
	void RegisterInteractable(UInteractableComponent* Component);

	/** Unregisters an interactable component from the world spatial registry */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Subsystem")
	void UnregisterInteractable(UInteractableComponent* Component);

	/** Fast broad-phase radius query returning all valid interactables within Radius of Location */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Subsystem")
	int32 GetInteractablesInRadius(const FVector& Location, float Radius, TArray<UInteractableComponent*>& OutInteractables) const;

	/** Gets all currently registered interactables */
	const TArray<TWeakObjectPtr<UInteractableComponent>>& GetAllInteractables() const { return RegisteredInteractables; }

	/** Gets the total count of registered interactables */
	UFUNCTION(BlueprintPure, Category = "Interaction|Subsystem")
	int32 GetRegisteredCount() const;

private:
	/** Internal list of registered interactables */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UInteractableComponent>> RegisteredInteractables;
};
