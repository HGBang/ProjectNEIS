// Fill out your copyright notice in the Description page of Project Settings.


#include "AuroraAICharacter.h"
#include "AuroraAIAnimInstance.h"
#include "AuroraAIController.h"
#include "../../Effect/DefaultEffect.h"
#include "../AIState.h"
#include "../../Player/PlayerCharacter.h"
#include "../../Player/KHH/TerraCharacter.h"
#include "../../UI/KHH/FrozenCaveWidget.h"
#include "../../GameMode/KHH/IceLandGameModeBase.h"
#include "../../Effect/DefaultEffect.h"
#include "../../GameMode/KHH/IceLandGameModeBase.h"

AAuroraAICharacter::AAuroraAICharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	m_Name = TEXT("Aurora");

	m_StartEnd = false;

	m_IsDass = false;

	m_AttackEnd = false;

	// Dissolve 세팅
	m_DissCurTime = 0.f;
	m_DissTime = 1.5f;
	m_DissEnable = false;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh>	MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/ParagonAurora/Characters/Heroes/Aurora/Skins/GlacialEmpress/Meshes/Aurora_GlacialEmpress.Aurora_GlacialEmpress'"));

	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance>	AnimClass(TEXT("/Script/Engine.AnimBlueprint'/Game/KHH/AI/AB_AuroraAI.AB_AuroraAI_C'"));

	if (AnimClass.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimClass.Class);
	}

	m_DashAttack = CreateDefaultSubobject<USphereComponent>(TEXT("DashAttack"));

	m_DashAttack->SetupAttachment(GetCapsuleComponent());

	m_DashAttack->SetRelativeLocation(FVector(0.0, 0.0, -92.0));

	m_DashAttack->InitSphereRadius(180.f);

	m_DashAttack->SetCollisionProfileName(TEXT("AIAttackOverlap"));

	GetCapsuleComponent()->SetCapsuleHalfHeight(92.f);

	GetMesh()->SetRelativeLocation(FVector(0.0, 0.0, -92.0));

	GetMesh()->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));	

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("AI"));

	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);

	GetCapsuleComponent()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	m_State = EAICharacterState::Alive;

	AIControllerClass = nullptr;

	AIControllerClass = AAuroraAIController::StaticClass();
}

void AAuroraAICharacter::BeginPlay()
{
	Super::BeginPlay();

	m_DashAttack->OnComponentBeginOverlap.AddDynamic(this, &AAuroraAICharacter::Dash);

	m_DashAttack->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	int32 ElementCount = GetMesh()->GetNumMaterials();

	for (int32 i = 0; i < ElementCount; ++i)
	{
		m_MtrlArray.Add(GetMesh()->CreateDynamicMaterialInstance(i));
	}

	m_Anim = Cast<UAuroraAIAnimInstance>(GetMesh()->GetAnimInstance());

	m_Anim->ChangeAnim(EAIAnimType::Start);

	AIceLandGameModeBase* GameMode = Cast<AIceLandGameModeBase>(GetWorld()->GetAuthGameMode());

	m_FrozenCaveWidget = GameMode->GetFrozenCaveWidget();

	m_FrozenCaveWidget->SetCurHP(m_AIState->GetHPMax());
	m_FrozenCaveWidget->SetMaxHP(m_AIState->GetHPMax());
}

void AAuroraAICharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AAuroraAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	m_FrozenCaveWidget->SetCurHP(m_AIState->GetHP());

	if (m_IsDass)
	{
		FVector DashDirection = GetActorForwardVector();
		FVector DashVelocity = DashDirection * 10000;

		GetCharacterMovement()->Velocity += DashVelocity * DeltaTime * 2.0;
	}

	for (int i = 0; i < m_SkillCoolTime.Num(); i++)
	{
		if (m_SkillCoolTime[i].IsCool)
		{
			m_SkillCoolTime[i].CurCooltime += DeltaTime;

			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%f"), m_SkillCoolTime[i].CurCooltime));

			if (m_SkillCoolTime[i].CurCooltime >= m_SkillCoolTime[i].MaxCooltime)
			{
				m_SkillCoolTime[i].IsCool = false;
				m_SkillCoolTime[i].CurCooltime = 0;
			}
		}
	}

	if (m_DissEnable)
	{
		m_DissCurTime += DeltaTime;

		if (m_DissCurTime >= m_DissTime)
		{
			AIceLandGameModeBase* GameMode = Cast<AIceLandGameModeBase>(GetWorld()->GetAuthGameMode());

			GameMode->SetFadeOut(true);

			Destroy();
		}

		// 비율을 구한다.
		float Ratio = m_DissCurTime / m_DissTime;
		Ratio = 1.f - Ratio;
		Ratio = Ratio * 2.f - 1.f;

		for (auto& Mtrl : m_MtrlArray)
		{
			Mtrl->SetScalarParameterValue(TEXT("Dissolve"), Ratio);
		}
	}
}

void AAuroraAICharacter::SetAttackIndex(int Index)
{
	UAuroraAIAnimInstance* Anim = Cast<UAuroraAIAnimInstance>(GetMesh()->GetAnimInstance());

	Anim->SetAttackIndex(Index);
}

void AAuroraAICharacter::SetSkillCoolTime(float CoolTime, float MaxCoolTime, bool IsCool)
{
	FAuroraSkillCoolTime SkillCoolTIme;

	SkillCoolTIme.CurCooltime = 0;
	SkillCoolTIme.MaxCooltime = MaxCoolTime;
	SkillCoolTIme.IsCool = false;

	m_SkillCoolTime.Add(SkillCoolTIme);
}

float AAuroraAICharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), m_AIState->GetHP()));

	if (Damage > 0.f)
	{
		m_AIState->SetDamage(Damage);
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), m_AIState->GetHP()));

	return Damage;
}

void AAuroraAICharacter::Attack()
{
	UAuroraAIAnimInstance* Anim = Cast<UAuroraAIAnimInstance>(GetMesh()->GetAnimInstance());

	Anim->Attack();
}

void AAuroraAICharacter::PlaySkill(int Index)
{
	UAuroraAIAnimInstance* Anim = Cast<UAuroraAIAnimInstance>(GetMesh()->GetAnimInstance());

	Anim->TestSkill(Index);
}

void AAuroraAICharacter::Death()
{
	m_State = EAICharacterState::Death;

	/*GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);*/

	for (auto& Mtrl : m_MtrlArray)
	{
		Mtrl->SetScalarParameterValue(TEXT("DissolveEnable"), 1.f);
	}

	m_DissEnable = true;
}

void AAuroraAICharacter::Freeze()
{
	FVector Start = GetActorLocation() - GetActorUpVector() * GetHalfHeight();

	FVector End = Start;

	FHitResult result;

	FCollisionQueryParams param(NAME_None, false, this);

	bool Collision = GetWorld()->SweepSingleByChannel(result, Start, End, FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel11, FCollisionShape::MakeSphere(400.f), param);

#if ENABLE_DRAW_DEBUG

	FColor DrawColor = Collision ? FColor::Red : FColor::Green;

	DrawDebugSphere(GetWorld(), (Start + End) / 2.f, 400.f, 20, DrawColor, false, 20.f);

#endif

	if (Collision)
	{
		FActorSpawnParameters ActorParam;
		ActorParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FHitResult LineResult;

		FDamageEvent DmgEvent;

		ATerraCharacter* Terra = Cast<ATerraCharacter>(result.GetActor());

		if (Terra->GetTerraType() == ETerraType::Default)
		{
			Terra->TakeDamage(10.f, DmgEvent, GetController(), this);
		}

		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Gurad")));

			FVector	SocketLoc = GetMesh()->GetSocketLocation(TEXT("hand_l_Projectile"));

			ADefaultEffect* Projectile = GetWorld()->SpawnActor<ADefaultEffect>(SocketLoc, GetActorRotation(), ActorParam);

			Projectile->SetParticleAsset(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonTerra/FX/Particles/Terra/Abilities/Shield/FX/P_Terra_ShieldBlockHit.P_Terra_ShieldBlockHit'"));

			USoundBase* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Script/Engine.SoundWave'/Game/KHH/Sound/Terra/AnyConv_com__swordEff0.AnyConv_com__swordEff0'"));

			UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());

			Terra->TakeDamage(10.f / 2.f, DmgEvent, GetController(), this);
		}

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%s"), *result.GetActor()->GetFName().ToString()));
		GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Emerald, FString::Printf(TEXT("TakeDamage")));
	}

	else
	{

	}
}

void AAuroraAICharacter::Dash(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("DashAttack")));

	if (m_IsDass)
	{
		FString ProfileName = OtherComp->GetCollisionProfileName().ToString();

		if (ProfileName == TEXT("Player"))
		{
			FActorSpawnParameters ActorParam;
			ActorParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ATerraCharacter* Player = Cast<ATerraCharacter>(OtherActor);

			if (!IsValid(Player))
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("Not Player")));

				return;
			}

			FDamageEvent DmgEvent;

			if (Player->GetTerraType() == ETerraType::Default)
			{
				Player->TakeDamage(10.f, DmgEvent, GetController(), this);
			}

			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Gurad")));

				FVector	SocketLoc = GetMesh()->GetSocketLocation(TEXT("hand_l_Projectile"));

				ADefaultEffect* Projectile = GetWorld()->SpawnActor<ADefaultEffect>(SocketLoc, GetActorRotation(), ActorParam);

				Projectile->SetParticleAsset(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonTerra/FX/Particles/Terra/Abilities/Shield/FX/P_Terra_ShieldBlockHit.P_Terra_ShieldBlockHit'"));

				USoundBase* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Script/Engine.SoundWave'/Game/KHH/Sound/Terra/AnyConv_com__swordEff0.AnyConv_com__swordEff0'"));

				UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());

				Player->TakeDamage(10.f / 2.f, DmgEvent, GetController(), this);
			}

			UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
		}
	}
}

void AAuroraAICharacter::Ultimate()
{
	FVector Start = GetActorLocation() - GetActorUpVector() * GetHalfHeight();

	FVector End = Start;

	FHitResult result;

	FCollisionQueryParams param(NAME_None, false, this);

	bool Collision = GetWorld()->SweepSingleByChannel(result, Start, End, FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel11, FCollisionShape::MakeSphere(600.f), param);

#if ENABLE_DRAW_DEBUG

	FColor DrawColor = Collision ? FColor::Red : FColor::Green;

	DrawDebugSphere(GetWorld(), (Start + End) / 2.f, 600.f, 20, DrawColor, false, 20.f);

#endif

	if (Collision)
	{
		FActorSpawnParameters ActorParam;
		ActorParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FHitResult LineResult;

		FDamageEvent DmgEvent;

		ATerraCharacter* Terra = Cast<ATerraCharacter>(result.GetActor());

		if (Terra->GetTerraType() == ETerraType::Default)
		{
			Terra->TakeDamage(10.f, DmgEvent, GetController(), this);
		}

		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Gurad")));

			FVector	SocketLoc = GetMesh()->GetSocketLocation(TEXT("hand_l_Projectile"));

			ADefaultEffect* Projectile = GetWorld()->SpawnActor<ADefaultEffect>(SocketLoc, GetActorRotation(), ActorParam);

			Projectile->SetParticleAsset(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonTerra/FX/Particles/Terra/Abilities/Shield/FX/P_Terra_ShieldBlockHit.P_Terra_ShieldBlockHit'"));

			USoundBase* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Script/Engine.SoundWave'/Game/KHH/Sound/Terra/AnyConv_com__swordEff0.AnyConv_com__swordEff0'"));

			UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());

			Terra->TakeDamage(10.f / 2.f, DmgEvent, GetController(), this);
		}

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%s"), *result.GetActor()->GetFName().ToString()));
		GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Emerald, FString::Printf(TEXT("TakeDamage")));
	}

	else
	{

	}
}
