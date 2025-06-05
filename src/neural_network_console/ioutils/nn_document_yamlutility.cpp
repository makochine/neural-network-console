// Copyright 2016,2017,2018,2019,2020,2021 Sony Corporation.
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
#ifdef _WIN32
#include <windows.h>
#endif
#pragma hdrstop

#include "nn_document_yamlutility.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
int MonitoringReportYAMLToCSV(const char *file_name, CSVDataset &csv)
{
	csv.Clear();
	csv.SetColumnNames(FString("epoch,Monitor_time,cost,time,train_error,train_errors,valid_error,valid_errors"));

	FStringList yaml;
	int trial = 0;
    int success = false;
	while (trial++ < 3) {
		success = yaml.LoadFromFile(file_name) && yaml.GetSize();
        if (success) {
			break;
		}
#ifdef _WIN32
		Sleep(30);
#endif
	}

	int line = 0;
	while (line < yaml.GetSize()) {
		FString s = yaml.GetAt(line);
		const int epoch_index = s.SplitBy(":").AsInt();
		csv.SetValue(epoch_index, FString("epoch"), FString(epoch_index + 1));
		line++;
		if (s == "") {
			while (line < yaml.GetSize()){
				if (yaml.GetAt(line).SubString(1, 1) != " ") {
					break;
				}else {
					if (s != "") {
						s += ",";
					}
					s += yaml.GetAt(line);
					line++;
				}
			}
		}
		s = s.ReplaceStr(" ", "");
		s = s.ReplaceStr("{", "");
		s = s.ReplaceStr("}", "");
		while (s != ""){
			FString s1 = s.SplitBy(",");
			FString col_name = s1.SplitBy(":");

			csv.SetValue(epoch_index, col_name, s1);
		}
	}

	return success;
}
//---------------------------------------------------------------------------
};
#pragma package(smart_init)
