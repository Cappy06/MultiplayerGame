// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainG/AlienTypes/TurningInPlace.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "AlienCharacter.generated.h"

UCLASS()
class MAING_API AAlienCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAlienCharacter();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
	// function we need to override in any class where we plan on replication variables is a virtual
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
	// in side this getlifetimereplicatedprops we register variables to be replicated

	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bAiming);

	void PlayElimMontage();

	virtual void OnRep_ReplicatedMovement() override;

	void Elim();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	virtual void Destroyed() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();

	UFUNCTION()
	void ReceiveDamage(AActor *DamagedActor, float Damage, const UDamageType *DamageType, class AController *InstigatorController, AActor *DamageCauser);
	void UpdateHUDHealth();
	// Poll for any reelvant classes and intializae the hud
	void PollInit();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent *CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent *FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent *OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon *OverlappingWeapon; // variable to be replicated which means when it changes on the server, then wed like change s on the client

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon *LastWeapon); // rep notifies - this function called ont the client when overlapping weapon replicates on the client
													   // rep notify can have input parameters of only of the type that is being called like overlapping weapin is being called so it should be type of class AWeapon

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent *Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed(); // reliable as in multiplayer DOc, execution on Server so RPC is prefixed by Server

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage *FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage *HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage *ElimMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();
	/**
	 * Player health
	 */

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	class AAlienPlayerController *AlienPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();
	/**
	 * Dissolve effect
	 */

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent *DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat *DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic *DynamicDissolveMaterialInstance;

	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic *DynamicDissolveMaterialInstance1;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance *DissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance *DissolveMaterialInstance1;

	/**
	 * Elim bot
	 */

	UPROPERTY(EditAnywhere)
	UParticleSystem *ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent *ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue *ElimBotSound;

	UPROPERTY()
	class AAlienPlayerState *AlienPlayerState;

public:
	void SetOverlappingWeapon(AWeapon *Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon *GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent *GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
};
