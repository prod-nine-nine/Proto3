// Fill out your copyright notice in the Description page of Project Settings.


#include "MechBase.h"
#include "MechSurvivalProjectile.h"
#include "MechSurvivalCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"

#include "DrawDebugHelpers.h"
#include "Engine.h"

// Sets default values
AMechBase::AMechBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule

	GetCapsuleComponent()->InitCapsuleSize(55.f* mechScale, 96.0f * mechScale);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f * mechScale, 1.75f * mechScale, 64.f * mechScale)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f * mechScale, -4.4f * mechScale, -155.7f * mechScale));
	Mesh1P->SetRelativeScale3D(FVector(mechScale, mechScale, mechScale));

	Mesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh3P"));
	Mesh3P->SetOwnerNoSee(true);
	Mesh3P->bCastDynamicShadow = false;
	Mesh3P->CastShadow = false;
	Mesh3P->SetupAttachment(RootComponent);

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

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.
}

// Called when the game starts or when spawned
void AMechBase::BeginPlay()
{
	Super::BeginPlay();
	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	jumpMin = GetCharacterMovement()->JumpZVelocity - jumpDiff;
	basePlayerMovement = GetCharacterMovement()->MaxWalkSpeed;

	SpawnDefaultController();
}

// Called every frame
void AMechBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (chargingJump && jumpChargeTime + DeltaTime <= maxJumpChargeTime)
	{
		jumpChargeTime += DeltaTime;
		if (jumpChargeTime > maxJumpChargeTime * 0.1 && !moveChangeOnce)
		{
			GetCharacterMovement()->MaxWalkSpeed = basePlayerMovement * 0.2;
			moveChangeOnce = true;
		}
		//GEngine->AddOnScreenDebugMessage(-1, 0.5, FColor::Purple, FString::Printf(TEXT("%f"), jumpChargeTime));
	}
	else if (chargingJump && jumpChargeTime < maxJumpChargeTime)
	{
		jumpChargeTime = maxJumpChargeTime;
	}


	if (boost)
	{
		LaunchCharacter(GetActorForwardVector() * 100, false, false);
		boostTimer += DeltaTime;
		if (boostTimer > maxBoostTime)
		{
			BoostOff();
		}
	}
}

// Called to bind functionality to input
void AMechBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMechBase::chargeJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMechBase::jump);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AMechBase::OnFire);

	//Bind interact event
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMechBase::OnInteract);

	//bind boost button
	PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &AMechBase::BoostOn);
	PlayerInputComponent->BindAction("Boost", IE_Released, this, &AMechBase::BoostOff);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AMechBase::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMechBase::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMechBase::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMechBase::LookUpAtRate);

}

void AMechBase::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{

			const FRotator SpawnRotation = GetControlRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			// spawn the projectile at the muzzle
			World->SpawnActor<AMechSurvivalProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
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

void AMechBase::OnInteract()
{
	if (pilot)
	{
		pilot->SetActorLocation(GetActorLocation() + GetActorForwardVector() * 200);
		pilot->SetActorEnableCollision(true);
		pilot->SetActorRotation(GetActorRotation());
		GetWorld()->GetFirstPlayerController()->Possess(pilot);
		SpawnDefaultController();
		pilot = 0;
	}
}

void AMechBase::chargeJump()
{
	moveChangeOnce = false;
	jumpChargeTime = 0;
	chargingJump = true;
}

void AMechBase::jump()
{
	if (!GetCharacterMovement()->IsFalling())
	{
		float actualJump = jumpMin + ((jumpChargeTime / maxJumpChargeTime) * jumpDiff);

		LaunchCharacter(FVector(0, 0, actualJump), false, true);
	}

	chargingJump = false;

	GetCharacterMovement()->MaxWalkSpeed = basePlayerMovement;
}

void AMechBase::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMechBase::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMechBase::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMechBase::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}