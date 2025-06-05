// Copyright 2017,2018,2019,2020,2021 Sony Corporation.
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

#include "import_caffe.h"
#include "sdeep_console_global_settings.h"
#include "nn_document.h"
#include "caffe_utility.h"

using namespace elfe;
using namespace sdeep_console;

//---------------------------------------------------------------------------
int import_caffe(FStringList &args)
{
	//Check args
	if (args.GetSize() != 5) {
		printf("usage : sdeep_console_cli_util import_caffe -i INPROTOTXT -e ENGINE -o OUTPROJ\n");
		printf("\n");
		printf("optional argments:\n");
		printf("  -i INPROTOTXT\n");
		printf("  \tinput caffe prototxt (*.prototxt)\n");
		printf("  -e ENGINE\n");
		printf("  \tsdeep engine type (\"sDeePy\" or \"NNabla\")\n");
		printf("  -o OUTPRJ\n");
		printf("  \toutput project file\n");
		return 0;
	}

	FString input_prototxt_file_name;
	FString engine;
	FString output_project_file_name;
	for (int i = 2 ; i < 5 ; i++) {
		FString option = args.GetAt(i);
		if (option.SubString(1, 2) == "-i"){
			option.SplitBy(" ");
			input_prototxt_file_name = option;
		}else if (option.SubString(1, 2) == "-e"){
			option.SplitBy(" ");
			engine = option;
		}else if (option.SubString(1, 2) == "-o"){
			option.SplitBy(" ");
			output_project_file_name = option;
		}
	}
	if (input_prototxt_file_name == "") {
		printf("error : argument -i is required.\n");
		return 1;
	}else {
		printf("input prototxt file name : %s\n", input_prototxt_file_name.PChar());
	}
	if (engine == "") {
		printf("error : argument -e is required.\n");
		return 1;
	}else {
		printf("engine type : %s\n", engine.PChar());
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

	//Open prototxt
	NNDocument project;
	for (int i = 0; i < (int)SDeepEngineType::NumEngineType ; i++) {
		SDeepEngineType sdeep_engine_type = SDeepEngineType(i);
		const NNEngineInfo &engine_info = settings.GetEngineInfo().GetAt(i);
		project.RegisterNNEngineInfo(sdeep_engine_type, &engine_info);
	}
	for (int i = 0; i < settings.GetDocumentConverterSettingList().GetSize() ; i++) {
		project.RegisterNNDocumentConverterSetting(&settings.GetDocumentConverterSettingList().GetAt(i));
	}

	if (engine == "NNabla") {
		project.Clear(SDeepEngineType::NNabla);
		if (!CaffeProtoTxtToNNStructure_NNabla(input_prototxt_file_name.PChar(), project, errors) || !project.GetNumLayer()) {
			printf("error : An error occurred while loading caffe prototxt file.\n");
			for (int ie = 0 ; ie < errors.GetSize() ; ie++) {
				printf("%s\n", errors.GetAt(ie).PChar());
			}
			return 1;
		}
	}else {
		printf("error : Engine type \"%s\" is not supported.\n", engine.PChar());
		return 1;
	}

	//Save project
	if (!project.SaveToFile(output_project_file_name)) {
		printf("error : An error occurred while saving output project file.\n");
		return 1;
	}

	printf("The network was successfully imported from caffe prototxt.\n");

	return 0;
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
