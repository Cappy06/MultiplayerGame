// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/AlienAnimInstance.h"
#include "Character/AlienCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

void UAlienAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    AlienCharacter = Cast<AAlienCharacter>(TryGetPawnOwner());
}

void UAlienAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);

    if (AlienCharacter == nullptr)
    {
        AlienCharacter = Cast<AAlienCharacter>(TryGetPawnOwner());
    }
    if (AlienCharacter == nullptr)
        return;

    FVector Velocity = AlienCharacter->GetVelocity();
    Velocity.Z = 0.f;
    Speed = Velocity.Size();

    bIsInAir = AlienCharacter->GetCharacterMovement()->IsFalling();
    bIsAccelerating = AlienCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
    bWeaponEquipped = AlienCharacter->IsWeaponEquipped();
    EquippedWeapon = AlienCharacter->GetEquippedWeapon();
    bIsCrouched = AlienCharacter->bIsCrouched;
    bAiming = AlienCharacter->IsAiming();
    TurningInPlace = AlienCharacter->GetTurningInPlace();
    bRotateRootBone = AlienCharacter->ShouldRotateRootBone();
    bElimmed = AlienCharacter->IsElimmed();

    // Offset Yaw for Strafing
    FRotator AimRotation = AlienCharacter->GetBaseAimRotation();
    FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(AlienCharacter->GetVelocity());
    YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

    CharacterRotationLastFrame = CharacterRotation;
    CharacterRotation = AlienCharacter->GetActorRotation();
    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
    const float Target = Delta.Yaw / DeltaTime;
    const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
    Lean = FMath::Clamp(Interp, -90.f, 90.f);
    AO_Yaw = AlienCharacter->GetAO_Yaw();
    AO_Pitch = AlienCharacter->GetAO_Pitch();
    if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && AlienCharacter->GetMesh())
    {
        LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
        FVector OutPosition;
        FRotator OutRotation;
        AlienCharacter->GetMesh()->TransformToBoneSpace(FName("RightHand"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
        LeftHandTransform.SetLocation(OutPosition);
        LeftHandTransform.SetRotation(FQuat(OutRotation));
        if (AlienCharacter->IsLocallyControlled())
        {
            bLocallyControlled = true;
            FTransform RightHandTransform = AlienCharacter->GetMesh()->GetSocketTransform(FName("RightHand"), ERelativeTransformSpace::RTS_World);
            // RightHandRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - AlienCharacter->GetHitTarget()));
            FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - AlienCharacter->GetHitTarget()));
            RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
        }
    }
}
