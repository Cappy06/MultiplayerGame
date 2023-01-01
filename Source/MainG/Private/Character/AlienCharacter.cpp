// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/AlienCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "AlienComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Character/AlienAnimInstance.h"
#include "MainG/MainG.h"
#include "PlayerController/AlienPlayerController.h"
#include "GameMode/AlienGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerState/AlienPlayerState.h"

// Sets default values
AAlienCharacter::AAlienCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh()); // camera boom ajusted as per collosion mesh
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true; // rotating the camera boom along with controller

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// widget construction whenever player loads a new
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	// combat variable to be replicated
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void AAlienCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// registering overlapping variables to be replicated
	// macro
	DOREPLIFETIME_CONDITION(AAlienCharacter, OverlappingWeapon, COND_OwnerOnly); // DOREPLIFETIME(Where the replication class resides, replication vriable)
																				 // overlapping weapon only replicate on the client - So in here we see on the server as well as on the client(owner, like I am controlling the clinet), then I am the owner

	DOREPLIFETIME(AAlienCharacter, Health);
}
void AAlienCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}
void AAlienCharacter::Elim()
{
	// drop the weapon if get killed
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AAlienCharacter::ElimTimerFinished,
		ElimDelay);
}
// Called when the game starts or when spawned
void AAlienCharacter::MulticastElim_Implementation()
{
	bElimmed = true;
	PlayElimMontage();
	// Start dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);

		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	if (DissolveMaterialInstance1)
	{
		DynamicDissolveMaterialInstance1 = UMaterialInstanceDynamic::Create(DissolveMaterialInstance1, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance1);

		DynamicDissolveMaterialInstance1->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance1->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();
	// Disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (AlienPlayerController)
	{
		DisableInput(AlienPlayerController);
	}
	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// spawn elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation());
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation());
	}
}

void AAlienCharacter::ElimTimerFinished()
{
	AAlienGameMode *AlienGameMode = GetWorld()->GetAuthGameMode<AAlienGameMode>();
	if (AlienGameMode)
	{
		AlienGameMode->RequestRespawn(this, Controller);
	}
}

void AAlienCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}
}

void AAlienCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateHUDHealth();
	if (HasAuthority())
	{

		OnTakeAnyDamage.AddDynamic(this, &AAlienCharacter::ReceiveDamage);
	}
}
// Called every frame
void AAlienCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// as soon as overlapping weapon is set on the server, then all client will be showing that pickup widget
	// so we show the pickupwidget to true - in every frame. Not optimal, but we will learn about the approacj
	/*	if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}*/
	// AimOffset(DeltaTime);
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
	HideCameraIfCharacterClose();
	PollInit();
}

// Called to bind functionality to input
void AAlienCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	// Super::SetupPlayerInputComponent(PlayerInputComponent);
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAlienCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AAlienCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &AAlienCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AAlienCharacter::LookUp);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &AAlienCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AAlienCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AAlienCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AAlienCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AAlienCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AAlienCharacter::FireButtonReleased);
}
void AAlienCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr)
		return;
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // standing still, not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		// bUseControllerRotationYaw = false;

		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}
void AAlienCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AAlienCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	// UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void AAlienCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void AAlienCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void AAlienCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}
/*void AAlienCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}*/
void AAlienCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled())
		return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}
void AAlienCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}
void AAlienCharacter::UpdateHUDHealth()
{
	AlienPlayerController = AlienPlayerController == nullptr ? Cast<AAlienPlayerController>(Controller) : AlienPlayerController;
	if (AlienPlayerController)
	{
		AlienPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}
void AAlienCharacter::PollInit()
{
	if (AlienPlayerState == nullptr)
	{
		AlienPlayerState = GetPlayerState<AAlienPlayerState>();
		if (AlienPlayerState)
		{
			AlienPlayerState->AddToScore(0.f);
			AlienPlayerState->AddToDefeats(0);
		}
	}
}
void AAlienCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
	if (DynamicDissolveMaterialInstance1)
	{
		DynamicDissolveMaterialInstance1->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void AAlienCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AAlienCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}
void AAlienCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}
void AAlienCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;

	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}
void AAlienCharacter::ReceiveDamage(AActor *DamagedActor, float Damage, const UDamageType *DamageType, AController *InstigatorController, AActor *DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();
	if (Health == 0.f)
	{
		AAlienGameMode *AlienGameMode = GetWorld()->GetAuthGameMode<AAlienGameMode>();
		if (AlienGameMode)
		{
			AlienPlayerController = AlienPlayerController == nullptr ? Cast<AAlienPlayerController>(Controller) : AlienPlayerController;
			AAlienPlayerController *AttackerController = Cast<AAlienPlayerController>(InstigatorController);
			AlienGameMode->PlayerEliminated(this, AlienPlayerController, AttackerController);
		}
	}
}
void AAlienCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}
void AAlienCharacter::PlayElimMontage()
{
	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}
void AAlienCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;

	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AAlienCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}
void AAlienCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}
void AAlienCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}
// whenevr a client presses something, it doesnt replciated in other client - so this is the example
void AAlienCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}
void AAlienCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}
// Finishing

// crouching is replicated, so no use of informing the server
void AAlienCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch(); // contains inbuilt replication, coruching is inbuilt function for character class
	}
}

void AAlienCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AAlienCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}
float AAlienCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void AAlienCharacter::SetOverlappingWeapon(AWeapon *Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	// for server - to see EText
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled()) // character is actually being controlled whethter the client or server
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}
bool AAlienCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}
AWeapon *AAlienCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr)
		return nullptr;
	return Combat->EquippedWeapon;
}
void AAlienCharacter::OnRep_OverlappingWeapon(AWeapon *LastWeapon) // repnotifiers for client only
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true); // we only see the widget text E-Only pickup to be replicated on thec client which plays, we dont see on the server here because rep notifies dont get called on the serer. They only get called when the variable replicates and replication only works one way from server to client
	}
	// Lastweapon is the pointer to the previous weapon after it was overlapped and the character moved away, so it should be hidden, so the lastweapon's show pickupwidge t is set to false
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}
bool AAlienCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

FVector AAlienCharacter::GetHitTarget() const
{
	if (Combat == nullptr)
		return FVector();
	return Combat->HitTarget;
}