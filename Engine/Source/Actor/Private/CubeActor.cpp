#include "pch.h"
#include "Actor/Public/CubeActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include <iostream>

IMPLEMENT_CLASS(ACubeActor, AActor)

ACubeActor::ACubeActor()
{
	std::cout << "[DEBUG] CubeActor Constructor: bUseScript = true, bCanEverTick = true" << std::endl;
	bUseScript = true;
	bCanEverTick = true;
}

UClass* ACubeActor::GetDefaultRootComponent()
{
    return UStaticMeshComponent::StaticClass();
}

void ACubeActor::InitializeComponents()
{
    Super::InitializeComponents();
    Cast<UStaticMeshComponent>(GetRootComponent())->SetStaticMesh("Data/Shapes/Cube.obj");
}
