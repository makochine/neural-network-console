// Copyright 2024,2025 Sony Group Corporation.
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
#include "cross_platform_file_util.h"
#include "torch_local_training_thread.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
TorchLocalTrainingThread::TorchLocalTrainingThread(
	FString name,
	FString result_dir,
	TComponent *AOwner,
	NNDocument *nn_document,
	SDeepConsoleGlobalSettings *settings,
	int schedule,
	int terminate_with_ctrl_c,
	int resume) :
		SDeepTrainingThread(name, result_dir, AOwner, nn_document, settings, schedule, resume),
		terminate_with_ctrl_c_(terminate_with_ctrl_c)
{
	if (nn_document_->GetCommandType() == SDeepCommandType::CompareWithCPU) {
		//Do nothing.
	}
}
//---------------------------------------------------------------------------
void __fastcall TorchLocalTrainingThread::Execute()
{
	//Create result file folder
	UnicodeString result_dir = result_dir_.PChar();

	//monitoring_report file name
	UnicodeString monitoring_report_file_name = result_dir + UnicodeString("\\monitoring_report.yml");
	if (!resume_ && FileExists(monitoring_report_file_name)) {
		DeleteFile(monitoring_report_file_name);
	}

    nn_document_->SaveToFile(nn_document_->GetFileName());

	//Execute learning
	const FString sdp_path = settings_->GetLocalNNablaSDPCommand().ReplaceStr(".\\", AnsiString(UnicodeString(ExtractFilePath(ParamStr(0)))).c_str());

	//Get Process info
	const FString process_info = settings_-> ProcessorTypeStr();
	const int local_sdeep_gpu_index_ = settings_->GetLocalSDeepGPUIndex();

	FString command = FString("python \"") +
			(nn_document_->GetCommandType() != SDeepCommandType::Script ?
				sdp_path +	FString("\" train") : nn_document_->GetScriptFileName() + FString("\"")) +
			FString(" -c \"") +
			FString(nn_document_->GetFileName()) +
			FString("\" -o \"") +
			result_dir_ +
			FString("\" -C \"") +
			process_info +
			FString(":")+
			FString(local_sdeep_gpu_index_)+
			FString("\"");

	if (resume_) {
		command += FString(" -r");
	}
	nn_document_->SetCommandType(SDeepCommandType::Normal);
	CommandLog(command);

	PipeIO pipe_io(command.PChar());

	process_created_ = true;

	if (!pipe_io.GetSuccess()) {
		PostLog(FString("Failed to create NNabla process.\r\n\tCheck path of Python and NNabla at Setup window."));
		Terminate();
	}

	int count = 0;
    int ctrl_c_sent = 0;
	while (!Terminated || !pipe_io.GetFinished()){
		FString pipe_in = pipe_io.ReadString();
		if (pipe_in != "") {
			PostLog(pipe_in);

			training_completed_ |= pipe_in.Pos("Training Completed") > 0;
			WriteResultIni();

			const int special_termination =
				pipe_in.Pos("Error:") ||
				pipe_in.Pos("cost=nan") || pipe_in.Pos("train_error=nan") || pipe_in.Pos("valid_error=nan") ||
				pipe_in.Pos("cost=inf") || pipe_in.Pos("train_error=inf") || pipe_in.Pos("valid_error=inf");

			if (training_completed_) {
				successfully_finished_ = true;
				// break;
			}else if (special_termination) {
				break;
			}
		}
		if (pipe_io.GetFinished()) {
			count++;
			if (count >= 50) {
				break;
			}
		}
		if (Terminated){
			if (terminate_with_ctrl_c_){
				if (!ctrl_c_sent){
					pipe_io.SendCtrlC();
					ctrl_c_sent = true;
				}
			}else {
				break;
			}
		}
		Sleep(10);
	}
}
//---------------------------------------------------------------------------
void TorchLocalTrainingThread::PostLog(FString log_str)
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

					s.SplitBy("epoch ");
					FString last_epoch_str = s.SplitBy(" of ");
					if (s.Pos("begin")){
						last_epoch_ = 0;
					}else {
						last_epoch_ = StrToIntDef(last_epoch_str.PChar(), last_epoch_);
					}
					FString max_epoch_str = s.SplitBy(" ");
					max_epoch_ = StrToIntDef(max_epoch_str.PChar(), max_epoch_);
					s.SplitBy("train_error=");
					if (s != "") {
						FString last_training_str = s.SplitBy(", valid_error=");
						last_training_ = StrToFloatDef(last_training_str.PChar(), NAN_VALUE);
						if (s != "") {
							FString last_validation_str = s.SplitBy("}");
							last_validation_ = StrToFloatDef(last_validation_str.PChar(), NAN_VALUE);
							if (best_validation_ > NAN_VALUE_THRESHOLD || last_validation_ < best_validation_) {
								best_validation_ = last_validation_;
								best_epoch_ = last_epoch_;
							}
						}
					}
				}
			}
        }
	}
}

}
//---------------------------------------------------------------------------
#pragma package(smart_init)
