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

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(Mesh1P);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Create a gun mesh component
	LaserParticle1 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LaserParticle1"));
	LaserParticle1->SetAutoActivate(false);
	LaserParticle1->SetupAttachment(RootComponent);

	LaserParticle2 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LaserParticle2"));
	LaserParticle2->SetAutoActivate(false);
	LaserParticle2->SetupAttachment(RootComponent);

	LaserParticle3 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LaserParticle3"));
	LaserParticle3->SetAutoActivate(false);
	LaserParticle3->SetupAttachment(RootComponent);

	LaserParticle4 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LaserParticle4"));
	LaserParticle4->SetAutoActivate(false);
	LaserParticle4->SetupAttachment(RootComponent);

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
		UGameplayStatics::OpenLevel(GetWorld(), restartLevel);
	}
	// try and play the sound if specified
	if (PlayerHit != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PlayerHit, GetActorLocation(), 1.0f);
	}
}

void AMechSurvivalCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	baseMovement = GetCharacterMovement()->MaxWalkSpeed;

	GetCharacterMovement()->MaxWalkSpeed = (armed) ? baseMovement : baseMovement * sprintMultiplier;

	// try and play the sound if specified
	if (PlayerScan != NULL)
	{
		ActivePlayerScan = UGameplayStatics::CreateSound2D(this, PlayerScan, 1.0f);
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString("initalise"));
	}
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

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &AMechSurvivalCharacter::SwitchEquip);

	PlayerInputComponent->BindAction("Escape", IE_Pressed, this, &AMechSurvivalCharacter::OnEscape);

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

	if (firing && canShoot)
	{
		LaserParticle1->Activate();
		LaserParticle2->Activate();
		LaserParticle3->Activate();
		LaserParticle4->Activate();
		LaserSparks->Activate();

		const FVector ShootStart = Mesh1P->GetBoneLocation(FName("MultiTool_BF1"));

		const FVector ShootEnd = ShootStart + GetControlRotation().Vector() * minerRange;

		FHitResult hit;

		FCollisionQueryParams traceParams;
		traceParams.AddIgnoredActor(this);

		GetWorld()->LineTraceSingleByChannel(hit, ShootStart, ShootEnd, ECC_WorldStatic, traceParams);

		FVector LaserStart = Mesh1P->GetBoneLocation(FName("MultiTool_BF1"));

		LaserParticle1->SetBeamSourcePoint(0,LaserStart,0);
		LaserParticle1->SetBeamEndPoint(0, (hit.bBlockingHit) ? hit.ImpactPoint : ShootEnd);

		LaserStart = Mesh1P->GetBoneLocation(FName("MultiTool_BF2"));

		LaserParticle2->SetBeamSourcePoint(0, LaserStart, 0);
		LaserParticle2->SetBeamEndPoint(0, (hit.bBlockingHit) ? hit.ImpactPoint : ShootEnd);

		LaserStart = Mesh1P->GetBoneLocation(FName("MultiTool_BF3"));

		LaserParticle3->SetBeamSourcePoint(0, LaserStart, 0);
		LaserParticle3->SetBeamEndPoint(0, (hit.bBlockingHit) ? hit.ImpactPoint : ShootEnd);

		LaserStart = Mesh1P->GetBoneLocation(FName("MultiTool_BF4"));

		LaserParticle4->SetBeamSourcePoint(0, LaserStart, 0);
		LaserParticle4->SetBeamEndPoint(0, (hit.bBlockingHit) ? hit.ImpactPoint : ShootEnd);

		LaserSparks->SetWorldLocation(hit.ImpactPoint);
		LaserSparks->SetWorldRotation(FRotator(0, GetControlRotation().Yaw + 180, 0));

		//DrawDebugLine(GetWorld(), ShootStart, ShootEnd, FColor::Blue, false, 10, 0, 1);

		if (ActivePlayerScan && !ActivePlayerScan->IsPlaying())
		{
			//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString("play"));
			//ActivePlayerScan->Play();
		}

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
	else if (firing && !armed)
	{
		//OnFireStop();
		LaserParticle1->Deactivate();
		LaserParticle2->Deactivate();
		LaserParticle3->Deactivate();
		LaserParticle4->Deactivate();
		LaserSparks->Deactivate();
		LaserParticle1->SetBeamSourcePoint(0, GetActorLocation(), 0);
		LaserParticle2->SetBeamSourcePoint(0, GetActorLocation(), 0);
		LaserParticle3->SetBeamSourcePoint(0, GetActorLocation(), 0);
		LaserParticle4->SetBeamSourcePoint(0, GetActorLocation(), 0);
		LaserParticle1->SetBeamEndPoint(0, GetActorLocation());
		LaserParticle2->SetBeamEndPoint(0, GetActorLocation());
		LaserParticle3->SetBeamEndPoint(0, GetActorLocation());
		LaserParticle4->SetBeamEndPoint(0, GetActorLocation());
		if (ActivePlayerScan && ActivePlayerScan->IsPlaying())
		{
			//ActivePlayerScan->Stop();
		}
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString("stop"));
	}
}

void AMechSurvivalCharacter::OnFire()
{
	firing = true;

	//// try and play the sound if specified
	//if (FireSound != NULL)
	//{
	//	UGameplayStatics::PlaySound2D(this, FireSound);
	//}
}

void AMechSurvivalCharacter::OnFireStop()
{
	firing = false;
	LaserParticle1->Deactivate();
	LaserParticle2->Deactivate();
	LaserParticle3->Deactivate();
	LaserParticle4->Deactivate();
	LaserSparks->Deactivate();
	LaserParticle1->SetBeamSourcePoint(0, GetActorLocation(), 0);
	LaserParticle2->SetBeamSourcePoint(0, GetActorLocation(), 0);
	LaserParticle3->SetBeamSourcePoint(0, GetActorLocation(), 0);
	LaserParticle4->SetBeamSourcePoint(0, GetActorLocation(), 0);
	LaserParticle1->SetBeamEndPoint(0, GetActorLocation());
	LaserParticle2->SetBeamEndPoint(0, GetActorLocation());
	LaserParticle3->SetBeamEndPoint(0, GetActorLocation());
	LaserParticle4->SetBeamEndPoint(0, GetActorLocation());
	if (ActivePlayerScan && ActivePlayerScan->IsPlaying())
	{
		//ActivePlayerScan->Stop();
	}
	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString("stop"));
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
			// try and play the sound if specified
			if (MechEnter != NULL)
			{
				UGameplayStatics::PlaySoundAtLocation(this, MechEnter, mech->GetActorLocation(), 1.0f);
			}
		}
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString("Mech broken find scrap to repair"));
		}
	}
}

void AMechSurvivalCharacter::SwitchEquip()
{
	armed = !armed;
	GetCharacterMovement()->MaxWalkSpeed = (armed) ? baseMovement : baseMovement * sprintMultiplier;
}

void AMechSurvivalCharacter::OnEscape()
{
	UGameplayStatics::OpenLevel(GetWorld(), restartLevel);
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
