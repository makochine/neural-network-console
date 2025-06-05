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

#pragma hdrstop

#include "sdeep_evaluation_thread.h"
#include "fstring.h"
#include "file_util.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
SDeepEvaluationThread::SDeepEvaluationThread(
	FString name,
	FString result_dir,
	TComponent *AOwner,
	NNDocument *nn_document,
	SDeepConsoleGlobalSettings *settings,
	int schedule) :
		SDeepProcessThread(name, result_dir, AOwner, nn_document, settings, schedule),
		data_num_(1),
		last_data_(0),
        auto_evaluation_(false)
{
	//Log file
	log_file_name_ = UnicodeString(result_dir_.PChar()) + UnicodeString("\\evaluation_log.txt");
	if (FileExists(log_file_name_)){
		DeleteFile(log_file_name_);
	}

	//Output results
	UnicodeString output_file_name = UnicodeString(result_dir_.PChar()) + UnicodeString("\\output_result.csv");
	if (FileExists(output_file_name)){
		DeleteFile(output_file_name);
	}

	//Confusion matrix file
	FStringList files = EnumFiles((result_dir_ + FString("\\")).PChar());
	for (int i = 0 ; i < files.GetSize() ; i++){
		if (files.GetAt(i).Pos("confusion_matrix") == 1 && ExtractFileExt(files.GetAt(i).PChar()) == ".csv"){
			FString file_name = result_dir_ + FString("\\") + files.GetAt(i);
			if (FileExists(file_name.PChar())){
				DeleteFile(file_name.PChar());
			}
		}
	}

	Log(FString("Evaluation process is started."), true);
};
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
