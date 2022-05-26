// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelCreatorPugin.h"
#include "ImageUtils.h"
#include "Factories/WorldFactory.h"
#include "Misc/FileHelper.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkyLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMeshActor.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "FLevelCreatorPuginModule"

void FLevelCreatorPuginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FLevelCreatorPuginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

bool FLevelCreatorPuginModule::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FParse::Command(&Cmd, TEXT("newlevel")))
	{
		//first argument level name
		FString LevelName = FParse::Token(Cmd, true);

		//check if that name already exists		
		FString Name = FString::Printf(TEXT("/Game/%s.%s"), *LevelName, *LevelName);

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*Name);

		if (AssetData.IsValid())
		{
			//change name if it already exists
			uint64 SuffixAssetName = FPlatformTime::Cycles64();
			LevelName = LevelName + FString::Printf(TEXT("_%llu"), SuffixAssetName);	

			UE_LOG(LogTemp, Warning, TEXT("This name already exist, name changed in %s"), *LevelName);
		}
		
		//second argument texture filename
		FString Directory = FPaths::ProjectContentDir();
		FString Path = Directory + FParse::Token(Cmd, true);

		//verify texture
		TArray<uint8> TextureArray;
		FFileHelper::LoadFileToArray(TextureArray, *Path);
		UTexture2D* LevelTexture = FImageUtils::ImportBufferAsTexture2D(TextureArray);

		if (!LevelTexture)
		{
			UE_LOG(LogTemp, Warning, TEXT("Texture not found"));
			return true;
		}
		

		UWorldFactory* NewWorld = NewObject<UWorldFactory>();
		
		UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Game/%s"), *LevelName));

		UObject* NewLevelObject = NewWorld->FactoryCreateNew(NewWorld->SupportedClass, Package, *LevelName, EObjectFlags::RF_Standalone |
			EObjectFlags::RF_Public, nullptr, GWarn);

		FAssetRegistryModule::AssetCreated(NewLevelObject);

		UWorld* WorldCasted = Cast<UWorld>(NewLevelObject);

		WorldCasted->Modify();

		//Lights
		ADirectionalLight* DirectionalLight = WorldCasted->SpawnActor<ADirectionalLight>();
		DirectionalLight->SetFolderPath("/Lighting");

		FString SkySpherePath = TEXT("/Engine/EngineSky/BP_Sky_Sphere.BP_Sky_Sphere");
		UBlueprint* SkySphere = LoadObject<UBlueprint>(nullptr, *SkySpherePath);
		AActor* MySkySphere = WorldCasted->SpawnActor<AActor>(SkySphere->GeneratedClass);	
		MySkySphere->SetFolderPath("/Lighting");
		
		ASkyLight* SkyLight = WorldCasted->SpawnActor<ASkyLight>();
		SkyLight->SetFolderPath("/Lighting");		

	
		//Floor
		UStaticMesh* MyFloor = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/MapTemplates/SM_Template_Map_Floor.SM_Template_Map_Floor"));
		AStaticMeshActor* Floor = WorldCasted->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass());

		FTransform MyTransform;
		MyTransform.SetLocation(FVector(0, 0, 0));	
		MyTransform.SetScale3D(FVector(LevelTexture->GetSizeX() / 10.f, LevelTexture->GetSizeY() / 10.f, 1));
		Floor->SetActorTransform(MyTransform);		
		Floor->GetStaticMeshComponent()->SetStaticMesh(MyFloor);		

		FVector SizeFloor = Floor->GetComponentsBoundingBox().GetSize();
		

		//Walls
		UBlueprint* Wall = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Wall.Wall"));
		UBlueprint* BreakableWall = LoadObject<UBlueprint>(nullptr, TEXT("/Game/DestroyableWall.DestroyableWall"));
		FColor Black = FColor(0, 0, 0, 255);
		FColor Red = FColor(255, 0, 0, 255);

		float StartX = Floor->GetActorLocation().X + ((SizeFloor.X * 0.5f) - 50);
		float StartY = Floor->GetActorLocation().Y - ((SizeFloor.Y * 0.5f) - 50);
		FVector VectorToAdd = FVector(0, 0, 0);
		FTransform MyWallTransform;
		MyWallTransform.SetLocation(FVector(StartX, StartY, 100));


		//reading texture		
		const FColor* ColorsImageData = static_cast<const FColor*>(LevelTexture->PlatformData->Mips[0].BulkData.LockReadOnly());

		for (int32 X = 0; X < LevelTexture->GetSizeX(); X++)
		{
			for (int32 Y = 0; Y < LevelTexture->GetSizeY(); Y++)
			{
				FColor PixelColor = ColorsImageData[Y * LevelTexture->GetSizeX() + X];
				
				if (PixelColor == Black)
				{
					AStaticMeshActor* MyWall = WorldCasted->SpawnActor<AStaticMeshActor>(Wall->GeneratedClass);
					MyWallTransform.AddToTranslation(VectorToAdd);
					MyWall->SetActorLocation(MyWallTransform.GetLocation());										
				}
				else if (PixelColor == Red)
				{
					AStaticMeshActor* MyBreakableWall = WorldCasted->SpawnActor<AStaticMeshActor>(BreakableWall->GeneratedClass);
					MyWallTransform.AddToTranslation(VectorToAdd);
					MyBreakableWall->SetActorLocation(MyWallTransform.GetLocation());
				}
				else
				{
					MyWallTransform.AddToTranslation(VectorToAdd);
				}
				
				VectorToAdd.X = 0;
				VectorToAdd.Y = 100;
			}

			VectorToAdd.X = -100;
			VectorToAdd.Y = -(100 * (LevelTexture->GetSizeY() -1));
		}

		LevelTexture->PlatformData->Mips[0].BulkData.Unlock();	

		WorldCasted->PostEditChange();
		WorldCasted->MarkPackageDirty();

		return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLevelCreatorPuginModule, LevelCreatorPugin)