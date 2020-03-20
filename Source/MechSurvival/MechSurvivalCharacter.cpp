// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MechSurvivalCharacter.h"
#include "MechBase.h"
#include "UpgradeBase.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"

#include "DrawDebugHelpers.h"
#include "Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AMechSurvivalCharacter

AMechSurvivalCharacter::AMechSurvivalCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));
	Mesh1P->SetHiddenInGame(false, true);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Create a gun mesh component
	LaserParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LaserParticle"));
	LaserParticle->SetAutoActivate(false);
	LaserParticle->SetupAttachment(RootComponent);

	LaserSparks = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LaserSparks"));
	LaserSparks->SetAutoActivate(false);
	LaserSparks->SetupAttachment(RootComponent);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.
}

void AMechSurvivalCharacter::damagePlayer(float damage)
{
	health -= damage;
	if (health < 0)
	{
		UGameplayStatics::OpenLevel(GetWorld(), FName("Level_Greybox"));
	}
}

void AMechSurvivalCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMechSurvivalCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AMechSurvivalCharacter::OnFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AMechSurvivalCharacter::OnFireStop);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMechSurvivalCharacter::OnInteract);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AMechSurvivalCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMechSurvivalCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMechSurvivalCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMechSurvivalCharacter::LookUpAtRate);
}

void AMechSurvivalCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (firing)
	{
		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector ShootStart = FP_MuzzleLocation->GetComponentLocation();

		const FVector ShootEnd = ShootStart + GetControlRotation().Vector() * minerRange;

		FHitResult hit;

		FCollisionQueryParams traceParams;
		traceParams.AddIgnoredActor(this);

		GetWorld()->LineTraceSingleByChannel(hit, ShootStart, ShootEnd, ECC_WorldStatic, traceParams);

		LaserParticle->SetBeamSourcePoint(0,ShootStart,0);
		LaserParticle->SetBeamEndPoint(0, (hit.bBlockingHit) ? hit.ImpactPoint : ShootEnd);

		LaserSparks->SetWorldLocation(hit.ImpactPoint);
		LaserSparks->SetWorldRotation(FRotator(0, GetControlRotation().Yaw + 180, 0));

		//DrawDebugLine(GetWorld(), ShootStart, ShootStart + ShootDir.Vector() * minerRange, FColor::Blue, false, 10, 0, 1);

		if (hit.bBlockingHit)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString(hit.Actor.Get()->GetName()));
			AUpgradeBase* upgrade = Cast<AUpgradeBase>(hit.Actor.Get());

			if (upgrade)
			{
				TEnumAsByte<TYPE> queryType = upgrade->getType();

				if (UpgradeType == NONE || queryType == SCRAP)
				{
					TEnumAsByte<TYPE> type = upgrade->mine(DeltaTime);

					if (type == SCRAP)
					{
						scrapAmount++;
						upgrade->Destroy();
					}
					else if (type != NONE && UpgradeType == NONE)
					{
						UpgradeType = type;
						upgrade->Destroy();
					}
				}
			}
			else
			{
				AMechBase* mech = Cast<AMechBase>(hit.Actor.Get());
				if (mech)
				{
					if (UpgradeType != NONE && UpgradeType != SCRAP)
					{
						switch (UpgradeType)
						{
						case JUMP:
							mech->jumpEnabled = true;
							UpgradeType = NONE;
							break;
						case GUN:
							mech->gunEnabled = true;
							UpgradeType = NONE;
							break;
						case BOOST:
							mech->boostEnabled = true;
							UpgradeType = NONE;
							break;
						default:
							break;
						}
					}
					else if (scrapAmount > 0)
					{
						if (mech->healMech(10))
						{
							scrapAmount--;
						}
					}
				}
			}
		}
	}
}

void AMechSurvivalCharacter::OnFire()
{
	firing = true;

	LaserParticle->SetActive(true);
	LaserSparks->SetActive(true);

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AMechSurvivalCharacter::OnFireStop()
{
	firing = false;
	LaserParticle->SetActive(false);
	LaserSparks->SetActive(false);
	LaserParticle->SetBeamSourcePoint(0, GetActorLocation(), 0);
	LaserParticle->SetBeamEndPoint(0, GetActorLocation());
}

void AMechSurvivalCharacter::OnInteract()
{
	FVector camLoc = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();

	FHitResult hit;

	FCollisionQueryParams traceParams;
	traceParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(hit, camLoc, camLoc + (FirstPersonCameraComponent->GetForwardVector() * range), ECC_WorldStatic, traceParams);

	//DrawDebugLine(GetWorld(), camLoc, camLoc + (FirstPersonCameraComponent->GetForwardVector() * range), FColor::Red, false, 10, 0, 1);

	AMechBase* mech = Cast<AMechBase>(hit.Actor);

	if (mech)
	{
		if (mech->mechEnabled)
		{
			OnFireStop();
			//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString("hitMech"));
			mech->setPilot(this);
			SetActorEnableCollision(false);
			GetWorld()->GetFirstPlayerController()->Possess(mech);
		}
	}
}

void AMechSurvivalCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMechSurvivalCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMechSurvivalCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMechSurvivalCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}
