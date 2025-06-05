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

#include "nnabla_training_thread.h"
#include "cross_platform_file_util.h"
#include "nnabla_utility.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
int NNablaTrainingThread::CreateNNablaProtoBufForTraining(
	const NNDocument &nn_document, const SDeepConsoleGlobalSettings &settings, //in
	const char *file_name) //out
{
	FStringList weight_file_names;
	FStringList dest_weight_file_names;
	FStringList variable_names;// just for screen display
	FStringList options;

	NNStructures structures;
	structures = nn_document;

	for (int is = 0 ; is < structures.GetNumStructure() ; is++){
		structures.SetActiveStructure(is);
		if (nn_document.IsStructureUsed(structures.GetName())){
			FlattenCurrentStructure(nn_document, structures, true);

			for (int il = 0 ; il < structures.GetNumLayer(); il++) {
				const NNLayer &layer = structures.GetLayer(il);
				if (layer.GetType() == "Parameter" || layer.GetType() == "WorkingMemory") {
					// Parameter layer
					FStringList repeat_extensions = structures.GetRepeatExtensions(layer, true);
					FString file_names = layer.GetPropertyValue(FString("File"));
					if (file_names != "") for (int ire = 0; ire < repeat_extensions.GetSize() ; ire++) {
						FString tmp = file_names.SplitBy(";");
						FString file_name = tmp.SplitBy("*");
						FString option = tmp;
						if (file_name != ""){
							if (FileExists(file_name.PChar())) {
								weight_file_names.Add(file_name);
								variable_names.Add(repeat_extensions.GetAt(ire));
								dest_weight_file_names.Add(repeat_extensions.GetAt(ire).ReplaceStr("/", "~") + FString(".txt"));
								options.Add(option);
							}else {
								//Error
								IndicatorLog(FString("Error : Parameter file ") + file_name + FString(" is not found."));
							}

						}else {
							IndicatorLog(FString("Error : Parameter file for ") + repeat_extensions.GetAt(ire) + FString(" is missing."));
						}
					}
				}
			}
		}
	}

	//Encode files to file_name
	if (weight_file_names.GetSize()) {
		AnsiString tmp_path = ExtractFilePath(file_name);
		AnsiString tmp_dir = tmp_path + "initial_parameters";
		if (DirectoryExists(tmp_dir)) {
			DeleteDir(tmp_dir.c_str());
		}
		ForceDirectories(tmp_dir);

		for (int i = 0 ; i < weight_file_names.GetSize() ; i++) {
			int duplex = false;
			for (int i2 = i - 1 ; i2 >= 0 ; i2--){
				if (dest_weight_file_names.GetAt(i) == dest_weight_file_names.GetAt(i2)){
					duplex = true;
					if (weight_file_names.GetAt(i) != weight_file_names.GetAt(i2) ||
						options.GetAt(i) != options.GetAt(i2)) {
						IndicatorLog(
							FString("Error : parameter file ") + variable_names.GetAt(i2) + FString(" is inconsistent. (") +
							weight_file_names.GetAt(i) + options.GetAt(i) + FString(" != ") + weight_file_names.GetAt(i2) + options.GetAt(i2) + FString(")"));
					}
				}
			}

			if (!duplex){
				IndicatorLog(FString("Copying weight ... ") + variable_names.GetAt(i));
				if (options.GetAt(i).Pos("inv_kernel")) {
					NNWeight weight;
					weight.Load(weight_file_names.GetAt(i).PChar());
					weight.InverseKernel();
					weight.Save((FString(tmp_dir.c_str()) + "\\" + dest_weight_file_names.GetAt(i)).PChar());
				}else if (options.GetAt(i).Pos("sqrt")) {
					NNWeight weight;
					weight.Load(weight_file_names.GetAt(i).PChar());
					weight.Sqrt();
					weight.Save((FString(tmp_dir.c_str()) + "\\" + dest_weight_file_names.GetAt(i)).PChar());
				}else if (options.GetAt(i).Pos("sqr")) {
					NNWeight weight;
					weight.Load(weight_file_names.GetAt(i).PChar());
					weight.Sqr();
					weight.Save((FString(tmp_dir.c_str()) + "\\" + dest_weight_file_names.GetAt(i)).PChar());
				}else if (!CopyFile(UnicodeString(weight_file_names.GetAt(i).PChar()).c_str(), UnicodeString(tmp_dir + "\\" + dest_weight_file_names.GetAt(i).PChar()).c_str(), false)){
					//Error
					IndicatorLog(FString("Error : Failed to copy parameter file from ") + weight_file_names.GetAt(i) + FString(" to ") + dest_weight_file_names.GetAt(i));
				}
			}
		}

		FString sdpex_path = settings.GetLocalNNablaSDPCommand().ReplaceStr(".\\", AnsiString(UnicodeString(ExtractFilePath(ParamStr(0)))).c_str());
		FString command =
			FString("python \"") +
			sdpex_path +
			FString("\" encode_param -e NNabla -i \"") +
			FString(tmp_dir.c_str()) +
			FString("\" -p \"") +
			FString(file_name) +
			FString("\"");
		PipeIO sdp = PipeIO(command.PChar());
		int count = 0;
		while (true){
			FString s = sdp.ReadString();
			if (s != "") {
				IndicatorLog(s);
			}
			Sleep(10);
			if (sdp.GetFinished()) {
				count++;
				if (count >= 50) {
					break;
				}
			}
		}
		sdp.WaitProcessFinished();
		ForceDirectories(tmp_dir);
	}

	return weight_file_names.GetSize() > 0;
}
//---------------------------------------------------------------------------
NNablaTrainingThread::NNablaTrainingThread(
	FString name,
	FString result_dir,
	TComponent *AOwner,
	NNDocument *nn_document,
	SDeepConsoleGlobalSettings *settings,
	int schedule,
	int terminate_with_ctrl_c,
	int resume) :
		SDeepTrainingThread(name, result_dir, AOwner, nn_document, settings, schedule, resume),
        terminate_with_ctrl_c_(terminate_with_ctrl_c),
		prototxt_file_name_((result_dir + FString("\\net.nntxt")).PChar())
{
	FStringList errors;
	FStringList prototxt, param_csv;
	CreateNNablaProtoTxtForTraining(*nn_document, *settings, AnsiString(prototxt_file_name_).c_str(), prototxt, param_csv, errors);
	/*
	TStringList *str = new TStringList();
	for (int i = 0 ; i < prototxt.GetSize() ; i++){
		str->Add(prototxt.GetAt(i).PChar());
	}
	str->SaveToFile(prototxt_file_name_, TEncoding::UTF8);
	delete str;
	*/
	if (nn_document_->GetCommandType() == SDeepCommandType::CompareWithCPU) {
		LocalSDeepProcessorType bak = settings->GetLocalSDeepProcessorType();
		settings->SetLocalSDeepProcessorType(LocalSDeepProcessorType::CPU);
		prototxt_file_name_2_ = (result_dir + FString("\\net_2.nntxt")).PChar();
		FStringList prototxt;
		CreateNNablaProtoTxtForTraining(*nn_document, *settings, AnsiString(prototxt_file_name_2_).c_str(), prototxt, param_csv, errors);
		settings->SetLocalSDeepProcessorType(bak);
	}
}
//---------------------------------------------------------------------------
void __fastcall NNablaTrainingThread::Execute()
{
	//Create result file folder
	UnicodeString result_dir = result_dir_.PChar();

	//monitoring_report file name
	UnicodeString monitoring_report_file_name = result_dir + UnicodeString("\\monitoring_report.yml");
	if (!resume_ && FileExists(monitoring_report_file_name)) {
		DeleteFile(monitoring_report_file_name);
	}

	//Encode Initial parameters
	AnsiString protobuf_file_name = (result_dir_ + FString("\\initial_parameters.h5")).PChar();
	int initial_parameter_exists =
		CreateNNablaProtoBufForTraining(
			*nn_document_, *settings_, protobuf_file_name.c_str());

	//Execute learning
	FString command = GetCommand(initial_parameter_exists, protobuf_file_name);
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
void NNablaTrainingThread::PostLog(FString log_str)
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
//---------------------------------------------------------------------------
};
#pragma package(smart_init)
