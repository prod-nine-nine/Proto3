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

	Mesh3P = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CharacterMesh3P"));
	Mesh3P->SetOwnerNoSee(true);
	Mesh3P->bCastDynamicShadow = false;
	Mesh3P->CastShadow = false;
	Mesh3P->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(Mesh1P);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.
}

// Called when the game starts or when spawned
void AMechBase::BeginPlay()
{
	Super::BeginPlay();

	//jumpMin = GetCharacterMovement()->JumpZVelocity - jumpDiff;
	basePlayerMovement = GetCharacterMovement()->MaxWalkSpeed;

	MI = UMaterialInstanceDynamic::Create(Mesh3P->GetMaterial(0), this);
	Mesh3P->SetMaterial(0, MI);

	SpawnDefaultController();
}

// Called every frame
void AMechBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//if (chargingJump && jumpChargeTime + DeltaTime <= maxJumpChargeTime)
	//{
	//	jumpChargeTime += DeltaTime;
	//	if (jumpChargeTime > maxJumpChargeTime * 0.1 && !moveChangeOnce)
	//	{
	//		GetCharacterMovement()->MaxWalkSpeed = basePlayerMovement * 0.2;
	//		moveChangeOnce = true;
	//	}
	//	//GEngine->AddOnScreenDebugMessage(-1, 0.5, FColor::Purple, FString::Printf(TEXT("%f"), jumpChargeTime));
	//}
	//else if (chargingJump && jumpChargeTime < maxJumpChargeTime)
	//{
	//	jumpChargeTime = maxJumpChargeTime;
	//}

	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("%d"), canBoostJump));

	if (GetVelocity().Z < -10 && !canBoostJump && jumping)
	{
		canBoostJump = true;
	}

	if (jumping && jumpChargeTime + DeltaTime <= maxJumpChargeTime && canBoostJump)
	{
		LaunchCharacter(GetActorUpVector() * jumpStrength , false, true);
		jumpChargeTime += DeltaTime;
	}
	else if (!jumping && !(GetCharacterMovement()->IsFalling()))
	{
		jumpChargeTime = 0;
	}


	if (boost)
	{
		LaunchCharacter(GetActorForwardVector() * boostAmount * DeltaTime, false, false);
		boostTimer += DeltaTime;
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("booooost : %f"), boostAmount*DeltaTime));
		if (boostTimer > maxBoostTime)
		{
			BoostOff();
		}
	}

	if (currentDurability <= 0 && mechEnabled)
	{
		mechEnabled = false;
		if (pilot)
		{
			OnInteract();
		}
	}
	else if (currentDurability > 0 && !mechEnabled)
	{
		mechEnabled = true;
	}

	float scalar = (1 - (((currentDurability / maxDurability) * 0.5)+0.5 ));

	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("%f"), scalar));

	MI->SetScalarParameterValue(FName("Amount"), scalar);
}

// Called to bind functionality to input
void AMechBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMechBase::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMechBase::StopJumping);

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
	if (!gunEnabled) { return; }
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{

			const FRotator SpawnRotation = GetControlRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : FVector());

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
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation(), 1.0f);
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

//void AMechBase::chargeJump()
//{
//	if (!jumpEnabled) { return; }
//	moveChangeOnce = false;
//	jumpChargeTime = 0;
//	chargingJump = true;
//}

void AMechBase::Jump()
{
	if (jumpEnabled && !(GetCharacterMovement()->IsFalling()))
	{
		jumping = true;
		ACharacter::Jump();
	}

	/*if (!GetCharacterMovement()->IsFalling() && jumpEnabled)
	{
		float actualJump = jumpMin + ((jumpChargeTime / maxJumpChargeTime) * jumpDiff);

		LaunchCharacter(FVector(0, 0, actualJump), false, true);
	}

	chargingJump = false;

	GetCharacterMovement()->MaxWalkSpeed = basePlayerMovement;*/
}

void AMechBase::StopJumping()
{
	jumping = false;
	canBoostJump = false;
	ACharacter::StopJumping();
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