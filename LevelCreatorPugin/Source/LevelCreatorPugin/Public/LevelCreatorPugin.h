// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FLevelCreatorPuginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<SDockTab> CreateTab(const FSpawnTabArgs& TabArgs);
	FText GetText() const;
	void TextCommitted(const FText& InText, ETextCommit::Type InCommitType);
	FReply ButtonClicked();

	void GenerateWorld();

	FAssetData WallPath;
	FAssetData BreakableWallPath;
	FAssetData Texture;
	TSharedPtr<FAssetThumbnailPool> MyThumbnailPool;
	FName BrushName;
	FString LevelName;
	FString DefaultText;
	FString WallPathdefault;
	FString BreakableWallPathdefault;
};
