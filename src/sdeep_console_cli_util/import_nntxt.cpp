// Copyright 2018,2019,2020,2021 Sony Corporation.
// Copyright 2021,2022,2023,2024,2025 Sony Group Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//---------------------------------------------------------------------------

#pragma hdrstop

#include "import_nntxt.h"
#include "sdeep_console_global_settings.h"
#include "nn_document.h"
#include "nnabla_utility.h"

using namespace elfe;
using namespace sdeep_console;

//---------------------------------------------------------------------------
int import_nntxt(FStringList &args)
{
	//Check args
	if (args.GetSize() != 4) {
		printf("usage : sdeep_console_cli_util import_nntxt -i INNNTXT -o OUTPROJ\n");
		printf("\n");
		printf("optional argments:\n");
		printf("  -i INNNTXT\n");
		printf("  \tinput nnabla nntxt (*.nntxt)\n");
		printf("  -o OUTPRJ\n");
		printf("  \toutput project file (*.sdcproj)\n");
		return 0;
	}

	FString input_nntxt_file_name;
	FString output_project_file_name;
	for (int i = 2 ; i < 4 ; i++) {
		FString option = args.GetAt(i);
		if (option.SubString(1, 2) == "-i"){
			option.SplitBy(" ");
			input_nntxt_file_name = option;
		}else if (option.SubString(1, 2) == "-o"){
			option.SplitBy(" ");
			output_project_file_name = option;
		}
	}
	if (input_nntxt_file_name == "") {
		printf("error : argument -i is required.\n");
		return 1;
	}else {
		printf("input nntxt file name : %s\n", input_nntxt_file_name.PChar());
	}
	if (output_project_file_name == "") {
		printf("error : argument -o is required.\n");
		return 1;
	}else {
		printf("output project file name : %s\n", output_project_file_name.PChar());
	}

	//Setup
	SDeepConsoleGlobalSettings settings;
	FStringList errors;
	settings.SetupEngines(FString("settings/"), errors);
	if (errors.GetSize()) {
		for (int ie = 0; ie < errors.GetSize() ; ie++) {
			printf("error : %s\n", errors.GetAt(ie).PChar());
		}
		return 1;
	}

	//Open nntxt
	NNDocument project;
	for (int i = 0; i < (int)SDeepEngineType::NumEngineType ; i++) {
		SDeepEngineType sdeep_engine_type = SDeepEngineType(i);
		const NNEngineInfo &engine_info = settings.GetEngineInfo().GetAt(i);
		project.RegisterNNEngineInfo(sdeep_engine_type, &engine_info);
	}
	for (int i = 0; i < settings.GetDocumentConverterSettingList().GetSize() ; i++) {
		project.RegisterNNDocumentConverterSetting(&settings.GetDocumentConverterSettingList().GetAt(i));
	}

	project.Clear(SDeepEngineType::NNabla);
	if (!NntxtToDocument(input_nntxt_file_name.PChar(), project, errors) || !project.GetNumLayer()) {
		printf("error : An error occurred while loading nnabla nntxt file.\n");
		for (int ie = 0 ; ie < errors.GetSize() ; ie++) {
			printf("%s\n", errors.GetAt(ie).PChar());
		}
		return 1;
	}

	//Save project
	if (!project.SaveToFile(output_project_file_name)) {
		printf("error : An error occurred while saving output project file.\n");
		return 1;
	}

	printf("The network was successfully imported from nnabla nntxt.\n");

	return 0;
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
