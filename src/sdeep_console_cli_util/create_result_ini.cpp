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

#include "create_result_ini.h"

#include "sdeep_console_global_settings.h"
#include "nn_document.h"
#include "nn_document_yamlutility.h"

using namespace elfe;
using namespace sdeep_console;

//---------------------------------------------------------------------------
int create_result_ini(FStringList &args)
{
	//Check args
	if (args.GetSize() != 5) {
		printf("usage : sdeep_console_cli_util create_result_ini -i INPRJ -y INYML -o OUTINI\n");
		printf("\n");
		printf("optional argments:\n");
		printf("  -i INPRJ\n");
		printf("  \tinput_project file to mutate(*.sdcproj)\n");
		printf("  -y INYML\n");
		printf("  \tinput_yaml file(monitoring_report.yml)\n");
		printf("  -o OUTINI\n");
		printf("  \toutput_project file(result.ini)\n");
		return 0;
	}

	FString input_project_file_name;
	FString input_yaml_file_name;
	FString output_ini_file_name;
	for (int i = 2 ; i < 5 ; i++) {
		FString option = args.GetAt(i);
		if (option.SubString(1, 2) == "-i"){
			option.SplitBy(" ");
			input_project_file_name = option;
		}else if (option.SubString(1, 2) == "-y"){
			option.SplitBy(" ");
			input_yaml_file_name = option;
		}else if (option.SubString(1, 2) == "-o"){
			option.SplitBy(" ");
			output_ini_file_name = option;
		}
	}
	if (input_project_file_name == "") {
		printf("error : argument -i is required.\n");
		return 1;
	}else {
		printf("input project file name : %s\n", input_project_file_name.PChar());
	}
	if (input_yaml_file_name == "") {
		printf("error : argument -y is required.\n");
		return 1;
	}else {
		printf("input yaml file name : %s\n", input_yaml_file_name.PChar());
	}
	if (output_ini_file_name == "") {
		printf("error : argument -o is required.\n");
		return 1;
	}else {
		printf("output ini file name : %s\n", output_ini_file_name.PChar());
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


	CSVDataset monitor_csv;
	if (!MonitoringReportYAMLToCSV(input_yaml_file_name.PChar(), monitor_csv)){
		printf("error : An error occurred while loading input yaml file.\n");
		return 1;
	}

	//Get best validation error
	const int epoch_col = monitor_csv.GetColumnIndex(FString("epoch"));
	const int terr_col = monitor_csv.GetColumnIndex(FString("train_error"));
	const int verr_col = monitor_csv.GetColumnIndex(FString("valid_error"));
	float last_terr = 0;
	float last_verr = 0;
	float min_verr = 0;
	int last_epoch = 0;
	int min_epoch = 0;
	if (epoch_col < 0 || terr_col < 0 || verr_col < 0) {
		printf("error : Illigal input yaml file format.\n");
		return 1;
	}else {
        int first = true;
		for (int id = 0 ; id < monitor_csv.GetNumData() ; id++) {
			FString epoch = monitor_csv.GetValue(id, epoch_col);
			if (epoch != ""){
				last_epoch = epoch.AsInt();
			}
			FString terr = monitor_csv.GetValue(id, terr_col);
			if (terr != ""){
				last_terr = terr.AsFloat();
			}
			FString verr = monitor_csv.GetValue(id, verr_col);
			if (verr != ""){
				last_verr = verr.AsFloat();
				if (first || last_verr < min_verr){
					min_verr = last_verr;
                    min_epoch = last_epoch;
					first = false;
				}
			}
		}
	}

	//Get cost_multiply_add
	const double cost_multiply_add = project.GetStatistics(FString("CostMultiplyAdd"))->GetSum();

	//Output result.ini
	FFastIni ini = FFastIni(output_ini_file_name.PChar());
	ini.Write<double>("result", "cost_multiply_add", cost_multiply_add);
	ini.Write<double>("result", "training", last_terr);
	ini.Write<double>("result", "validation", last_verr);
	ini.Write<int>("result", "epoch", last_epoch);
	ini.Write<int>("result", "max_epoch", project.GetMaxEpoch());
	ini.Write<int>("result", "best_epoch", min_epoch);
	ini.Write<double>("result", "best_validation", min_verr);
	for (int is = 0 ; is < project.GetStatistics().GetSize() ; is++){
		FString statistics_name = project.GetStatistics().GetAt(is).GetLabel();
		ini.Write<double>("statistics", statistics_name.PChar(), project.GetRuntimeNetworkStatistics(statistics_name));
	}
	ini.Flush();

	printf("The result.ini was successfully created.\n");

	return 0;
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
