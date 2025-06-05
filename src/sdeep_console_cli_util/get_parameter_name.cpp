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

#include "get_parameter_name.h"
#include "sdeep_console_global_settings.h"
#include "nn_document.h"
#include "nnabla_utility.h"

using namespace elfe;
using namespace sdeep_console;

//---------------------------------------------------------------------------
int get_parameter_name(FStringList &args)
{
	//Check args
	if (args.GetSize() != 5) {
		printf("usage : sdeep_console_cli_util get_parameter_name -i INPRJ -o OUTPROJ -j JOBID\n");
		printf("\n");
		printf("optional argments:\n");
		printf("  -i INPRJ\n");
		printf("  \tinput project file\n");
		printf("  -o OUTPRJ\n");
		printf("  \toutput project file\n");
		printf("  -j JOBID\n");
		printf("  \tjob id string\n");
		return 0;
	}

	FString input_project_file_name;
	FString output_project_file_name;
	FString job_id;
	for (int i = 2 ; i < 5 ; i++) {
		FString option = args.GetAt(i);
		if (option.SubString(1, 2) == "-i"){
			option.SplitBy(" ");
			input_project_file_name = option;
		}else if (option.SubString(1, 2) == "-o"){
			option.SplitBy(" ");
			output_project_file_name = option;
		}else if (option.SubString(1, 2) == "-j"){
			option.SplitBy(" ");
			job_id = option;
		}
	}
	if (input_project_file_name == "") {
		printf("error : argument -i is required.\n");
		return 1;
	}else {
		printf("input project file name : %s\n", input_project_file_name.PChar());
	}
	if (output_project_file_name == "") {
		printf("error : argument -o is required.\n");
		return 1;
	}else {
		printf("output project file name : %s\n", output_project_file_name.PChar());
	}
	if (job_id == "") {
		printf("error : argument -j is required.\n");
		return 1;
	}else {
		printf("job id : %s\n", job_id.PChar());
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

	//Open project
	NNDocument project;
	for (int i = 0; i < (int)SDeepEngineType::NumEngineType ; i++) {
		SDeepEngineType sdeep_engine_type = SDeepEngineType(i);
		const NNEngineInfo &engine_info = settings.GetEngineInfo().GetAt(i);
		project.RegisterNNEngineInfo(sdeep_engine_type, &engine_info);
	}
	for (int i = 0; i < settings.GetDocumentConverterSettingList().GetSize() ; i++) {
		project.RegisterNNDocumentConverterSetting(&settings.GetDocumentConverterSettingList().GetAt(i));
	}
	if (!project.LoadFromFile(input_project_file_name, FString(""), false, false)) {
		printf("error : An error occurred while loading input project file.\n");
		return 1;
	}

	//Set parameter name
	NNStructures structures;
	structures = project;
	FStringList parameter_names, parameter_file_names;
	for (int is = 0 ; is < structures.GetNumStructure() ; is++){
		structures.SetActiveStructure(is);
		if (project.IsStructureUsed(structures.GetName())){
			FlattenCurrentStructure(project, structures, true);

			for (int il = 0 ; il < structures.GetNumLayer(); il++) {
				const NNLayer &layer = structures.GetLayer(il);
				if (layer.GetType() == "Parameter" || layer.GetType() == "WorkingMemory") {
					// Parameter layer
					FStringList repeat_extensions = structures.GetRepeatExtensions(layer, true);
					for (int ire = 0; ire < repeat_extensions.GetSize() ; ire++) {
						if (parameter_names.IndexOf(repeat_extensions.GetAt(ire)) < 0){
							parameter_names.Add(repeat_extensions.GetAt(ire));
							parameter_file_names.Add(job_id + FString("/") + repeat_extensions.GetAt(ire).ReplaceStr("/", "~"));
						}
					}
				}
			}
		}
	}
	project.FillParameterName(parameter_names, parameter_file_names);

	//Save project
	if (!project.SaveToFile(output_project_file_name)) {
		printf("error : An error occurred while saving output project file.\n");
		return 1;
	}

	printf("Parameter name was correctly set.\n");

	return 0;
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
