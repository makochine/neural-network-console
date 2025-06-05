// Copyright 2019,2020,2021 Sony Corporation.
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

#include "nnabla_remote_training_thread.h"
#include "cross_platform_file_util.h"
#include "nnabla_utility.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
NNablaRemoteTrainingThread::NNablaRemoteTrainingThread(
	FString name,
	FString result_dir,
	TComponent *AOwner,
	NNDocument *nn_document,
	SDeepConsoleGlobalSettings *settings,
	int instance_id,
	int num_node,
	int resume) :
	NNablaTrainingThread(name, result_dir, AOwner, nn_document, settings, false, true, resume),
	instance_id_(instance_id),
	num_node_(num_node)
{
	remote_ = true;
}
//---------------------------------------------------------------------------
FString NNablaRemoteTrainingThread::GetCommand(int initial_parameter_exists, AnsiString protobuf_file_name)
{
	FString command = FString(AnsiString(
		UnicodeString("python \"") +
		ExtractFilePath(ParamStr(0)) +
		UnicodeString("libs\\Python\\Lib\\site-packages\\nnabla_nnc\\cli.py") +
		UnicodeString("\" ")
		).c_str());
	const FString projet_file_name = FString(AnsiString(ExtractFileName(nn_document_->GetFileName().PChar())).c_str());
	const FString project_name =
		FString("NNCW-") +
		projet_file_name.SubString(1, projet_file_name.Length() - ExtractFileExt(projet_file_name.PChar()).Length());


	switch (nn_document_->GetCommandType()) {
	/*
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
	*/
	default:
		command +=
			FString("train -s \"") +
			result_dir_ + FString("\\network.ini\" -n \"") + //sdcproj file
			project_name + //project name
			FString("\" -o \"") +
			result_dir_ + //output_dir
			FString("\" -g ") +
			FString(instance_id_);//instance group
		if (num_node_ > 1) {
			command +=
				FString(" -N ") +
				FString(num_node_);
		}
		if (resume_) {
			command +=
				FString(" -r");
		}
		break;
	}

	return command;
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
