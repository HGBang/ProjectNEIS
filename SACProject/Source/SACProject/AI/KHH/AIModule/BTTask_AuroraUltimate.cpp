// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_AuroraUltimate.h"
#include "../AuroraAICharacter.h"
#include "../AuroraAIAnimInstance.h"
#include "../../AIState.h"

UBTTask_AuroraUltimate::UBTTask_AuroraUltimate()
{
	NodeName = TEXT("AuroraUltimate");

	bNotifyTick = true;

	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_AuroraUltimate::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::ExecuteTask(OwnerComp, NodeMemory);

    AAIController* Controller = OwnerComp.GetAIOwner();

    int32 HP = Controller->GetBlackboardComponent()->GetValueAsInt(TEXT("HP"));

    if (HP <= 0)
    {
        Controller->StopMovement();

        return EBTNodeResult::Failed;
    }

    AAuroraAICharacter* Auroar = Cast<AAuroraAICharacter>(Controller->GetPawn());

    if (!IsValid(Auroar))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("!IsValid(Auroar)")));

        return EBTNodeResult::Failed;
    }

    AActor* Target = Cast<AActor>(Controller->GetBlackboardComponent()->GetValueAsObject(TEXT("Target")));

    if (!IsValid(Target))
    {
        Controller->StopMovement();

        Auroar->GetAIAnimInstance()->ChangeAnim(EAIAnimType::Idle);

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("!IsValid(Target)")));

        return EBTNodeResult::Failed;
    }

    int32 AttackIndex = Controller->GetBlackboardComponent()->GetValueAsInt(TEXT("AttackIndex"));

    Auroar->PlaySkill(AttackIndex);

    Auroar->GetAIAnimInstance()->ChangeAnim(EAIAnimType::Skill1);

    return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_AuroraUltimate::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Type();
}

void UBTTask_AuroraUltimate::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* Controller = OwnerComp.GetAIOwner();

    int32 HP = Controller->GetBlackboardComponent()->GetValueAsInt(TEXT("HP"));

    if (HP <= 0)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);

        Controller->StopMovement();

        return;
    }

    AAuroraAICharacter* Auroar = Cast<AAuroraAICharacter>(Controller->GetPawn());

    if (!IsValid(Auroar))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);

        return;
    }

    AActor* Target = Cast<AActor>(Controller->GetBlackboardComponent()->GetValueAsObject(TEXT("Target")));

    if (Auroar->GetAttackEnd())
    {
        if (HP <= 0)
        {
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);

            Controller->StopMovement();

            Auroar->GetAIAnimInstance()->ChangeAnim(EAIAnimType::Idle);

            return;
        }

        FVector	AILoc = Auroar->GetActorLocation();
        FVector	TargetLoc = Target->GetActorLocation();

        AILoc.Z -= Auroar->GetHalfHeight();
        TargetLoc.Z -= Auroar->GetHalfHeight();

        float Distance = FVector::Distance(AILoc, TargetLoc);

        Distance -= Auroar->GetCapsuleRadius();

        UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Target->GetRootComponent());

        if (IsValid(Capsule))
        {
            Distance -= Capsule->GetScaledCapsuleRadius();
        }

        if (Distance > Auroar->GetAIState()->GetAttackDistance())
        {
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);

            Controller->GetBlackboardComponent()->SetValueAsInt(TEXT("AttackIndex"), 3);
        }

        else
        {
            FVector Dir = TargetLoc - AILoc;
            Dir.Z = 0.0;

            Dir.Normalize();

            Auroar->SetActorRotation(FRotator(0.0, Dir.Rotation().Yaw, 0.0));

            FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);

            Auroar->GetAIAnimInstance()->ChangeAnim(EAIAnimType::Idle);
        }

        Auroar->SetAttackEnd(false);
    }
}

void UBTTask_AuroraUltimate::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
