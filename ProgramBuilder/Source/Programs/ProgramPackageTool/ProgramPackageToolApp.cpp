// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProgramPackageToolApp.h"

#include "Runtime/Launch/Public/RequiredProgramMainCPPInclude.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Docking/WorkspaceItem.h"
#include "Styling/StarshipCoreStyle.h"

#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Json.h"
#include "DesktopPlatformModule.h"

IMPLEMENT_APPLICATION(ProgramPackageTool, "ProgramPackageTool");

#define LOCTEXT_NAMESPACE "ProgramPackageTool"

namespace WorkspaceMenu
{
	TSharedRef<FWorkspaceItem> DeveloperMenu = FWorkspaceItem::NewGroup(LOCTEXT("DeveloperMenu", "Developer"));
}

auto OpenFileDialogForProgramTargetFile(TSharedPtr<SMultiLineEditableTextBox> ProgramTargetFileTextBox)
{
	auto& FileManager = IFileManager::Get();
	FString ProgramTargetDir = ProgramTargetFileTextBox->GetText().ToString();
	FString DefaultPath;
	if (FPaths::DirectoryExists(ProgramTargetDir))
	{
		DefaultPath = ProgramTargetDir;
	}
	else
	{
		DefaultPath = FPlatformProcess::BaseDir();
	}
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	auto ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	TArray<FString> OutFilenames;
	DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("ProgramTargetFile"),
		DefaultPath,
		TEXT(""),
		TEXT("ProgramTargetFile (*.target)|*.target"),
		EFileDialogFlags::None,
		OutFilenames);
	if (!OutFilenames.IsEmpty())
	{
		FString ProgramTargetFile = FPaths::ConvertRelativePathToFull(DefaultPath, OutFilenames[0]);
		ProgramTargetFileTextBox->SetText(FText::FromString(ProgramTargetFile));
	}
	return FReply::Handled();
}

//bool FDesktopPlatformLinux::OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName)
auto OpenFileDialogForDeploymentDirectory(TSharedPtr<SMultiLineEditableTextBox> DeploymentDirTextBox)
{
	auto& FileManager = IFileManager::Get();
	FString DefaultPath = DeploymentDirTextBox->GetText().ToString();
	if (FPaths::DirectoryExists(DefaultPath))
	{
		DefaultPath = DefaultPath;
	}
	else
	{
		DefaultPath = FPlatformProcess::BaseDir();
	}
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	auto ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	FString OutFolderName;
	DesktopPlatform->OpenDirectoryDialog(
		ParentWindowHandle,
		TEXT("DeploymentDirectory"),
		DefaultPath,
		OutFolderName);
	FString ProgramTargetFile = FPaths::ConvertRelativePathToFull(DefaultPath, OutFolderName);
	DeploymentDirTextBox->SetText(FText::FromString(ProgramTargetFile));
	return FReply::Handled();
}

auto OnPackageButtonClicked(TSharedPtr<SMultiLineEditableTextBox> ProgramTargetFileTextBox, TSharedPtr<SMultiLineEditableTextBox> DeploymentDirTextBox)
{
	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	auto ExecutableTargetFilePath = ProgramTargetFileTextBox->GetText().ToString();
	auto ExecutableTargetFolderPath = FPaths::GetPath(ExecutableTargetFilePath);
	if (FPaths::FileExists(ExecutableTargetFilePath))
	{
		FString ExecutableTargetJsonString;
		if (FFileHelper::LoadFileToString(ExecutableTargetJsonString, *ExecutableTargetFilePath))
		{
			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ExecutableTargetJsonString);
			TSharedPtr<FJsonObject>  ExecutableTargetJsonObject;
			if (FJsonSerializer::Deserialize(JsonReader, ExecutableTargetJsonObject))
			{
				FString TargetName;
				if (ExecutableTargetJsonObject->TryGetStringField(TEXT("TargetName"), TargetName))
				{
					TSet<FString> DeploymentFileFilter = { "SymbolFile" };
					TSet<FString> DeploymentFiles;
					const TArray<TSharedPtr<FJsonValue>>* BuildProductsArray = nullptr;
					if (ExecutableTargetJsonObject->TryGetArrayField(TEXT("BuildProducts"), BuildProductsArray))
					{
						for (size_t i = 0; i < BuildProductsArray->Num(); i++)
						{
							TSharedPtr<FJsonObject>* BuildProduct = nullptr;
							(*BuildProductsArray)[i]->TryGetObject(BuildProduct);
							if (BuildProduct)
							{
								FString BuildProductPath, BuildProductType;
								if ((*BuildProduct)->TryGetStringField("Path", BuildProductPath) &&
									(*BuildProduct)->TryGetStringField("Type", BuildProductType))
								{
									if (!DeploymentFileFilter.Contains(BuildProductType))
									{
										DeploymentFiles.Add(BuildProductPath);
									}
								}
							}
						}
						//
						const TArray<TSharedPtr<FJsonValue>>* RuntimeDependenciesArray = nullptr;
						if (ExecutableTargetJsonObject->TryGetArrayField(TEXT("RuntimeDependencies"), RuntimeDependenciesArray))
						{
							for (size_t i = 0; i < RuntimeDependenciesArray->Num(); i++)
							{
								TSharedPtr<FJsonObject>* RuntimeDependeny = nullptr;
								(*RuntimeDependenciesArray)[i]->TryGetObject(RuntimeDependeny);
								if (RuntimeDependeny)
								{
									FString RuntimeDependenyPath, RuntimeDependenyType;
									if ((*RuntimeDependeny)->TryGetStringField("Path", RuntimeDependenyPath) &&
										(*RuntimeDependeny)->TryGetStringField("Type", RuntimeDependenyType))
									{
										DeploymentFiles.Add(RuntimeDependenyPath);
									}
								}
							}
						}
					}
					FString ProjectDir = FPaths::GetPath(FPaths::GetPath(FPaths::GetPath(ProgramTargetFileTextBox->GetText().ToString())));
					FString DeploymentDir = DeploymentDirTextBox->GetText().ToString();
					DeploymentDir.RemoveFromEnd("/");
					FString EngineDir = FPaths::EngineDir();
					EngineDir.RemoveFromEnd("/");

					FString ExecutableDir = FPlatformProcess::BaseDir();
					ExecutableDir.RemoveFromEnd(TEXT("/"));

					for (auto DeploymentFile : DeploymentFiles)
					{
						if (FPaths::GetExtension(DeploymentFile) == TEXT("uproject")) continue;
						if (DeploymentFile.RemoveFromStart(TEXT("$(EngineDir)/")))
						{
							FString Src = FString::Format(TEXT("{0}/{1}"), { EngineDir, DeploymentFile });
							FString Dst = FString::Format(TEXT("{0}/Engine/{1}"), { DeploymentDir, DeploymentFile });
							if (!PlatformFile.CreateDirectoryTree(*FPaths::GetPath(Dst)))
							{
								UE_LOG(LogTemp, Error, TEXT("CREATE DIRECTORY TREE %s FAILED "), *Src, *Dst);
							}
							if (!PlatformFile.CopyFile(*Dst, *Src))
							{
								UE_LOG(LogTemp, Error, TEXT("COPY FILE FROM %s TO %s FAILED "), *Src, *Dst);
							}
						}
						else if (DeploymentFile.RemoveFromStart(TEXT("$(ProjectDir)/")))
						{
							FString Src = FString::Format(TEXT("{0}/{1}"), { ProjectDir, DeploymentFile });
							FString Dst = FString::Format(TEXT("{0}/{1}/{2}"), { DeploymentDir, TargetName, DeploymentFile });
							if (!PlatformFile.CreateDirectoryTree(*FPaths::GetPath(Dst)))
							{
								UE_LOG(LogTemp, Error, TEXT("CREATE DIRECTORY TREE %s FAILED "), *Src, *Dst);
							}
							if (!PlatformFile.CopyFile(*Dst, *Src))
							{
								UE_LOG(LogTemp, Error, TEXT("COPY FILE FROM %s TO %s FAILED "), *Src, *Dst);
							}
						}
						else
						{
							UE_LOG(LogTemp, Error, TEXT("UNABLE TO PROCESS %s"), *DeploymentFile);
						}
					}
				}
			}
		}
	}
	return FReply::Handled();
}


int RunProgramPackageTool(const TCHAR* CommandLine)
{
	FTaskTagScope TaskTagScope(ETaskTag::EGameThread);

	// start up the main loop
	GEngineLoop.PreInit(CommandLine);

	// Make sure all UObject classes are registered and default properties have been initialized
	ProcessNewlyLoadedUObjects();

	// Tell the module manager it may now process newly-loaded UObjects when new C++ modules are loaded
	FModuleManager::Get().StartProcessingNewlyLoadedObjects();

	// crank up a normal Slate application using the platform's standalone renderer
	FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer());

	FSlateApplication::InitHighDPI(true);

	// set the application name
	FGlobalTabmanager::Get()->SetApplicationTitle(LOCTEXT("AppTitle", "ProgramPackageTool"));
	FAppStyle::SetAppStyleSetName(FStarshipCoreStyle::GetCoreStyle().GetStyleSetName());

	// launch the main window of the ProgramPackageTool module
	//FProgramPackageToolModule& ProgramPackageToolModule = FModuleManager::LoadModuleChecked<FProgramPackageToolModule>(FName("ProgramPackageToolModule"));
	//ProgramPackageToolModule.AppStarted();

	auto LeftRightLabel = [](const FName& InIconName = FName(), const FText& InLabel = FText::GetEmpty(), const FName& InTextStyle = TEXT("ButtonText")) -> TSharedRef<SWidget>
	{
		TSharedPtr<SHorizontalBox> HBox = SNew(SHorizontalBox);
		float Space = InIconName.IsNone() ? 0.0f : 8.f;

		if (!InIconName.IsNone())
		{
			HBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FAppStyle::Get().GetBrush(InIconName))
				];
		}

		if (!InLabel.IsEmpty())
		{
			HBox->AddSlot()
				.VAlign(VAlign_Center)
				.Padding(Space, 0.5f, 0.f, 0.f)  // Compensate down for the baseline since we're using all caps
				.AutoWidth()
				[
					SNew(STextBlock)
					.TextStyle(&FAppStyle::Get().GetWidgetStyle< FTextBlockStyle >(InTextStyle))
					.Justification(ETextJustify::Center)
					.Text(InLabel)
				];
		}

		return SNew(SBox).HeightOverride(16.f)[HBox.ToSharedRef()];
	};
	auto ProjectDirs = FPaths::GetGameLocalizationPaths();
	FString ExecutableDir = FPlatformProcess::BaseDir();
	ExecutableDir.RemoveFromEnd(TEXT("/"));
	FString BinariesDir = FPaths::GetPath(ExecutableDir);// FPaths::ConvertRelativePathToFull((FString::Format(TEXT("{0}/"),{ FPlatformProcess::BaseDir() });
	FString ProjectDir = FPaths::GetPath(BinariesDir);// FPaths::ConvertRelativePathToFull((FString::Format(TEXT("{0}/"),{ FPlatformProcess::BaseDir() });
	//ExecutableDir.AppendChar('/');
	//BinariesDir.AppendChar('/');
	//ProjectDir.AppendChar('/');
	//FString EngineDir = FPaths::EngineDir();
	FString ProjectName = FApp::GetProjectName();
	FString ProgramTargetFileName = FString::Printf(TEXT("%s.target"), FApp::GetProjectName());
	FString DefaultProgramTargetFile = FString::Format(TEXT("{0}/{1}.target"), { ExecutableDir, FApp::GetProjectName() });
	FString DefaultDeploymentDir = FString::Format(TEXT("{0}/Package/"), { ProjectDir });

	TSharedPtr<SMultiLineEditableTextBox> ProgramTargetFileTextBox;
	TSharedPtr<SMultiLineEditableTextBox> DeploymentDirTextBox;

	TSharedPtr<SWindow> MainWindow = SNew(SWindow)
		.ClientSize(FVector2D(640, 160))
		[
			SNew(SBox)
			.Padding(10)
		[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					.Padding(5)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("", "ProgramTargetFile      :"))
						
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.Padding(5)
					[
						SAssignNew(ProgramTargetFileTextBox, SMultiLineEditableTextBox)
						.Text(FText::FromString(DefaultProgramTargetFile))
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
						.OnClicked_Static(&OpenFileDialogForProgramTargetFile, ProgramTargetFileTextBox)
						[
							LeftRightLabel("Icons.FolderClosed")
						]
					]
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					.Padding(5)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("", "DeploymentDirectory :"))

					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.Padding(5)
					[
						SAssignNew(DeploymentDirTextBox, SMultiLineEditableTextBox)
						.Text(FText::FromString(DefaultDeploymentDir))
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
						.OnClicked_Static(&OpenFileDialogForDeploymentDirectory, DeploymentDirTextBox)
						[
							LeftRightLabel("Icons.FolderClosed")
						]
					]
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Package")))
					.OnClicked_Static(&OnPackageButtonClicked, ProgramTargetFileTextBox, DeploymentDirTextBox)
				]
			]
		]
	;

	FSlateApplication::Get().AddWindow(MainWindow.ToSharedRef());

	// loop while the server does the rest
	while (!IsEngineExitRequested())
	{
		BeginExitIfRequested();

		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FStats::AdvanceFrame(false);
		FTSTicker::GetCoreTicker().Tick(FApp::GetDeltaTime());
		FSlateApplication::Get().PumpMessages();
		FSlateApplication::Get().Tick();
		FPlatformProcess::Sleep(0.01);

		GFrameCounter++;
	}

	FCoreDelegates::OnExit.Broadcast();
	FSlateApplication::Shutdown();
	FModuleManager::Get().UnloadModulesAtShutdown();

	GEngineLoop.AppPreExit();
	GEngineLoop.AppExit();

	return 0;
}

#undef LOCTEXT_NAMESPACE
