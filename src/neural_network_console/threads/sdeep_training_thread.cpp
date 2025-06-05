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

#include "sdeep_training_thread.h"
#include "fstring.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
SDeepTrainingThread::SDeepTrainingThread(
	FString name,
	FString result_dir,
	TComponent *AOwner,
	NNDocument *nn_document,
	SDeepConsoleGlobalSettings *settings,
	int schedule,
	int resume) :
		SDeepProcessThread(name, result_dir, AOwner, nn_document, settings, schedule),

		training_completed_(false),
		last_epoch_(0),
		max_epoch_(1),
		last_training_(NAN_VALUE),
		last_validation_(NAN_VALUE),
		best_epoch_(0),
		best_validation_(NAN_VALUE),
		resume_(resume)
{
	//Create result file folder
	UnicodeString result_dir_str = result_dir_.PChar();
	if (!DirectoryExists(result_dir_str)) {
		ForceDirectories(result_dir_str);
	}

	//Create network.ini file
	FString network_ini_file_name = result_dir_ + FString("\\network.ini");
	if (!FileExists(network_ini_file_name.PChar())) {
		nn_document_->SaveToFile(network_ini_file_name, true, true, true);//Silent save, save tmp config(training plugin name etc.)
	}

	//Result ini
	UnicodeString result_file_name = result_dir_str + "\\result.ini";
	FFastIni ini = FFastIni(AnsiString(result_file_name).c_str());
	ini.Write<double>("result", "cost_multiply_add", nn_document_->GetRuntimeNetworkStatistics(FString("CostMultiplyAdd")));
	double log_myu = 0.0, log_sigma = 0.0;
	nn_document_->GetStructureSearchExpectiation(log_myu, log_sigma);
	if (log_sigma > 0.0) {
		ini.Write<double>("result", "structure_expectation_of_log_myu", log_myu);
		ini.Write<double>("result", "structure_expectation_of_log_sigma", log_sigma);
	}
	ini.Write<int>("result", "schedule", schedule);

	//result.ini (statistics)
	for (int is = 0 ; is < nn_document_->GetStatistics().GetSize() ; is++){
		FString statistics_name = nn_document_->GetStatistics().GetAt(is).GetLabel();
		ini.Write<double>("statistics", statistics_name.PChar(), nn_document_->GetRuntimeNetworkStatistics(statistics_name));
	}

	//Log file
	log_file_name_ = result_dir_str + UnicodeString("\\log.txt");
	if (!resume_ && FileExists(log_file_name_)){
		DeleteFile(log_file_name_);
	}
	if (!schedule) {
		Log(FString("Training process is started."), true);
	}

	//Progress file
	UnicodeString progress_file_name = result_dir_str + UnicodeString("\\progress.txt");
	if (FileExists(progress_file_name)){
		DeleteFile(progress_file_name);
	}
};
//---------------------------------------------------------------------------
void SDeepTrainingThread::WriteResultIni()
{
	UnicodeString log_file_name = (result_dir_ + FString("\\result.ini")).PChar();
	FFastIni ini = FFastIni(AnsiString(log_file_name).c_str());
	ini.Write<int>("result", "complete", training_completed_);
	if (last_epoch_ >= 0) {
		ini.Write<int>("result", "epoch", last_epoch_);
		ini.Write<int>("result", "max_epoch", max_epoch_);
	}
	if (last_training_ < NAN_VALUE_THRESHOLD) {
		ini.Write<double>("result", "training", last_training_);
	}
	if (last_validation_ < NAN_VALUE_THRESHOLD) {
		ini.Write<double>("result", "validation", last_validation_);
	}
	if (best_epoch_ >= 0) {
		ini.Write<int>("result", "best_epoch", best_epoch_);
	}
	if (best_validation_ < NAN_VALUE_THRESHOLD) {
		ini.Write<double>("result", "best_validation", best_validation_);
	}
	updated_ = true;
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
