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

#include <IOUtils.hpp>

#include "nnabla_local_training_thread.h"
#include "cross_platform_file_util.h"
#include "nnabla_utility.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
NNablaLocalTrainingThread::NNablaLocalTrainingThread(
	FString name,
	FString result_dir,
	TComponent *AOwner,
	NNDocument *nn_document,
	SDeepConsoleGlobalSettings *settings,
	int schedule,
	int terminate_with_ctrl_c,
	int resume) :
    NNablaTrainingThread(name, result_dir, AOwner, nn_document, settings, schedule, terminate_with_ctrl_c, resume)
{
}
//---------------------------------------------------------------------------
FString NNablaLocalTrainingThread::GetCommand(int initial_parameter_exists, AnsiString protobuf_file_name)
{
	const FString sdp_path = settings_->GetLocalNNablaSDPCommand().ReplaceStr(".\\", AnsiString(UnicodeString(ExtractFilePath(ParamStr(0)))).c_str());
	FString command;

	switch (nn_document_->GetCommandType()) {
	case SDeepCommandType::Profiling:
		command =
			FString("python \"") +
			sdp_path +
			FString("\" profile -c \"") +
			FString(AnsiString(prototxt_file_name_).c_str()) +
			FString("\" -o \"") +
			result_dir_ +
			FString("\"");
		break;
	case SDeepCommandType::CompareWithCPU:
		command =
			FString("python \"") +
			sdp_path +
			FString("\" compare_with_cpu -c \"") +
			FString(AnsiString(prototxt_file_name_).c_str()) +
			FString("\" -c2 \"") +
			FString(AnsiString(prototxt_file_name_2_).c_str()) +
			FString("\" -o \"") +
			result_dir_ +
			FString("\"");
		break;
	default:// SDeepCommandType::Normal, TrainingPlugin
		command =
			FString("python \"") +
			(nn_document_->GetCommandType() != SDeepCommandType::Script ?
				sdp_path +	FString("\" train") : nn_document_->GetScriptFileName() + FString("\"")) +
			FString(" -c \"") +
			FString(AnsiString(prototxt_file_name_).c_str()) +
			(initial_parameter_exists ?
				FString("\" -p \"") +
				FString(AnsiString(protobuf_file_name).c_str()) :
				FString("")) +
			FString("\" -o \"") +
			result_dir_ +
			FString("\"");
		if (resume_) {
			command +=
				FString(" -r");
		}
		if (nn_document_->GetCommandType() == SDeepCommandType::Script) {
			if (nn_document_->GetScriptArgs() != "") {
				command +=
					FString(" ") +
					nn_document_->GetScriptArgs();
			}
		}
		if (nn_document_->GetOutOfCoreTrainingEnabled()) {
			command += FString(" -O");
			if (nn_document_->GetOutOfCoreTrainingGPUMemorySize()) {
				command += FString(" -m ") +
					FString(AnsiString(IntToStr(nn_document_->GetOutOfCoreTrainingGPUMemorySize())).c_str());
			}
			if (nn_document_->GetOutOfCoreTrainingWindowSize()) {
				command += FString(" -w ") +
					FString(AnsiString(IntToStr(nn_document_->GetOutOfCoreTrainingWindowSize())).c_str());
			}
		}
		break;
	}


	return command;
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
