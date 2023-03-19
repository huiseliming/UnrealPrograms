// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProgramPackageToolApp.h"
#include "UnixCommonStartup.h"

int main(int argc, char *argv[])
{
	return CommonUnixMain(argc, argv, &RunProgramPackageTool);
}
