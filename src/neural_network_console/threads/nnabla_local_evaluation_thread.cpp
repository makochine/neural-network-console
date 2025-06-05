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

#include "nnabla_local_evaluation_thread.h"
#include "nnabla_local_training_thread.h"
#include "cross_platform_file_util.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
NNablaLocalEvaluationThread::NNablaLocalEvaluationThread(
	FString name,
	FString result_dir,
	TComponent *AOwner,
	NNDocument *nn_document,
	SDeepConsoleGlobalSettings *settings,
	int schedule) :
		SDeepEvaluationThread(name, result_dir, AOwner, nn_document, settings, schedule),
		prototxt_file_name_((result_dir + FString("\\results.nnp")).PChar()),
		protobuf_file_name_((result_dir + FString("\\parameters.protobuf")).PChar())
{
	if (!FileExists(prototxt_file_name_)) {
		prototxt_file_name_ = (result_dir + FString("\\net.nntxt")).PChar();
		if (!FileExists(prototxt_file_name_)) {
			prototxt_file_name_ = (result_dir + FString("\\net.prototxt")).PChar();
			if (!FileExists(prototxt_file_name_)) {
				prototxt_file_name_ = "";
			}
		}
	}
	//FStringList errors;
	//NNablaLocalTrainingThread::CreateNNablaProtoTxtForTraining(*nn_document, *settings, AnsiString(prototxt_file_name_).c_str(), errors);

	if (!FileExists(protobuf_file_name_)) {
		protobuf_file_name_ = (result_dir + FString("\\parameters.h5")).PChar();
		if (!FileExists(protobuf_file_name_)) {
			protobuf_file_name_ = "";
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall NNablaLocalEvaluationThread::Execute()
{
	//Create result file folder
	UnicodeString result_dir = result_dir_.PChar();

	//Runtime
    FString dataset_file_name;
	for (int i = 0; i < nn_document_->GetNumExecutor(); i++) {
		const NNExecutor &executor = nn_document_->GetExecutor(i);
		int dataset_index = nn_document_->GetDatasetIndexByName(executor.GetDatasetNames().GetText(','));
		if (dataset_index >= 0) {
			dataset_file_name = nn_document_->GetDataset(dataset_index).GetFileName();
		}
	}

	//Execute Evaluation
	FString sdp_path = settings_->GetLocalNNablaSDPCommand().ReplaceStr(".\\", AnsiString(UnicodeString(ExtractFilePath(ParamStr(0)))).c_str());
	FString command =
		FString("python \"") +
		sdp_path +
		FString("\" forward -c \"") +
		FString(AnsiString(prototxt_file_name_).c_str()) +
		(protobuf_file_name_ != "" ?
			FString("\" -p \"") +
			FString(AnsiString(protobuf_file_name_).c_str()) :
			FString("")) +
		FString("\" -d \"") +
		dataset_file_name +
		FString("\" -o \"") +
		FString(AnsiString(result_dir).c_str()) +
		FString("\" --replace_path");
	CommandLog(command);

	PipeIO pipe_io(command.PChar());

	process_created_ = true;

	if (!pipe_io.GetSuccess()) {
		PostLog(FString("Failed to create NNabla process.\r\n\tCheck path of Python and NNabla at Setup window."));
		Terminate();
	}

	int count = 0;
	while (!Terminated){
		FString pipe_in = pipe_io.ReadString();
		if (pipe_in != "") {
			PostLog(pipe_in);

			const int evaluation_completed = pipe_in.Pos("Evaluation Completed") > 0;
			if (evaluation_completed) {
				successfully_finished_ = true;
				break;
			}
		}
		if (pipe_io.GetFinished()) {
			count++;
			if (count >= 50) {
				break;
			}
		}
		Sleep(10);
	}
}
//---------------------------------------------------------------------------
void NNablaLocalEvaluationThread::PostLog(FString log_str)
{
	FStringList new_logs;
	new_logs.SplitStr(log_str, FString("\r\n"), FString(""));
	if (new_logs.GetSize()) {
		if (new_logs.GetAt(new_logs.GetSize() - 1) == "") {
			new_logs.Delete(new_logs.GetSize() - 1);
		}
		if (new_logs.GetSize()) {
			Log(new_logs);

			for (int il = 0 ; il < new_logs.GetSize() ; il++) {
				if (new_logs.GetAt(il) != "") {
					FString s = new_logs.GetAt(il);

					s.SplitBy(" data ");
					FString data_index_str = s.SplitBy(" / ");
					last_data_ = StrToIntDef(data_index_str.PChar(), last_data_);
					FString data_num_str = s;
					data_num_ = StrToIntDef(data_num_str.PChar(), data_num_);
				}
            }
		}
	}
}
//---------------------------------------------------------------------------
};
#pragma package(smart_init)
