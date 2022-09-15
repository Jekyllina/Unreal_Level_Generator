// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelCreatorPugin.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistryModule.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Factories/WorldFactory.h"
#include "Framework/Docking/TabManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SSeparator.h"
#include "WorkspaceMenuStructureModule.h"
#include "WorkspaceMenuStructure.h"

#include "Brushes/SlateImageBrush.h"
#include "Framework/MultiBox/SToolBarButtonBlock.h"

#define LOCTEXT_NAMESPACE "FLevelCreatorPuginModule"

static const FName MyDockTab("Level Creator");

void FLevelCreatorPuginModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MyDockTab, FOnSpawnTab::CreateRaw(this, &FLevelCreatorPuginModule::CreateTab))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
}

void FLevelCreatorPuginModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MyDockTab);
}

void FLevelCreatorPuginModule::GenerateWorld()
{	
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

	//take texture filename	
	FString TexturePath = Texture.GetAsset()->GetPathName();

	//verify texture
	/*TArray<uint8> TextureArray;
	FFileHelper::LoadFileToArray(TextureArray, *TexturePath);
	UTexture2D* LevelTexture = FImageUtils::ImportBufferAsTexture2D(TextureArray);*/
	UTexture2D* LevelTexture = Cast<UTexture2D>(Texture.GetAsset());

	checkf(LevelTexture, TEXT("Texture not found"));


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
	MyTransform.SetScale3D(FVector(LevelTexture->GetSizeX() / 10.0, LevelTexture->GetSizeY() / 10.0, 1));
	Floor->SetActorTransform(MyTransform);
	Floor->GetStaticMeshComponent()->SetStaticMesh(MyFloor);

	FVector SizeFloor = Floor->GetComponentsBoundingBox().GetSize();


	//Walls		
	UBlueprint* Wall = LoadObject<UBlueprint>(nullptr, *WallPathdefault);
	UBlueprint* BreakableWall = LoadObject<UBlueprint>(nullptr, *BreakableWallPathdefault);

	AStaticMeshActor* MyWall = WorldCasted->SpawnActor<AStaticMeshActor>(Wall->GeneratedClass);
	FVector WallSize = MyWall->GetComponentsBoundingBox().GetSize();
	float StartX = Floor->GetActorLocation().X + ((SizeFloor.X * 0.5f) - (WallSize.X * 0.5f));
	float StartY = Floor->GetActorLocation().Y - ((SizeFloor.Y * 0.5f) - (WallSize.Y * 0.5f));
	FVector Position = FVector(StartX, StartY, WallSize.Z * 0.5f);
	FVector VectorToAdd = FVector(0, 0, 0);

	AStaticMeshActor* MyWall2 = WorldCasted->SpawnActor<AStaticMeshActor>(BreakableWall->GeneratedClass);
	FVector WallSize2 = MyWall2->GetComponentsBoundingBox().GetSize();
	
	MyWall->Destroy();
	MyWall2->Destroy();


	//reading texture		
	const FColor* ColorsImageData = static_cast<const FColor*>(LevelTexture->PlatformData->Mips[0].BulkData.LockReadOnly());
	FColor Black = FColor(0, 0, 0, 255);
	FColor Red = FColor(255, 0, 0, 255);
	FColor White = FColor(255, 255, 255, 255);

	for (int32 X = 0; X < LevelTexture->GetSizeX(); X++)
	{
		for (int32 Y = 0; Y < LevelTexture->GetSizeY(); Y++)
		{
			FColor PixelColor = ColorsImageData[Y * LevelTexture->GetSizeX() + X];

			if (PixelColor == Black)
			{
				AStaticMeshActor* MyWallSpawned = WorldCasted->SpawnActor<AStaticMeshActor>(Wall->GeneratedClass);
				Position += VectorToAdd;
				Position.Z = WallSize.Z * 0.5f;
				MyWallSpawned->SetActorLocation(Position);
			}
			else if (PixelColor == Red)
			{
				AStaticMeshActor* MyBreakableWallSpawned = WorldCasted->SpawnActor<AStaticMeshActor>(BreakableWall->GeneratedClass);
				Position += VectorToAdd;
				Position.Z = WallSize2.Z * 0.5f;
				MyBreakableWallSpawned->SetActorLocation(Position);
			}
			else if(PixelColor == White)
			{
				Position += VectorToAdd;
			}

			VectorToAdd.X = 0;
			VectorToAdd.Y = 100;
		}

		VectorToAdd.X = -100;
		VectorToAdd.Y = -(100 * (LevelTexture->GetSizeY() - 1));
	}

	LevelTexture->PlatformData->Mips[0].BulkData.Unlock();

	WorldCasted->PostEditChange();
	WorldCasted->MarkPackageDirty();
}

TSharedRef<SDockTab> FLevelCreatorPuginModule::CreateTab(const FSpawnTabArgs& TabArgs)
{
	MyThumbnailPool = MakeShareable(new FAssetThumbnailPool(3, false));
	
	if (DefaultText.IsEmpty())
	{
		DefaultText = "Insert_name_here";
	}

	/*FString ImagePath = FString::Printf(TEXT("/LevelCreatorPugin/Icon128.Icon128"));
	BrushName = FName(*ImagePath);*/

	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().VAlign(EVerticalAlignment::VAlign_Center).HAlign(EHorizontalAlignment::HAlign_Left).AutoWidth()
				.Padding(20, 0)
				[
					SNew(STextBlock).Text(FText::FromString("Wall 1"))
				]
				+ SHorizontalBox::Slot().HAlign(EHorizontalAlignment::HAlign_Left).AutoWidth().Padding(10, 0)
				[
					SNew(SSeparator).Orientation(EOrientation::Orient_Vertical).Thickness(3)
				]
				+ SHorizontalBox::Slot().Padding(15, 0).MaxWidth(200)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UBlueprint::StaticClass())
					.ObjectPath_Lambda([this]()->FString {return WallPath.GetAsset()->GetPathName(); })
					.OnObjectChanged_Lambda([this](const FAssetData& Data)->void {WallPath = Data; })
					.ThumbnailPool(MyThumbnailPool)
					.DisplayThumbnail(true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().VAlign(EVerticalAlignment::VAlign_Center).HAlign(EHorizontalAlignment::HAlign_Left).AutoWidth()
				.Padding(20, 0)
				[
					SNew(STextBlock).Text(FText::FromString("Wall 2"))
				]
				+ SHorizontalBox::Slot().HAlign(EHorizontalAlignment::HAlign_Left).AutoWidth().Padding(10, 0)
				[
					SNew(SSeparator).Orientation(EOrientation::Orient_Vertical).Thickness(3)
				]
				+ SHorizontalBox::Slot().Padding(15, 0).MaxWidth(200)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UBlueprint::StaticClass())
					.ObjectPath_Lambda([this]()->FString {return BreakableWallPath.GetAsset()->GetPathName(); })
					.OnObjectChanged_Lambda([this](const FAssetData& Data)->void {BreakableWallPath = Data; })
					.ThumbnailPool(MyThumbnailPool)
					.DisplayThumbnail(true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().VAlign(EVerticalAlignment::VAlign_Center).HAlign(EHorizontalAlignment::HAlign_Left).AutoWidth()
				.Padding(15.5, 0)
				[
					SNew(STextBlock).Text(FText::FromString("Texture"))
				]
				+ SHorizontalBox::Slot().HAlign(EHorizontalAlignment::HAlign_Left).AutoWidth().Padding(10, 0)
				[
					SNew(SSeparator).Orientation(EOrientation::Orient_Vertical).Thickness(2.5)
				]
				+ SHorizontalBox::Slot().Padding(15, 0).MaxWidth(200)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UTexture2D::StaticClass())
					.ObjectPath_Lambda([this]()->FString {return Texture.GetAsset()->GetPathName(); })
					.OnObjectChanged_Lambda([this](const FAssetData& Data)->void {Texture = Data; })
					.ThumbnailPool(MyThumbnailPool)
					.DisplayThumbnail(true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 30)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().VAlign(EVerticalAlignment::VAlign_Center).HAlign(EHorizontalAlignment::HAlign_Left).AutoWidth()
				.Padding(10, 0)
				[
					SNew(STextBlock).Text(FText::FromString("Level Name"))
				]
				+ SHorizontalBox::Slot().Padding(15, 0)
				[
					SNew(SEditableTextBox).Text_Raw(this, &FLevelCreatorPuginModule::GetText)
					.OnTextCommitted_Raw(this, &FLevelCreatorPuginModule::TextCommitted)
				]				
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 20)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().VAlign(EVerticalAlignment::VAlign_Center).HAlign(EHorizontalAlignment::HAlign_Left).AutoWidth()
				.Padding(10, 0)
				[
					SNew(SButton).Text(LOCTEXT("Creationbutton", "Create"))
					.OnClicked_Raw(this, &FLevelCreatorPuginModule::ButtonClicked)
				]
			]
		];
}

FText FLevelCreatorPuginModule::GetText() const
{
	return FText::FromString(DefaultText);
}

void FLevelCreatorPuginModule::TextCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	if (InCommitType == ETextCommit::OnUserMovedFocus || InCommitType == ETextCommit::OnEnter)
	{
		LevelName = InText.ToString();		
		
		int32 Index;

		while ((LevelName.FindChar(' ', Index) == true))
		{
			UE_LOG(LogTemp, Warning, TEXT("%d"), Index);

			if (Index != -1)
			{
				LevelName[Index] = '_';
			}
		}		
	}
}

FReply FLevelCreatorPuginModule::ButtonClicked()
{
	if (LevelName.IsEmpty())
	{
		EAppReturnType::Type Answer = FMessageDialog::Open(EAppMsgType::YesNoCancel,
			LOCTEXT("MissingLevelName", "You didn't insert a name, do you want to use a random name?"));

		if (Answer == EAppReturnType::Yes)
		{
			uint64 SuffixName = FPlatformTime::Cycles64();
			LevelName = FString::Printf(TEXT("Level_%llu"), SuffixName);		
		}			
	}

	if (!Texture.GetAsset()->GetPathName().Compare(TEXT("None")))
	{
		EAppReturnType::Type Answer = FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("MissingTexture", "You didn't choose a texture for the level"));

		return FReply::Unhandled();
	}

	if (!WallPath.GetAsset()->GetPathName().Compare(TEXT("None")) || !BreakableWallPath.GetAsset()->GetPathName().Compare(TEXT("None")))
	{
		EAppReturnType::Type Answer = FMessageDialog::Open(EAppMsgType::YesNoCancel, 
			LOCTEXT("MissingWall", "You didn't choose one or more walls, do you want to use the default?"));

		if (Answer == EAppReturnType::Yes)
		{
			if (!WallPath.GetAsset()->GetPathName().Compare(TEXT("None")))
			{
				WallPathdefault = TEXT("/Game/Wall.Wall");				
			}
			else
			{
				WallPathdefault = WallPath.GetAsset()->GetPathName();
			}
			
			if (!BreakableWallPath.GetAsset()->GetPathName().Compare(TEXT("None")))
			{
				BreakableWallPathdefault = TEXT("/Game/Wall02.Wall02");
			}
			else
			{
				BreakableWallPathdefault = BreakableWallPath.GetAsset()->GetPathName();
			}

			FLevelCreatorPuginModule::GenerateWorld();
		}		
	}
	else
	{
		WallPathdefault = WallPath.GetAsset()->GetPathName();
		BreakableWallPathdefault = BreakableWallPath.GetAsset()->GetPathName();
		FLevelCreatorPuginModule::GenerateWorld();
	}	

	DefaultText = LevelName;

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLevelCreatorPuginModule, LevelCreatorPugin)