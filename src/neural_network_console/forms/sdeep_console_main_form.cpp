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
#include <Windows.h>

#include <fmx.h>
#pragma hdrstop

#include <stdlib.h>
#include <sys/stat.h>
#include <System.IOUtils.hpp>
#include <System.JSON.hpp>
#include <System.DateUtils.hpp>
#include <System.Net.HttpClient.hpp>
#include <IdHTTP.hpp>
#include "FMX.Clipboard.hpp"

#include "flist.h"
#include "fstring.h"
#include "ffastini.h"
#include "file_util.h"
#include "color_util.h"
#include "cross_platform_file_util.h"
#include "gaussian_process.h"

#include "nnstructure_to_vector.h"

#include "nn_document_yamlutility.h"

#include "nnabla_utility.h"
#include "caffe_utility.h"
#include "torch_utility.h"

#include "sdeep_console_setup_form.h"
#include "sdeep_console_create_dataset_form.h"
#include "sdeep_console_edit_text_form.h"
#include "sdeep_console_create_unit_form.h"
#include "sdeep_console_main_form.h"
#include "sdeep_console_plugin_form.h"
#include "sdeep_console_replace_form.h"
#include "color_defines.h"

#include "script_thread.h"
#include "nnabla_local_training_thread.h"
#include "torch_local_training_thread.h"
#include "torch_local_evaluation_thread.h"
#include "nnabla_remote_training_thread.h"
#include "nnabla_local_evaluation_thread.h"
#include "translation.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"

TFo_Main *Fo_Main;

//Auth
static int auth_failed = false;
static const int auth_timeout_in_ms = 10000;

//Common
static const int default_panel_height = 48;
static const int default_list_item_height = 112;

static const double sdeep_console_transition_coef = 0.2;

static const int default_grid_text_hspace = 16;
static const int default_grid_text_vspace = 1;

//Network
static const int default_property_header_height = 48;
static const int default_property_height = 28;

//List
static const int list_col_width = 620;
static const int list_col_space = 16;

//Dataset
static const int default_dataset_text_space = 16;

//Training Result
static const int monitor_hspace = 128;
static const int monitor_hspace_2 = 16;
static const int monitor_vspace = 64;
static const int graph_scale_size = 16;

static int annonymous_thread_count = 0;
//---------------------------------------------------------------------------
void __fastcall TDocumentIOFunc::Invoke() {
	annonymous_thread_count++;
	switch (type_) {
	case DocumentIOType::Open:
		document_->LoadFromFile(FString(AnsiString(file_name_).c_str()), FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()));
		TThread::Synchronize(TThread::CurrentThread, CallRefreshLayerComponent);
		break;
	case DocumentIOType::Save:
		document_->SaveToFile(FString(AnsiString(file_name_).c_str()));
		break;
	case DocumentIOType::OpenDataset:
		document_->LoadDatasetFromFile(option_, FString(AnsiString(file_name_).c_str()));
		break;
	case DocumentIOType::CreateDataset:
		{
			FString sdp_path = settings_->GetLocalNNablaSDPCommand().ReplaceStr(".\\", AnsiString(UnicodeString(ExtractFilePath(ParamStr(0)))).c_str());
			FString command =
				FString("python \"") +
				sdp_path +
				FString("\"") +
				FString(AnsiString(file_name_).c_str());
			PipeIO sdp = PipeIO(command.PChar());
			if (debug_) {
				SyncAndShowMessage(command.PChar());
			}
			int count = 0;
			int terminate = false;
			while (true){
				FString s = sdp.ReadString();
				if (s != "") {
					if (!SyncAndIndicatorLog(s.PChar())){
						terminate = true;
						break;
					}
				}
				Sleep(10);
				if (sdp.GetFinished()) {
					count++;
					if (count >= 50) {
						break;
					}
				}
			}
			if (!terminate) {
				sdp.WaitProcessFinished();
			}
			if (debug_) {
				SyncAndShowMessage(sdp.GetLog().PChar());
			}
		}
		break;
	case DocumentIOType::OpenResultCSV:
		document_->LoadResultCSVFromFile(option_, FString(AnsiString(file_name_).c_str()));
		break;
	case DocumentIOType::ExtractWeight_NNabla:
		{
			FString s = FString(AnsiString(file_name_).c_str());
			AnsiString protobuf_file_name = s.SplitBy(",").PChar();
			if (FileExists(protobuf_file_name + ".nnp")) {
				protobuf_file_name += ".nnp";
			}else if (FileExists(protobuf_file_name + ".protobuf")) {
				protobuf_file_name += ".protobuf";
			}else {
				protobuf_file_name += ".h5";
			}
			//Check if the parameter file is updated.
			struct stat stat_buf;
			if (stat(protobuf_file_name.c_str(), &stat_buf) == 0) {
				if (!option_){
					struct stat stat_buf;
					if (stat(protobuf_file_name.c_str(), &stat_buf) == 0) {
						//Overwrite weight files if the parameter file is updated.
						FStringList modified_txt;
						double modified_time = 0.0;
						if (modified_txt.LoadFromFile((s + FString("\\modified.txt")).PChar())){
							if (modified_txt.GetSize() == 1){
								modified_time = modified_txt.GetAt(0).AsFloat();
							}
						}
						option_ |= stat_buf.st_mtime > modified_time;
					}
				}
			}
			AnsiString output_dir = s.PChar();
			if (!DirectoryExists(output_dir) || option_) {//option_ = overwrite
				FString sdpex_path = settings_->GetLocalNNablaSDPCommand().ReplaceStr(".\\", AnsiString(UnicodeString(ExtractFilePath(ParamStr(0)))).c_str());
				FString command =
					FString("python \"") +
					sdpex_path +
					FString("\" decode_param -p \"") +
					FString(protobuf_file_name.c_str()) +
                    FString("\" ") +
					FString("-e NNabla ") +
					FString("-o \"") +
					FString(output_dir.c_str()) +
					FString("\"");
				PipeIO sdp = PipeIO(command.PChar());
				if (debug_) {
					SyncAndShowMessage(command.PChar());
				}
				int count = 0;
				while (true){
					FString s = sdp.ReadString();
					if (s != "") {
						SyncAndIndicatorLog(s.PChar());
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
				FStringList modified_txt;
				modified_txt.Add(FString((double)stat_buf.st_mtime));
				modified_txt.SaveToFile((s + FString("\\modified.txt")).PChar());
				if (debug_) {
					SyncAndShowMessage(sdp.GetLog().PChar());
				}
			}
		}
		break;
	case DocumentIOType::ExtractNnpt:
		{
			FString s = FString(AnsiString(file_name_).c_str());
			AnsiString nnpt_file_name = s.SplitBy(",").PChar();
			UnicodeString output_dir = nnpt_file_name.SubString(1, nnpt_file_name.Length() - ExtractFileExt(nnpt_file_name).Length()) + ".files";
			UnicodeString output_sdcproj = output_dir + "\\config.sdcproj";
			if (!DirectoryExists(output_dir) || option_) {//option_ = overwrite
				FString sdpex_path = settings_->GetLocalNNablaSDPCommand().ReplaceStr(".\\", AnsiString(UnicodeString(ExtractFilePath(ParamStr(0)))).c_str());
				FString command =
					FString("python \"") +
					sdpex_path +
					FString("\" decode_param -p \"") +
					FString(nnpt_file_name.c_str()) +
					FString("\" ") +
					FString("-e Torch ") +
					FString("-o \"") +
					FString(AnsiString(output_dir).c_str());
					FString("\"");
				PipeIO sdp = PipeIO(command.PChar());
				if (debug_) {
					SyncAndShowMessage(command.PChar());
				}
				int count = 0;
				while (true){
					FString s = sdp.ReadString();
					if (s != "") {
						SyncAndIndicatorLog(s.PChar());
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
				if (debug_) {
					SyncAndShowMessage(sdp.GetLog().PChar());
				}
			}
		}
		break;
	case DocumentIOType::PythonCall:
		{
			FString command = FString("python ") + FString(AnsiString(file_name_).c_str());
            PipeIO sdp = PipeIO(command.PChar());
			if (debug_) {
				SyncAndShowMessage(command.PChar());
			}
			int count = 0;
			int terminate = false;
			while (true){
				FString s = sdp.ReadString();
				if (s != "") {
					if (!SyncAndIndicatorLog(s.PChar())){
						terminate = true;
						sdp.SendCtrlC();
					}
				}
				Sleep(10);
				if (sdp.GetFinished()) {
					count++;
					if (count >= 50) {
						break;
					}
				}
			}
			if (!terminate) {
				sdp.WaitProcessFinished();
			}
			if (debug_) {
				SyncAndShowMessage(sdp.GetLog().PChar());
			}
		}
		if (option_){
			annonymous_thread_count--;
			return; //return without unlock
		}
		break;
	case DocumentIOType::Auth:
		annonymous_thread_count--;
		return;//return without unlock
	case DocumentIOType::UploadDataset:
		{
			FString s = FString(AnsiString(file_name_).c_str());
			UnicodeString csv_file_name = s.SplitBy(",").PChar();
			UnicodeString tar_file_name = csv_file_name.SubString(1, csv_file_name.Length() - ExtractFileExt(csv_file_name).Length()) + ".tar";
			if (FileExists(tar_file_name)){
				DeleteFile(tar_file_name);
			}
			FString sdpex_path = settings_->GetLocalNNablaSDPCommand().ReplaceStr(".\\", AnsiString(UnicodeString(ExtractFilePath(ParamStr(0)))).c_str());
			int terminate = false;

			//Create tar file
			{
				FString command =
					FString("python \"") +
					sdpex_path +
					FString("\" create_tar \"") +
					FString(AnsiString(csv_file_name).c_str()) +
					FString("\" \"") +
					FString(AnsiString(tar_file_name).c_str()) +
					FString("\"");
				PipeIO sdp = PipeIO(command.PChar());
				if (debug_) {
					SyncAndShowMessage(command.PChar());
				}
				int count = 0;
				while (true){
					FString s = sdp.ReadString();
					if (s != "") {
						if (!SyncAndIndicatorLog(s.PChar())){
							terminate = true;
							break;
						}
					}
					Sleep(10);
					if (sdp.GetFinished()) {
						count++;
						if (count >= 50) {
							break;
						}
					}
				}
				if (!terminate) {
					sdp.WaitProcessFinished();
				}
				if (debug_) {
					SyncAndShowMessage(sdp.GetLog().PChar());
				}
			}
			if (!terminate && FileExists(tar_file_name)){
				//Upload tar file

				FString command =
					FString("python \"") +
					sdpex_path +
					FString("\" upload \"") +
					FString(s) +
					FString("\" \"") +
					FString(AnsiString(tar_file_name).c_str()) +
					FString("\"");
				PipeIO sdp = PipeIO(command.PChar());
				if (debug_) {
					SyncAndShowMessage(command.PChar());
				}
				int count = 0;
				while (true){
					FString s = sdp.ReadString();
					if (s != "") {
						if (!SyncAndIndicatorLog(s.PChar())){
							terminate = true;
							break;
						}
					}
					Sleep(10);
					if (sdp.GetFinished()) {
						count++;
						if (count >= 50) {
							break;
						}
					}
				}
				if (!terminate) {
					sdp.WaitProcessFinished();
				}
				if (debug_) {
					SyncAndShowMessage(sdp.GetLog().PChar());
				}
			}
		}
		break;
	case DocumentIOType::Import:
		{
			UnicodeString output_dir =
				file_name_.SubString(1, file_name_.Length() - ExtractFileExt(file_name_).Length()) + ".files";
			FString command =
				FString("python \"") +
				FString(AnsiString(ExtractFilePath(ParamStr(0)) + UnicodeString("libs\\scripts\\import.py")).c_str()) +
				FString("\" -i \"") +
				FString(AnsiString(file_name_).c_str()) +
				FString("\" -o \"") +
				FString(AnsiString(output_dir).c_str()) +
				FString("\"");
			PipeIO script = PipeIO(command.PChar());
			if (debug_) {
				SyncAndShowMessage(command.PChar());
			}
			int count = 0;
			while (true){
				FString s = script.ReadString();
				if (s != "") {
					SyncAndIndicatorLog(s.PChar());
				}
				Sleep(10);
				if (script.GetFinished()) {
					count++;
					if (count >= 50) {
						break;
					}
				}
			}
			script.WaitProcessFinished();
			if (debug_) {
				SyncAndShowMessage(script.GetLog().PChar());
			}
			ChDir(ExtractFileDir(ParamStr(0)));
		}
		break;
	case DocumentIOType::Export:
		{
			UnicodeString file_name, format;
			switch (option_){
			case 0:
				file_name = "model.nnp";
				format = "NNP";
				break;
			case 1:
				file_name = "model.nnb";
				format = "NNB";
				break;
			case 2:
				file_name = "model.onnx";
				format = "ONNX";
				break;
			case 3:
				file_name = "saved_model";
				format = "SAVED_MODEL";
				break;
			case 4:// not supported
				file_name = "model.ckpt";
				format = "TF_CKPT_V1";
				break;
			case 5:// not supported
				file_name = "model.meta";
				format = "TF_CKPT_V2";
				break;
			case 6:
				file_name = "model.tflite";
				format = "TFLITE";
				break;
			}

			FString command =
				FString("python \"") +
				FString(AnsiString(ExtractFilePath(ParamStr(0)) + UnicodeString("libs\\scripts\\export.py")).c_str()) +
				FString("\" -n \"");
			if (FileExists(AnsiString(file_name_ + UnicodeString("\\results.nnp")))){
				command +=
					FString(AnsiString(file_name_ + UnicodeString("\\results.nnp")).c_str());
			}else {
				command +=
					FString(AnsiString(file_name_ + UnicodeString("\\net.nntxt")).c_str()) +
					FString("\" -p \"") +
					FString(AnsiString(file_name_ + UnicodeString("\\parameters.h5")).c_str());
			}
			command +=
				FString("\" -o \"") +
				FString(AnsiString(file_name_ + UnicodeString("\\") + file_name).c_str()) +
				FString("\" -f \"") +
				FString(AnsiString(format).c_str()) +
				FString("\"");
			PipeIO script = PipeIO(command.PChar());
			if (debug_) {
				SyncAndShowMessage(command.PChar());
			}
			int count = 0;
			while (true){
				FString s = script.ReadString();
				if (s != "") {
					SyncAndIndicatorLog(s.PChar());
				}
				Sleep(10);
				if (script.GetFinished()) {
					count++;
					if (count >= 50) {
						break;
					}
				}
			}
			script.WaitProcessFinished();
			if (debug_) {
				SyncAndShowMessage(script.GetLog().PChar());
			}
			ChDir(ExtractFileDir(ParamStr(0)));
		}
		break;
	case DocumentIOType::ExportReport:
		{
			FString report_type, ext;
			switch (option_){
			case 1:
				report_type = FString("html");
				ext = FString(".html");
				break;
			default:
				report_type = FString("pptx");
				ext = FString(".pptx");
			}
			FString command =
				FString("python \"") +
				FString(AnsiString(ExtractFilePath(ParamStr(0)) + UnicodeString("libs\\scripts\\export_")).c_str()) +
				report_type +
				FString(".py\" -i \"") +
				FString(AnsiString(file_name_).c_str()) +
				FString("\" -o \"") +
				FString(AnsiString(file_name_.SubString(1, file_name_.Length() - 4)).c_str()) +
				ext +
				FString("\"");
			if (option_ == 0){
				command +=
					FString(" -t \"") +
					FString(AnsiString(ExtractFilePath(ParamStr(0)) + UnicodeString("libs\\scripts\\template.pptx")).c_str()) +
					FString("\"");
			}
			PipeIO script = PipeIO(command.PChar());
			if (debug_) {
				SyncAndShowMessage(command.PChar());
			}
			int count = 0;
			while (true){
				FString s = script.ReadString();
				if (s != "") {
					SyncAndIndicatorLog(s.PChar());
				}
				Sleep(10);
				if (script.GetFinished()) {
					count++;
					if (count >= 50) {
						break;
					}
				}
			}
			script.WaitProcessFinished();
			if (debug_) {
				SyncAndShowMessage(script.GetLog().PChar());
			}
			ChDir(ExtractFileDir(ParamStr(0)));
		}
		break;
	default:
		break;
	}
	TThread::Synchronize(TThread::CurrentThread, DocumentUnlock);
	annonymous_thread_count--;
}
//---------------------------------------------------------------------------
void __fastcall TDocumentIOFunc::CallRefreshLayerComponent()
{
	Fo_Main->RefreshLayerComponentTree(document_->GetSDeepEngineType());
}
//---------------------------------------------------------------------------
void __fastcall TDocumentIOFunc::Close()
{
	Fo_Main->Close();
}
//---------------------------------------------------------------------------
void __fastcall TDocumentIOFunc::IndicatorLog()
{
	Fo_Main->AnimationIndicatorLog(message_);
	result_ = !Fo_Main->animation_indicator_abort_;
};
//---------------------------------------------------------------------------
__fastcall TFo_Main::TFo_Main(TComponent* Owner)
	: TForm(Owner),
	activity_(SDeepConsoleActivity::Browser),
	setting_file_(NULL),

	update_count_(0),
	view_update_count_(0),
	control_button_type_(ControlButtonType::VisibleFalse),
	sdeep_console_transition_coef_(0.2f),

	network_tab_index_2_(-1),
	network_tab_delete_focus_(false),
	drag_network_tab_(false),
	drag_network_tab_start_x_(0),
	drag_network_tab_x_shift_(0),

	network_focuse_index_(0),
	component_drag_mode_(ComponentDragMode::None),
	network_left_(0.0),
	network_current_left_(0.0),
	network_top_(0.0),
	network_current_top_(0.0),
	network_zoom_(0),
	network_current_zoom_(0.0),
	network_mouse_down_(false),

	network_mouse_down_mode_(NetworkMouseDownMode::None),
	network_start_x_(0.0f),
	network_start_y_(0.0f),
	network_last_x_(0.0f),
	network_last_y_(0.0f),
    last_layer_id_(-1),

	property_top_(0.0f),
	property_current_top_(0.0f),
    property_current_pos_(-1),
	visible_property_num_(0),
	current_property_index_(-1),
	current_property_layer_id_(-1),

	dataset_update_count_(0),
	dataset_internal_update_count_(0),
	dataset_image_update_count_(0),
	dataset_csv_update_count_(0),

	monitor_zoom_(0),
	monitor_date_(0),
	monitor_update_request_(false),

	result_tradeoff_update_count_(0),
	result_tradeoff_zoom_(0),
	result_last_mouse_down_(false),
	result_last_mouse_x_(0.0f),
	result_last_mouse_y_(0.0f),
	network_overview_update_count_(0),
	network_overview_zoom_(1.0),
	network_overview_current_zoom_(1.0),
	network_overview_last_x_(0.0),
	network_overview_last_y_(0.0),
	progress_training_max_(1),
	progress_training_current_(0),
	progress_evaluation_max_(1),
	progress_evaluation_current_(0),
	result_weight_zoom_(4),
	result_weight_current_zoom_(log(4.0)),
	result_weight_top_(0.0),
	result_weight_current_top_(0.0),
	others_table_type_(OthersTableType::Default),
	dataset_header_height_(48),
	dataset_header_width_(160),
	csv_graph_exists_(false),
    show_dataset_plugin_result_(false),
	//training_result_graph_mode_(TrainingResultGraphMode::LearningCurve),


	sdeep_evaluation_thread_(NULL),

	structure_search_last_gaussian_process_beta_(4.0),
	structure_search_last_gaussian_process_gaussian_kernel_sigma_(10.0),
	structure_search_candidate_index_(-1),

	online_debug_mode_(false),
	screen_cursor_on_(false),

	network_overview_(NULL),
	network_overview_structure_index_(0),

	image_cache_list_(NULL),
	csv_cache_list_(NULL),

	animation_indicator_active_(false),
	animation_indicator_start_time_(0),
	animation_indicator_abort_(false),
	animation_indicator_hide_log_(false),
	closed_(false),

	time_created_(0),

	debug_bmp_(NULL),

    setup_request_(false)
{
	default_dataset_col_width_ = 256;
	default_dataset_row_height_ = 128;

	memset(monitor_update_count_, 0, sizeof(monitor_update_count_));
	memset(monitor_value_range_, 0, sizeof(monitor_value_range_));
	memset(monitor_current_value_range_, 0, sizeof(monitor_current_value_range_));

	memset(result_tradeoff_value_range_, 0, sizeof(result_tradeoff_value_range_));
	result_tradeoff_value_range_[0] = 1.0;
	result_tradeoff_value_range_[1] = 1.0;
	memset(result_current_tradeoff_value_range_, 0, sizeof(result_current_tradeoff_value_range_));
	result_current_tradeoff_value_range_[0] = 1.0;
	result_current_tradeoff_value_range_[1] = 1.0;
}
//---------------------------------------------------------------------------
int TFo_Main::AddHint(TControl *control, FString hint)
{
	const int index = controls_.IndexOf(control);
	if (index < 0){
		if (hint != ""){
			controls_.Add(control);

			control_hints_.Add(hint);
		}
	}else if (hint != ""){
		control_hints_.GetAt(index) = hint;
	}else {
		controls_.Delete(index);
		control_hints_.Delete(index);
	}
	return true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::FormShow(TObject *Sender)
{
	//Invisible Components
	Ed_NetworkName->Visible = false;
	Im_CursorForCapture->Visible = false;
	Pa_Hint->Visible = false;
	Pa_Processing->Visible = false;
	Bu_Save->Visible = false;
	Bu_SaveAs->Visible = false;
	Bu_Undo->Visible = false;
	Bu_Redo->Visible = false;
	Bu_Cut->Visible = false;
	Bu_Copy->Visible = false;
	Bu_Paste->Visible = false;
	Bu_Property->Visible = false;
	Co_Property->Visible = false;
	Lo_EditProperty->Visible = false;
	Bu_PropertyBrowse->Visible = false;
	Bu_Statistics->Visible = false;
	Im_Warning->Visible = false;
	Im_BackgroundBrain->Visible = false;
	Bu_Network->Visible = false;
	Bu_NetworkTab2->Visible = false;
	Sc_NetworkY->Visible = false;
	Sc_NetworkX->Visible = false;
	Bu_Table->Visible = false;
	Sc_TableY->Visible = false;
	Sc_TableX->Visible = false;
	RB_ResultsWeight->Visible = false;
	Sc_Monitor->Visible = false;
	Bu_MonitorSolve->Visible = false;
	Im_Checked->Visible = false;
	Im_NotChecked->Visible = false;
    Co_HStatistics->Visible = false;
	Bu_EvaluationSolve->Visible = false;
	Re_StatusBar->Visible = false;
	Bu_OpenDataset->Visible = false;
	Bu_CreateDataset->Visible = false;
	Bu_OpenDataset2->Visible = false;
	Bu_CreateDataset2->Visible = false;
	Lo_RightSub->Visible = false;
	Lo_RightSub->Width = 800;
    PB_Debug->Visible = false;
	Bu_Overview->Visible = false;
    Lo_DataPreview->Visible = false;

	//Fo_SplashWindow = new TFo_SplashWindow(this);
	La_Company->Text = "Copyright (C) 2017-2025 Sony Group Corporation";
	Fr_Browser->Align = TAlignLayout::Client;
	Fr_Browser->Li_TopLine->Visible = false;

	Fr_ProjectFiles->Align = TAlignLayout::Client;

	Fr_DatasetFiles->Visible = false;
	Fr_DatasetFiles->Align = TAlignLayout::Client;
	Fr_DatasetFiles2->Align = TAlignLayout::Client;
	Bu_OpenDataset->Position->Y = 0;
	Bu_CreateDataset->Position->Y = 0;
	Bu_OpenDataset2->Position->Y = Re_DatasetTop2->Position->Y;
	Bu_CreateDataset2->Position->Y = Re_DatasetTop2->Position->Y;
	Fr_AISolutions->Visible = false;
	Fr_AISolutions->Align = TAlignLayout::Client;
	La_AISolutions->Visible = false;
	La_AISolutions->Position->Y = 0;
	Fr_NNablaExamples->Visible = false;
	Fr_NNablaExamples->Align = TAlignLayout::Client;
	La_InteractiveDemos->Visible = false;
	La_InteractiveDemos->Position->Y = 0;

	Fr_Component->Align = TAlignLayout::Client;
	Fr_Component->Li_TopLine->Visible = false;
	Fr_Component->Sc_ListY->Margins->Top = 48;

	Pa_Splash->Align = TAlignLayout::Center;
	La_SplashNote->Text = StringReplace(La_SplashNote->Text, "(Product Name)", PRODUCT_NAME, TReplaceFlags() << rfReplaceAll);
	La_SplashVersion->Text = (
		FString("version ") +
		GetBinaryFileVersion(AnsiString(ParamStr(0)).c_str())
		).PChar();

	//Network Tab
	Lo_NetworkClient->Align = TAlignLayout::Client;
	PB_Network->Align = TAlignLayout::Client;
	PB_Property->Align = TAlignLayout::Client;
	PB_Statistics->Align = TAlignLayout::Client;

	//Dataset Tab
	Lo_Table->Align = TAlignLayout::Client;
	PB_Table->Align = TAlignLayout::Client;
	Fr_DatasetFiles->Visible = false;
	Fr_DatasetFiles2->Visible = false;
	//Lo_CSVGraphType->Visible = false;

	//Monitor Tab
	Lo_Result->Align = TAlignLayout::Client;
	Bu_Monitor->Visible = false;
	PB_MonitorPerformance->Align = TAlignLayout::Client;
	PB_MonitorPerformance->Visible = false;
	Bu_MonitorPerformance->Visible = false;

	//Evaluation
	Lo_EvaluationDataset->Align = TAlignLayout::Client;

	//Debug Tab
	Me_Debug->Align = TAlignLayout::Client;
	TI_Debug->Visible = false;

	SetScale(1.0);

	//Right pane
	Co_Training->Align = TAlignLayout::Client;
	Co_Run->Align = TAlignLayout::Client;
	Lo_DataPreview->Align = TAlignLayout::Client;

	//Screen flash proof
	Transparency = false;
	WindowState = TWindowState::wsMaximized;


	time_created_ = timeGetTime();

	//Initialization
	setting_file_ = new FFastIni(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\settings.ini").c_str());
	result_tradeoff_h_statistics_str_ = FString("CostMultiplyAdd");

	{
		FFastIni default_setting_file = FFastIni(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\default_settings.ini").c_str());
		settings_.LoadFromIni(&default_setting_file);
	}
	settings_.LoadFromIni(setting_file_);
	settings_.RefreshCustomUnits(FString(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\custom_units").c_str()));

	//Translate
	SetupTranslation(Lang);
	default_lang_ = Lang->Lang;
	ListMenuItemForTranslation(this);
	Translate();
	SetScale(settings_.GetGUIScale());

    //Check installation
	if (!DirectoryExists(ExtractFilePath(ParamStr(0)) + "/settings") ||
		!DirectoryExists(ExtractFilePath(ParamStr(0)) + "/libs")){
		Activate();
		ShowMessage(
			GetTranslatedText("Neural Network Console is not installed correctly.") +
			"\n" +
			GetTranslatedText("Please try to install Neural Network Console again."));
		Close();
		return;
	}

	//Setup Engines
	FStringList errors;
	settings_.SetupEngines(FString(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\").c_str()), errors);
	settings_.SetupLinkCollection(FString(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\").c_str()), errors);
	OpenInstanceList();
	if (errors.GetSize()) {
		Activate();
		ShowMessage(errors.GetText().PChar());
		Close();
		return;
	}
	for (int i = 0; i < (int)SDeepEngineType::NumEngineType ; i++) {
		SDeepEngineType sdeep_engine_type = SDeepEngineType(i);

		const NNEngineInfo &engine_info = settings_.GetEngineInfo().GetAt((int)sdeep_engine_type);
		nn_document_.RegisterNNEngineInfo(sdeep_engine_type, &engine_info);
		nn_document_tmp_.RegisterNNEngineInfo(sdeep_engine_type, &engine_info);
		result_network_.RegisterNNEngineInfo(sdeep_engine_type, &engine_info);
	}
	for (int i = 0; i < settings_.GetDocumentConverterSettingList().GetSize() ; i++) {
		nn_document_.RegisterNNDocumentConverterSetting(&settings_.GetDocumentConverterSettingList().GetAt(i));
		nn_document_tmp_.RegisterNNDocumentConverterSetting(&settings_.GetDocumentConverterSettingList().GetAt(i));
		result_network_.RegisterNNDocumentConverterSetting(&settings_.GetDocumentConverterSettingList().GetAt(i));
	}
    SetupPresetProjectMenu();

	//Open project list
	AnsiString project_list_file_name = ExtractFilePath(ParamStr(0)) + "settings\\project_list.txt";
	if (!FileExists(project_list_file_name)) {
		AnsiString initial_project_list_file_name = ExtractFilePath(ParamStr(0)) + "settings\\initial_project_list.txt";
		if (FileExists(initial_project_list_file_name)) {
			Ioutils::TFile::Copy(initial_project_list_file_name, project_list_file_name, false);
		}
	}
	project_list_.SetFileName(project_list_file_name.c_str());
	project_list_.LoadFromFile();

	//Open dataset list
	AnsiString dataset_list_file_name = ExtractFilePath(ParamStr(0)) + "settings\\dataset_list.txt";
	if (!FileExists(dataset_list_file_name)) {
		AnsiString initial_dataset_list_file_name = ExtractFilePath(ParamStr(0)) + "settings\\initial_dataset_list.txt";
		if (FileExists(initial_dataset_list_file_name)) {
			Ioutils::TFile::Copy(initial_dataset_list_file_name, dataset_list_file_name, false);
		}
	}
	dataset_list_.SetFileName(dataset_list_file_name.c_str());
	dataset_list_.LoadFromFile();

	//Other setups
	SetCSVGraphType();

	image_cache_list_ = new ImageCacheList();
	csv_cache_list_ = new CSVCacheList();
	Ti_Animation->Enabled = true;

	//Start Activity
	SetActivity(activity_);

	CheckLogLevel();
	Log(FString("launch"));

	RefreshListSettings();
}
//---------------------------------------------------------------------------
void TFo_Main::SetupPresetProjectMenu()
{
	const FStringList &preset_projects_transformers = settings_.GetEngineInfo().GetAt((int)SDeepEngineType::Transformers).GetPresetProjects();
	for (int ip = 0 ; ip < preset_projects_transformers.GetSize() ; ip++) {
		FString project_name = preset_projects_transformers.GetAt(ip);
		while (true){
			int p = project_name.Pos("\\");
			if (p) {
				project_name = project_name.SubString(p + 1, project_name.Length() - p);
			}else{
				break;
			}
		}
		project_name = project_name.SubString(1, project_name.Length() - 8);
		project_name = project_name.ReplaceStr(FString("_"), FString(" "));
		project_name = project_name.SubString(1, 1).ToUpper() + project_name.SubString(2, project_name.Length() - 1);

		TMenuItem *mi = new TMenuItem(NULL);
		mi->Text = project_name.PChar();
		mi->OnClick = PresetProjectClick;
		mi->Tag = (int)SDeepEngineType::Transformers * 0x10000 + ip;
		PM_NewProjectTransformers->AddObject(mi);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PresetProjectClick(TObject *Sender)
{
	const int tag = ((TMenuItem*)Sender)->Tag;
	const SDeepEngineType engine_type = (SDeepEngineType)(tag / 0x10000);
	assert(engine_type < SDeepEngineType::NumEngineType);
	const FStringList &preset_projects = settings_.GetEngineInfo().GetAt((int)engine_type).GetPresetProjects();
	const int preset_project_index = tag % 0x10000;
	assert(preset_project_index < preset_projects.GetSize());

	SetActivity(SDeepConsoleActivity::Default);
	Bu_NetworkTabClick(this);
	NewProject(SDeepEngineType::NNabla);
	OpenProject(preset_projects.GetAt(preset_project_index).PChar(), true);
    nn_document_.SetFileName(FString(""));
	TC_DefaultChange(Sender);//to set network_overview_
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshListSettings()
{
	//Browser menu
	{
		FListP<ListUIAttribute> attributes;
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Name"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space - 3);
			new_attribute.SetPosition(2, Fr_Browser->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_DARKEST);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_LIGHTEST);
			new_attribute.SetFontSize(16);
		}
		Fr_Browser->Setup(&settings_, "", 1, -1, 48, 0, false, false, true, attributes);
		Fr_Browser->SetColor(
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_BRAND,
			SDEEP_CONSOLE_COLOR_BRAND,
			SDEEP_CONSOLE_COLOR_LIGHTER);
		// Fr_Browser->OnItemChanged = DatasetItemChanged;

		Fr_Browser->SetMenu(NULL);
		ListItems items;
		{
			ListItem &list_item = items.Add();
			list_item.SetRenamable(false);
			list_item.SetOriginalIndex(0);
			list_item.AddAttribute(FString("Name"), GetTranslatedFString("PROJECT"));
		}
		{
			ListItem &list_item = items.Add();
			list_item.SetRenamable(false);
			list_item.SetOriginalIndex(0);
			list_item.AddAttribute(FString("Name"), GetTranslatedFString("DATASET"));
		}
		{
			ListItem &list_item = items.Add();
			list_item.SetRenamable(false);
			list_item.SetOriginalIndex(0);
			list_item.AddAttribute(FString("Name"), GetTranslatedFString("AI Solutions"));
		}
		{
			ListItem &list_item = items.Add();
			list_item.SetRenamable(false);
			list_item.SetOriginalIndex(0);
			list_item.AddAttribute(FString("Name"), GetTranslatedFString("Neural Network Libraries - Examples"));
		}

		const FStringList link_collection = settings_.GetLinkCollection();
		for (int i = 0 ; i < link_collection.GetSize() ; i++){
			ListItem &list_item = items.Add();
			list_item.SetRenamable(false);
			list_item.SetOriginalIndex(i);
			FString line = link_collection.GetAt(i);
			list_item.AddAttribute(FString("Name"), GetTranslatedFString(line.SplitBy(",").PChar()));
		}

		Fr_Browser->SetItems(items);
		Fr_Browser->OnItemChanged = BrowserItemChanged;
	}

	//Components
	{
		FListP<ListUIAttribute> attributes;
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Name"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, 0);
			new_attribute.SetPosition(2, Fr_Component->Width - list_col_space);
			new_attribute.SetPosition(3, 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_FOREGROUND);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(16);
		}
		Fr_Component->Ed_ListSearch->Width = 160;
		const int label_visible = Fr_Component->Width > Fr_Component->Ed_ListSearch->Width + 128 + 48;
		Fr_Component->Setup(
			&settings_, label_visible ? GetTranslatedText("Components") : UnicodeString(""),
			1, -1, 24, default_panel_height, false, true, label_visible, attributes);
		Fr_Component->Ed_ListSearch->StyleLookup = "transparentedit";
		Fr_Component->SetColor(
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTER);

		// Fr_Browser->OnItemChanged = DatasetItemChanged;

		Fr_Component->SetMenu(NULL);
		Fr_Component->SetEnterMode(1);
		Fr_Component->OnItemDblClick = ComponentDblClick;
		Fr_Component->OnMouseDown = ComponentMouseDown;
		Fr_Component->OnMouseMove = ComponentMouseMove;
		Fr_Component->OnMouseUp = ComponentMouseUp;
		Fr_Component->OnSearchTextChanged = RefreshLayerComponents;
	}

	//Project and dataset lists
	for (int i = 0 ; i < 5 ; i++) {
		TFr_List *fr = NULL;
		switch (i){
		case 0:
			fr = Fr_ProjectFiles;
			break;
		case 1:
			fr = Fr_DatasetFiles;
			break;
		case 2:
			fr = Fr_DatasetFiles2;
			break;
		case 3:
			fr = Fr_AISolutions;
			break;
		case 4:
			fr = Fr_NNablaExamples;
			break;
		}
		FListP<ListUIAttribute> attributes;
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Name"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, 272 - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_FOREGROUND);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(16);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("DateTime"));
			new_attribute.SetPosition(0, fr->Width > 272 + 272 ? fr->Width - 272 : 272);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, fr->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTDATE);
			new_attribute.SetFontSize(13);
			new_attribute.SetTextAlign(TTextAlign::Trailing);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Description"));
			new_attribute.SetPosition(0, 272);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, fr->Width - 144);
			new_attribute.SetPosition(3, 120 - list_col_space - 20);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTDESC);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(13);
			new_attribute.SetWordWrap(true);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Location"));
			new_attribute.SetPosition(0, 272);
			new_attribute.SetPosition(1, 120 - list_col_space - 20);
			new_attribute.SetPosition(2, fr->Width - list_col_space);
			new_attribute.SetPosition(3, 120 - list_col_space);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTPATH);
			new_attribute.SetFontSize(13);
		}
		int menu_width = 0;
		if (i == 0){
			menu_width = Bu_OpenProject->Position->X + Bu_OpenProject->Width;
		}else {
			menu_width = Bu_OpenDataset->Position->X + Bu_OpenDataset->Width;
		}
		fr->Setup(&settings_, "", 1, -1, 120, default_panel_height, true,
			(i < 3 || i > 4) && fr->Width - menu_width > fr->Ed_ListSearch->Width + fr->Bu_Menu->Width + 48,
			true, attributes);
		fr->SetMenu((i < 3 || i > 4) && fr->Width - menu_width > fr->Bu_Menu->Width ? PM_List : NULL);
		fr->SetColor(
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_LIGHT);
		fr->OnSearchTextChanged = i == 0 ? RefreshProjectFiles : RefreshDatasetFiles;
	}
	Fr_ProjectFiles->OnItemChanged = ProjectsItemChanged;
	Fr_ProjectFiles->OnItemClick = ProjectsItemClick;
	Fr_DatasetFiles->OnItemChanged = DatasetsItemChanged;
	Fr_DatasetFiles->OnItemClick = DatasetsItemChanged;
	Fr_AISolutions->OnItemClick = AISolutionsItemClick;
	Fr_NNablaExamples->OnItemClick = NNablaExamplesItemClick;

	//Dataset Tab
	{
		const int dataset_list_height = 120;
		FListP<ListUIAttribute> attributes;
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Name"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_DatasetList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_FOREGROUND);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(16);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Description"));
			new_attribute.SetPosition(0, 16);
			new_attribute.SetPosition(1, list_col_space + 32);
			new_attribute.SetPosition(2, Fr_DatasetList->Width - list_col_space);
			new_attribute.SetPosition(3, dataset_list_height - list_col_space);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTDESC);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(13);
			new_attribute.SetWordWrap(true);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Main"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_DatasetList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTPARETO);
			new_attribute.SetFontSize(13);
			new_attribute.SetTextAlign(TTextAlign::Trailing);
		}
		Fr_DatasetList->Setup(&settings_, GetTranslatedText("Datasets"), 1, -1, dataset_list_height, default_panel_height, false, false, true, attributes);
		Fr_DatasetList->OnItemChanged = DatasetItemChanged;
		Fr_DatasetList->OnApplyRenameListItem = DatasetItemRename;
		Fr_DatasetList->OnItemDelete = DatasetItemDelete;
		Fr_DatasetList->SetColor(
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHT);
	}
	Fr_DatasetFiles2->OnItemClick = Datasets2ItemClick;

	//Config Tab
	{
		const int config_item_height = 100;
		FListP<ListUIAttribute> attributes;
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Name"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_ConfigList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_FOREGROUND);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(16);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Description"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space + 32);
			new_attribute.SetPosition(2, Fr_ConfigList->Width - list_col_space);
			new_attribute.SetPosition(3, config_item_height - list_col_space);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTDESC);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(13);
			new_attribute.SetWordWrap(true);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Type"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_ConfigList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTPARETO);
			new_attribute.SetFontSize(13);
			new_attribute.SetTextAlign(TTextAlign::Trailing);
		}
		Fr_ConfigList->Setup(&settings_, GetTranslatedText("Config"), 1, -1, config_item_height, default_panel_height, false, false, true, attributes);
		Fr_ConfigList->OnItemChanged = RefreshConfigTab;
		Fr_ConfigList->OnItemDelete = ConfigItemDelete;
		Fr_ConfigList->OnApplyRenameListItem = ConfigItemRename;
		Fr_ConfigList->SetColor(
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHT);
	}

	//Monitor Tab
	{
		const int result_item_height = 160;
		FListP<ListUIAttribute> attributes;
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Name"));
			new_attribute.SetPosition(0, list_col_space + 32 + list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_ResultsList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 20);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_FOREGROUND);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(16);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Comment"));
			new_attribute.SetPosition(0, list_col_space + 32 + list_col_space);
			new_attribute.SetPosition(1, list_col_space + 20);
			new_attribute.SetPosition(2, Fr_ResultsList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 40);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_HALF);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_LIGHTBRAND);
			new_attribute.SetFontSize(16);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("*PieGraph"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space + 6);
			new_attribute.SetPosition(2, list_col_space + 32);
			new_attribute.SetPosition(3, list_col_space + 38);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_LIGHTBRAND);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Caption"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space + 48);
			new_attribute.SetPosition(2, list_col_space + 120);
			new_attribute.SetPosition(3, result_item_height - list_col_space);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_HALF);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_LIGHTBRAND);
			new_attribute.SetFontSize(13);
			new_attribute.SetWordWrap(true);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Content"));
			new_attribute.SetPosition(0, list_col_space + 120);
			new_attribute.SetPosition(1, list_col_space + 48);
			new_attribute.SetPosition(2, Fr_ResultsList->Width - list_col_space);
			new_attribute.SetPosition(3, result_item_height - list_col_space);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_DARK);
			new_attribute.SetFontColorSelected(SDEEP_CONSOLE_COLOR_BRAND);
			new_attribute.SetFontSize(13);
			new_attribute.SetWordWrap(true);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Error"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_ResultsList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_HALF);
			new_attribute.SetFontSize(13);
			new_attribute.SetTextAlign(TTextAlign::Trailing);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Ongoing"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_ResultsList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTRED);
			new_attribute.SetFontSize(13);
			new_attribute.SetTextAlign(TTextAlign::Trailing);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Pareto"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_ResultsList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTPARETO);
			new_attribute.SetFontSize(13);
			new_attribute.SetTextAlign(TTextAlign::Trailing);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Evaluated"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, result_item_height - list_col_space - 16);
			new_attribute.SetPosition(2, Fr_ResultsList->Width - list_col_space);
			new_attribute.SetPosition(3, result_item_height - list_col_space);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTEVAL);
			new_attribute.SetFontSize(13);
			new_attribute.SetTextAlign(TTextAlign::Trailing);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("*CheckMark"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space + 6);
			new_attribute.SetPosition(2, list_col_space + 32);
			new_attribute.SetPosition(3, list_col_space + 38);
			// new_attribute.SetFontColors(0xff000000 | FadeColor(SDEEP_CONSOLE_COLOR_BRAND, SDEEP_CONSOLE_COLOR_LIGHTBRAND, 0.5));
			new_attribute.SetFontColors(0x80ffffff & SDEEP_CONSOLE_COLOR_LIGHTEST);
		}
		{
			ListUIAttribute &new_attribute = attributes.Add();
			new_attribute.SetAttributeName(FString("Scheduled"));
			new_attribute.SetPosition(0, list_col_space);
			new_attribute.SetPosition(1, list_col_space);
			new_attribute.SetPosition(2, Fr_ResultsList->Width - list_col_space);
			new_attribute.SetPosition(3, list_col_space + 24);
			new_attribute.SetFontColors(SDEEP_CONSOLE_COLOR_TEXTSTAT);
			new_attribute.SetFontSize(13);
			new_attribute.SetTextAlign(TTextAlign::Trailing);
		}
		Fr_ResultsList->Setup(&settings_, GetTranslatedText("Results History"), 1, -1, result_item_height, default_panel_height, true, false, true, attributes);
		Fr_ResultsList->SetMenu(PM_ListResults);
		Fr_ResultsList->OnApplyRenameListItem = TrainingResultRename;
		Fr_ResultsList->OnItemDblClick = ResultsListDblClick;
		Fr_ResultsList->OnItemDelete = ResultsListDelete;
		Fr_ResultsList->SetColor(
			SDEEP_CONSOLE_COLOR_LIGHTER,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHTEST,
			SDEEP_CONSOLE_COLOR_LIGHT);
	}
}
//---------------------------------------------------------------------------
int TFo_Main::Log(FString args, int force_send)
{
	if ((settings_.GetLogLevel() > 0) || force_send) {
		AnsiString log_file_name = ExtractFilePath(ParamStr(0)) + AnsiString("settings\\log\\") + FormatDateTime("yyyymmdd", Now()) + ".log";
		if (!DirectoryExists(ExtractFileDir(log_file_name))){
			ForceDirectories(ExtractFileDir(log_file_name));
		}
		FILE *F = fopen(log_file_name.c_str(), "at");
		if (F){
			fprintf(F, "%s%s\n", AnsiString(FormatDateTime("[hh:nn:ss] ", Now())).c_str(), args.PChar());
			fclose(F);
			F = NULL;
		}
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
int TFo_Main::LogTraceback(TStrings *str)
{
	if (settings_.GetLogLevel() >= 3) {
		UnicodeString s = StringReplace(str->GetText(), "\r", "", TReplaceFlags() << rfReplaceAll);
		const int p = s.Pos("Traceback");
		if (s > 0){
			AnsiString log_file_name = ExtractFilePath(ParamStr(0)) + AnsiString("settings\\log\\") + FormatDateTime("yyyymm", Now()) + ".txt";
			if (!DirectoryExists(ExtractFileDir(log_file_name))){
				ForceDirectories(ExtractFileDir(log_file_name));
			}
			FILE *F = fopen(log_file_name.c_str(), "at");
			if (F){
				fprintf(F, "%s\n", AnsiString(s.SubString(p, s.Length() - p)).c_str());
				fclose(F);
				F = NULL;
			}
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshLayerComponents()
{
	RefreshLayerComponentTree(nn_document_.GetSDeepEngineType());
}
//---------------------------------------------------------------------------
int TFo_Main::RefreshLayerComponentTree(SDeepEngineType sdeep_engine_type)
{
	int result = false;

	ListItems items;

	int expanded = true;//Todo

	nn_layer_component_items_.Clear();
	FString last_category;
	int match_category = false;
	int num_component_in_category = 1;
	int unit_layer_component_index = -1;
	for (int ic = 0 ; ic < settings_.GetEngineInfo().GetAt((int)sdeep_engine_type).GetComponents().GetSize() ; ic++) {
		const NNLayerComponent &component = settings_.GetEngineInfo().GetAt((int)sdeep_engine_type).GetComponents().GetAt(ic);
		if (last_category != component.GetCategory()) {
			last_category = component.GetCategory();
			if (component.GetType() == "Unit") {
				unit_layer_component_index = ic;
			}

			if (!num_component_in_category && items.GetSize()){
				//No component in last category
				items.Delete(items.GetSize() - 1);
			}

			//Create new category
			ListItem &list_item = items.Add();
			list_item.SetRenamable(false);
			list_item.SetOriginalIndex(-1);
			list_item.AddAttribute(FString("Name"), FString("* ") + component.GetCategory());
			//expanded = settings_.GetEngineInfo().GetAt((int)sdeep_engine_type).GetComponents().GetFoldCategory().IndexOf(component.GetCategory()) < 0;

			num_component_in_category = 0;
			match_category = Fr_Component->Ed_ListSearch->Text != "" && component.GetCategory().ToLower().Pos(AnsiString(Fr_Component->Ed_ListSearch->Text).LowerCase().c_str()) > 0;
		}

		if (!component.GetHidden() && expanded &&
			(match_category ||
			 Fr_Component->Ed_ListSearch->Text == "" ||
			 (component.GetType().ToLower().Pos(AnsiString(Fr_Component->Ed_ListSearch->Text).LowerCase().c_str()) > 0))) {
			//Create new layer_component
			ListItem &list_item = items.Add();
			list_item.SetRenamable(false);
			list_item.SetOriginalIndex(ic);
			list_item.AddAttribute(FString("Name"), FString("**  ") + component.GetType());
			list_item.AddAttribute(FString("Index"), FString(ic));
			num_component_in_category++;
		}
		result |= true;
	}
	if (!num_component_in_category && items.GetSize()){
		//No component in last category
		items.Delete(items.GetSize() - 1);
	}


	const FStringList &custom_units = settings_.GetCustomUnits();
	if (custom_units.GetSize() && unit_layer_component_index >= 0) {
		int num_custom_unit = 0;
		match_category = Fr_Component->Ed_ListSearch->Text != "" && FString("Custom Unit").ToLower().Pos(AnsiString(Fr_Component->Ed_ListSearch->Text).LowerCase().c_str()) > 0;

		for (int i = 0 ; i < custom_units.GetSize() ; i++) {
			if (match_category ||
				Fr_Component->Ed_ListSearch->Text == "" ||
				(custom_units.GetAt(i).ToLower().Pos(AnsiString(Fr_Component->Ed_ListSearch->Text).LowerCase().c_str()) > 0)) {
				if (num_custom_unit++ == 0) {
					ListItem &list_item = items.Add();
					list_item.SetRenamable(false);
					list_item.SetOriginalIndex(-1);
					list_item.AddAttribute(FString("Name"), FString("* Custom Unit"));
				}
				ListItem &list_item = items.Add();
				list_item.SetRenamable(false);
				list_item.SetOriginalIndex(unit_layer_component_index);
				list_item.AddAttribute(FString("Name"), FString("**  ") + custom_units.GetAt(i));
				list_item.AddAttribute(FString("Index"), FString(unit_layer_component_index));
				list_item.AddAttribute(FString("UIndex"), FString(i));
            }
		}
	}

	Fr_Component->SetItems(items);
	//Fr_Component->SetWheelEnabled(false);

	return result;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_SaveClick(TObject *Sender)
{
	if (nn_document_.GetLocked()){
		return;
	}
	Save();
}
//---------------------------------------------------------------------------
void TFo_Main::NewProject(SDeepEngineType sdeep_engine_type, bool need_refresh_component)
{
	nn_document_.Clear(sdeep_engine_type);
	nn_graph_mapper_.Clear();

	network_tab_names_.Clear();
	network_tab_rect_.Clear();
	network_tab_delete_rect_.Clear();

	network_tab_index_2_ = -1;
	network_tab_delete_focus_ = false;
	drag_network_tab_ = false;
	drag_network_tab_start_x_ = 0;
	drag_network_tab_x_shift_ = 0;

	network_focuse_index_ = 0;
	component_drag_mode_ = ComponentDragMode::None;
	network_left_ = 0.0;
	network_current_left_ = 0.0;
	network_top_ = 0.0;
	network_current_top_ = 0.0;
	network_zoom_ = 0;
	network_current_zoom_ = 0.0;
	network_mouse_down_ = false;

	network_mouse_down_mode_ = NetworkMouseDownMode::None;
	network_start_x_ = 0.0f;
	network_start_y_ = 0.0f;
	network_last_x_ = 0.0f;
	network_last_y_ = 0.0f;
    last_layer_id_ = -1;

	property_top_ = 0.0f;
	property_current_top_ = 0.0f;
    property_current_pos_ = -1;
	visible_property_num_ = 0;
	current_property_index_ = -1;
	current_property_layer_id_ = -1;

	monitor_zoom_ = 0;

	result_tradeoff_zoom_ = 0;
	result_last_mouse_down_ = false;
	result_last_mouse_x_ = 0.0f;
	result_last_mouse_y_ = 0.0f;
	network_overview_structure_index_ = 0;
	network_overview_update_count_ = 0;
	network_overview_zoom_ = 1.0;
	network_overview_current_zoom_ = 1.0;
	result_weight_zoom_ = 4;
	result_weight_current_zoom_ = 0.0;
	result_weight_top_ = 0.0;
	result_weight_current_top_ = 0.0;

	monitor_performance_last_x_ = 0.0f;
	monitor_performance_last_y_ = 0.0f;

	structure_search_last_gaussian_process_beta_ = 4.0;
	structure_search_last_gaussian_process_gaussian_kernel_sigma_ = 10.0;
	structure_search_candidate_index_ = -1;

	table_ptrs_.Clear();
	table_ui_state_.Clear();

	memset(monitor_update_count_, 0, sizeof(monitor_update_count_));
	memset(monitor_value_range_, 0, sizeof(monitor_value_range_));
	memset(monitor_current_value_range_, 0, sizeof(monitor_current_value_range_));

	memset(result_tradeoff_value_range_, 0, sizeof(result_tradeoff_value_range_));
	result_tradeoff_value_range_[0] = 1.0;
	result_tradeoff_value_range_[1] = 1.0;
	memset(result_current_tradeoff_value_range_, 0, sizeof(result_current_tradeoff_value_range_));
	result_current_tradeoff_value_range_[0] = 1.0;
	result_current_tradeoff_value_range_[1] = 1.0;

	structure_search_candidate_myu_.Clear();
	structure_search_candidate_sigma_.Clear();
	structure_search_candidate_cost_.Clear();
	structure_search_candidate_gain_.Clear();

	ClearEditBox();
	Fr_DatasetList->SetItemIndex(0);
	ResetMonitor();
	ResetEvaluationResult();

	if (need_refresh_component) {
		RefreshLayerComponentTree(sdeep_engine_type);
		RefreshRunMenu();
	}
	Me_Debug->Lines->Clear();

	Lo_MonitorBottom->Align = TAlignLayout::Bottom;
	Lo_MonitorBottom->Height = 229;
	RB_MonitorLog->IsChecked = true;
	Sp_TrainingMonitorBottom->Position->Y = Lo_MonitorBottom->Position->Y - Sp_TrainingMonitorBottom->Height;

	RB_EvaluationOutputResult->IsChecked = true;
	RB_ResultsLearningCurve->IsChecked = true;

	Fr_ResultsList->ResetScroll();

	network_overview_ = &nn_document_;
}
//---------------------------------------------------------------------------
UnicodeString TFo_Main::GetResultDir(int result_index)
{
	return
		UnicodeString(nn_document_.GetResultPath(FString("\\")).PChar()) +
		UnicodeString(nn_document_.GetResultName(result_index).PChar());
}
//---------------------------------------------------------------------------
int TFo_Main::OpenProject(UnicodeString file_name, int preset_project)
{
	if (!FileExists(file_name)) {
		Activate();
		ShowMessage(GetTranslatedText("Error : Project file is not found.") + UnicodeString("\n\"") + file_name + UnicodeString("\""));
		return false;
	}

	Bu_NetworkTabClick(this);

	if (!preset_project) {
		Log(FString("open project"));
	}

	ShowAnimationIndicator(GetTranslatedText("Loading project..."));
	nn_graph_mapper_.Clear();
	//unsigned int time = timeGetTime();

	//nn_document_.LoadFromFile(FString(AnsiString(file_name).c_str()));
	//*
	nn_document_.Lock();
    Ti_AnimationTimer(this);
	TThread::CreateAnonymousThread(
		new TDocumentIOFunc(&nn_document_, &settings_, file_name, DocumentIOType::Open, 0, online_debug_mode_)
		)->Start();
	WaitForDocumentUnlock();
	//*/

	if (!preset_project) {
		RegisterProject(file_name);
	}

	HideAnimationIndicator();

	CheckDatasetRequirement();
	//ShowMessage(IntToStr((int)(timeGetTime() - time)));
    UnicodeString path = ExtractFilePath(ParamStr(0));
	if (nn_document_.GetIsSample() && file_name.Pos(path) > 0){
		Log(FString("sample project : name=") + FString(AnsiString(StringReplace(file_name, path, "", TReplaceFlags() << rfReplaceAll)).c_str()));
	}
	if (preset_project && file_name.Pos(path) > 0){
		Log(FString("preset project : name=") + FString(AnsiString(StringReplace(file_name, path, "", TReplaceFlags() << rfReplaceAll)).c_str()));
	}

    RefreshRunMenu();

	InstallPackages();

	return true;
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshRunMenu()
{
	Co_Run->Items->Clear();

	SDeepEngineType sdeep_engine_type = nn_document_.GetSDeepEngineType();
	if (sdeep_engine_type >= SDeepEngineType::NumEngineType) {
		return;
	}
	const NNScripts &scripts = settings_.GetEngineInfo().GetAt((int)sdeep_engine_type).GetScripts();

	for (int i = 0 ; i < scripts.GetSize() ; i++) {
        Co_Run->Items->Add(scripts.GetAt(i).GetName().PChar());
	}
	if (Co_Run->Items->Count > 0) {
		Co_Run->ItemIndex = 0;
	}
}
//---------------------------------------------------------------------------
int TFo_Main::Save()
{
	if (nn_document_.GetFileName() != "") {
		Log(FString("save project"));

		ShowAnimationIndicator(GetTranslatedText("Saving project..."));
		animation_indicator_start_time_ -= 1000;
		nn_document_.Lock();
	    Ti_AnimationTimer(this);
		CheckAnimationIndicator();

		TThread::CreateAnonymousThread(
			new TDocumentIOFunc(&nn_document_, &settings_, nn_document_.GetFileName().PChar(), DocumentIOType::Save, 0, online_debug_mode_)
			)->Start();
		WaitForDocumentUnlock();

		RegisterProject(nn_document_.GetFileName().PChar());

		HideAnimationIndicator();
		return true;
	}
	return SaveAs();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_SaveAsClick(TObject *Sender)
{
	if (nn_document_.GetLocked()){
		return;
	}
	SaveAs();
}
//---------------------------------------------------------------------------
int TFo_Main::SaveAs()
{
	SD_Project->FileName = nn_document_.GetFileName().PChar();
	if (SD_Project->Execute()) {
		Bu_NetworkTabClick(this);//To hide results and evaluation tab
		ResetMonitor();
		ResetEvaluationResult();

		Log(FString("save new project"));

		ShowAnimationIndicator(GetTranslatedText("Saving project..."));
		nn_document_.Lock();
        Ti_AnimationTimer(this);
		TThread::CreateAnonymousThread(
			new TDocumentIOFunc(&nn_document_, &settings_, SD_Project->FileName, DocumentIOType::Save, 0, online_debug_mode_)
			)->Start();
		WaitForDocumentUnlock();

		RegisterProject(SD_Project->FileName);

		HideAnimationIndicator();
		nn_document_.RefreshResults(FString("\\"), true);
		TC_DefaultChange(this);

		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
int TFo_Main::ApplyProperty()
{
	if (Ed_NetworkName->Visible) {
		if (nn_document_.GetActiveStructureIndex() == Ed_NetworkName->Tag) {
			nn_document_.PrepareUndoInAdvance();
			int result = nn_document_.SetName(FString(AnsiString(Ed_NetworkName->Text).c_str()));
			if (result) {
				nn_document_.KeepPreparedForUndo(FString("Change network name"));
			}
			return result;
		}
	}

	if (nn_document_.GetCurrentLayerID() != current_property_layer_id_) {
		return false;
	}

	if (nn_document_.GetCurrentPropertyIndex() != current_property_index_) {
		return false;
	}

	if (Lo_EditProperty->Visible || (Co_Property->Visible && Co_Property->Selected)) {
		const NNLayer *layer = nn_document_.GetLayerByID(current_property_layer_id_);
		if (layer) {
			const NNLayerProperty &property = layer->GetProperty(current_property_index_);
			FString undo_name = FString("Edit property : ") + layer->GetType() + FString("->") + property.GetName();

			if (nn_document_.GetUndoName(0) != undo_name) {
				nn_document_.KeepForUndo(undo_name);
			}

			int applied = false;
			int result = true;
			for (int il = 0 ; il < nn_document_.GetNumLayer() ; il++) {
				const NNLayer &layer2 = nn_document_.GetLayer(il);
				const int property_index = layer2.GetPropertyIndexByName(property.GetName());
				if (Lo_EditProperty->Visible && layer2.GetSelected() && property_index >= 0) {
					applied = true;
					result &= nn_document_.SetLayerPropertyValue(layer2.GetLayerID(), property_index, FString(AnsiString(Ed_Property->Text).c_str()));
				}
				if (Co_Property->Visible && Co_Property->Selected && layer2.GetSelected() && property_index >= 0) {
					applied = true;
					result &= nn_document_.SetLayerPropertyValue(layer2.GetLayerID(), property_index, FString(AnsiString(Co_Property->Selected->Text).c_str()));
				}
			}
			return applied && result;
		}
	}

	return false;
}
//---------------------------------------------------------------------------
void TFo_Main::FreeSDeepTrainingThread()
{
	//Terminate and delete all training threads
	for (int i = sdeep_training_thread_.GetSize() - 1 ; i >= 0  ; i--) {
		SDeepTrainingThread *thread = sdeep_training_thread_.GetAt(i);
		thread->Terminate();
		thread->WaitFor();
		delete thread;
		sdeep_training_thread_.Delete(i);
	}
}
//---------------------------------------------------------------------------
void TFo_Main::FreeSDeepEvaluationThread()
{
	if (sdeep_evaluation_thread_) {
		sdeep_evaluation_thread_->Terminate();
		sdeep_evaluation_thread_->WaitFor();
		delete sdeep_evaluation_thread_;
		sdeep_evaluation_thread_ = NULL;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::FormClose(TObject *Sender, TCloseAction &Action)
{
	animation_indicator_abort_ = true;
	closed_ = true;
}
//---------------------------------------------------------------------------
void SetControlVisible(TControl *control, bool visible)
{
	if (control->Visible != visible) {
		control->Visible = visible;
	}
}
//---------------------------------------------------------------------------
void SetControlEnabled(TControl *control, bool enabled)
{
	if (control->Enabled != enabled) {
		control->Enabled = enabled;
	}
}
//---------------------------------------------------------------------------
void SetMenuText(TMenuItem *menu_item, UnicodeString text)
{
	if (menu_item->Text != text) {
		menu_item->Text = text;
	}
}
//---------------------------------------------------------------------------
void SetRadioButtonChecked(TRadioButton *radio_button, bool checked, bool force = false)
{
	if (!radio_button->IsFocused || force) {
		if (radio_button->IsChecked != checked) {
			radio_button->IsChecked = checked;
		}
	}
}
//---------------------------------------------------------------------------
void SetCheckBoxChecked(TCheckBox *check_box, bool checked, bool force = false)
{
	if (!check_box->IsFocused || force) {
		if (check_box->IsChecked != checked) {
			check_box->IsChecked = checked;
		}
	}
}
//---------------------------------------------------------------------------
void SetEditBoxText(TEdit *edit_box, FString text)
{
	if (!edit_box->IsFocused){
		if (edit_box->Text != text.PChar()) {
			edit_box->Text = text.PChar();
		}
	}
}
//---------------------------------------------------------------------------
void SetEditBoxTextInt(TEdit *edit_box, int value)
{
	if (!edit_box->IsFocused){
		UnicodeString text = IntToStr(value);
		if (edit_box->Text != text) {
			edit_box->Text = text;
		}
	}
}
//---------------------------------------------------------------------------
void SetEditBoxTextDouble(TEdit *edit_box, double value)
{
	if (!edit_box->IsFocused){
		UnicodeString text = FloatToStr(value);
		if (edit_box->Text != text) {
			edit_box->Text = text;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ti_AnimationTimer(TObject *Sender)
{
	if (auth_failed) {
		Close();
		return;
	}

	const int is_nnabla = nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla;
	const int is_trainable = (nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla) ||
							 (nn_document_.GetSDeepEngineType() == SDeepEngineType::Torch);

	CheckSplashWindow();

	//Animation Indicator
	CheckAnimationIndicator();

	//GUI Enabled
	{
		int gui_enabled = !nn_document_.GetLocked();
		if (TC_Default->Enabled != gui_enabled) {
			TC_Default->Enabled = gui_enabled;
			//Re_TopBar->Enabled = gui_enabled;
			TC_Left->Enabled = gui_enabled;
			Lo_Right->Enabled = gui_enabled;
			Lo_RightSub->Enabled = gui_enabled;
		}
	}

	//Top Button (Hint)
	int hint_visible = false;
	UnicodeString hint_text;
	int hint_left = 0;
	int hint_top = 0;
	for (int ib = 0 ; ib < controls_.GetSize() ; ib++) {
		if (controls_.GetAt(ib)->IsMouseOver) {
			TPointF screen_pos = controls_.GetAt(ib)->LocalToAbsolute(PointF(0, controls_.GetAt(ib)->Height));
			TPointF pos = FormLayout->AbsoluteToLocal(screen_pos);
			hint_left = pos.x;
			hint_top = pos.y < Height - Pa_Hint->Height - 8 ? pos.y : Height - Pa_Hint->Height - 8;
			hint_text = control_hints_.GetAt(ib).PChar();
			hint_visible = true;
		}

	}
	if (hint_visible && (La_Hint->Text != hint_text || Pa_Hint->Width != La_Hint->Width + La_Hint->Margins->Left * 2)) {
		Pa_Hint->Visible = true;
		Pa_Hint->BringToFront();
		La_Hint->Text = hint_text;
		La_Hint->AutoSize = true;
		La_Hint->Position->X = La_Hint->Margins->Left;
		Pa_Hint->Width = La_Hint->Width + La_Hint->Margins->Left * 2;
		Pa_Hint->Position->X = hint_left < FormLayout->Width - Pa_Hint->Width - 8 ? hint_left : FormLayout->Width - Pa_Hint->Width - 8;
		Pa_Hint->CalloutOffset = 13 + (hint_left - Pa_Hint->Position->X);
		Pa_Hint->Position->Y = hint_top;
	}
	if ((Pa_Hint->Visible && !FA_Hint->Inverse) != hint_visible) {
		if (hint_visible) {
			FA_Hint->Inverse = false;
			Pa_Hint->Visible = true;
		}else {
			FA_Hint->Inverse = true;
			FA_Hint->Start();
		}
	}

	if (nn_document_.GetLocked()) {
		//Exit before access to nn_document_
		return;
	}

	if (Im_CursorForCapture->Visible != screen_cursor_on_) {
		Im_CursorForCapture->Visible = screen_cursor_on_;
	}
	if (screen_cursor_on_) {
		TPointF screen_pos = ScreenToClient(Screen->MousePos());
		Im_CursorForCapture->Position->X = screen_pos.x - 6;
		Im_CursorForCapture->Position->Y = screen_pos.y - 6;
	}

	const unsigned int now = timeGetTime();

	//Caption
	UnicodeString caption = PRODUCT_NAME;
	if (activity_ != SDeepConsoleActivity::Browser) {
		switch (nn_document_.GetSDeepEngineType()) {
		case SDeepEngineType::NNabla:
			caption += " (NNabla)";
			break;
		case SDeepEngineType::Transformers:
			caption += " (Transformers)";
			break;
		case SDeepEngineType::Torch:
			caption += " (Torch)";
            break;
		default:
			break;
		}

		if (nn_document_.GetFileName() != "") {
			caption = ExtractFileName(UnicodeString(nn_document_.GetFileName().PChar())) + UnicodeString(" - ") + caption;
		}
	}
	if (Caption != caption) {
		Caption = caption;
	}

	//Tab
	SetControlVisible(Re_Home, activity_ == SDeepConsoleActivity::Browser);
	SetControlVisible(Bu_NetworkTab, activity_ == SDeepConsoleActivity::Default);
	SetControlVisible(Re_NetworkTab, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Network);
	SetControlVisible(Bu_DatasetTab, activity_ == SDeepConsoleActivity::Default);
	SetControlVisible(Re_DatasetTab, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Dataset);
	SetControlVisible(Bu_ConfigTab, activity_ == SDeepConsoleActivity::Default);
	SetControlVisible(Re_ConfigTab, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Config);
	SetControlVisible(Bu_TrainingTab, activity_ == SDeepConsoleActivity::Default);
	SetControlVisible(Re_TrainingTab, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Results);
	SetControlVisible(Bu_EvaluationTab, activity_ == SDeepConsoleActivity::Default);
	SetControlVisible(Re_EvaluationTab, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Evaluation);


	//Menu Button
	SetControlEnabled(Bu_Back, !network_mouse_down_);
	SetControlVisible(Bu_Save, activity_ == SDeepConsoleActivity::Default);
	SetControlEnabled(Bu_Save, !network_mouse_down_);
	SetControlVisible(Bu_SaveAs, activity_ == SDeepConsoleActivity::Default);
	SetControlEnabled(Bu_SaveAs, !network_mouse_down_ && !GetExecuting());

	SetControlVisible(Bu_Undo, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Network);
	SetControlEnabled(Bu_Undo, nn_document_.GetNumUndo() > 0 && !network_mouse_down_);
	SetControlEnabled(MIN_Undo, Bu_Undo->Enabled);
	UnicodeString undo_caption = GetTranslatedText("Undo");
	if (MIN_Undo->Enabled) {
		undo_caption += UnicodeString(" - ") + UnicodeString(nn_document_.GetUndoName(0).PChar());
	}
	SetMenuText(MIN_Undo, undo_caption);

	SetControlVisible(Bu_Redo, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Network);
	SetControlEnabled(Bu_Redo, nn_document_.GetNumRedo() > 0 && !network_mouse_down_);
	SetControlEnabled(MIN_Redo, Bu_Redo->Enabled);
	UnicodeString redo_caption = GetTranslatedText("Redo");
	if (MIN_Redo->Enabled) {
		redo_caption += UnicodeString("  - ") + UnicodeString(nn_document_.GetRedoName(0).PChar());
	}
	SetMenuText(MIN_Redo, redo_caption);

	SetControlVisible(Bu_Cut, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Network);
	SetControlEnabled(Bu_Cut, nn_document_.GetNumSelectedLayer() > 0 && !network_mouse_down_);
	SetControlEnabled(MIN_Cut, Bu_Cut->Enabled);

	SetControlVisible(Bu_Copy, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Network);
	SetControlEnabled(Bu_Copy, Bu_Cut->Enabled && !network_mouse_down_);
	SetControlEnabled(MIN_Copy, Bu_Copy->Enabled);

	SetControlVisible(Bu_Paste, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Network);
	if (TPlatformServices::Current->SupportsPlatformService(__uuidof(IFMXExtendedClipboardService))) {
		/*
		int paste_enabled = false;
		_di_IFMXClipboardService ClipboardService = TPlatformServices::Current->GetPlatformService(__uuidof(IFMXClipboardService));
		UnicodeString s;
		try {
			s = ClipboardService->GetClipboard().AsString();//Memory leak
			paste_enabled = s.SubString(1, 16) == "[Network_Global]";
		} catch (...) {
		}
		if (!paste_enabled) {
			if (Bu_Paste->Tag > 0) {
				Bu_Paste->Tag = Bu_Paste->Tag - 1;
				paste_enabled = Bu_Paste->Tag > 0;
			}
		}else {
			Bu_Paste->Tag = 3;
		}
		*/
		/*
		TMemo *memo = new TMemo(this);
		memo->Lines->BeginUpdate();
		//memo->PasteFromClipboard();//Memory leak
		memo->Lines->EndUpdate();
		int paste_enabled = false;
		if (memo->Lines->Count) {
			paste_enabled = memo->Lines->Strings[0] == "[Network_Global]";
		}
		delete memo;
		*/
		/*
		int paste_enabled = false;
		_di_IFMXClipboardService ClipboardService = TPlatformServices::Current->GetPlatformService(__uuidof(IFMXClipboardService));
		UnicodeString s;
		if (ClipboardService->GetClipboard().TryAsType(s)) {//Memory leak
			paste_enabled = s.SubString(1, 16) == "[Network_Global]";//ClipboardService->GetClipboard().AsString()
			if (!paste_enabled) {
				if (Bu_Paste->Tag > 0) {
					Bu_Paste->Tag = Bu_Paste->Tag - 1;
					paste_enabled = Bu_Paste->Tag > 0;
				}
			}else {
				Bu_Paste->Tag = 3;
			}
		}
		*/
		_di_IFMXExtendedClipboardService ClipboardService = TPlatformServices::Current->GetPlatformService(__uuidof(IFMXExtendedClipboardService));
		int paste_enabled = ClipboardService->HasText();
		SetControlEnabled(Bu_Paste, paste_enabled && !network_mouse_down_);
		SetControlEnabled(MIN_Paste, Bu_Paste->Enabled);
	}
	SetControlVisible(Bu_Unitize, activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Network);
	SetControlEnabled(Bu_Unitize, nn_document_.CanUnitize() && !network_mouse_down_);

	SetControlEnabled(MIN_Delete, (nn_document_.GetNumSelectedLayer() > 0 || nn_document_.GetCurrentLinkID() >= 0) && !network_mouse_down_);

	SetControlEnabled(MIN_SelectAll, !network_mouse_down_);
	SetControlEnabled(MIN_InverseSelection, !network_mouse_down_);
	SetControlEnabled(MIN_ClearSelection, Bu_Cut->Enabled && !network_mouse_down_);


	SetControlVisible(MIN_Import, is_trainable);
	SetControlVisible(MIN_Export, is_nnabla);

	SetControlVisible(MIN_ImportCaffePrototxt, is_nnabla);
	SetControlVisible(MIN_ImportNnpNntxt, is_nnabla);
	SetControlVisible(MIN_ImportNnpNntxtasUnit, is_nnabla);

	SetControlVisible(MIN_ImportNnpt, !is_nnabla);
	SetControlVisible(MIN_ImportNnptasUnit, !is_nnabla);


	SetControlVisible(MINE_PythonCodeNNabla, is_nnabla);
	SetControlVisible(MINE_Sep0, is_nnabla);
	SetControlVisible(MINE_pptx, is_nnabla);
	SetControlVisible(MINE_html, is_nnabla);

	{
		UnicodeString zoom_str = IntToStr((int)(pow(2.0, network_zoom_ * 0.5) * 100)) + "%";
		if (La_NetworkZoom->Text != zoom_str) {
			La_NetworkZoom->Text = zoom_str;
		}
	}

	SetControlVisible(La_TypeConfig, is_trainable);
	SetControlVisible(Co_TypeConfig, is_trainable);
	SetControlEnabled(Co_TypeConfig, is_trainable);
	SetControlVisible(La_MonitorInterval, is_trainable);
	SetControlVisible(Ed_MonitorInterval, is_trainable);
	SetControlVisible(La_MonitorIntervalUnit, is_trainable);

	SetControlVisible(La_UpdateInterval, is_trainable);
	SetControlVisible(Ed_UpdateInterval, is_trainable);
	SetControlVisible(La_UpdateInterval2, is_trainable);

	SetControlVisible(La_OptimizerRange, is_trainable);
	SetControlVisible(La_OptimizerRangeFrom, is_trainable);
	SetControlVisible(La_OptimizerRangeTo, is_trainable);
	SetControlVisible(Ed_OptimizerRangeFrom, is_trainable);
	SetControlVisible(Ed_OptimizerRangeTo, is_trainable);
	SetControlVisible(Co_OptimizerRangeUnit, is_trainable);
	SetControlVisible(La_WeightDecay, is_trainable);
	SetControlVisible(Ed_WeightDecay, is_trainable);
	SetControlVisible(La_LearningRateScheduler, is_trainable);
	SetControlVisible(Co_LearningRateScheduler, is_trainable);

	SetControlVisible(La_LRUpdateInterval, is_trainable);
	SetControlVisible(Ed_LRUpdateInterval, is_trainable);
	SetControlVisible(Co_LRUpdateIntervalUnit, is_trainable);
	SetControlVisible(Ch_LearningRateWarmup, is_trainable);
	SetControlVisible(Ed_LearningRateWarmupLength, is_trainable);
	SetControlVisible(Co_LearningRateWarmupLengthUnit, is_trainable);

	//Control button
	control_button_type_ = ControlButtonType::VisibleFalse;
	int evaluate_visible = false;
	if (!TC_Default->Visible || nn_document_.GetLocked()){
		//Controller is not available
	}else {
		switch (nn_document_.GetSDeepEngineType()) {
		case SDeepEngineType::NNabla:
        case SDeepEngineType::Torch:
			if (TC_Default->ActiveTab == TI_Network) {
				//EDIT Tab
				if (Co_Training->ItemIndex != 2){// != "Create Training Result"
					//Train, Profile, Plugin
					if (Co_Instance->ItemIndex >= 0 && Co_Instance->ItemIndex != 0){// != "Local Processor"
						//Remote
						if (Co_Instance->ItemIndex < instance_available_.GetSize() && instance_available_.GetAt(Co_Instance->ItemIndex)){
							//Available
							control_button_type_ = ControlButtonType::RunTraining;
						}else {
							//Not Available
							control_button_type_ = ControlButtonType::RunDisabledWarning;
						}
					}else {
						//Local
						if (!GetTraining(true)){
							//No local traninig jobs
							control_button_type_ = ControlButtonType::RunTraining;
						}else {
							//Local training job already exists
							control_button_type_ = ControlButtonType::ScheduleTraining;
						}
					}
				}else {
					//Create Training Result
					if (!GetTraining(true)){
						//No local traninig jobs
						control_button_type_ = ControlButtonType::RunTraining;
					}else {
						//Local training job already exists
					}
				}
			}else if ((TC_Default->ActiveTab == TI_Results || TC_Default->ActiveTab == TI_Evaluation) && TC_Left->ActiveTab == TI_TrainingLeft){
				//TRAINING / Evaluation Tab
				if (Fr_ResultsList->GetItemIndex() >= 0 &&
					Fr_ResultsList->GetItemIndex() < nn_document_.GetResults().GetSize()) {
					//Job selected
					const FString &job_name = nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetDir();
					if (GetTraining(job_name)){
						//Training
						control_button_type_ = ControlButtonType::StopTraining;
					}else if (GetEvaluating(job_name)){
						//Evaluating
						control_button_type_ = ControlButtonType::StopEvaluation;
					}else if (TC_Default->ActiveTab == TI_Results){
						evaluate_visible = true;
						//Not executing
						if (nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetComplete()){
							//Enable to run evaluation
							control_button_type_ = ControlButtonType::RunEvaluation;
						}else {
							//Cannot run evaluation (not completed)
							control_button_type_ = ControlButtonType::RunDisabled;
						}
					}
				}else {
					//There is no selected job
					if (GetExecuting()){
						//There is executing job
						control_button_type_ = ControlButtonType::StopAll;
					}else {
						//No executing jobs
					}
				}
			}
			break;
		default:
			if (GetExecuting()){
				control_button_type_ = ControlButtonType::Stop;
			}else {
				control_button_type_ = ControlButtonType::Run;
			}
		}
	}

	//Contorol Button View
	switch (control_button_type_){
	case ControlButtonType::RunTraining:
		AddHint(Bu_Control, FString("Run training (F5)"));
		break;
	case ControlButtonType::ScheduleTraining:
		AddHint(Bu_Control, FString("Schedule training (F5)"));
		break;
	case ControlButtonType::StopTraining:
		AddHint(Bu_Control, FString("Stop training (Shift+F5)"));
		break;
	case ControlButtonType::RunEvaluation:
		AddHint(Bu_Control, FString("Run evaluation (F6)"));
		break;
	case ControlButtonType::StopEvaluation:
		AddHint(Bu_Control, FString("Stop evaluation (Shift+F6)"));
		break;
	default:
		AddHint(Bu_Control, FString(""));
		break;
	}
	SetControlVisible(Bu_Control, control_button_type_ != ControlButtonType::VisibleFalse && TC_Default->ActiveTab != TI_Browser);
	UnicodeString button_caption = GetTranslatedText("Run");
	if (control_button_type_ == ControlButtonType::ScheduleTraining){
		button_caption = GetTranslatedText("Schedule");
	}else if (control_button_type_ == ControlButtonType::Stop || control_button_type_ == ControlButtonType::StopTraining || control_button_type_ == ControlButtonType::StopEvaluation){
		button_caption = GetTranslatedText("Stop");
	}else if (control_button_type_ == ControlButtonType::StopAll){
		button_caption = GetTranslatedText("Stop All");
	}
	if (La_Control->Text != button_caption){
		La_Control->Text = button_caption;
	}
	const int button_color =
		control_button_type_ == ControlButtonType::Run ||
		control_button_type_ == ControlButtonType::RunTraining ||
		control_button_type_ == ControlButtonType::ScheduleTraining ||
		control_button_type_ == ControlButtonType::RunEvaluation;
	SetControlVisible(RR_ControlDark, button_color);
	SetControlVisible(RR_ControlLight, !button_color);
	La_Control->TextSettings->FontColor = button_color ? SDEEP_CONSOLE_COLOR_LIGHTEST : SDEEP_CONSOLE_COLOR_DARKEST;
	SetControlVisible(La_Evaluate, evaluate_visible);

	SetControlVisible(Co_Run, is_trainable);

	bool training_enabled = TC_Default->ActiveTab == TI_Network;// || TC_Default->ActiveTab == TI_Config
	if (nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla ||
        nn_document_.GetSDeepEngineType() == SDeepEngineType::Torch) {
        training_enabled &= true;
	}
	else {
        training_enabled &= false;
    }

	SetControlVisible(Co_Training, training_enabled);
	SetControlVisible(Co_Run, training_enabled);
	SetControlVisible(Co_Instance, training_enabled);
	const int node_num_visible = training_enabled && Co_NumNode->Items->Count > 1;
	La_NodeX->Position->X = Co_NumNode->Position->X - La_NodeX->Width;
	SetControlVisible(Co_NumNode, node_num_visible);
	SetControlVisible(La_NodeX, node_num_visible);


	int run_evaluation_enabled = false;
	{
		const int result_index = nn_document_.GetCurrentResultIndex();
		if (result_index >= 0 && nn_document_.GetResults().GetSize()) {
			const NNTrainingResult &result = nn_document_.GetResult(result_index);
			run_evaluation_enabled = result.GetComplete();
		}
	}

	SetControlVisible(PB_NetworkTab, is_trainable);

	if (TC_Left->ActiveTab == TI_TrainingLeft){
		const int result_selected =
			TC_Default->Visible &&
			(TC_Left->ActiveTab == TI_TrainingLeft) &&
			Fr_ResultsList->GetItemIndex() >= 0 &&
			Fr_ResultsList->GetItemIndex() < nn_document_.GetResults().GetSize();
		if (MILR_OpenResultLocation->Enabled != result_selected) {
			MILR_OpenResultLocation->Enabled = result_selected;
            MILR_Duplicate->Enabled = result_selected;
			MILR_OpenLearningCurveForComparison->Enabled = result_selected;
			MILRM_CommandPrompt->Enabled = result_selected;
		}

		int selected_and_not_executing = result_selected;
		if (selected_and_not_executing) {
			if (GetTraining()) {
				const int executing = GetTraining(nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetDir());
				selected_and_not_executing &= !executing;
			}
			if (GetEvaluating()) {
				selected_and_not_executing &= sdeep_evaluation_thread_->GetName() != nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetDir();
			}
		}
		//Set before SetControlEnabled(MIRN_PlotWeight
		SetControlEnabled(MILR_DeleteFromDisk, selected_and_not_executing);

		SetControlVisible(MILR_Export, is_nnabla);
		SetControlEnabled(MILR_Export, !GetTraining() && !GetEvaluating() && run_evaluation_enabled);//result_selected && run_evaluation_enabled //deadlock

		SetControlEnabled(MILR_ResumeTraining, selected_and_not_executing && !run_evaluation_enabled);
		SetControlEnabled(MILR_RetrainInPlace, selected_and_not_executing);
		SetControlEnabled(MILR_RetrainNotInPlace, result_selected);
		SetControlEnabled(MILR_ForceComplete, selected_and_not_executing && !run_evaluation_enabled);
		int schedule_exists = false;
		for (int ir = 0; ir < nn_document_.GetResults().GetSize() ; ir++) {
			schedule_exists |= nn_document_.GetResults().GetAt(ir).GetSchedule();
		}
		int scheduled = false;
		if (result_selected) {
			scheduled = nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetSchedule();
		}
		SetControlEnabled(MILR_CancelSchedule, scheduled);
		SetControlEnabled(MILR_CancelAllScheduledTask, schedule_exists);
		SetControlEnabled(MILR_ResumeScheduledTask, schedule_exists && !GetTraining());
	}


	if (Bu_FullScreen->IsPressed != FullScreen) {
		Bu_FullScreen->IsPressed = FullScreen;
	}

	//Common
	sdeep_console_transition_coef_ = settings_.GetGUITransition() ? 0.2f : 1.0f;
	const float sdeep_console_fast_transition_coef = sqrt(sdeep_console_transition_coef_);

	//Browser
	if (TC_Left->ActiveTab == TI_BrowserLeft) {
		Fr_Browser->UIUpdate();
	}
	if (TC_Default->ActiveTab == TI_Browser) {
		if (Fr_ProjectFiles->Visible) {
			Fr_ProjectFiles->UIUpdate();
		}
		if (Fr_DatasetFiles->Visible) {
			Fr_DatasetFiles->UIUpdate();
		}
		if (Fr_AISolutions->Visible) {
			Fr_AISolutions->UIUpdate();
		}
		if (Fr_NNablaExamples->Visible) {
			Fr_NNablaExamples->UIUpdate();
		}
	}


	//Network Tab
	SetControlEnabled(Fr_Component, !network_mouse_down_);
	if (!nn_document_.GetLocked()) {
		if (TC_Left->ActiveTab == TI_NetworkLeft){
			Fr_Component->UIUpdate();
		}

		const FListP<NNGraphItem> &graph = nn_graph_mapper_.GetGraph();
		if (!network_mouse_down_ ||
			network_mouse_down_mode_ != NetworkMouseDownMode::DragLayer){
			FListT<int> hover_layer_ids;
			for (int i = graph.GetSize() - 1 ; i >= 0 ; i--) {
				const NNGraphItem &item = graph.GetAt(i);
				if (item.sub_id == NNGraphMapperLayerSubID::LayerRect) {
					if (item.Contains(network_last_x_, network_last_y_)){
						hover_layer_ids.Add(item.id);
					}
				}
			}
			int hover_changed = false;
			if (hover_layer_ids.GetSize() != nn_document_.GetCurrentHoverLayerIDs().GetSize()){
				hover_changed = true;
			}else {
				for (int i = 0 ; i < hover_layer_ids.GetSize() ; i++){
					if (hover_layer_ids.GetAt(i) != nn_document_.GetCurrentHoverLayerIDs().GetAt(i)){
						hover_changed = true;
					}
				}
			}
			if (hover_changed){
				nn_document_.ClearInitialOffset();
				nn_document_.SetCurrentHoverLayerIDs(hover_layer_ids);
			}
		}

		if (view_update_count_ != nn_document_.GetViewUpdateCount() ||
			update_count_ != nn_document_.GetUpdateCount()) {
			nn_document_.SetRelatedNetworks();
			nn_graph_mapper_.Update(nn_document_, update_count_ != nn_document_.GetUpdateCount(), Bu_Property->IsFocused);
			update_count_ = nn_document_.GetUpdateCount();
			view_update_count_ = nn_document_.GetViewUpdateCount();
			if (TC_Default->Visible && TC_Default->ActiveTab == TI_Network) {
				PB_NetworkTab->Repaint();
				PB_Network->Repaint();
				Sc_Property->Tag = 0;
				PB_Property->Repaint();
			}
			PB_Statistics->Repaint();
		}

		int sc_property_visible = false;
		if (Sc_Property->Tag == 0) {
			const int property_size = default_property_height * visible_property_num_;
			if (property_size > PB_Property->Height - default_property_height) {
				Sc_Property->Tag = -1;
				Sc_Property->Value = (property_current_top_ * Sc_Property->Max) / property_size;
				Sc_Property->ViewportSize = (PB_Property->Height - default_property_header_height) * Sc_Property->Max / property_size;
				Sc_Property->Tag = 0;
				sc_property_visible = true;
			}
		}else {
			sc_property_visible = true;
		}
		if (Sc_Property->Visible != sc_property_visible) {
			Sc_Property->Visible = sc_property_visible;
			if (!sc_property_visible) {
				property_top_ = 0.0;
			}
		}

		double sum_diff = 0.0;
		if (Sc_Property->Tag == 0 && property_current_pos_ >= 0) {
			if ((property_current_pos_ + 1) * default_property_height - property_top_ > PB_Property->Height - default_property_header_height) {
				property_top_ = (property_current_pos_ + 1) * default_property_height + default_property_header_height - PB_Property->Height;
			}
			if (property_current_pos_ * default_property_height < property_top_) {
				property_top_ = property_current_pos_ * default_property_height;
			}
		}

		const double last_property_current_top = property_current_top_;
		property_current_top_ = (1.0 - sdeep_console_transition_coef_) * property_current_top_ + sdeep_console_transition_coef_ * property_top_;
		sum_diff +=
			fabs(last_property_current_top - property_current_top_) * 1000;

		if (TC_Default->Visible && sum_diff >= 0.5) {
			PB_Property->Repaint();
		}

		//Network Tab
		sum_diff = 0.0;
		int tab_width = (PB_NetworkTab->Width - PB_NetworkTab->Height) / nn_document_.GetNumStructure();
		if (tab_width > 192){
			tab_width = 192;
		}
		if (tab_width < PB_NetworkTab->Height * 1.5){
			tab_width = PB_NetworkTab->Height * 1.5;
		}
		FStringList last_network_tab_names = network_tab_names_;
		FListT<TRectF> last_network_tab_rect = network_tab_rect_;
		network_tab_names_.Clear();
		network_tab_rect_.Clear();
		const int drag_network_index = !drag_network_tab_ ? 0 :
			(tab_width * nn_document_.GetActiveStructureIndex() + drag_network_tab_x_shift_ + tab_width / 2) / tab_width;
		for (int i = 0; i < nn_document_.GetNumStructure() ; i++) {
			const NNStructure &structure = nn_document_.GetStructure(i);
			network_tab_names_.Add(structure.GetName());
			int pos = i;
			if (drag_network_tab_ && i < nn_document_.GetActiveStructureIndex() && drag_network_index <= i){
				pos ++;
			}
			if (drag_network_tab_ && i > nn_document_.GetActiveStructureIndex() && drag_network_index >= i){
				pos --;
			}
			TRectF rect = RectF(
				pos * (tab_width), 0,
				(pos + 1) * (tab_width), PB_NetworkTab->Height);
			const int index = last_network_tab_names.IndexOf(structure.GetName());
			if (index >= 0){
				network_tab_rect_.Add(last_network_tab_rect.GetAt(index));
			}else {
				network_tab_rect_.Add(Rect(rect.left, rect.top, rect.left, rect.bottom));
			}

			TRectF last_rect = network_tab_rect_.GetAt(i);
			TRectF update_rect = last_rect;
			if (drag_network_tab_ && i == nn_document_.GetActiveStructureIndex()){
				update_rect.left = rect.left + drag_network_tab_x_shift_;
				update_rect.right = rect.right + drag_network_tab_x_shift_;
				network_tab_rect_.SetAt(i, update_rect);
			}else {
				update_rect.left = (1.0 - sdeep_console_fast_transition_coef) * last_rect.left + sdeep_console_fast_transition_coef * rect.left;
				update_rect.right = (1.0 - sdeep_console_fast_transition_coef) * last_rect.right + sdeep_console_fast_transition_coef * rect.right;
				network_tab_rect_.SetAt(i, update_rect);
            }
			sum_diff +=
				fabs(network_tab_rect_.GetAt(i).left - last_rect.left) +
				fabs(network_tab_rect_.GetAt(i).right - last_rect.right);
		}

		{
			//Add button
			TRect rect = Rect(
				nn_document_.GetNumStructure() * (tab_width), 0,
				nn_document_.GetNumStructure() * (tab_width) + PB_NetworkTab->Height, PB_NetworkTab->Height);
			TRectF last_rect = network_tab_add_rect_;
			network_tab_add_rect_.top = rect.top;
			network_tab_add_rect_.bottom = rect.bottom;
			network_tab_add_rect_.left = (1.0 - sdeep_console_fast_transition_coef) * last_rect.left + sdeep_console_fast_transition_coef * rect.left;
			network_tab_add_rect_.right = (1.0 - sdeep_console_fast_transition_coef) * last_rect.right + sdeep_console_fast_transition_coef * rect.right;
			sum_diff +=
				fabs(network_tab_add_rect_.left - last_rect.left) +
				fabs(network_tab_add_rect_.right - last_rect.right);
		}
		if (TC_Default->Visible && sum_diff >= 0.005) {
			PB_NetworkTab->Repaint();
		}
	}
    La_NetworkSearch->Visible = Ed_NetworkSearch->Text == "" && !Ed_NetworkSearch->IsFocused;

	//Network Graph (Network Tab)
	double update_count_from_network_tab = 0.0;
	if (!nn_document_.GetLocked()) {
		if (TC_Default->ActiveTab == TI_Network) {
			if ((Lo_EditProperty->Visible || Co_Property->Visible) &&
				 current_property_layer_id_ != nn_document_.GetCurrentLayerID()) {
				 ClearEditBox();
			}

			double sum_diff = 0.0;
			if (Sc_NetworkX->Tag == 0 && Sc_NetworkY->Tag == 0) {
				float current_cursor_position[4];
				memset(current_cursor_position, 0, sizeof(current_cursor_position));
				if (nn_document_.GetCurrentCursorPosition(current_cursor_position)) {
					if (network_left_ + PB_Network->Width / pow(2.0, network_zoom_ * 0.5) < current_cursor_position[2] + NNLayerDefaultSpace) {
						network_left_ = current_cursor_position[2] + NNLayerDefaultSpace - PB_Network->Width / pow(2.0, network_zoom_ * 0.5);
					}
					if (network_left_ > current_cursor_position[0] - NNLayerDefaultSpace) {
						network_left_ = current_cursor_position[0] - NNLayerDefaultSpace >= 0 ? current_cursor_position[0] - NNLayerDefaultSpace : 0.0;
					}
					if (network_top_ + PB_Network->Height / pow(2.0, network_zoom_ * 0.5) < current_cursor_position[3] + NNLayerDefaultSpace) {
						network_top_ = current_cursor_position[3] + NNLayerDefaultSpace - PB_Network->Height / pow(2.0, network_zoom_ * 0.5);
					}
					if (network_top_ > current_cursor_position[1] - NNLayerDefaultSpace) {
						network_top_ = current_cursor_position[1] - NNLayerDefaultSpace >= 0 ? current_cursor_position[1] - NNLayerDefaultSpace : 0.0;
					}
				}
			}

			const double last_network_current_left = network_current_left_;
			const double last_network_current_top = network_current_top_;
			const double last_network_current_zoom = network_current_zoom_;
			/*if (GetKeyState(VK_SHIFT) < 0 && network_focuse_index_ == 1 && (Sc_NetworkY->Visible || Lo_NetworkX->Visible)) {
				const double zoom_x = log((PB_Network->Width - (Sc_NetworkY->Visible ? Sc_NetworkY->Width : 0)) / nn_document_.GetNetworkViewSizeX());
				const double zoom_y = log((PB_Network->Height - (Lo_NetworkX->Visible ? Sc_NetworkX->Height : 0)) / nn_document_.GetNetworkViewSizeY());
				const double zoom = zoom_x < zoom_y ? zoom_x : zoom_y;
				network_current_left_ = (1.0 - sdeep_console_transition_coef_) * network_current_left_;
				network_current_top_ = (1.0 - sdeep_console_transition_coef_) * network_current_top_;
				network_current_zoom_ = (1.0 - sdeep_console_transition_coef_) * network_current_zoom_ + sdeep_console_transition_coef_ * zoom;
			}else */{
				const double network_current_right_x = network_current_left_ + PB_Network->Width / pow(2.0, network_current_zoom_ * 0.5);
				//const double network_current_bottom_y = network_current_top_ + PB_Network->Height / pow(2.0, network_current_zoom_ * 0.5);
				const double network_target_right_x = network_left_ + PB_Network->Width / pow(2.0, network_zoom_ * 0.5);
				//const double network_target_bottom_y = network_top_ + PB_Network->Height / pow(2.0, network_zoom_ * 0.5);
				network_current_left_ = (1.0 - sdeep_console_transition_coef_) * network_current_left_ + sdeep_console_transition_coef_ * network_left_;
				network_current_top_ = (1.0 - sdeep_console_transition_coef_) * network_current_top_ + sdeep_console_transition_coef_ * network_top_;
				const double network_current_right = (1.0 - sdeep_console_transition_coef_) * network_current_right_x + sdeep_console_transition_coef_ * network_target_right_x;
				//const double network_current_bottom = (1.0 - sdeep_console_transition_coef_) * network_current_bottom_y + sdeep_console_transition_coef_ * network_target_bottom_y;
				network_current_zoom_ = log(PB_Network->Width / (network_current_right - network_current_left_)) / log(sqrt(2.0));
			}
			sum_diff +=
				fabs(last_network_current_left - network_current_left_) +
				fabs(last_network_current_top - network_current_top_) +
				fabs(last_network_current_zoom - network_current_zoom_) * 1000;

			int sc_network_x_visible = false;
			int sc_network_y_visible = false;
			if (Sc_NetworkX->Tag == 0) {
				if (nn_document_.GetNetworkViewSizeX() > (PB_Network->Width - Sc_NetworkY->Width) / pow(2.0, network_zoom_ * 0.5)) {
					Sc_NetworkX->Tag = -1;
					Sc_NetworkX->Value = (network_current_left_ * Sc_NetworkX->Max) / nn_document_.GetNetworkViewSizeX();
					Sc_NetworkX->ViewportSize = ((PB_Network->Width - Sc_NetworkY->Width) / pow(2.0, network_zoom_ * 0.5)) * Sc_NetworkX->Max / nn_document_.GetNetworkViewSizeX();
					Sc_NetworkX->Tag = 0;
					sc_network_x_visible = true;
				}
			}else {
				sc_network_x_visible = true;
			}
			if (Sc_NetworkY->Tag == 0) {
				if (nn_document_.GetNetworkViewSizeY() > (PB_Network->Height - Sc_NetworkX->Height) / pow(2.0, network_zoom_ * 0.5)) {
					Sc_NetworkY->Tag = -1;
					Sc_NetworkY->Value = (network_current_top_ * Sc_NetworkY->Max) / nn_document_.GetNetworkViewSizeY();
					Sc_NetworkY->ViewportSize = ((PB_Network->Height - Sc_NetworkX->Height) / pow(2.0, network_zoom_ * 0.5)) * Sc_NetworkY->Max / nn_document_.GetNetworkViewSizeY();
					Sc_NetworkY->Tag = 0;
					sc_network_y_visible = true;
				}
			}else {
				sc_network_y_visible = true;
			}
			if (Sc_NetworkX->Visible != sc_network_x_visible) {
				Sc_NetworkX->Visible = sc_network_x_visible;
				if (!sc_network_x_visible) {
					network_left_ = 0.0;
				}
			}
			if (Sc_NetworkY->Visible != sc_network_y_visible) {
				Sc_NetworkY->Visible = sc_network_y_visible;
				if (!sc_network_y_visible) {
					network_top_ = 0.0;
				}
			}

			sum_diff += nn_graph_mapper_.Animation(sdeep_console_transition_coef_);
			if (TC_Default->Visible && sum_diff >= 0.5) {
				PB_Network->Repaint();
			}
			update_count_from_network_tab = sum_diff * pow(2.0, network_current_zoom_ * 0.5) / network_overview_current_zoom_;
		}

		if (last_layer_id_ != nn_document_.GetCurrentLayerID()) {
			last_layer_id_ = nn_document_.GetCurrentLayerID();
			int is_comment_layer = false;
			if (last_layer_id_ >= 0) {
				const NNLayer *layer = nn_document_.GetLayerByID(last_layer_id_);
				if (layer) {
					const FString &layer_type = layer->GetType();
					is_comment_layer = layer_type == "Comment";
				}
			}
			PreviewData(!is_comment_layer);
		}
	}

	//Dataset Tab
	if (TC_Left->ActiveTab == TI_DatasetLeft){
		Fr_DatasetList->UIUpdate();
	}
	if (TC_Default->ActiveTab == TI_Dataset){
		const int dataset_index = Fr_DatasetList->GetItemIndex();
		const int dataset_selected = dataset_index >= 0  && dataset_index < nn_document_.GetNumDataset();
		SetControlEnabled(MIDL_RenameDataset, dataset_selected);
		SetControlEnabled(MIDL_DeleteDataset, dataset_selected);
		SetControlEnabled(MIDL_MoveUp, dataset_index > 0);
		SetControlEnabled(MIDL_MoveDown, dataset_selected && dataset_index < nn_document_.GetNumDataset() - 1);
		if (dataset_selected) {
			const NNDataset &dataset = nn_document_.GetDataset(dataset_index);
			UnicodeString dataset_file_name =
				dataset.GetFileName().PChar();
			if (dataset_file_name == ""){
				dataset_file_name = "(Not Set)";
			}
			if (Ed_DatasetFileName->Text != dataset_file_name) {
				Ed_DatasetFileName->Text = dataset_file_name;
			}
			SetCheckBoxChecked(Ch_DatasetMain, nn_document_.GetMainDatasetIndex() == dataset_index, true);
			SetCheckBoxChecked(Ch_DatasetShuffle, dataset.GetShuffle());
			SetCheckBoxChecked(Ch_DatasetEnableCache, dataset.GetEnableCache());
			SetCheckBoxChecked(Ch_DatasetImageNormalization, dataset.GetImageNormalization());
		}

		if (Fr_DatasetFiles2->Visible){
			Fr_DatasetFiles2->UIUpdate();
		}
	}
	SetControlVisible(Lo_CSVGraphType, csv_graph_exists_);

	TableUIState &table_ui_state = GetCurrentTableUIState();

	TPopupMenu *popup = NULL;
	int result_csv = 0;
	if (TC_Default->ActiveTab == TI_Evaluation) {
		if (Fr_ResultsList->GetItemIndex() >= 0){
			popup = RB_EvaluationConfusionMatrix->IsChecked ? PM_ConfusionMatrix : PM_Table;
		}else {
			popup = NULL;
		}
		result_csv = true;
	}else {
		popup = PM_Table;
		result_csv = false;
	}
	if (PB_Table->PopupMenu != popup) {
		PB_Table->PopupMenu = popup;
	}
	SetControlEnabled(MIT_CheckConsistency, !result_csv && !GetExecuting() && TC_Default->ActiveTab == TI_Dataset);
	SetControlVisible(
		MIT_Plugin,
		is_nnabla && result_csv &&
		(RB_EvaluationOutputResult->IsChecked || (RB_EvaluationOthers->IsChecked && (others_table_type_ == OthersTableType::Default || others_table_type_ == OthersTableType::Dataset))) &&
		plugins_.GetSize());
	SetControlVisible(MIT_PluginDataset, !result_csv && (TC_Default->ActiveTab == TI_Dataset || (Lo_RightSub->Visible && Re_RightDatasetPreviewTop->Visible)));
	if (nn_document_.GetNumDataset()){
		const CSVDataset &dataset = GetCurrentTable();

		const int dataset_num_data = dataset.GetNumData();
		const int dataset_num_column = dataset.GetNumColumn();
		if ((TC_Default->ActiveTab == TI_Dataset || TC_Default->ActiveTab == TI_Evaluation || Lo_RightSub->Visible) && Lo_Table->Visible) {
			double sum_diff = 0.0;

			if (dataset_image_update_count_ != image_cache_list_->GetUpdateCount()) {
				dataset_image_update_count_ = image_cache_list_->GetUpdateCount();
				sum_diff += 1.0;
			}
			if (dataset_csv_update_count_ != csv_cache_list_->GetUpdateCount()) {
				dataset_csv_update_count_ = csv_cache_list_->GetUpdateCount();
				sum_diff += 1.0;
			}
			if (dataset_update_count_ != dataset_internal_update_count_) {
				dataset_update_count_ = dataset_internal_update_count_;
				sum_diff += 1.0;
			}
			if (table_ui_state.row_index >= 0 && table_ui_state.row_index < dataset_num_data &&
				table_ui_state.col_index >= 0 && table_ui_state.col_index < dataset_num_column) {
				if (popup == PM_Table) {
					UnicodeString data_text = dataset.GetValue(table_ui_state.row_index, table_ui_state.col_index).PChar();
					const int is_file = IsFile(data_text);
					SetControlEnabled(MIT_OpenData, is_file);
					SetControlEnabled(MIT_OpenFileLocation, is_file);
					SetControlVisible(MIT_Sort, TC_Default->ActiveTab == TI_Evaluation);
					SetControlEnabled(MIT_Sort, !is_file && (&GetCurrentTable() == &nn_document_.GetResultCSV(0) || (&GetCurrentTable() == &nn_document_.GetResultCSV(2) && others_table_type_ == OthersTableType::Dataset)));
				}else {
					const CSVDataset &cm = GetCurrentTable();
					if (table_ui_state.row_index < cm.GetRowNames().GetSize() &&
						table_ui_state.col_index < cm.GetColumnNames().GetSize()){
						SetControlEnabled(MIC_List,
							cm.GetRowName(table_ui_state.row_index).Pos("=") && (
								cm.GetColumnName(table_ui_state.col_index).Pos("=") ||
								cm.GetColumnName(table_ui_state.col_index).Pos("__")
								));
					}
				}
			}

			int sc_dataset_x_visible = false;
			int sc_dataset_y_visible = false;

			if (dataset_num_data) {
				if (table_ui_state.col_index > dataset_num_column - 1){
					table_ui_state.col_index = dataset_num_column - 1;
					sum_diff += 1.0;
				}
				if (table_ui_state.row_index > dataset_num_data - 1){
					table_ui_state.row_index = dataset_num_data - 1;
					sum_diff += 1.0;
				}
				if (table_ui_state.col_index < 0){
					table_ui_state.col_index = 0;
					sum_diff += 1.0;
				}
				if (table_ui_state.row_index < 0){
					table_ui_state.row_index = 0;
					sum_diff += 1.0;
				}
				const TRectD rect_bak = table_ui_state.current_cursor_rect;
				if (table_ui_state.current_cursor_rect.Width > 0) {
					if (Sc_TableX->Tag == 0 && Sc_TableY->Tag == 0) {
						if (table_ui_state.cursor_rect.Right - table_ui_state.left > PB_Table->Width) {
							table_ui_state.left = table_ui_state.cursor_rect.Right - PB_Table->Width;
						}
						if (table_ui_state.cursor_rect.Left - dataset_header_width_ < table_ui_state.left) {
							table_ui_state.left = table_ui_state.cursor_rect.Left - dataset_header_width_;
						}
						const double left_bak = table_ui_state.current_left;
						table_ui_state.current_left = (1.0 - sdeep_console_transition_coef_) * table_ui_state.current_left + sdeep_console_transition_coef_ * table_ui_state.left;
						sum_diff += fabs(table_ui_state.current_left - left_bak);

						if (table_ui_state.cursor_rect.Bottom - table_ui_state.top > PB_Table->Height) {
							table_ui_state.top = table_ui_state.cursor_rect.Bottom - PB_Table->Height;
						}
						if (table_ui_state.cursor_rect.Top - dataset_header_height_ < table_ui_state.top) {
							table_ui_state.top = table_ui_state.cursor_rect.Top - dataset_header_height_;
						}
						const double top_bak = table_ui_state.current_top;
						table_ui_state.current_top = (1.0 - sdeep_console_transition_coef_) * table_ui_state.current_top + sdeep_console_transition_coef_ * table_ui_state.top;
						sum_diff += fabs(table_ui_state.top - top_bak);
					}

					table_ui_state.current_cursor_rect = TRectD(
						(1.0 - sdeep_console_fast_transition_coef) * table_ui_state.current_cursor_rect.Left + sdeep_console_fast_transition_coef * table_ui_state.cursor_rect.Left,
						(1.0 - sdeep_console_fast_transition_coef) * table_ui_state.current_cursor_rect.Top + sdeep_console_fast_transition_coef * table_ui_state.cursor_rect.Top,
						(1.0 - sdeep_console_fast_transition_coef) * table_ui_state.current_cursor_rect.Right + sdeep_console_fast_transition_coef * table_ui_state.cursor_rect.Right,
						(1.0 - sdeep_console_fast_transition_coef) * table_ui_state.current_cursor_rect.Bottom + sdeep_console_fast_transition_coef * table_ui_state.cursor_rect.Bottom
						);
				}else {
					table_ui_state.current_cursor_rect = table_ui_state.cursor_rect;
				}

				sum_diff += fabs(table_ui_state.current_cursor_rect.Left - rect_bak.Left);
				sum_diff += fabs(table_ui_state.current_cursor_rect.Top - rect_bak.Top);
				sum_diff += fabs(table_ui_state.current_cursor_rect.Right - rect_bak.Right);
				sum_diff += fabs(table_ui_state.current_cursor_rect.Bottom - rect_bak.Bottom);
				if (Sc_TableX->Tag == 0) {
					const double dataset_view_size = dataset_num_column * default_dataset_col_width_;
					if (dataset_view_size > PB_Table->Width - dataset_header_width_) {
						Sc_TableX->Tag = -1;
						Sc_TableX->Value = (table_ui_state.current_left * Sc_TableX->Max) / dataset_view_size;
						Sc_TableX->ViewportSize = (PB_Table->Width - dataset_header_width_) * Sc_TableX->Max / dataset_view_size;
						sc_dataset_x_visible = true;
						Sc_TableX->Tag = 0;
					}
				}else {
					sc_dataset_x_visible = true;
				}
				if (Sc_TableY->Tag == 0) {
					const double dataset_view_size = dataset_num_data * default_dataset_row_height_;
					if (dataset_view_size > PB_Table->Height - dataset_header_height_) {
						Sc_TableY->Tag = -1;
						Sc_TableY->Value = (table_ui_state.current_top * Sc_TableY->Max) / dataset_view_size;
						Sc_TableY->ViewportSize = (PB_Table->Height - dataset_header_height_) * Sc_TableY->Max / dataset_view_size;
						sc_dataset_y_visible = true;
						Sc_TableY->Tag = 0;
					}
				}else {
					sc_dataset_y_visible = true;
				}
			}

			if (Sc_TableY->Visible != sc_dataset_y_visible) {
				Sc_TableY->Visible = sc_dataset_y_visible;
			}
			if (!sc_dataset_y_visible && table_ui_state.current_top != 0.0) {
				table_ui_state.current_top = 0.0;
				sum_diff += 1.0;
			}
			if (Sc_TableX->Visible != sc_dataset_x_visible) {
				Sc_TableX->Visible = sc_dataset_x_visible;
			}
			if (!sc_dataset_x_visible && table_ui_state.current_left != 0.0) {
				table_ui_state.current_left = 0.0;
				sum_diff += 1.0;
			}
			if ((TC_Default->Visible || Lo_RightSub->Visible) && sum_diff > 0.5) {
				PB_Table->Repaint();
				PB_DataPreview->Repaint();
			}
		}
	}

	//Config Tab
	if (TC_Left->ActiveTab == TI_ConfigLeft){
		Fr_ConfigList->UIUpdate();
	}
	if (TC_Default->ActiveTab == TI_Config && !nn_document_.GetLocked()) {
		// Global Config
		if (TC_Config->ActiveTab == TI_GlobalConfig) {
			SetControlEnabled(MICL_Rename, false);
			SetControlEnabled(MICL_Delete, false);
			SetControlEnabled(MICL_MoveUp, false);
			SetControlEnabled(MICL_MoveDown, false);
			if (!Me_ProjectDescription->IsFocused){
				FString description = nn_document_.GetDescription();
				description = description.ReplaceStr("\\n", "\r\n");
				description = description.ReplaceStr("\\t", "\t");
				description = description.ReplaceStr("\\\\", "\\");
				if (Me_ProjectDescription->Text != description.PChar()) {
					Me_ProjectDescription->Text = description.PChar();
				}
			}

			SetEditBoxTextInt(Ed_MaxEpoch, nn_document_.GetMaxEpoch());
			SetCheckBoxChecked(Ch_SaveBest, nn_document_.GetSaveBest());
			SetEditBoxTextInt(Ed_BatchSize, nn_document_.GetBatchSize());
			if (!Co_TypeConfig->IsFocused){
				if (Co_TypeConfig->ItemIndex != ((int)nn_document_.GetTypeConfig())) {
					Co_TypeConfig->ItemIndex = ((int)nn_document_.GetTypeConfig());
				}
				/*
				if (Co_TypeConfig->ItemIndex != ((int)nn_document_.GetTypeConfig() && settings_.GetLocalSDeepProcessorType() != LocalSDeepProcessorType::CPU)) {
					Co_TypeConfig->ItemIndex = ((int)nn_document_.GetTypeConfig() && settings_.GetLocalSDeepProcessorType() != LocalSDeepProcessorType::CPU);
				}
				*/
			}
			SetEditBoxTextInt(Ed_MonitorInterval, nn_document_.GetMonitorInterval());
			SetCheckBoxChecked(Ch_AutoEvaluation, nn_document_.GetAutoEvaluation());

			//Structure Search
			SetCheckBoxChecked(Ch_StructureSearchEnabled, nn_document_.GetStructureSearchEnabled());
			SetControlEnabled(Pa_StructureSearch, Ch_StructureSearchEnabled->IsChecked);
			if (!Co_StructureSearchMethod->IsFocused){
				if (Co_StructureSearchMethod->ItemIndex != (int)nn_document_.GetStructureSearchMethod()) {
					Co_StructureSearchMethod->ItemIndex = (int)nn_document_.GetStructureSearchMethod();
				}
			}
			if (Co_StructureSearchObjective->Enabled != (nn_document_.GetStructureSearchMethod() != StructureSearchMethod::Random)) {
				Co_StructureSearchObjective->Enabled = (nn_document_.GetStructureSearchMethod() != StructureSearchMethod::Random);
				if (!Co_StructureSearchObjective->Enabled && nn_document_.GetStructureSearchObjective() != StructureSearchObjective::ErrorAndCalculation) {
					nn_document_.SetStructureSearchObjective(StructureSearchObjective::ErrorAndCalculation);
					nn_document_.UpdateConfig();
				}
			}
			if (!Co_StructureSearchObjective->IsFocused){
				if (Co_StructureSearchObjective->ItemIndex != (int)nn_document_.GetStructureSearchObjective()) {
					Co_StructureSearchObjective->ItemIndex = (int)nn_document_.GetStructureSearchObjective();
				}
			}
			if (!Ed_StructureSearchConstraintValidationMin->IsFocused){
				const double constraint = nn_document_.GetStructureSearchConstraint(FString("Validation"), 0);
				UnicodeString constraint_text = constraint >= 0.0 ? FloatToStr(constraint) : UnicodeString("");
				if (Ed_StructureSearchConstraintValidationMin->Text != constraint_text) {
					Ed_StructureSearchConstraintValidationMin->Text = constraint_text;
				}
			}
			if (!Ed_StructureSearchConstraintValidationMax->IsFocused){
				const double constraint = nn_document_.GetStructureSearchConstraint(FString("Validation"), 1);
				UnicodeString constraint_text = constraint >= 0.0 ? FloatToStr(constraint) : UnicodeString("");
				if (Ed_StructureSearchConstraintValidationMax->Text != constraint_text) {
					Ed_StructureSearchConstraintValidationMax->Text = constraint_text;
				}
			}
			if (!Ed_StructureSearchConstraintCostMultiplyAddMin->IsFocused){
				const double constraint = nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 0);
				UnicodeString constraint_text = constraint >= 0.0 ? FloatToStr(constraint) : UnicodeString("");
				if (Ed_StructureSearchConstraintCostMultiplyAddMin->Text != constraint_text) {
					Ed_StructureSearchConstraintCostMultiplyAddMin->Text = constraint_text;
				}
			}
			if (!Ed_StructureSearchConstraintCostMultiplyAddMax->IsFocused){
				const double constraint = nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 1);
				UnicodeString constraint_text = constraint >= 0.0 ? FloatToStr(constraint) : UnicodeString("");
				if (Ed_StructureSearchConstraintCostMultiplyAddMax->Text != constraint_text) {
					Ed_StructureSearchConstraintCostMultiplyAddMax->Text = constraint_text;
				}
			}
			SetCheckBoxChecked(Ch_EarlyStopping, nn_document_.GetEarlyStopping());
			if (!Ed_TimeLimit->IsFocused){
				UnicodeString s;
				if (nn_document_.GetTimeLimit() > 0.0){
					int total_sec = (int)((double)(nn_document_.GetTimeLimit()) * 24 * 60 * 60);
					s = IntToStr(total_sec / (24 * 60 * 60));
					if (s.Length() < 2) {
						s = UnicodeString("0") + s;
					}
					s +=
						UnicodeString(":") +
						FormatDateTime("hh:mm:ss", total_sec * 1.0 / (24 * 60 * 60));
				}
				if (Ed_TimeLimit->Text != s) {
					Ed_TimeLimit->Text = s;
				}
			}

			//Out-of-Core Training
			SetCheckBoxChecked(Ch_OutOfCoreTrainingEnabled, nn_document_.GetOutOfCoreTrainingEnabled());
			if (!Ed_OutOfCoreTrainingGPUMemorySize->IsFocused){
				UnicodeString size = IntToStr(nn_document_.GetOutOfCoreTrainingGPUMemorySize());
				if (Ed_OutOfCoreTrainingGPUMemorySize->Text != size) {
					Ed_OutOfCoreTrainingGPUMemorySize->Text = size;
				}
			}
			if (!Ed_OutOfCoreTrainingWindowSize->IsFocused){
				UnicodeString size = IntToStr(nn_document_.GetOutOfCoreTrainingWindowSize());
				if (Ed_OutOfCoreTrainingWindowSize->Text != size) {
					Ed_OutOfCoreTrainingWindowSize->Text = size;
				}
			}
		}

		if (TC_Config->ActiveTab == TI_OptimizerConfig && TI_OptimizerConfig->Tag < nn_document_.GetNumOptimizer()) {
			const int optimizer_index = TI_OptimizerConfig->Tag;
			NNOptimizer &optimizer = nn_document_.GetOptimizer(optimizer_index);
			SetControlEnabled(MICL_Rename, true);
			SetControlEnabled(MICL_Delete, nn_document_.GetNumOptimizer() > 0);
			SetControlEnabled(MICL_MoveUp, optimizer_index > 0);
			SetControlEnabled(MICL_MoveDown, optimizer_index < nn_document_.GetNumOptimizer() - 1);

			if (!Ed_OptimizerRangeFrom->IsFocused){
				const int range = optimizer.GetRangeFrom();
				UnicodeString constraint_text = range >= 1 ? IntToStr(range) : UnicodeString("");
				if (Ed_OptimizerRangeFrom->Text != constraint_text) {
					Ed_OptimizerRangeFrom->Text = constraint_text;
				}
			}
			if (!Ed_OptimizerRangeTo->IsFocused){
				const int range = optimizer.GetRangeTo();
				UnicodeString constraint_text = range >= 1 ? IntToStr(range) : UnicodeString("");
				if (Ed_OptimizerRangeTo->Text != constraint_text) {
					Ed_OptimizerRangeTo->Text = constraint_text;
				}
			}
			if (!Co_OptimizerRangeUnit->IsFocused && Co_OptimizerRangeUnit->ItemIndex != optimizer.GetRangeUnit()){
				Co_OptimizerRangeUnit->ItemIndex = optimizer.GetRangeUnit();
			}
			SetEditBoxText(Ed_OptimizerNetwork, optimizer.GetNetworkName());
			SetEditBoxText(Ed_OptimizerDataset, optimizer.GetDatasetNames().GetText(','));

			if (!Co_Updater->IsFocused){
				if (nn_document_.GetSolverComponents()) {
					if (Co_Updater->Items->Count != nn_document_.GetSolverComponents()->GetSize()) {
						Co_Updater->ItemIndex = -1;
						Co_Updater->Items->Clear();
						for (int is = 0 ; is < nn_document_.GetSolverComponents()->GetSize() ; is++){
							Co_Updater->Items->Add(nn_document_.GetSolverComponents()->GetAt(is).GetName().PChar());
						}
					}

					UnicodeString updater = Co_Updater->ItemIndex >= 0 ? Co_Updater->Items->Strings[Co_Updater->ItemIndex] : UnicodeString("");
					if (updater != optimizer.GetSolverName().PChar()) {
						Co_Updater->ItemIndex = Co_Updater->Items->IndexOf(optimizer.GetSolverName().PChar());
					}
				}
			}
			SetEditBoxTextInt(Ed_UpdateInterval, optimizer.GetUpdateInterval());
			SetEditBoxTextDouble(Ed_WeightDecay, optimizer.GetWeightDecay());

			const int scheduler_index = is_trainable ? Co_LearningRateScheduler->Items->IndexOf(optimizer.GetLearningRateScheduler().PChar()) : 1;
			if (!Co_LearningRateScheduler->IsFocused && Co_LearningRateScheduler->ItemIndex != scheduler_index){
				Co_LearningRateScheduler->ItemIndex = scheduler_index;
			}
			if (scheduler_index == 0 && TC_LearningRateScheduler->ActiveTab != TI_Cosine){
				SetActiveTab(TC_LearningRateScheduler, TI_Cosine);
			}
			if (scheduler_index == 1 && TC_LearningRateScheduler->ActiveTab != TI_Exponential){
				SetActiveTab(TC_LearningRateScheduler, TI_Exponential);
			}
			if (scheduler_index == 2 && TC_LearningRateScheduler->ActiveTab != TI_Polynomial){
				SetActiveTab(TC_LearningRateScheduler, TI_Polynomial);
			}
			if (scheduler_index == 3 && TC_LearningRateScheduler->ActiveTab != TI_Step){
				SetActiveTab(TC_LearningRateScheduler, TI_Step);
			}
			//Exponential
			SetEditBoxTextDouble(Ed_LearningRateMultiplier, optimizer.GetLearningRateMultiplier());
			SetEditBoxTextDouble(Ed_LRUpdateInterval, optimizer.GetLearningRateUpdateInterval());
			if (!Co_LRUpdateIntervalUnit->IsFocused && Co_LRUpdateIntervalUnit->ItemIndex != optimizer.GetLearningRateUpdateIntervalUnit()){
				Co_LRUpdateIntervalUnit->ItemIndex = optimizer.GetLearningRateUpdateIntervalUnit();
			}

			//Step
			SetEditBoxTextDouble(Ed_StepSchedulerGamma, optimizer.GetLearningRateMultiplier());
			SetEditBoxText(Ed_StepSchedulerSteps, optimizer.GetLearningRateStepsStr());
			if (!Co_StepSchedulerIntervalUnit->IsFocused && Co_StepSchedulerIntervalUnit->ItemIndex != optimizer.GetLearningRateUpdateIntervalUnit()){
				Co_StepSchedulerIntervalUnit->ItemIndex = optimizer.GetLearningRateUpdateIntervalUnit();
			}

			//Polynomial
			SetEditBoxTextDouble(Ed_PolynomialSchedulerPower, optimizer.GetLearningRatePower());

			const int max_updater_param = 9;
			TLabel *updater_labels[max_updater_param] = {La_UpdaterParam1, La_UpdaterParam2, La_UpdaterParam3, La_UpdaterParam4, La_UpdaterParam5, La_UpdaterParam6, La_UpdaterParam7, La_UpdaterParam8, La_UpdaterParam9};
			TEdit *updater_edits[max_updater_param] = {Ed_UpdaterParam1, Ed_UpdaterParam2, Ed_UpdaterParam3, Ed_UpdaterParam4, Ed_UpdaterParam5, Ed_UpdaterParam6, Ed_UpdaterParam7, Ed_UpdaterParam8, Ed_UpdaterParam9};
			for (int i = 0 ;i < max_updater_param ; i++) {
				SetControlVisible(updater_labels[i], i < optimizer.GetSolverParameterNum());
				SetControlVisible(updater_edits[i], i < optimizer.GetSolverParameterNum());
				if (updater_labels[i]->Visible) {
					UnicodeString caption = UnicodeString(optimizer.GetSolverParameterName(i).PChar()) + ":";
					if (updater_labels[i]->Text != caption) {
						updater_labels[i]->Text = caption;
					}
				}
				if (updater_edits[i]->Visible) {
					SetEditBoxTextDouble(updater_edits[i], optimizer.GetSolverParameter(i));
				}
			}

			SetCheckBoxChecked(Ch_LearningRateWarmup, optimizer.GetLearningRateWarmupScheduler() == "Linear");
			SetEditBoxTextDouble(Ed_LearningRateWarmupLength, optimizer.GetLearningRateWarmupLength());
			if (!Co_LearningRateWarmupLengthUnit->IsFocused && Co_LearningRateWarmupLengthUnit->ItemIndex != optimizer.GetLearningRateWarmupLengthUnit()){
				Co_LearningRateWarmupLengthUnit->ItemIndex = optimizer.GetLearningRateWarmupLengthUnit();
			}
		}

		if (TC_Config->ActiveTab == TI_MonitorConfig && TI_MonitorConfig->Tag < nn_document_.GetNumMonitor()) {
			const int monitor_index = TI_MonitorConfig->Tag;
			SetControlEnabled(MICL_Rename, false);
			SetControlEnabled(MICL_Delete, false);
			SetControlEnabled(MICL_MoveUp, false);
			SetControlEnabled(MICL_MoveDown, false);

			SetEditBoxText(Ed_MonitorNetwork, nn_document_.GetMonitor(monitor_index).GetNetworkName());
			SetEditBoxText(Ed_MonitorDataset, nn_document_.GetMonitor(monitor_index).GetDatasetNames().GetText(','));
		}

		if (TC_Config->ActiveTab == TI_ExecutorConfig && TI_ExecutorConfig->Tag < nn_document_.GetNumExecutor()) {
			const int executor_index = TI_ExecutorConfig->Tag;
			NNExecutor &executor = nn_document_.GetExecutor(executor_index);
			SetControlEnabled(MICL_Rename, true);
			SetControlEnabled(MICL_Delete, nn_document_.GetNumExecutor() > 0);
			SetControlEnabled(MICL_MoveUp, executor_index > 0);
			SetControlEnabled(MICL_MoveDown, executor_index < nn_document_.GetNumExecutor() - 1);
			SetEditBoxText(Ed_ExecutorNetwork, executor.GetNetworkName());
			SetEditBoxText(Ed_ExecutorDataset, executor.GetDatasetNames().GetText(','));

			SetEditBoxTextInt(Ed_NumEvaluations, executor.GetNumEvaluations());
			SetRadioButtonChecked(RB_ExecutorMean, executor.GetRepeatEvaluationType() == 0);
			SetRadioButtonChecked(RB_ExecutorStd, executor.GetRepeatEvaluationType() == 2);
			SetRadioButtonChecked(RB_ExecutorLast, executor.GetRepeatEvaluationType() == 1);
			SetCheckBoxChecked(Ch_ExecutorNeedBackPropagation, executor.GetNeedBackPropagation());
		}

	}

	//Training Result (Training Monitor) Tab
	int move_to_eval_tab = false;
	if (!nn_document_.GetLocked()) {
		if (TC_Left->ActiveTab == TI_TrainingLeft) {
			//List
			if (Fr_ResultsList->GetItemIndex() != nn_document_.GetCurrentResultIndex() &&
				Fr_ResultsList->GetItemIndex() < nn_document_.GetResults().GetSize() &&
				(Fr_ResultsList->GetItemIndex() >= 0 || !GetTraining())) {
				nn_document_.SetCurrentResultIndex(Fr_ResultsList->GetItemIndex());
				OpenResult();
				OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
				if (RB_ResultsWeight->IsChecked) {
                    RB_ResultsLearningCurve->IsChecked = true;
				}
			}
			int list_item_index_2 = -1;
			if (result_name_[1] != "") {
				for (int ir = 0 ; ir < nn_document_.GetResults().GetSize() ; ir++) {
					if (nn_document_.GetResults().GetAt(ir).GetDir() == result_name_[1]) {
						list_item_index_2 = ir;
					}
				}
			}
			if (Fr_ResultsList->GetItemIndex2() != list_item_index_2){
				Fr_ResultsList->SetItemIndex2(list_item_index_2);
			}
			Fr_ResultsList->UIUpdate();
		}
		if ((Lo_MonitorBottom->Align == TAlignLayout::Bottom) != is_trainable){
			Lo_MonitorBottom->Align = is_trainable ? TAlignLayout::Bottom : TAlignLayout::Client;
		}
		SetControlVisible(Lo_Result, is_trainable);
		SetControlVisible(Sp_TrainingMonitorBottom, is_trainable);
		SetControlVisible(Re_MonitorLogTop, is_trainable);
		SetControlVisible(Re_TrainingTop2, is_trainable);

		SetControlEnabled(Co_ResultsTradeoffType, RB_ResultsTradeoffGraph->IsChecked);
		SetControlVisible(Me_Monitor, RB_MonitorLog->IsChecked);
		SetControlVisible(PB_MonitorPerformance, RB_MonitorCPU->IsChecked || RB_MonitorGPU->IsChecked);

		for (int it = sdeep_training_thread_.GetSize() - 1 ; it >= 0 ; it--) {
			SDeepTrainingThread *thread = sdeep_training_thread_.GetAt(it);
			if (thread->GetProcessCreated()){
				if (animation_indicator_active_) {
					HideAnimationIndicator();
				}
			}else {
				while (thread->GetNumIndicatorLog()){
					AnimationIndicatorLog(thread->GetIndicatorLog().PChar());
				}
			}
			const int realtime_monitoring = thread->GetName() == nn_document_.GetCurrentResultName();
			if (thread->GetNumLatestLog() || thread->GetUpdated()){
				thread->ResetUpdated();
				int monitor_update_request = false;
				while (thread->GetNumLatestLog()) {
					FString s = thread->GetLatestLog();
					if (s.Pos("cost=") > 0 && s.Pos("[INFO]") > 0) {
						//includes graph update
						monitor_update_request = true;
					}
					//Me_Monitor->Lines->Add(Trim(s.PChar()));
				}
				nn_document_.RefreshResults(FString("\\"));

				if (TC_Left->ActiveTab == TI_TrainingLeft) {
					if (TC_Default->Visible) {
						RefreshResultsList(false);
					}
					if (Fr_ResultsList->GetItemIndex() != nn_document_.GetCurrentResultIndex()) {
						Fr_ResultsList->SetItemIndex(nn_document_.GetCurrentResultIndex());
					}
				}
				if (realtime_monitoring) {
					if (!monitor_update_request){
						OpenResult();//Load text
                        OpenEvaluationResult(false, false);
					}else {
						monitor_update_request_ = monitor_update_request;//reserve update for monitoring_report(Graph)
					}

					//Early stopping
					if (nn_document_.GetStructureSearchEnabled() &&
						nn_document_.GetEarlyStopping() &&
						monitor_csv_[1].GetNumData() &&
						nn_document_.GetCurrentResultName() == thread->GetName()) {
						const int verr_col_current = monitor_csv_[0].GetColumnIndex(FString("valid_error"));
						const int verr_col_comparison = monitor_csv_[1].GetColumnIndex(FString("valid_error"));
						if (verr_col_current >= 0 && verr_col_comparison >= 0) {
							for (int id = monitor_csv_[0].GetNumData() - 1 ; id >= 0 ; id--) {
								FString verr_current = monitor_csv_[0].GetValue(id, verr_col_current);
								FString verr_comparison = monitor_csv_[1].GetValue(id, verr_col_current);
								if (verr_current != "" && verr_comparison != "") {
									if (verr_current.AsFloat() > verr_comparison.AsFloat() * 2.0) {
										thread->Terminate();
									}
									break;
								}
							}
						}
					}
				}
			}

			static TDateTime last = 0.0;
			const TDateTime now = Now();
			if (realtime_monitoring && last < now - 1.0 / (24 * 60 * 60 * 10)) {//100 ms
				last = now;
				UnicodeString last_total_time = Ed_MonitorTimeElapsed->Text;
				RefreshMonitorTimeFromLog();

				//Time limit
				if (nn_document_.GetTimeLimit() > 0) if (Ed_MonitorTimeTotal->Text != "--:--:--:--") {
					FString s = FString(AnsiString(Ed_MonitorTimeTotal->Text).c_str());
					const int days = s.SplitBy(":").AsInt();
					const int hours = s.SplitBy(":").AsInt();
					const int minutes = s.SplitBy(":").AsInt();
					const int seconds = s.AsInt();
					const double total_time = days + (hours + (minutes + seconds * (1.0 / 60.0)) * (1.0 / 60.0)) * (1.0 / 24.0);
					if (total_time > nn_document_.GetTimeLimit()) {
						thread->Terminate();
					}
				}

				//State
				if (last_total_time != Ed_MonitorTimeElapsed->Text && Me_Monitor->Lines->Count) {
					UnicodeString result_dir = GetResultDir();
					UnicodeString state_file_name = result_dir + UnicodeString("\\progress.txt");
					struct stat stat_buf;
					if (stat(AnsiString(state_file_name).c_str(), &stat_buf) == 0) {
						static int state_date = 0;
						if (state_date != stat_buf.st_mtime){
							FStringList sl;
							if (sl.LoadFromFile(AnsiString(state_file_name).c_str())){
								Me_Monitor->Lines->BeginUpdate();
								Me_Monitor->Lines->Strings[Me_Monitor->Lines->Count - 1] = sl.GetText().PChar();
								Me_Monitor->Lines->EndUpdate();
								PB_MonitorPerformance->Repaint();
								state_date = stat_buf.st_mtime;
							}
						}
					}
				}
			}

			if (thread->Finished) {
				const FString name = thread->GetName();
				const FString result_dir = thread->GetResultDir();

				const int stopped = thread->GetStopped();
				const int successfully_finished = thread->GetSuccessfullyFinished();
				if (!stopped) {
					if (thread->GetSuccessfullyFinished()) {
						if (realtime_monitoring) {
                            //Load network graph if if has modified
							AnsiString result_network_file_name = UnicodeString(result_dir.PChar()) + UnicodeString("\\results.nntxt");
							if (FileExists(result_network_file_name)) {
								result_network_name_ = FString("");
								OpenResult();
							}
						}
						Log(FString("training ends"));
						if (thread->GetOption() == "Inference" && TC_Default->ActiveTab == TI_Results) {
							move_to_eval_tab = true;
						}
					}else {
						Log(FString("training ends with an error"));
						LogTraceback(Me_Monitor->Lines);
					}
				}
				sdeep_training_thread_.Delete(it);
				RefreshMonitorTimeFromLog();

				nn_document_.RefreshResults(FString("\\"));
				if (TC_Left->ActiveTab == TI_TrainingLeft) {
					Fr_ResultsList->SetItemIndex(nn_document_.GetCurrentResultIndex());
					if (TC_Default->Visible) {
						RefreshResultsList(false);
					}
				}

				if (!stopped && successfully_finished){
					if (nn_document_.GetAutoEvaluation()) {
						if (ExecuteEvaluation(name, result_dir, true)){
							return;
                        }
					}
					if (TrainNext()) {
						return;
					}
				}
			}
		}
		if (monitor_update_request_ && nn_document_.GetCurrentResultIndex() >= 0) {
			static unsigned int last = 0;
			if (now >= last + 250 || now < last) {
				//Check file update in every 250 ms (too frequent update makes training slower especially in small network)
				UnicodeString result_dir = GetResultDir();
				UnicodeString monitoring_report_file_name = result_dir + UnicodeString("\\monitoring_report.yml");
				struct stat stat_buf;
				if (stat(AnsiString(monitoring_report_file_name).c_str(), &stat_buf) == 0) {
					if (monitor_date_ != stat_buf.st_mtime || !GetTraining()){
						OpenResult();
					}
				}
				last = now;
			}
		}
		if (TC_Default->ActiveTab == TI_Results) {
			{
				UnicodeString zoom_str;
				if (RB_ResultsLearningCurve->IsChecked) {
					zoom_str = IntToStr((int)(pow(2.0, monitor_zoom_ * 0.5) * 100)) + "%";
				}else if (RB_ResultsTradeoffGraph->IsChecked) {
					zoom_str = IntToStr((int)(pow(2.0, result_tradeoff_zoom_ * 0.5) * 100)) + "%";
				}else {
					zoom_str = IntToStr((int)(pow(2.0, result_weight_zoom_ * 0.5) * 100)) + "%";
				}
				if (La_ResultZoom->Text != zoom_str) {
					La_ResultZoom->Text = zoom_str;
				}
			}

			//Graph
			if (TC_Default->Visible) {
				double sum_diff = 0.0;
				SetControlEnabled(MIM_View, RB_ResultsLearningCurve->IsChecked || RB_ResultsTradeoffGraph->IsChecked);
				SetControlEnabled(MIM_SaveCSVAs, RB_ResultsLearningCurve->IsChecked || RB_ResultsTradeoffGraph->IsChecked);

				if (RB_ResultsLearningCurve->IsChecked) {
				//if (training_result_graph_mode_ == TrainingResultGraphMode::LearningCurve) {
					//learning Curve
					if (monitor_update_count_[0] != monitor_csv_[0].GetUpdateCount() || monitor_update_count_[1] != monitor_csv_[1].GetUpdateCount()){
						memset(monitor_value_range_, 0, sizeof(monitor_value_range_));
						sum_diff += 1.0;

						for (int im = 0 ; im < 2 ; im++) {
							monitor_update_count_[im] = monitor_csv_[im].GetUpdateCount();
							if (monitor_value_range_[1] < monitor_csv_[im].GetNumData() + 1){//max epoch + 1
								monitor_value_range_[1] = monitor_csv_[im].GetNumData() + 1;
							}

							//Check graph range
							monitor_csv_[im].GetColumnMinMax(FString("cost"), monitor_value_range_[2], monitor_value_range_[3]);
							if (monitor_value_range_[2] > 1e+8){
                                monitor_value_range_[2] = 1e+8;
								if (monitor_value_range_[3] > 1e+8){
									monitor_value_range_[3] = 1e+8;
								}
							}
							if (MIMV_TrainingValidation->IsChecked || MIMV_Training->IsChecked) {
								monitor_csv_[im].GetColumnMinMax(FString("train_error"), monitor_value_range_[4], monitor_value_range_[5]);
							}
							if (MIMV_TrainingValidation->IsChecked || MIMV_Validation->IsChecked) {
								monitor_csv_[im].GetColumnMinMax(FString("valid_error"), monitor_value_range_[4], monitor_value_range_[5]);
							}
							if (monitor_value_range_[4] > 1e+8){
								monitor_value_range_[4] = 1e+8;
								if (monitor_value_range_[5] > 1e+8){
									monitor_value_range_[5] = 1e+8;
								}
							}
							if ((monitor_value_range_[2] <= 0.0 || monitor_value_range_[4] <= 0.0) && RB_GraphLogScale->IsChecked) {
								//linear graph for negative value
								RB_GraphLinearScale->IsChecked = true;
								RB_GraphLogScale->IsChecked = false;
							}
						}

						for (int i = 1 ; i < 3 ; i++) {
							monitor_value_range_[i * 2 + 1] =
								(monitor_value_range_[i * 2 + 1] - monitor_value_range_[i * 2 + 0]) / pow(2.0, monitor_zoom_ * 0.5) + monitor_value_range_[i * 2 + 0];
							if (RB_GraphLogScale->IsChecked){
								if (monitor_value_range_[i * 2 + 0] > 0.0 && monitor_value_range_[i * 2 + 1] > 0.0) {
									if (monitor_value_range_[i * 2 + 0] < monitor_value_range_[i * 2 + 1]) {
										const double log_mpx = log(monitor_value_range_[i * 2 + 1] / monitor_value_range_[i * 2 + 0]);
										monitor_value_range_[i * 2 + 0] /= exp(log_mpx / 20);
										monitor_value_range_[i * 2 + 1] *= exp(log_mpx / 20);
									}
									const double log_mpx = log(monitor_value_range_[i * 2 + 1] / monitor_value_range_[i * 2 + 0]);
									if (log_mpx < log(10.0)) {
										const double center = log(monitor_value_range_[i * 2 + 0] * monitor_value_range_[i * 2 + 1]) * 0.5;
										monitor_value_range_[i * 2 + 0] = exp(center - log(10.0) * 0.5);
										monitor_value_range_[i * 2 + 1] = exp(center + log(10.0) * 0.5);
									}
								}else {
									monitor_value_range_[i * 2 + 0] = 0.01;
									monitor_value_range_[i * 2 + 1] = 0.1;
								}
							}else {
								const int clip_at_zero = monitor_value_range_[i * 2 + 0] >= 0.0;
								if (monitor_value_range_[i * 2 + 0] < monitor_value_range_[i * 2 + 1]) {
									const double diff = monitor_value_range_[i * 2 + 1] - monitor_value_range_[i * 2 + 0];
									monitor_value_range_[i * 2 + 0] -= diff / 20;
									monitor_value_range_[i * 2 + 1] += diff / 20;
								}
								if (monitor_value_range_[i * 2 + 1] - monitor_value_range_[i * 2 + 0] < 0.02) {
									const double center = (monitor_value_range_[i * 2 + 0] + monitor_value_range_[i * 2 + 1]) * 0.5;
									monitor_value_range_[i * 2 + 0] = center - 0.01;
									monitor_value_range_[i * 2 + 1] = center + 0.01;
								}
								if (monitor_value_range_[i * 2 + 0] < 0.0 && clip_at_zero) {
									monitor_value_range_[i * 2 + 0] = 0.0;
								}
							}
						}
					}
					if (monitor_csv_[0].GetNumData() || monitor_csv_[1].GetNumData()) {
						//Graph range transition
						for (int i = 0 ; i < 3 ; i++) {
							if (monitor_current_value_range_[i * 2 + 0] == monitor_current_value_range_[i * 2 + 1] ||//min == max
								log(monitor_current_value_range_[i * 2 + 0]) - log(monitor_value_range_[i * 2 + 0]) > 9.21 ||// > 10000x
                                log(monitor_current_value_range_[i * 2 + 1]) - log(monitor_value_range_[i * 2 + 1]) > 9.21) {
								for (int i2 = 0 ; i2 < 2 ; i2++) {
									monitor_current_value_range_[i * 2 + i2] = monitor_value_range_[i * 2 + i2];
								}
								sum_diff += 1.0;
							}else{
								const double sum_diff_den = (i > 0 ? PB_Monitor->Height : PB_Monitor->Width) * 10.0 / (monitor_value_range_[i * 2 + 1] - monitor_value_range_[i * 2 + 0]);
								for (int i2 = 0 ; i2 < 2 ; i2++) {
									const double last = monitor_current_value_range_[i * 2 + i2];
									monitor_current_value_range_[i * 2 + i2] = (1.0 - sdeep_console_transition_coef_) * monitor_current_value_range_[i * 2 + i2] + sdeep_console_transition_coef_ * monitor_value_range_[i * 2 + i2];
									sum_diff += fabs(last - monitor_current_value_range_[i * 2 + i2]) * sum_diff_den;
								}
							}
						}
					}
				}else if (RB_ResultsTradeoffGraph->IsChecked) {
					//Tradeoff
					if (result_tradeoff_update_count_ != tradeoff_csv_.GetUpdateCount()) {
						memset(result_tradeoff_value_range_, 0, sizeof(result_tradeoff_value_range_));
						sum_diff += 1.0;

						result_tradeoff_update_count_ = tradeoff_csv_.GetUpdateCount();

						//Check graph range
						tradeoff_csv_.GetColumnMinMax(result_tradeoff_h_statistics_str_, result_tradeoff_value_range_[0], result_tradeoff_value_range_[1]);
						if (MIMV_TrainingValidation->IsChecked || MIMV_Training->IsChecked) {
							tradeoff_csv_.GetColumnMinMax(FString("terr"), result_tradeoff_value_range_[2], result_tradeoff_value_range_[3]);
						}
						if (MIMV_TrainingValidation->IsChecked || MIMV_Validation->IsChecked) {
							tradeoff_csv_.GetColumnMinMax(FString("verr"), result_tradeoff_value_range_[2], result_tradeoff_value_range_[3]);
						}
						if (result_tradeoff_value_range_[2] <= 0.0 && RB_GraphLogScale->IsChecked) {
							//linear graph for negative value
							RB_GraphLinearScale->IsChecked = true;
							RB_GraphLogScale->IsChecked = false;
						}

						if (result_tradeoff_value_range_[0] > 0.0 && result_tradeoff_value_range_[1] > 0.0) {
							if (result_tradeoff_value_range_[0] < result_tradeoff_value_range_[1]) {
								const double log_mpx = log(result_tradeoff_value_range_[1] / result_tradeoff_value_range_[0]);
								result_tradeoff_value_range_[0] /= exp(log_mpx / 20);
								result_tradeoff_value_range_[1] *= exp(log_mpx / 20);
							}
							const double log_mpx = log(result_tradeoff_value_range_[1] / result_tradeoff_value_range_[0]);
							if (log_mpx < log(10.0)) {
								const double center = log(result_tradeoff_value_range_[0] * result_tradeoff_value_range_[1]) * 0.5;
								result_tradeoff_value_range_[0] = exp(center - log(10.0) * 0.5);
								result_tradeoff_value_range_[1] = exp(center + log(10.0) * 0.5);
							}
						}else {
							result_tradeoff_value_range_[0] = 1.0;
							result_tradeoff_value_range_[1] = 10.0;
						}
						result_tradeoff_value_range_[3] =
							(result_tradeoff_value_range_[3] - result_tradeoff_value_range_[2]) / pow(2.0, result_tradeoff_zoom_ * 0.5) + result_tradeoff_value_range_[2];
						if (RB_GraphLogScale->IsChecked){
							if (result_tradeoff_value_range_[2] > 0.0 && result_tradeoff_value_range_[3] > 0.0) {
								if (result_tradeoff_value_range_[2] < result_tradeoff_value_range_[3]) {
									const double log_mpx = log(result_tradeoff_value_range_[3] / result_tradeoff_value_range_[2]);
									result_tradeoff_value_range_[2] /= exp(log_mpx / 20);
									result_tradeoff_value_range_[3] *= exp(log_mpx / 20);
								}
								const double log_mpx = log(result_tradeoff_value_range_[3] / result_tradeoff_value_range_[2]);
								if (log_mpx < log(10.0)) {
									const double center = log(result_tradeoff_value_range_[2] * result_tradeoff_value_range_[3]) * 0.5;
									result_tradeoff_value_range_[2] = exp(center - log(10.0) * 0.5);
									result_tradeoff_value_range_[3] = exp(center + log(10.0) * 0.5);
								}
							}else {
								result_tradeoff_value_range_[2] = 0.01;
								result_tradeoff_value_range_[3] = 0.1;
							}
						}else {
							const int clip_at_zero = result_tradeoff_value_range_[2] > 0.0;
							if (result_tradeoff_value_range_[2] < result_tradeoff_value_range_[3]) {
								const double diff = result_tradeoff_value_range_[3] - result_tradeoff_value_range_[2];
								result_tradeoff_value_range_[2] -= diff / 20;
								result_tradeoff_value_range_[3] += diff / 20;
							}
							if (result_tradeoff_value_range_[3] - result_tradeoff_value_range_[2] < 0.02) {
								const double center = (result_tradeoff_value_range_[2] + result_tradeoff_value_range_[3]) * 0.5;
								result_tradeoff_value_range_[2] = center - 0.01;
								result_tradeoff_value_range_[3] = center + 0.01;
							}
							if (result_tradeoff_value_range_[2] < 0.0 && clip_at_zero) {
								result_tradeoff_value_range_[2] = 0.0;
							}
						}
					}

					if (tradeoff_csv_.GetNumData()) {
						//Graph range transition
						for (int i = 0 ; i < 2 ; i++) {
							if (result_current_tradeoff_value_range_[i * 2 + 0] == result_current_tradeoff_value_range_[i * 2 + 1]) {
								for (int i2 = 0 ; i2 < 2 ; i2++) {
									result_current_tradeoff_value_range_[i * 2 + i2] = result_tradeoff_value_range_[i * 2 + i2];
								}
								sum_diff += 1.0;
							}else{
								const double sum_diff_den = (i > 0 ? PB_Monitor->Height : PB_Monitor->Width) * 10.0 / (result_current_tradeoff_value_range_[i * 2 + 1] - result_current_tradeoff_value_range_[i * 2 + 0]);
								for (int i2 = 0 ; i2 < 2 ; i2++) {
									const double last = monitor_current_value_range_[i * 2 + i2];
									result_current_tradeoff_value_range_[i * 2 + i2] = (1.0 - sdeep_console_transition_coef_) * result_current_tradeoff_value_range_[i * 2 + i2] + sdeep_console_transition_coef_ * result_tradeoff_value_range_[i * 2 + i2];
									sum_diff += fabs(last - result_current_tradeoff_value_range_[i * 2 + i2]) * sum_diff_den;
								}
							}
						}
					}
				}else if (RB_ResultsWeight->IsChecked) {
					//Weight Graph
					const double weight_last_zoom = result_weight_current_zoom_;
					result_weight_current_zoom_ = (1.0 - sdeep_console_transition_coef_) * result_weight_current_zoom_ + sdeep_console_transition_coef_ * result_weight_zoom_;
					sum_diff += fabs(pow(2.0, weight_last_zoom * 0.5) - pow(2.0, result_weight_current_zoom_ * 0.5)) * (PB_Monitor->Width > PB_Monitor->Height ? PB_Monitor->Width : PB_Monitor->Height);

					const double last_top = result_weight_current_top_;
					result_weight_current_top_ = (1.0 - sdeep_console_transition_coef_) * result_weight_current_top_ + sdeep_console_transition_coef_ * result_weight_top_;
					sum_diff += fabs(last_top - result_weight_current_top_);
				}
				if (Sc_Monitor->Visible != Sc_Monitor->Max > 0 && RB_ResultsWeight->IsChecked) {
					Sc_Monitor->Visible = Sc_Monitor->Max > 0 && RB_ResultsWeight->IsChecked;
				}
				if (Sc_Monitor->Visible && (Sc_Monitor->Tag == 0)) {
					Sc_Monitor->Tag = -1;
					Sc_Monitor->Value = result_weight_current_top_;
					Sc_Monitor->Tag = 0;
				}
				if (Lo_MonitorGain->Visible != RB_ResultsWeight->IsChecked) {
					Lo_MonitorGain->Visible = RB_ResultsWeight->IsChecked;
				}
				if (sum_diff > 0.5) {
					PB_Monitor->Repaint();
				}
			}

			//Log
			int show_solve_button = false;
			if (Me_Monitor->Lines->Count > 1) {
				UnicodeString s = Me_Monitor->Lines->Strings[Me_Monitor->Lines->Count - 2];
				const int p = s.Pos("To manually release the lock, delete ");
				if (p) {
					UnicodeString dir_name = s.SubString(p + 37, s.Length());
					if (DirectoryExists(dir_name)) {
						show_solve_button = true;
						training_monitor_solve_dir_ = dir_name;
						Bu_MonitorSolve->Position->X = Me_Monitor->Width - Bu_MonitorSolve->Width - 32;
						Bu_MonitorSolve->Position->Y = Me_Monitor->Height - Bu_MonitorSolve->Height - 32;
					}
				}
			}
			SetControlVisible(Bu_MonitorSolve, show_solve_button);

		}
	}

	//Evaluation Tab
	if (!nn_document_.GetLocked()) {
		SetControlEnabled(Co_EvaluationConfusionMatrixModal, RB_EvaluationConfusionMatrix->IsChecked);
		SetControlEnabled(Co_EvaluationOthers, RB_EvaluationOthers->IsChecked);

		if (sdeep_evaluation_thread_) {
			if (sdeep_evaluation_thread_->GetProcessCreated() && animation_indicator_active_) {
				HideAnimationIndicator();
			}
			if (sdeep_evaluation_thread_->GetNumLatestLog()){
				while (sdeep_evaluation_thread_->GetNumLatestLog()) {
					FString s = sdeep_evaluation_thread_->GetLatestLog();
					//Me_Monitor->Lines->Add(Trim(s.PChar()));
				}
				if (sdeep_evaluation_thread_->GetName() == nn_document_.GetCurrentResultName()) {
					OpenEvaluationResult(false, false);
				}
			}

			static TDateTime last = 0.0;
			const TDateTime now = Now();
			if (last < now - 1.0 / (24 * 60 * 60 * 10)) {//100 ms
				last = now;
				UnicodeString last_total_time = Ed_EvaluationTimeElapsed->Text;
				RefreshEvaluationTimeFromLog();

				//State
				if (last_total_time != Ed_EvaluationTimeElapsed->Text && Me_Evaluation->Lines->Count) {
					UnicodeString result_dir = GetResultDir();
					UnicodeString state_file_name = result_dir + UnicodeString("\\progress.txt");
					struct stat stat_buf;
					if (stat(AnsiString(state_file_name).c_str(), &stat_buf) == 0) {
						static int state_date = 0;
						if (state_date != stat_buf.st_mtime){
							FStringList sl;
							if (sl.LoadFromFile(AnsiString(state_file_name).c_str())){
								Me_Evaluation->Lines->Strings[Me_Evaluation->Lines->Count - 1] = sl.GetText().PChar();
								state_date = stat_buf.st_mtime;
							}
						}
					}
				}
			}

			if (sdeep_evaluation_thread_->Finished) {
				const int stopped = sdeep_evaluation_thread_->GetStopped();
				const int auto_evaluation = sdeep_evaluation_thread_->GetAutoEvaluation();
				if (!stopped) {
					if (sdeep_evaluation_thread_->GetSuccessfullyFinished()) {
						Log(FString("evaluation ends"));
					}else {
						Log(FString("evaluation ends with an error"));
						LogTraceback(Me_Evaluation->Lines);
					}
				}

				delete sdeep_evaluation_thread_;
				sdeep_evaluation_thread_ = NULL;
				RefreshEvaluationTimeFromLog();

				nn_document_.RefreshResults(FString("\\"));
				if (TC_Left->ActiveTab == TI_TrainingLeft) {
					//Back result index from last to current
					if (Fr_ResultsList->GetItemIndex() < nn_document_.GetResults().GetSize()){
						nn_document_.SetCurrentResultIndex(Fr_ResultsList->GetItemIndex());
					}
					if (TC_Default->Visible) {
						RefreshResultsList(false);
					}
				}
				OpenEvaluationResult(true, TC_Default->ActiveTab == TI_Evaluation);

				if (!stopped && auto_evaluation && nn_document_.GetAutoEvaluation()) {
					if (TrainNext()) {
                        return;
					}
				}
			}

			//Log
			int show_solve_button = false;
			if (Me_Evaluation->Lines->Count > 1) {
				UnicodeString s = Me_Evaluation->Lines->Strings[Me_Evaluation->Lines->Count - 2];
				const int p = s.Pos("To manually release the lock, delete ");
				if (p) {
					UnicodeString dir_name = s.SubString(p + 37, s.Length());
					if (DirectoryExists(dir_name)) {
						show_solve_button = true;
						evaluation_solve_dir_ = dir_name;
						Bu_EvaluationSolve->Position->X = Me_Evaluation->Width - Bu_MonitorSolve->Width - 32;
						Bu_EvaluationSolve->Position->Y = Me_Evaluation->Height - Bu_MonitorSolve->Height - 32;
					}
				}
			}
			SetControlVisible(Bu_EvaluationSolve, show_solve_button);
		}
	}

	//Right bar
	{
		//Overview Network
		SetControlEnabled(MIRN_OpenInNetworkTab, MILR_RetrainNotInPlace->Enabled);
		SetControlEnabled(MIRN_OpenInNetworkTabWithWeight, MILR_RetrainNotInPlace->Enabled);
		SetControlEnabled(MILR_OpenInNetworkTab, MILR_RetrainNotInPlace->Enabled);
		SetControlEnabled(MILR_OpenInNetworkTabWithWeight, MILR_RetrainNotInPlace->Enabled);
		SetControlEnabled(MIM_OpenInNetworkTab, MILR_RetrainNotInPlace->Enabled);
		SetControlEnabled(MIM_OpenInNetworkTabWithWeight, MILR_RetrainNotInPlace->Enabled);
		PB_NetworkOverview->PopupMenu = TC_Default->ActiveTab == TI_Results ? PM_ResultNetwork : NULL;

		const int update_count = network_overview_ ? network_overview_->GetViewUpdateCount() : -1;

		double sum_diff = update_count_from_network_tab;
		if (network_overview_update_count_ != update_count && network_overview_structure_index_ >= 0) {
			if (network_overview_){
                network_overview_->SetRelatedNetworks();
				result_graph_mapper_.Update(*network_overview_, false);
			}else {
				static NNStructures empty_network;
				result_graph_mapper_.Update(empty_network, false);
			}
			network_overview_update_count_ = update_count;
			sum_diff += 1.0;
		}

        FString overview_name;
		if (network_overview_){
			network_overview_zoom_ = 0.75;
			if (network_overview_->GetNetworkViewSizeX() * network_overview_zoom_ > PB_NetworkOverview->Width) {
				network_overview_zoom_ = PB_NetworkOverview->Width / network_overview_->GetNetworkViewSizeX();
			}
			if (network_overview_->GetNetworkViewSizeY() * network_overview_zoom_ > PB_NetworkOverview->Height) {
				network_overview_zoom_ = (PB_NetworkOverview->Height) / network_overview_->GetNetworkViewSizeY();
			}

			if (network_overview_structure_index_ >= 0){
				const double network_last_zoom = network_overview_current_zoom_;
				network_overview_current_zoom_ = (1.0 - sdeep_console_transition_coef_) * network_overview_current_zoom_ + sdeep_console_transition_coef_ * network_overview_zoom_;
				sum_diff += fabs(network_last_zoom - network_overview_current_zoom_) * (PB_NetworkOverview->Width > PB_NetworkOverview->Height ? PB_NetworkOverview->Width : PB_NetworkOverview->Height);

				if (network_overview_->GetNumStructure() > 1){
					overview_name +=
						FString("(") +
						FString(network_overview_->GetActiveStructureIndex() + 1) +
						FString("/") +
						FString(network_overview_->GetNumStructure()) +
						FString(")");
				}
				overview_name += FString(": ") + network_overview_->GetName();
			}else {
				//network_overview_structure_index_ == -1
				overview_name += FString(": ") + GetTranslatedFString("Config");
			}
		}
		SetControlVisible(Bu_OverviewPrevious, network_overview_ != NULL);
		SetControlVisible(Bu_OverviewNext, network_overview_ != NULL);

		overview_name = GetTranslatedFString("Overview") + overview_name;
		if (La_Overview->Text != overview_name.PChar()){
			La_Overview->Text = overview_name.PChar();
		}

		SetControlVisible(Lo_OverviewTop, !Lo_DataPreview->Visible);
		SetControlVisible(Lo_Overview, network_overview_structure_index_ >= 0 && !Lo_DataPreview->Visible);
		SetControlVisible(Lo_OverviewConfig, network_overview_structure_index_ < 0 && !Lo_DataPreview->Visible);
		if (network_overview_structure_index_ >= 0){
			sum_diff += result_graph_mapper_.Animation(sdeep_console_transition_coef_);
			if (TC_Default->Visible && sum_diff > 0.5) {
				PB_NetworkOverview->Repaint();
				PB_Statistics->Repaint();
			}
		}else {
			//Config
			const FString config_text = network_overview_ ? network_overview_->GetConfigText() : FString("");
			if (last_config_text_ != config_text.PChar()){
				last_config_text_ = config_text;
				Me_OverviewConfig->Text = config_text.PChar();
			}
		}

		//Statistics
		SetControlVisible(Lo_Statistics, is_trainable);

		//Progress
		FString training_name = FString("----");
		int training_max = 1;
		int training_current = 0;
		if (sdeep_training_thread_.GetSize()) {
			const int thread_index = (now / 1000) % sdeep_training_thread_.GetSize();
			training_name = sdeep_training_thread_.GetAt(thread_index)->GetName();
			training_max = sdeep_training_thread_.GetAt(thread_index)->GetMaxEpoch();
			training_current = sdeep_training_thread_.GetAt(thread_index)->GetLastEpoch();
		}
		if (TC_Left->ActiveTab == TI_TrainingLeft){
			training_name = nn_document_.GetCurrentResultName();
			training_max = progress_training_max_;
			training_current = progress_training_current_;
		}
		if (La_TaskTrainingJob->Text != training_name.PChar()){
			La_TaskTrainingJob->Text = training_name.PChar();
		}
		if (PB_TrainingEpoch->Max != training_max) {
			PB_TrainingEpoch->Max = training_max;
		}
		if (PB_TrainingEpoch->Value != training_current) {
			PB_TrainingEpoch->Value = training_current;
		}

		FString evaluation_name = FString("----");
		int evaluation_max = 1;
		int evaluation_current = 0;
		if (sdeep_evaluation_thread_) {
			evaluation_name = sdeep_evaluation_thread_->GetName();
			evaluation_max = sdeep_evaluation_thread_->GetDataNum();
			evaluation_current = sdeep_evaluation_thread_->GetLastData();
		}
		if (TC_Left->ActiveTab == TI_TrainingLeft){
			evaluation_name = nn_document_.GetCurrentResultName();
			evaluation_max = progress_evaluation_max_;
			evaluation_current = progress_evaluation_current_;
		}
		if (La_TaskEvaluationJob->Text != evaluation_name.PChar()){
			La_TaskEvaluationJob->Text = evaluation_name.PChar();
		}
		if (PB_EvaluationData->Max != evaluation_max) {
			PB_EvaluationData->Max = evaluation_max;
		}
		if (PB_EvaluationData->Value != evaluation_current) {
			PB_EvaluationData->Value = evaluation_current;
		}

	}

	static int startup_sqeuence = 0;
	if (startup_sqeuence == 0 && Sender == Ti_Animation){
		startup_sqeuence = 1;
		settings_.SaveToIni(setting_file_);
		startup_sqeuence = 2;
		return;
	}

	if (startup_sqeuence == 2 && Sender == Ti_Animation){
		startup_sqeuence = 3;
		RefreshPlugins();
		startup_sqeuence = 4;
		return;
	}

	if (startup_sqeuence == 4 && Sender == Ti_Animation && !nn_document_.GetLocked()){
		startup_sqeuence = 5;
		if (ParamCount() > 0){
			for (int ip = 1 ; ip <= ParamCount() ; ip++){
				if (ParamStr(ip).LowerCase().Pos(".sdcproj")){
					Bu_NetworkTabClick(this);
					NewProject(SDeepEngineType::NNabla);
					SetActivity(SDeepConsoleActivity::Default);
					OpenProject(ParamStr(ip));
					TC_DefaultChange(Sender);//to set network_overview_
					break;
				}
			}
		}
		startup_sqeuence = 6;
		return;
	}
	if (setup_request_) {
		setup_request_ = false;
		Bu_SetupClick(Sender);
	}
	if (move_to_eval_tab) {
        Bu_EvaluationTabClick(Sender);
	}
}
//---------------------------------------------------------------------------
const CSVDataset &TFo_Main::GetCurrentTable()
{
	int result_csv = false;
	int csv_index = 0;
	if (TC_Default->ActiveTab == TI_Evaluation) {
		result_csv = true;
		if (RB_EvaluationConfusionMatrix->IsChecked){
			csv_index = 1;
		}else if (RB_EvaluationOthers->IsChecked){
			csv_index = 2;
		}else {
			csv_index = 0;
		}
	}else if (TC_Default->ActiveTab == TI_Browser && (Fr_AISolutions->Visible || Fr_NNablaExamples->Visible || (Fr_DatasetFiles->Visible && Re_RightBlankTop->Visible))) {//NNablaExamples & Dataset plugin results
		result_csv = true;
		csv_index = 2;
	}else {//Dataset
		if (activity_ == SDeepConsoleActivity::Default && show_dataset_plugin_result_){
			result_csv = true;
			csv_index = 2;
		}else{
			result_csv = false;
			csv_index = Fr_DatasetList->GetItemIndex() >=0 ? Fr_DatasetList->GetItemIndex() : 0;
        }
	}
	return result_csv ? nn_document_.GetResultCSV(csv_index) : nn_document_.GetDataset(csv_index);
}
//---------------------------------------------------------------------------
TableUIState &TFo_Main::GetCurrentTableUIState()
{
	const CSVDataset &dataset = GetCurrentTable();
	const int index = table_ptrs_.IndexOf(&dataset);
	if (index >= 0){
		return table_ui_state_.GetAt(index);
	}else {
		table_ptrs_.Add(&dataset);
		TableUIState &new_state = table_ui_state_.Add();
		new_state.left = new_state.top = new_state.current_left = new_state.current_top = 0.0;
		new_state.col_index = new_state.row_index = 0;
		new_state.cols_in_screen = 1;
		new_state.rows_in_screen = 1;
		new_state.cursor_rect = TRectD(0.0, 0.0, 0.0, 0.0);
		new_state.current_cursor_rect = TRectD(0.0, 0.0, 0.0, 0.0);
		return new_state;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ContinueStructureSearch()
{
	//Enum pareto optimal
	const FListP<NNTrainingResult> &results = nn_document_.GetResults();
	FListT<int> pareto_optimal_index_seed, pareto_optimal_index_all_sorted;
	FListP<NNStructures> result_structures;
	for (int ie = 0 ; ie < results.GetSize() ; ie++) {
		const NNTrainingResult &result = results.GetAt(ie);
		NNStructures &structures = result_structures.Add();
		nn_document_.SetCurrentResultIndex(ie);
		AnsiString network_file_name = GetResultDir() + UnicodeString("\\network");
		OpenNetworkFromIni(structures, network_file_name);

		if (result.GetParetoOptimal()){
			pareto_optimal_index_all_sorted.Add(ie);
			int add = true;

			const double validation_min = nn_document_.GetStructureSearchConstraint(FString("Validation"), 0);
			if (validation_min >= 0.0) {
				add &= result.GetBestValidation() >= validation_min;
			}
			const double validation_max = nn_document_.GetStructureSearchConstraint(FString("Validation"), 1);
			if (validation_max >= 0.0) {
				add &= result.GetBestValidation() <= validation_max;
			}

			const double cost_multiply_add_min = nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 0);
			if (cost_multiply_add_min >= 0) {
				add &= result.GetCostMultiplyAdd() >= cost_multiply_add_min;
			}
			const double cost_multiply_add_max = nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 1);
			if (cost_multiply_add_max >= 0) {
				add &= result.GetCostMultiplyAdd() <= cost_multiply_add_max;
			}

			if (add) {
				pareto_optimal_index_seed.Add(ie);
			}
		}
	}

	//Sort Pareto Optimal by CostMultiplyAdd
	for (int i = 1 ; i < pareto_optimal_index_all_sorted.GetSize() ; i++) {
		for (int i2 = i; i2 > 0 ; i2--) {
			if (results.GetAt(pareto_optimal_index_all_sorted.GetAt(i2)).GetCostMultiplyAdd() <
				results.GetAt(pareto_optimal_index_all_sorted.GetAt(i2 - 1)).GetCostMultiplyAdd()) {
				pareto_optimal_index_all_sorted.Swap(i2, i2 - 1);
			}else {
				break;
			}
		}
	}

	nn_document_.KeepForUndo(FString("Mutate"));
	int result = true;
	switch (nn_document_.GetStructureSearchMethod()) {
	case StructureSearchMethod::Random:
		MutateNetworkForStructureSearch_Random(result_structures, pareto_optimal_index_all_sorted, pareto_optimal_index_seed);
		break;
	case StructureSearchMethod::NetworkFeatureAndGaussianProcess:
	case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticProb:
	case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticArea:
		result = MutateNetworkForStructureSearch_NetworkFeatureAndGaussianProcess(result_structures, pareto_optimal_index_all_sorted, pareto_optimal_index_seed);
		break;
	default:
		;
	}

	if (result){
        control_button_type_ = ControlButtonType::RunTraining;
		Bu_ControlClick(this);

		//Search similar cost network for early stopping (must open comparison result after RunTraining)
		if (nn_document_.GetEarlyStopping()) {
			const FListP<NNTrainingResult> &results = nn_document_.GetResults();
			int result_index = -1;
			int max_cost_multiply_add = -1;
			const int sum_multiply_add = nn_document_.GetStatistics(FString("CostMultiplyAdd"))->GetSum();
			for (int i = 0 ; i < results.GetSize() ; i++) {
				const NNTrainingResult &result = results.GetAt(i);
				if (result.GetParetoOptimal()) {
					//find smaller than current, and larger than others
					if (result.GetCostMultiplyAdd() <= sum_multiply_add &&
						(max_cost_multiply_add < 0 || result.GetCostMultiplyAdd() > max_cost_multiply_add)) {
						result_index = i;
						max_cost_multiply_add = result.GetCostMultiplyAdd();
					}
				}
			}
			if (result_index >= 0) {
				OpenResult(result_index, true);
			}
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::MutateNetworkForStructureSearch_Random(const FListP<NNStructures> &result_structures, const FListT<int> &pareto_optimal_index_all_sorted, const FListT<int> &pareto_optimal_index_seed)
{
	const FListP<NNTrainingResult> &results = nn_document_.GetResults();

	//Open seed network
	if (pareto_optimal_index_seed.GetSize()) {
		const int seed_result = rand() % pareto_optimal_index_seed.GetSize();
		const int seed_net_index = pareto_optimal_index_seed.GetAt(seed_result);
		NNStructures seed_net;
		seed_net = result_structures.GetAt(seed_net_index);
		nn_document_.NNStructures::operator=(seed_net);
	}

	//Mutate and create unique network
	int trial = 0;
	while (trial < 100){
		NNStructures backup;
		backup = nn_document_;
		nn_document_.Mutate();
		nn_document_.CompleteNetworks(nn_document_);

		const double cost_multiply_add = nn_document_.GetRuntimeNetworkStatistics(FString("CostMultiplyAdd"));
		const double cost_multiply_add_min = nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 0);
		if (cost_multiply_add_min >= 0) {
			if (cost_multiply_add < cost_multiply_add_min){
				nn_document_.NNStructures::operator=(backup);
				trial++;
				continue;
			}
		}
		const double cost_multiply_add_max = nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 1);
		if (cost_multiply_add_max >= 0) {
			if (cost_multiply_add > cost_multiply_add_max){
				nn_document_.NNStructures::operator=(backup);
				trial++;
				continue;
			}
		}

		int same_found = false;

		for (int ir = 0; ir < results.GetSize() && !same_found ; ir++) {
			same_found = nn_document_.IsSame(result_structures.GetAt(ir));
		}

		if (same_found) {
			nn_document_.NNStructures::operator=(backup);
			trial++;
			continue;
		}
		break;
	}
	nn_document_.SetStructureSearchExpectiation(0.0, 0.0);
}
//---------------------------------------------------------------------------
double erf_(double x)
{
	// constants
	double a1 =  0.254829592;
	double a2 = -0.284496736;
	double a3 =  1.421413741;
	double a4 = -1.453152027;
	double a5 =  1.061405429;
	double p  =  0.3275911;

	// Save the sign of x
	int sign = 1;
	if (x < 0)
		sign = -1;
	x = fabs(x);

	// A&S formula 7.1.26
	double t = 1.0/(1.0 + p*x);
	double y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

	return sign*y;
}

double p(double myu, double var, double a, double b)
{
	if (a == b) {
		return 0.0;
	}
	if (a > b) {
		double d = a;
		a = b;
		b = d;
	}

	a = (a - myu) / sqrt(2 * var);
	b = (b - myu) / sqrt(2 * var);

	if (a > 0.0 && b > 0.0) {
		return (erf_(b) - erf_(a)) / 2;
	}else if (a <0.0 && b > 0.0) {
		return (erf_(b) + erf_(-a)) / 2;
	}else {
		return (erf_(-a) - erf_(-b)) / 2;
	}
}

int TFo_Main::MutateNetworkForStructureSearch_NetworkFeatureAndGaussianProcess(const FListP<NNStructures> &result_structures, const FListT<int> &pareto_optimal_index_all_sorted, const FListT<int> &pareto_optimal_index_seed)
{
	const FListP<NNTrainingResult> &results = nn_document_.GetResults();

	if (online_debug_mode_) {
		TI_Debug->Visible = true;
		SetActiveTab(TC_Default, TI_Debug);
		//Me_Debug->Lines->Clear();
	}

	//Prepare Gaussian Process
	//Setup
	static const double exploration_sigma = 1.0;
	static const double max_error_ratio = 3.0;

	//Extract feature from result networks
	if (online_debug_mode_) {
		Me_Debug->Lines->Add("Extract feature from result networks");
		Me_Debug->Lines->Add(DateTimeToStr(Now()));
	}
	CSVDataset error_and_feature;
	int index = 0;
	for (int ir = 0 ; ir < results.GetSize() ; ir++) {
		//Feature
		FListT<float> result;
		FStringList result_caption;
		NNStructureToVector(result_structures.GetAt(ir), result, result_caption, nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla);

		if (index == 0) {
			result_caption.Insert(0, FString("error"));
			error_and_feature.SetColumnNames(result_caption);
		}

		int same_index = -1;
		for (int i2 = 0 ; i2 < index ; i2++) {
			int same = true;
			for (int id = 0 ; id < result.GetSize() && same ; id++) {
				same &= fabs(error_and_feature.GetFloatValue(i2, id + 1) - result.GetAt(id)) < 0.000001;
			}
			if (same) {
				same_index = i2;
				break;
			}
		}

		if (same_index < 0){
			if (results.GetAt(ir).GetBestValidation() < NAN_VALUE_THRESHOLD) {
				//Error
				if (results.GetAt(ir).GetComplete()) {
					error_and_feature.SetValue(index, 0, FString(log(results.GetAt(ir).GetBestValidation())));
				}
			}
			//Feature
			for (int id = 0 ; id < result.GetSize() ; id++) {
				error_and_feature.SetValue(index, id + 1, FString(result.GetAt(id)));
			}
			index++;
		}
	}
	const int training_num = index;
	if (!training_num) {
		MutateNetworkForStructureSearch_Random(result_structures, pareto_optimal_index_all_sorted, pareto_optimal_index_seed);
		return true;
	}

	ShowAnimationIndicator(GetTranslatedText("Creating next structure to search..."), false, true);

	//Create Candidates
	if (online_debug_mode_) {
		Me_Debug->Lines->Add("Create Candidates");
		Me_Debug->Lines->Add(DateTimeToStr(Now()));
	}
	FListP<NNStructures> candidates;
	const int num_candidate = 2000;
	for (int it = 0 ; it < num_candidate && !animation_indicator_abort_ ; it++) {
		//Open seed network
		if (it < num_candidate / 2 && pareto_optimal_index_seed.GetSize()) {
			const int seed_result = rand() % pareto_optimal_index_seed.GetSize();
			nn_document_.NNStructures::operator =(result_structures.GetAt(pareto_optimal_index_seed.GetAt(seed_result)));
		}else {
			const int seed_result = rand() % result_structures.GetSize();
			nn_document_.NNStructures::operator =(result_structures.GetAt(seed_result));
		}
		nn_document_.Mutate();
		nn_document_.CompleteNetworks(nn_document_);

		const double cost_multiply_add = nn_document_.GetRuntimeNetworkStatistics(FString("CostMultiplyAdd"));
		const double cost_multiply_add_min = nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 0);
		if (cost_multiply_add_min >= 0) {
			if (cost_multiply_add < cost_multiply_add_min){
				continue;
			}
		}
		const double cost_multiply_add_max = nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 1);
		if (cost_multiply_add_max >= 0) {
			if (cost_multiply_add > cost_multiply_add_max){
				continue;
			}
		}

		int same_found = false;
		for (int ir = 0; ir < results.GetSize() && !same_found ; ir++) {
			same_found = nn_document_.IsSame(result_structures.GetAt(ir));
		}

		if (!same_found) {
			//Estimate possible error
			//Feature
			FListT<float> result;
			FStringList result_caption;
			NNStructureToVector(nn_document_, result, result_caption, nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla);

			//Feature
			for (int id = 0 ; id < result.GetSize() ; id++) {
				error_and_feature.SetValue(index, id + 1, FString(result.GetAt(id)));
			}

			candidates.Add(nn_document_);
			index++;
		}
		ProcessMessages();
	}
	if (animation_indicator_abort_){
		HideAnimationIndicator();
		return false;
	}
	assert(training_num + candidates.GetSize() == index);

	//Select effective col
	FListT<int> effective_col;
	for (int ic = error_and_feature.GetNumColumn() - 1 ; ic >= 1 ; ic--) {
		double max = 0.0, min = 0.0;
		error_and_feature.GetColumnMinMax(ic, min, max);
		if (min == max && error_and_feature.GetNumColumn() > 2) {
			error_and_feature.DeleteCol(ic);
		}else {
			int correl_col = -1;
			for (int ic2 = error_and_feature.GetNumColumn() - 1 ; ic2 > ic && correl_col < 0 ; ic2--) {
				if (error_and_feature.GetColumnCorrel(ic2, ic) > 0.99) {
					error_and_feature.DeleteCol(ic2);
					correl_col = ic2;
				}
			}
			if (correl_col < 0) {
				effective_col.Add(ic - 1);
			}
		}
		ProcessMessages();
	}
	//error_and_feature.SaveToFile(AnsiString(ExtractFilePath(ParamStr(0)) + "test.csv").c_str());
	effective_col.Sort();
	assert(effective_col.GetSize() + 1 == error_and_feature.GetNumColumn());
	if (online_debug_mode_) {
		Me_Debug->Lines->Add(UnicodeString("num result networks : ") + IntToStr(training_num));
		Me_Debug->Lines->Add(UnicodeString("num effective error and feature : ") + IntToStr(error_and_feature.GetNumColumn()));
	}
	/*
	FString features, values;
	for (int id = 0 ; id < effective_col.GetSize() ; id++) {
		if (id > 0) {
			features += ",";
			values += ",";
		}
		features += error_and_feature.GetColumnName(id + 1);
		values += error_and_feature.GetValue(0, id + 1);
	}
	Me_Debug->Lines->Add(features.PChar());
	Me_Debug->Lines->Add(values.PChar());
	*/

	//Train Gaussian Process
	if (online_debug_mode_) {
		Me_Debug->Lines->Add("Train Gaussian Process");
		Me_Debug->Lines->Add(DateTimeToStr(Now()));
	}
	GaussianProcess_GaussianKernel gp;
	gp.SetGaussianKernelSigma(structure_search_last_gaussian_process_gaussian_kernel_sigma_);
	gp.SetBeta(structure_search_last_gaussian_process_beta_);
	double min_error = 0.0, max_error = 0.0;
	error_and_feature.GetColumnMinMax(0, min_error, max_error);
	if (max_error > min_error + log(max_error_ratio)) {
		max_error = min_error + log(max_error_ratio);
	}
	for (int ir = 0 ; ir < training_num ; ir++) {
		FListT<double> x;
		for (int id = 1 ; id < error_and_feature.GetNumColumn() ; id++) {
			x.Add(error_and_feature.GetFloatValue(ir, id));
		}
		const FString &y_str = error_and_feature.GetValue(ir, 0);
		double y = y_str != "" ? y_str.AsFloat() : max_error;
		if (y > max_error) {
			y = max_error;
		}
		gp.AddData(x, y);
	}

	gp.Train();
	//Tuning
	for (int i = 0 ; i < 100 ; i++) {
		gp.SearchHyperParameter();
		ProcessMessages();
	}
	//Comment out to start from initial beta always
	//structure_search_last_gaussian_process_beta_ = gp.GetBeta();
	//structure_search_last_gaussian_process_gaussian_kernel_sigma_ = gp.GetGaussianKernelSigma();
	if (online_debug_mode_) {
		Me_Debug->Lines->Add(UnicodeString("gaussian_process_beta : ") + FloatToStr(gp.GetBeta()));
		Me_Debug->Lines->Add(UnicodeString("gaussian_kernel_sigma : ") + FloatToStr(gp.GetGaussianKernelSigma()));
		Me_Debug->Lines->Add(UnicodeString("log_likelihood : ") + FloatToStr(gp.EvaluateLogLikelihood()));
		Me_Debug->Lines->Add(UnicodeString("mean_square_error : ") + FloatToStr(gp.EvaluateTrainingMSE()));
		Me_Debug->Lines->Add(UnicodeString("Pareto Optimal"));
		Me_Debug->Lines->Add(UnicodeString("\ti\tindex\terror\tcost"));
		for (int i = 0 ; i < pareto_optimal_index_all_sorted.GetSize() ; i++){
			const NNTrainingResult &result = results.GetAt(pareto_optimal_index_all_sorted.GetAt(i));
			Me_Debug->Lines->Add(
				UnicodeString("\t") + IntToStr(i) +
				UnicodeString("\t") + IntToStr(pareto_optimal_index_all_sorted.GetAt(i)) +
				UnicodeString("\t") + FormatFloat("0.0000", result.GetBestValidation()) +
				UnicodeString("\t") + IntToStr((__int64)result.GetCostMultiplyAdd()));
		}
	}

	//Search possible network
	if (online_debug_mode_) {
		Me_Debug->Lines->Add("Search Possible Network");
		Me_Debug->Lines->Add(DateTimeToStr(Now()));
	}
	double best_gain_area = -1.0e+304;
	NNStructures best_structure;
	if (online_debug_mode_) {
		//Me_Debug->Lines->Add("index\ttruth\texisting\tmyu\tpossible\tgain\tarea");
		Me_Debug->Lines->Add("index\tcost\tmyu\tsigma\tgain");
	}
	structure_search_candidate_myu_.Clear();
	structure_search_candidate_sigma_.Clear();
	structure_search_candidate_cost_.Clear();
	structure_search_candidate_gain_.Clear();
	structure_search_candidate_index_ = -1;
	FStringList best_log;
	if (gp.GetReady()) {
		double best_myu2 = 1.0e+304;
		for (int it = 0 ; it < candidates.GetSize() ; it++) {
			const double cost_multiply_add = candidates.GetAt(it).GetStatistics(FString("CostMultiplyAdd"))->GetSum();

			//Predict
			FListT<double> x;
			double y_myu = 0.0, y_sigma = 1.0;
			for (int id = 0 ; id < effective_col.GetSize() ; id++) {
				x.Add(error_and_feature.GetFloatValue(training_num + it, id + 1));
			}
			gp.Predict(x, y_myu, y_sigma);
			if (y_myu < max_error && y_sigma >= 0.0) {
				double y_myu2 = y_myu;
				switch (nn_document_.GetStructureSearchMethod()) {
				case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticProb:
				case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticArea:
					y_myu2 -= y_sigma * exploration_sigma;
					break;
				default:
					break;
				}

				//Calc Area Gain
				double area_gain = 0.0;
				FStringList sl;
				sl.Add(FString("myu\t") + FString(y_myu));
				sl.Add(FString("sigma\t") + FString(y_sigma));
				sl.Add(FString("myu2\t") + FString(y_myu2));
				sl.Add(FString("left\ttop\tright\tbottom\tscore\tarea\tsum"));

				switch (nn_document_.GetStructureSearchObjective()) {
				case StructureSearchObjective::Error:
					{
						const NNTrainingResult &result = results.GetAt(pareto_optimal_index_all_sorted.GetAt(pareto_optimal_index_all_sorted.GetSize() - 1));
						switch (nn_document_.GetStructureSearchMethod()) {
						case StructureSearchMethod::NetworkFeatureAndGaussianProcess:
						case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticProb:
							area_gain =
								p(y_myu2, y_sigma * y_sigma, y_myu2 - y_sigma * 10, log(result.GetBestValidation()));
							break;
						case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticArea:
							area_gain =
								y_myu2 < log(result.GetBestValidation()) ?
									log(result.GetBestValidation()) - y_myu2 :
									0.0;
							break;
						default:
							break;
						}
					}
					break;
				case StructureSearchObjective::ErrorAndCalculation:
					switch (nn_document_.GetStructureSearchMethod()) {
					case StructureSearchMethod::NetworkFeatureAndGaussianProcess:
					case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticProb:
						{
							//Calc Area Prob Gain
							double last_error = -1.0e300;
							const double validation_max = nn_document_.GetStructureSearchConstraint(FString("Validation"), 1);
							for (int i = 0 ; i < pareto_optimal_index_all_sorted.GetSize() ; i++) {
								const NNTrainingResult &result = results.GetAt(pareto_optimal_index_all_sorted.GetAt(i));
								if (result.GetBestValidation() > 0.0){
									if (result.GetCostMultiplyAdd() > cost_multiply_add &&
										(result.GetBestValidation() <= validation_max || validation_max <= 0.0)) {
										const double upper_error =
											(last_error >= -1.0e299 && (validation_max <= 0.0 || last_error < log(validation_max))) ?
												last_error:
												(validation_max >= 0.0 ?
													log(validation_max) :
													log(result.GetBestValidation()) + y_sigma * 8.0);
										const double prob =
											p(y_myu2, y_sigma * y_sigma, log(result.GetBestValidation()), upper_error);
										const double upper_cost =
											result.GetCostMultiplyAdd() > cost_multiply_add * 1.5 ?
											cost_multiply_add * 1.5 :
											result.GetCostMultiplyAdd();
										const double cost_diff =
											(log(upper_cost) - log(cost_multiply_add));
										area_gain += prob * cost_diff;
										sl.Add(
											FString(cost_multiply_add) +
											FString("\t") +
											FString(upper_error) +
											FString("\t") +
											FString(upper_cost) +
											FString("\t") +
											FString(log(result.GetBestValidation())) +
											FString("\t") +
											FString(prob) +
											FString("\t") +
											FString(prob * cost_diff) +
											FString("\t") +
											FString(area_gain)
											);
									}
									last_error = log(result.GetBestValidation());
								}
							}
							const double remaining_prob =
								last_error >= -1.0e299 ?
								p(y_myu2, y_sigma * y_sigma, last_error - y_sigma * 8.0, last_error) :
								1.0;
							area_gain += remaining_prob * log(1.5);
							sl.Add(
								FString(cost_multiply_add) +
								FString("\t") +
								FString(last_error) +
								FString("\t") +
								FString(cost_multiply_add * 1.5) +
								FString("\t") +
								FString(last_error - y_sigma * 8.0) +
								FString("\t") +
								FString(remaining_prob) +
								FString("\t") +
								FString(remaining_prob * log(1.5)) +
								FString("\t") +
								FString(area_gain)
								);
						}
						break;
					case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticArea:
						{
							//Calc Area Gain
							double last_error = -1.0e300;
							const double validation_max = nn_document_.GetStructureSearchConstraint(FString("Validation"), 1);
							for (int i = 0 ; i < pareto_optimal_index_all_sorted.GetSize() ; i++) {
								const NNTrainingResult &result = results.GetAt(pareto_optimal_index_all_sorted.GetAt(i));
								if (result.GetBestValidation() < 0.0){
									if (result.GetCostMultiplyAdd() > cost_multiply_add &&
										(result.GetBestValidation() <= validation_max || validation_max <= 0.0)) {
										const double upper_error =
											(last_error >= -1.0e299 && (validation_max <= 0.0 || last_error < log(validation_max))) ?
												last_error:
												(validation_max >= 0.0 ?
													log(validation_max) :
													log(result.GetBestValidation() * 1.1));
										const double lower_error =
											log(result.GetBestValidation()) < y_myu2 ? y_myu2 : log(result.GetBestValidation());
										if (upper_error > lower_error) {
											const double length =
												upper_error - lower_error;
											const double upper_cost =
												result.GetCostMultiplyAdd() > cost_multiply_add * 1.5 ?
												cost_multiply_add * 1.5 :
												result.GetCostMultiplyAdd();
											const double cost_diff =
												(log(upper_cost) - log(cost_multiply_add));
											area_gain += length * cost_diff;
											sl.Add(
												FString(cost_multiply_add) +
												FString("\t") +
												FString(upper_error) +
												FString("\t") +
												FString(upper_cost) +
												FString("\t") +
												FString(lower_error) +
												FString("\t") +
												FString(length) +
												FString("\t") +
												FString(length * cost_diff) +
												FString("\t") +
												FString(area_gain)
												);
										}
									}
									last_error = log(result.GetBestValidation());
								}
							}
							if (last_error > y_myu2 || last_error < -1.0e299) {
								const double upper_error =
									last_error >= -1.0e299 ? last_error : y_myu2 + log(1.1);
								const double lower_error =
									y_myu2;
								const double remaining_length =
									upper_error - lower_error;
								area_gain += remaining_length * log(1.5);
								sl.Add(
									FString(cost_multiply_add) +
									FString("\t") +
									FString(upper_error) +
									FString("\t") +
									FString(cost_multiply_add * 1.5) +
									FString("\t") +
									FString(lower_error) +
									FString("\t") +
									FString(remaining_length) +
									FString("\t") +
									FString(remaining_length * log(1.5)) +
									FString("\t") +
									FString(area_gain)
									);
							}
						}
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}

				structure_search_candidate_myu_.Add(y_myu);
				structure_search_candidate_sigma_.Add(y_sigma);
				structure_search_candidate_cost_.Add(cost_multiply_add);
				structure_search_candidate_gain_.Add(area_gain);

				if (online_debug_mode_) {
					Me_Debug->Lines->Add(
						IntToStr(it) +
						UnicodeString(",") +
						IntToStr((__int64)cost_multiply_add) +
						UnicodeString(",") +
						FormatFloat("0.00000", y_myu) +
						UnicodeString(",") +
						FormatFloat("0.00000", y_sigma) +
						UnicodeString(",") +
						FloatToStr(area_gain)
						);
				}

				if (area_gain > best_gain_area || (area_gain == best_gain_area && y_myu2 < best_myu2)) {
					best_gain_area = area_gain;
					best_myu2 = y_myu2;
					best_structure = candidates.GetAt(it);
					best_log = sl;
					structure_search_candidate_index_ = structure_search_candidate_myu_.GetSize() - 1;
				}
			}
			ProcessMessages();
        }
	}

	if (online_debug_mode_) {
		for (int i = 0 ; i < best_log.GetSize() ; i++) {
			Me_Debug->Lines->Add(best_log.GetAt(i).PChar());
		}
	}

	if (best_structure.GetNumLayer() && best_gain_area > 0.0) {
		if (online_debug_mode_) {
			Me_Debug->Lines->Add(UnicodeString("Best Gain Area : ") + FloatToStr(best_gain_area));
		}
		nn_document_.NNStructures::operator =(best_structure);
		if (structure_search_candidate_myu_.GetSize() && structure_search_candidate_index_ >= 0) {
			nn_document_.SetStructureSearchExpectiation(
				structure_search_candidate_myu_.GetAt(structure_search_candidate_index_),
				structure_search_candidate_sigma_.GetAt(structure_search_candidate_index_));
		}
	}else {
		//Random if failed
		if (online_debug_mode_) {
			Me_Debug->Lines->Add("Gaussian Process Failed. Using Random Search.");
		}
		MutateNetworkForStructureSearch_Random(result_structures, pareto_optimal_index_all_sorted, pareto_optimal_index_seed);
	}
	if (online_debug_mode_) {
		Me_Debug->Lines->Add("Finished");
		Me_Debug->Lines->Add(DateTimeToStr(Now()));
		Me_Debug->Lines->Add("");
	}
	HideAnimationIndicator();
	/*
	RB_ResultsTradeoffGraph->IsChecked = true;
	SaveGraphBitmapToFile(
		UnicodeString(nn_document_.GetResultPath().PChar()) + FormatDateTime("yyyymmdd_hhnnss", Now()) + ".png");
	*/
    return true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkPaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_Network->Canvas;
	C->BeginScene();
	C->SetMatrix(PB_Network->AbsoluteMatrix);
	C->Clear(SDEEP_CONSOLE_COLOR_BACKGROUND);

	DrawNetworkBitmap(
		C, PB_Network->Width, PB_Network->Height,
		nn_graph_mapper_, network_current_left_, network_current_top_, pow(2.0, network_current_zoom_ * 0.5));


	if (Bu_Network->IsFocused &&
		network_current_zoom_ < -0.5 &&
		network_last_x_ >= 0 && network_last_y_ >= 0 &&
		network_last_x_ < nn_document_.GetNetworkViewSizeX() && network_last_y_ < nn_document_.GetNetworkViewSizeY()) {

		const int w = 360, h = 160;
		TBitmap *BMP = new TBitmap(w, h);
		TCanvas *BMPC = BMP->Canvas;
		BMPC->BeginScene();
		BMPC->Clear(0xffffffff);
		DrawNetworkBitmap(BMPC, BMP->Width, BMP->Height, nn_graph_mapper_, network_last_x_ - w / 2, network_last_y_ - h / 2, 1.0);
		BMPC->EndScene();

		const int x = (network_last_x_ - network_current_left_) * pow(2.0, network_current_zoom_ * 0.5);
		const int y = (network_last_y_ - network_current_top_) * pow(2.0, network_current_zoom_ * 0.5);
		C->DrawBitmap(
			BMP,
			RectF(0, 0, w, h),
			RectF(x - w/2, y - h/2, x + w/2, y + h/2),
			1.0f,
			false);
		delete BMP;

		const TRectF rect = RectF(x - w/2, y - h/2, x + w/2, y + h/2);
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_LIGHTFG2;
		C->Stroke->Thickness = 0.5;
		C->DrawRect(rect, 0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
	}

	C->EndScene();
}
//---------------------------------------------------------------------------
void TFo_Main::DrawNetworkBitmap(TCanvas *C, int width, int height, NNGraphMapper &graph_mapper, double current_left, double current_top, double zoom)
{
	const FListP<NNGraphItem> &graph = graph_mapper.GetGraph();
	for (int i = 0 ; i < graph.GetSize() ; i++) {
		const NNGraphItem &item = graph.GetAt(i);
		const TRectF rect =
			RectF(
				(item.current_rect[0] - current_left) * zoom,
				(item.current_rect[1] - current_top) * zoom,
				(item.current_rect[2] - current_left) * zoom,
				(item.current_rect[3] - current_top) * zoom);

		if (!((rect.right < 0 && rect.left < 0) ||
			  (rect.bottom < 0 && rect.top < 0) ||
			  (rect.right >= width && rect.left >= width) ||
			  (rect.bottom >= height && rect.top >= height) ) ||
			item.shape == NNGraphItemShape::Grid) {
			C->Fill->Color = item.color | 0xff000000;
			C->Fill->Kind = TBrushKind::Solid;
			C->Stroke->Color = item.color | 0xff000000;
			C->Stroke->Thickness = item.pen_width * zoom;
			C->Stroke->Kind = TBrushKind::Solid;

			if (item.alpha > 1.0f / 255) switch (item.shape) {
			case NNGraphItemShape::Rect:
				C->FillRect(rect, 0.0f, 0.0f, AllCorners, item.alpha, TCornerType::Round);
				break;
			case NNGraphItemShape::RectFrame:
				C->DrawRect(rect, 0.0f, 0.0f, AllCorners, item.alpha, TCornerType::Round);
				break;
			case NNGraphItemShape::RoundRect:
				C->FillRect(rect, 8.0f * zoom, 8.0f * zoom, AllCorners, item.alpha, TCornerType::Round);
				break;
			case NNGraphItemShape::RoundRectFrame:
				C->DrawRect(rect, 8.0f * zoom, 8.0f * zoom, AllCorners, item.alpha, TCornerType::Round);
				break;
			case NNGraphItemShape::DownTriangle:
				{
					TPathData *path = new TPathData();
					path->MoveTo(rect.TopLeft());
					path->LineTo(PointF(rect.right, rect.top));
					path->LineTo(PointF((rect.left + rect.right) * 0.5f, rect.bottom));
					path->LineTo(PointF(rect.left, rect.top));
					C->FillPath(path, item.alpha);
					delete path;
				}
				break;
			case NNGraphItemShape::InvertedTrapezoid:
				{
					TPathData *path = new TPathData();
					path->MoveTo(rect.TopLeft());
					path->LineTo(PointF(rect.right, rect.top));
					path->LineTo(PointF(rect.right, rect.bottom));
					path->LineTo(PointF(rect.left + rect.bottom - rect.top, rect.bottom));
					path->LineTo(PointF(rect.left, rect.top));
					C->FillPath(path, item.alpha);
					delete path;
				}
				break;
			case NNGraphItemShape::Ellipse:
				C->FillEllipse(rect, item.alpha);
				break;
			case NNGraphItemShape::Text:
			case NNGraphItemShape::TextWordWrap:
			case NNGraphItemShape::TextRight:
			case NNGraphItemShape::TextCenter:
			case NNGraphItemShape::TextCenterBold:
				C->Font->Size = item.font_size * zoom;
				if (C->Font->Size >= 4) {
					int word_wrap = false;
					TTextAlign halign, valign;
					switch (item.shape) {
					case NNGraphItemShape::TextRight:
						halign = TTextAlign::Trailing;
						valign = TTextAlign::Leading;
						break;
					case NNGraphItemShape::TextCenter:
					case NNGraphItemShape::TextCenterBold:
						halign = TTextAlign::Center;
						valign = TTextAlign::Center;
						break;
					case NNGraphItemShape::TextWordWrap:
						word_wrap = true;
					default:
						halign = TTextAlign::Leading;
						valign = TTextAlign::Leading;
						break;
					}
					if (item.shape == NNGraphItemShape::TextCenterBold){
						C->Font->Style = TFontStyles() << TFontStyle::fsBold;
					}else {
						C->Font->Style =TFontStyles();
					}
					C->FillText(
						rect,
						item.text.PChar(),
						word_wrap,
						item.alpha,
						TFillTextFlags(),
						halign,
						valign
						);
				}
				break;
			case NNGraphItemShape::Image:
				{
					TBitmap *bitmap = NULL;
					if (item.text == "Warning") {
						bitmap = Im_Warning->Bitmap;
					}
					if (bitmap) {
						C->DrawBitmap(
							bitmap,
							RectF(0, 0, bitmap->Width, bitmap->Height),
							rect,
							item.alpha,
							false);
					}
				}
				break;

			case NNGraphItemShape::Connector:
				{
					const float r = item.option * zoom;
					const float distance_x = fabs(rect.Left - rect.Right);
					const float distance_y = fabs(rect.Top - rect.Bottom);
					const float distance = sqrt(distance_x * distance_x + distance_y * distance_y);
					const float space = NNLayerDefaultR * zoom;
					if (rect.top < rect.bottom - space &&
						fabs(rect.left - rect.right) >= space &&
						(item.parent_sub_id[1] >= NNGraphMapperLayerSubID::InputPin2 && (int)item.parent_sub_id[1] < (int)NNGraphMapperLayerSubID::InputPin + NNGraphMapperMaxConnectorNum) &&
						item.optioni) {
						//Parameter Input Pin
						TPathData *path = new TPathData();
						path->MoveTo(PointF(rect.Left, rect.Top));
						path->LineTo(PointF(rect.Left, rect.Bottom - space));
						if (rect.left < rect.right) {
							//Normal
							for (int rad = 1 ; rad < 8 ; rad++) {
								path->LineTo(PointF(
									rect.Left + space - space * cos(0.5 * 3.1415926 * rad / 8.0),
									rect.Bottom - space + space * sin(0.5 * 3.1415926 * rad / 8.0)));
							}
						}else {
							//Inverse
							for (int rad = 1 ; rad < 8 ; rad++) {
								path->LineTo(PointF(
									rect.Left - space + space * cos(0.5 * 3.1415926 * rad / 8.0),
									rect.Bottom - space + space * sin(0.5 * 3.1415926 * rad / 8.0)));
							}
						}
						path->LineTo(PointF(rect.Right, rect.Bottom));
						C->DrawPath(path, item.alpha);
						delete path;
					}else if (rect.Top <= rect.Bottom || distance <= space){
						//Top down
						if (distance <= space || distance_x <= space * 2 || distance_y <= space * 2) {
							//Straight line
							C->DrawLine(rect.TopLeft(), rect.BottomRight(), item.alpha);
						}else {
							TPathData *path = new TPathData();
							path->MoveTo(PointF(rect.Left, rect.Top));
							path->LineTo(PointF(rect.Left, (rect.Top + rect.Bottom) * 0.5 - space));

							if (rect.Left < rect.Right){
								for (int rad = 1 ; rad < 8 ; rad++) {
									path->LineTo(PointF(
										rect.Left + space - space * cos(0.5 * 3.1415926 * rad / 8.0),
										(rect.Top + rect.Bottom) * 0.5 - space + space * sin(0.5 * 3.1415926 * rad / 8.0)));
								}
								path->LineTo(PointF(rect.Right - space, (rect.Top + rect.Bottom) * 0.5));
								for (int rad = 1 ; rad < 8 ; rad++) {
									path->LineTo(PointF(
										rect.Right - space + space * sin(0.5 * 3.1415926 * rad / 8.0),
										(rect.Top + rect.Bottom) * 0.5 + space - space * cos(0.5 * 3.1415926 * rad / 8.0)));
								}
							}else {
								for (int rad = 1 ; rad < 8 ; rad++) {
									path->LineTo(PointF(
										rect.Left - space + space * cos(0.5 * 3.1415926 * rad / 8.0),
										(rect.Top + rect.Bottom) * 0.5 - space + space * sin(0.5 * 3.1415926 * rad / 8.0)));
								}
								path->LineTo(PointF(rect.Right + space, (rect.Top + rect.Bottom) * 0.5));
								for (int rad = 1 ; rad < 8 ; rad++) {
									path->LineTo(PointF(
										rect.Right + space - space * sin(0.5 * 3.1415926 * rad / 8.0),
										(rect.Top + rect.Bottom) * 0.5 + space - space * cos(0.5 * 3.1415926 * rad / 8.0)));
								}
							}

							path->LineTo(PointF(rect.Right, rect.Bottom));
							C->DrawPath(path, item.alpha);
							delete path;
						}
					}else if (rect.Right < rect.Left + (NNLayerDefaultWidth / 2 + NNLayerDefaultHSpace + NNLayerDefaultSpace * 2 - 1) * zoom){
						//Bottom up
						const float escape_x =
							(rect.Left > rect.Right ? rect.Left :rect.Right) +
							(NNLayerDefaultWidth / 2 + NNLayerDefaultHSpace) * zoom;
						TPathData *path = new TPathData();
						path->MoveTo(PointF(rect.Left, rect.Top));
						for (int rad = 1 ; rad < 8 ; rad++) {
							path->LineTo(PointF(
								rect.Left + space - space * cos(0.5 * 3.1415926 * rad / 8.0),
								rect.Top + space * sin(0.5 * 3.1415926 * rad / 8.0)));
						}
						path->LineTo(PointF(escape_x - space, rect.Top + space));
						for (int rad = 1 ; rad < 8 ; rad++) {
							path->LineTo(PointF(
								escape_x - space + space * sin(0.5 * 3.1415926 * rad / 8.0),
								rect.Top + space * cos(0.5 * 3.1415926 * rad / 8.0)));
						}
						path->LineTo(PointF(escape_x, rect.Bottom));
						for (int rad = 1 ; rad < 8 ; rad++) {
							path->LineTo(PointF(
								escape_x - space + space * cos(0.5 * 3.1415926 * rad / 8.0),
								rect.Bottom - space * sin(0.5 * 3.1415926 * rad / 8.0)));
						}
						path->LineTo(PointF(rect.Right + space, rect.Bottom - space));
						for (int rad = 1 ; rad < 8 ; rad++) {
							path->LineTo(PointF(
								rect.Right + space - space * sin(0.5 * 3.1415926 * rad / 8.0),
								rect.Bottom - space * cos(0.5 * 3.1415926 * rad / 8.0)));
						}
						C->DrawPath(path, item.alpha);
						delete path;
					}else {
						//Bottom up、left to right
						const float center_x = rect.Left + (NNLayerDefaultWidth / 2 + NNLayerDefaultHSpace) * zoom;
						TPathData *path = new TPathData();
						path->MoveTo(PointF(rect.Left, rect.Top));
						for (int rad = 1 ; rad < 8 ; rad++) {
							path->LineTo(PointF(
								rect.Left + space - space * cos(0.5 * 3.1415926 * rad / 8.0),
								rect.Top + space * sin(0.5 * 3.1415926 * rad / 8.0)));
						}
						path->LineTo(PointF(center_x - space, rect.Top + space));
						for (int rad = 1 ; rad < 8 ; rad++) {
							path->LineTo(PointF(
								center_x - space + space * sin(0.5 * 3.1415926 * rad / 8.0),
								rect.Top + space * cos(0.5 * 3.1415926 * rad / 8.0)));
						}
						path->LineTo(PointF(center_x, rect.Bottom));
						for (int rad = 1 ; rad < 8 ; rad++) {
							path->LineTo(PointF(
								center_x + space - space * cos(0.5 * 3.1415926 * rad / 8.0),
								rect.Bottom - space * sin(0.5 * 3.1415926 * rad / 8.0)));
						}
						path->LineTo(PointF(rect.Right - space, rect.Bottom - space));
						for (int rad = 1 ; rad < 8 ; rad++) {
							path->LineTo(PointF(
								rect.Right - space + space * sin(0.5 * 3.1415926 * rad / 8.0),
								rect.Bottom - space * cos(0.5 * 3.1415926 * rad / 8.0)));
						}
						C->DrawPath(path, item.alpha);
						delete path;
					}

					//Pin
					if (rect.Width() * rect.Width() + rect.Height() * rect.Height() < zoom * zoom) {
						//Draw single ellipse
						C->FillEllipse(
							RectF(rect.left - r, rect.top - r, rect.left + r, rect.top + r),
							item.alpha);
					}else {
						C->FillEllipse(
							RectF(rect.left - r, rect.top - r, rect.left + r, rect.top + r),
							item.alpha);
						C->FillEllipse(
							RectF(rect.right - r, rect.bottom - r, rect.right + r, rect.bottom + r),
							item.alpha);
					}
				}
				break;
			case NNGraphItemShape::Grid:
				if (C->Stroke->Thickness >= 1.0) {
					for (float iy = fmod((float)-current_top, item.rect[3]) * zoom ; iy < PB_Network->Height ; ) {
						for (float ix = fmod((float)-current_left, item.rect[2]) * zoom ; ix < PB_Network->Width ; ) {
							C->DrawLine(PointF(ix, iy), PointF(ix, iy), item.alpha);
							ix += item.rect[2] * zoom;
						}
						iy += item.rect[3] * zoom;
					}
				}
				break;
			default:
				;
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkMouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, float X, float Y)
{
	ClearEditBox();
	PB_NetworkTab->Tag = 0;
	drag_network_tab_ = false;
	Bu_Network->SetFocus();

	/*
	//use button
	for (int iu = 0; iu < use_button_rect_.GetSize() ; iu++) {
		if (use_button_rect_.GetAt(iu).Contains(TPointF(X, Y))) {
			nn_document_.KeepForUndo(FString("Change network use"));
			nn_document_.SetUse((NNStructureUseType)iu, !nn_document_.GetUse((NNStructureUseType)iu));
			return;
		}
	}
	*/

	X /= pow(2.0, network_current_zoom_ * 0.5);
	Y /= pow(2.0, network_current_zoom_ * 0.5);
	Sc_NetworkX->Tag = 0;
	Sc_NetworkY->Tag = 0;
	X += network_current_left_;
	Y += network_current_top_;

	network_focuse_index_ = 1;
	if (Button == TMouseButton::mbLeft) {
		SetCaptured((_di_IControl)*PB_Network);

		PB_Network->PopupMenu = NULL;
		const FListP<NNGraphItem> &graph = nn_graph_mapper_.GetGraph();
		int processed = false;
		int on_layer = false;
		int undo_prepared = false;
		for (int i = graph.GetSize() - 1 ; i >= 0 && !network_mouse_down_ ; i--) {
			const NNGraphItem &item = graph.GetAt(i);
			if (item.can_focus) {
				if (item.Contains(X, Y)){
					if (item.z == NNGraphMapperZ::Link) {
						const float source_distance = sqrt(
							(item.current_rect[0] - X) * (item.current_rect[0] - X) +
							(item.current_rect[1] - Y) * (item.current_rect[1] - Y));
						const float dest_distance = sqrt(
							(item.current_rect[2] - X) * (item.current_rect[2] - X) +
							(item.current_rect[3] - Y) * (item.current_rect[3] - Y));

						//to allow to create new link
						if (source_distance >= 4) {//NNGraphMapperSocketSize / 2 //  && dest_distance >= 4
							nn_document_.ClearSelection();
							nn_document_.SetCurrentLinkID(item.id);
							nn_document_.PrepareUndoInAdvance();
							undo_prepared = true;
							if (nn_document_.StartLinkEdit_Existing(item.id, dest_distance < source_distance, X, Y)) {
								network_mouse_down_mode_ = NetworkMouseDownMode::DragLink;
								network_mouse_down_ = true;
							}
						}
					}else {
						switch (item.sub_id) {
						case NNGraphMapperLayerSubID::OutputPin:
						case NNGraphMapperLayerSubID::OutputPin2:
						case NNGraphMapperLayerSubID::OutputPin3:
							nn_document_.PrepareUndoInAdvance();
							undo_prepared = true;
							if (nn_document_.StartLinkEdit_New(item.id, (int)item.sub_id - (int)NNGraphMapperLayerSubID::OutputPin, X, Y)) {
								network_mouse_down_mode_ = NetworkMouseDownMode::DragLink;
								network_mouse_down_ = true;
							}
							break;
						default:
							on_layer = true;
							if (!Shift.Contains(ssCtrl)) {
								if (nn_document_.GetCurrentLayerID() >= 0 && Shift.Contains(ssShift)) {
									nn_document_.AddLink(nn_document_.GetCurrentLayerID(), 0, item.id, 0);
								}
								const int last_selected = nn_document_.GetLayerSelected(item.id);
								if (!last_selected) {
									nn_document_.ClearSelection();
									nn_document_.SetLayerSelected(item.id, true, false);
								}
								nn_document_.SetCurrentLayerID(item.id);
								processed = true;
							}else {
								if (nn_document_.GetCurrentLinkID() >= 0) {
									nn_document_.ClearSelection();
								}
								if (nn_document_.GetLayerSelected(item.id)) {
									nn_document_.SetLayerSelected(item.id, false, false);
									nn_document_.SetCurrentLayerID(-1);
									processed = true;
								}else {
									nn_document_.SetLayerSelected(item.id, true, false);
									nn_document_.SetCurrentLayerID(item.id);
									processed = true;
								}
							}
							break;
						}
					}
				}
			}
		}

		if (!network_mouse_down_) {
			if (!processed && !Shift.Contains(ssCtrl)) {
				nn_document_.ClearSelection();
				nn_document_.SetCurrentLayerID(-1);
			}

			if (on_layer){
				if (nn_document_.GetNumSelectedLayer()) {
					network_mouse_down_mode_ = NetworkMouseDownMode::DragLayer;
					network_mouse_down_ = true;
					if (Shift.Contains(ssAlt)) {
						nn_document_.PrepareUndoInAdvance();
						undo_prepared = true;
						nn_document_.Paste(nn_document_.Copy(), false);
						network_start_x_ = X;
						network_start_y_ = Y;
						network_last_x_ = X;
						network_last_y_ = Y;
                        PB_NetworkMouseMove(Sender, Shift, X + 1, Y);
                        return;
					}
				}

			}else {
				network_mouse_down_mode_ = NetworkMouseDownMode::RangeSelect;
				network_mouse_down_ = true;
			}
		}

		if (network_mouse_down_) {
			if (!undo_prepared) {
				nn_document_.PrepareUndoInAdvance();
			}
			network_start_x_ = X;
			network_start_y_ = Y;
		}
	}
	network_last_x_ = X;
	network_last_y_ = Y;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkMouseMove(TObject *Sender, TShiftState Shift,
		  float X, float Y)
{
	X /= pow(2.0, network_current_zoom_ * 0.5);
	Y /= pow(2.0, network_current_zoom_ * 0.5);
	X += network_current_left_;
	Y += network_current_top_;

	if (network_mouse_down_) {
		switch (network_mouse_down_mode_) {
		case NetworkMouseDownMode::DragLayer:
			{
				float dx = X - network_last_x_;
				float dy = Y - network_last_y_;

				if (dx != 0.0f || dy != 0.0f){
					if (!nn_document_.GetDragging()) {
						nn_document_.StartDrag();
					}
					nn_document_.Drag(dx, dy);
				}
			}
			break;
		case NetworkMouseDownMode::RangeSelect:
			{
				float rect[4];
				rect[0] = network_start_x_;
				rect[2] = X;
				rect[1] = network_start_y_;
				rect[3] = Y;
				nn_document_.SetSelectionRect(rect);
			}
			break;
		case NetworkMouseDownMode::DragLink:
			{
				int snap_layer_id = -1;
				int snap_input_pin_index = 0;
				const FListP<NNGraphItem> &graph = nn_graph_mapper_.GetGraph();
				for (int i = graph.GetSize() - 1 ; i >= 0 ; i--) {
					const NNGraphItem &item = graph.GetAt(i);
					if (item.z >= NNGraphMapperZ::LayerBG && item.z <= NNGraphMapperZ::LayerFG3 && item.can_focus) {
						if (item.Contains(X, Y)){
							snap_layer_id = item.id;
							if (item.sub_id >= NNGraphMapperLayerSubID::InputPin2 &&
								(int)item.sub_id < (int)NNGraphMapperLayerSubID::InputPin2 + NNGraphMapperMaxConnectorNum) {
								snap_input_pin_index = (int)item.sub_id - (int)NNGraphMapperLayerSubID::InputPin;
							}else if (item.sub_id >= NNGraphMapperLayerSubID::InputConnector2 &&
							    (int)item.sub_id < (int)NNGraphMapperLayerSubID::InputConnector2 - 1 + NNGraphMapperMaxConnectorNum) {
								snap_input_pin_index = (int)item.sub_id - (int)NNGraphMapperLayerSubID::InputConnector2 + 1;
							}
							break;
						}
					}
				}
				nn_document_.DragLink(X, Y, snap_layer_id, snap_input_pin_index);
			}
			break;
		case NetworkMouseDownMode::None:
		default:
			break;
		}
	}
	network_last_x_ = X;
	network_last_y_ = Y;

	if (Bu_Network->IsFocused &&
		network_current_zoom_ < -0.5 &&
		network_last_x_ < nn_document_.GetNetworkViewSizeX() && network_last_y_ < nn_document_.GetNetworkViewSizeY()) {
        nn_graph_mapper_.RequestRepaint();
    }

	network_tab_index_2_ = -1;
	network_tab_delete_focus_ = false;
	PB_NetworkTab->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkMouseUp(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, float X, float Y)
{
	//X += network_current_left_;
	//Y += network_current_top_;

	if (Button == TMouseButton::mbLeft){
		if (network_mouse_down_) {
			switch (network_mouse_down_mode_) {
			case NetworkMouseDownMode::DragLayer:
				if (nn_document_.GetDragging()) {
					if (nn_document_.EndDrag()) {
						nn_document_.KeepPreparedForUndo(FString("Drag Layer"));
						/*//Benchmark refresh display value
						unsigned int time = timeGetTime();
						for (int i = 0 ; i < 4 ; i++){
							nn_document_.RefreshDisplayValue(&nn_document_, true);
						}
						La_Test->Text = IntToStr((__int64)timeGetTime() - time);
						//*/
					}
				}
				break;
			case NetworkMouseDownMode::RangeSelect:
				nn_document_.ResetSelectionRect();
				break;
			case NetworkMouseDownMode::DragLink:
				if (nn_document_.EndLinkEdit()) {
					nn_document_.KeepPreparedForUndo(FString("Edit Link"));
				}
				break;
			case NetworkMouseDownMode::None:
			default:
				break;
			}
			network_mouse_down_ = false;
			network_mouse_down_mode_ = NetworkMouseDownMode::None;
		}
		PB_Network->PopupMenu = PM_Network;

		SetCaptured(NULL);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_NetworkKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	//La_Test->Text = UnicodeString("Network Keydown") + IntToStr(Key);
	switch (Key) {
	case 8:
		//BackSpace
		if (!network_mouse_down_) {
			const int layer_id = nn_document_.GetCurrentLayerID();
			if (layer_id >= 0) {
				nn_document_.KeepForUndo(FString("Delete"));
				nn_document_.RemoveLayer(layer_id);
			}
		}
		break;
	case 27:
		//Esc
		if (MIN_ClearSelection->Enabled) {
			nn_document_.ClearSelection();
		}
		break;
	case 45:
		//Insert
		if (!network_mouse_down_) {
			if (nn_document_.GetCurrentLayerID() >= 0) {
				AddLayer(0, 0, 0, 0, true);
			}
		}
		break;
	case 46:
		//Delete
		if (MIN_Delete->Enabled) {
			nn_document_.KeepForUndo(FString("Delete"));
			nn_document_.DeleteSelected();
		}
		break;
	case 37://Left
	case 38://Up
	case 39://Right
	case 40://Down
		if (!network_mouse_down_) {
			nn_document_.MoveCursor(Key - 37, Shift.Contains(ssCtrl));
		}
		break;
	case 48://Ctrl + 0
	case 96://Ctrl + 0
		if (Shift.Contains(ssCtrl)) {
			//Reset Zoom
			MINZ_DefaultClick(Sender);
		}
		break;
	case 65:
		//Ctrl+A
		if (MIN_SelectAll->Enabled) {
			if (Shift.Contains(ssCtrl)) {
				nn_document_.SelectAllLayer();
			}
		}
		break;
	case 67://Ctrl+C
		if (Bu_Copy->Enabled) {
			if (Shift.Contains(ssCtrl)) {
				Bu_CopyClick(Sender);
			}
		}
		break;
	case 68://Ctrl+D
		if (Bu_Copy->Enabled && Bu_Paste->Enabled) {
			nn_document_.KeepForUndo(FString("Duplicate"));
			nn_document_.Paste(nn_document_.Copy());
		}
		break;
	case 72://Ctrl+H
		MIN_ReplaceClick(Sender);
		break;
	case 77:
		//Ctrl+M
		if (!network_mouse_down_) {
			nn_document_.KeepForUndo(FString("Mutate"));
			if (Shift.Contains(ssCtrl)) {
				if (Shift.Contains(ssShift)) {
					nn_document_.Mutate();
				}else {
					FListT<int> added_layer_id;
					((NNStructure &)nn_document_).Mutate(added_layer_id);
					if (added_layer_id.GetSize()){
						for (int il = 0 ; il < added_layer_id.GetSize() ; il++){
							nn_document_.GlobalEscapeLayerName(added_layer_id.GetAt(il), FString(""));
						}
					}
				}
			}
		}
		break;
	case 78://Ctrl + N
		//New window
		RunApp(AnsiString(ParamStr(0)).c_str(), "");
		break;
	case 86://Ctrl+V
		if (Bu_Paste->Enabled) {
			if (Shift.Contains(ssCtrl)) {
				Bu_PasteClick(Sender);
			}
		}
		break;
	case 88://Ctrl+X
		if (Bu_Cut->Enabled) {
			if (Shift.Contains(ssCtrl)) {
				Bu_CutClick(Sender);
			}
		}
		break;
	case 113:
		//F2 : Edit
		if (nn_document_.GetCurrentLayerID() >= 0) {
			const NNLayer *layer = nn_document_.GetLayerByID(nn_document_.GetCurrentLayerID());
			if (layer) {
				if (nn_document_.GetCurrentPropertyIndex() < 0 && layer->GetNumProperty() >= 1) {
					nn_document_.SetCurrentPropertyIndex(0);
				}
			}
			Bu_Property->SetFocus();
		}
		break;
	}
}
//---------------------------------------------------------------------------
FListT<int> TFo_Main::GetComponentIndex()
{
	int component_index = -1;
	int custom_unit_index = -1;
	const int index = Fr_Component->GetItemIndex();
	if (index >= 0 && index <= Fr_Component->GetItems().GetSize()){
		const ListItem &item = Fr_Component->GetItem(index);
		if (item.GetOriginalIndex() >= 0){
			component_index = item.GetOriginalIndex();
		}
		FString uindex = item.GetAttribute(FString("UIndex"));
		if (uindex != "") {
			custom_unit_index = uindex.AsInt();
			if (custom_unit_index < 0 || custom_unit_index >= settings_.GetCustomUnits().GetSize()) {
				custom_unit_index = -1;
			}
		}
	}
	FListT<int> result;
	result.Add(component_index);
	result.Add(custom_unit_index);
	return result;
}
//---------------------------------------------------------------------------
NNLayer* TFo_Main::AddLayer(int auto_link, float x, float y, int escape, int insert)
{
	FListT<int> ci = GetComponentIndex();
	int component_index = ci.GetAt(0);
	int custom_unit_index = ci.GetAt(1);

	if (component_index >= 0) {
		nn_document_.KeepForUndo(FString(insert ? "Insert Layer : " : "Add layer : ") + settings_.GetEngineInfo().GetAt((int)nn_document_.GetSDeepEngineType()).GetComponents().GetAt(component_index).GetType());

		FString unit_network_name;;
		if (custom_unit_index >= 0) {
			int active_structure_index_bak = nn_document_.GetActiveStructureIndex();
			int current_layer_id_bak = nn_document_.GetCurrentLayerID();
			unit_network_name = settings_.GetCustomUnits().GetAt(custom_unit_index);
			//Import custom unit
			nn_document_tmp_.LoadFromFile(FString(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\custom_units\\").c_str()) + unit_network_name + FString(".sdcproj"));
			// Add Networks
			for (int in = 0 ; in < nn_document_tmp_.GetNumStructure() ; in++) {
				nn_document_tmp_.SetActiveStructure(in);

				const FString structure_name = in == 0 ? unit_network_name : nn_document_tmp_.GetName();
				if (nn_document_.GetStructureIndexByName(structure_name) < 0) {
					nn_document_.AddStructure();
					nn_document_.GetStructure_(nn_document_.GetNumStructure() - 1) = nn_document_tmp_.GetStructure(in);
					nn_document_.SetName(structure_name);
				}
			}
			nn_document_tmp_.Clear();
			nn_document_.SetActiveStructure(active_structure_index_bak);
            nn_document_.SetCurrentLayerID(current_layer_id_bak);
		}

		NNLayer *new_layer = insert ?
			nn_document_.InsertLayer(nn_document_.GetCurrentLayerID(), component_index) :
			&nn_document_.AddLayer(component_index, auto_link, x, y, escape);
		if (new_layer) {
			nn_document_.GlobalEscapeLayerName(new_layer->GetLayerID());
			if (custom_unit_index >= 0) {
				new_layer->SetPropertyValue(FString("Network"), unit_network_name);
			}
		}
		return new_layer;
	}
	return NULL;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkDblClick(TObject *Sender)
{
	if (nn_document_.GetCurrentLayerID() >= 0){
		const NNLayer *layer = nn_document_.GetLayerByID(nn_document_.GetCurrentLayerID());
		if (layer){
			if (layer->GetType() == "Unit"){
				//Edit unit network
				const FString network_name = layer->GetPropertyValue(FString("Network"));
				const int structure_index = nn_document_.GetStructureIndexByName(network_name);
				if (structure_index >= 0){
					nn_document_.KeepForUndo(FString("Change active network"));
					nn_document_.SetActiveStructure(structure_index);
					if (network_overview_structure_index_ >= 0){
						network_overview_structure_index_ = structure_index;
					}
				}
			}else{
				//Move focus to Layer Property
				PB_PropertyMouseDown(Sender, TMouseButton::mbRight, TShiftState(), -1, -1);
			}
		}
	}else if (AddLayer(true, network_last_x_, network_last_y_)) {
		nn_document_.StartDrag();
		nn_document_.EndDrag();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_PropertyPaint(TObject *Sender, TCanvas *Canvas)
{
	property_rect_.Clear();
	property_value_rect_.Clear();
	Bu_Help->Visible = nn_document_.GetCurrentLayerID() >= 0;

	TCanvas *C = PB_Property->Canvas;

	C->BeginScene();
	C->SetMatrix(PB_Property->AbsoluteMatrix);
	C->Clear(SDEEP_CONSOLE_COLOR_LIGHTER);
	if (nn_document_.GetLocked()) {
		C->EndScene();
		return;
	}

	C->Fill->Kind = TBrushKind::Solid;
	C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
	C->Stroke->Kind = TBrushKind::Solid;

	const int property_left_width = Lo_PropertyLeft->Width + Sp_Property->Width / 2;

	visible_property_num_ = 0;
	property_current_pos_ = -1;
	Sp_Property->Visible = nn_document_.GetCurrentLayerID() >= 0;
	if (nn_document_.GetCurrentLayerID() >= 0) {
		const NNLayer *layer = nn_document_.GetLayerByID(nn_document_.GetCurrentLayerID());
		if (layer) {
			FListT<char> property_visible;	// avoid vector<bool> issue
			int index = 0;
			for (int ip = 0 ; ip < layer->GetNumProperty() ; ip++) {
				property_visible.Add(nn_document_.GetLayerPropertyVisible(*layer, ip));
				if (property_visible.GetAt(ip)){
					const int property_y = default_property_header_height + index * default_property_height - property_current_top_;
					if (nn_document_.GetCurrentPropertyIndex() == ip) {
						//Fill background of current cell
						property_current_pos_ = index;
						C->Fill->Color = SDEEP_CONSOLE_COLOR_LIGHTEST;
						C->FillRect(
							RectF(0, property_y, PB_Property->Width, property_y + default_property_height),
							0.0f, 0.0f, AllCorners, 1.0, TCornerType::Round);
					}
					index++;
				}
			}

			C->Stroke->Thickness = 1;
			C->Font->Style = TFontStyles();
			C->Font->Size = 16;
			index = 0;
			for (int ip = 0 ; ip < layer->GetNumProperty() ; ip++) if (property_visible.GetAt(ip)){
				const NNLayerProperty &property = layer->GetProperty(ip);
				const int property_y = default_property_header_height + index * default_property_height - property_current_top_;
				TRectF property_rect = RectF(default_grid_text_hspace, property_y, property_left_width, property_y + default_property_height);
				TRectF property_value_rect = RectF(property_left_width, property_y, PB_Property->Width - default_grid_text_hspace, property_y + default_property_height);
				if ((property_rect.top >= 0 && property_rect.top < PB_Property->Height) ||
					(property_rect.bottom >= 0 && property_rect.bottom < PB_Property->Height)) {
					C->DrawLine(
						PointF(default_grid_text_hspace, property_y + default_property_height),
						PointF(PB_Property->Width - default_grid_text_hspace, property_y + default_property_height),
						1.0f);

					//Property Value
					const FString &name = property.GetName();
					int reset_color = false;
					if (name == "Input" || name == "Inputs") {
						C->Fill->Color = SDEEP_CONSOLE_COLOR_TEXTINPUT;
					}else if (name == "Output") {
						C->Fill->Color = SDEEP_CONSOLE_COLOR_TEXTOUTPUT;
					}else if (nn_document_.GetStatisticsIndexByLabel(name) >= 0) {
						C->Fill->Color = SDEEP_CONSOLE_COLOR_TEXTSTAT;
					}else {
						if (property.GetValue() != property.GetDisplayValue()) {
							C->Fill->Color = SDEEP_CONSOLE_COLOR_BRAND;
							reset_color = true;
						}else {
							C->Fill->Color = SDEEP_CONSOLE_COLOR_DARKEST;
						}
						if (property.GetValue() != property.GetDefaultValue()) {
							C->Font->Style = TFontStyles() << TFontStyle::fsBold;
						}

					}

					FString display_value =
						(property.GetType() == NNLayerPropertyType::PInt || property.GetType() == NNLayerPropertyType::UInt) ?
						property.GetDisplayValue().InsertThousandSeparator() :
						property.GetDisplayValue().ReplaceStr(",", ", ");
					if (property.GetValue() == "*Default") {
                        display_value += " (Default)";
					}
					C->FillText(
						RectF(
							property_value_rect.left + 2,
							property_value_rect.top + default_grid_text_vspace,
							property_value_rect.right,
							property_value_rect.bottom - default_grid_text_vspace),
						display_value.PChar(),
						false,
						property.GetEditable() ? 1.0f : 0.75f,
						TFillTextFlags(),
						TTextAlign::Leading,
						TTextAlign::Center);

					//Property Name
					if (reset_color) {
						C->Fill->Color = SDEEP_CONSOLE_COLOR_DARKEST;
					}
					C->Font->Style = TFontStyles();
					C->Fill->Color = 0xff000000 | FadeColor(C->Fill->Color, SDEEP_CONSOLE_COLOR_LIGHTER, 0.5);
					C->FillText(
						RectF(
							property_rect.left,
							property_rect.top + default_grid_text_vspace,
							property_rect.right - default_grid_text_hspace,
							property_rect.bottom - default_grid_text_vspace),
						property.GetName().PChar(),
						false,
						1.0f,
						TFillTextFlags(),
						TTextAlign::Leading,
						TTextAlign::Center);

					if (property.GetError() && property.GetName() != "Name") {
						C->DrawBitmap(
							Im_Warning->Bitmap,
							RectF(0, 0, Im_Warning->Width, Im_Warning->Height),
							RectF(
								property_value_rect.right - (property_value_rect.bottom - property_value_rect.top) + 3,
								property_value_rect.top + 3,
								property_value_rect.right - 3,
								property_value_rect.bottom - 3),
							1.0f,
							false
							);
					}
				}

				property_rect_.Add(property_rect);
				property_value_rect_.Add(property_value_rect);
				index++;
			}else {
				property_rect_.Add(RectF(0.0f, 0.0f, 0.0f, 0.0f));
				property_value_rect_.Add(RectF(0.0f, 0.0f, 0.0f, 0.0f));
			}
			visible_property_num_ = index;

			//Header

			C->Stroke->Thickness = 2;
			const NNLayerComponent *component = layer->GetComponent();
			if (component) {
				C->Stroke->Color = 0xff000000 | component->GetColor();
			}

			C->Fill->Color = SDEEP_CONSOLE_COLOR_LIGHTER;
			C->FillRect(
				RectF(0, 0, PB_Property->Width, default_property_header_height),
				0.0f, 0.0f, AllCorners, 1.0, TCornerType::Round);

			int offset = 0;
			if (nn_document_.GetNumSelectedLayer() == 1){
				C->Font->Style = TFontStyles();
				C->Font->Size = 42;//NNGraphMapperCapitalFontSize
				C->Fill->Color = C->Stroke->Color;
				C->FillRect(
					RectF(default_grid_text_hspace, 0, default_grid_text_hspace + default_property_header_height, default_property_header_height),
					0.0f, 0.0f, AllCorners, 1.0, TCornerType::Round);
				C->Fill->Color = SDEEP_CONSOLE_COLOR_LIGHTEST;
				C->FillText(
					RectF(default_grid_text_hspace, -3, default_grid_text_hspace + default_property_header_height, default_property_header_height - 3),
					layer->GetType().SubString(1, 1).PChar(),
					false,
					1.0f,
					TFillTextFlags(),
					TTextAlign::Center,
					TTextAlign::Center);
				C->Font->Style = TFontStyles();
				C->Fill->Color = SDEEP_CONSOLE_COLOR_DARKEST;
				C->Font->Size = 24;
				offset = default_grid_text_hspace + default_property_header_height;
			}else {
				//multiple layers are selected
				C->Font->Style = TFontStyles() << TFontStyle::fsBold;
				C->Fill->Color = SDEEP_CONSOLE_COLOR_TEXTRED;
				C->Font->Size = 16;
			}

			C->FillText(
				RectF(offset + default_grid_text_hspace, default_grid_text_vspace, PB_Property->Width - default_grid_text_hspace, default_property_header_height - default_grid_text_vspace),
				nn_document_.GetNumSelectedLayer() == 1 ? UnicodeString(layer->GetType().PChar()) : (IntToStr(nn_document_.GetNumSelectedLayer()) + UnicodeString(" layers are selected.")),
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center);
		}
	}

	C->EndScene();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_PropertyMouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, float X, float Y)
{
	ClearEditBox();
	Sc_Property->Tag = 0;
	Application->ProcessMessages();//need to clear old edit box before show new edit box
	Bu_Property->SetFocus();
	network_focuse_index_ = 2;
	if (Button == TMouseButton::mbLeft) {
		nn_document_.SetCurrentPropertyIndex(-1);

		for (int ip = 0 ; ip < property_rect_.GetSize() ; ip++) {
			const TRectF &rect = property_rect_.GetAt(ip);
			if (rect.Contains(Point(X, Y))){
				nn_document_.SetCurrentPropertyIndex(ip);
				break;
			}
		}

		for (int ip = 0 ; ip < property_value_rect_.GetSize() ; ip++) {
			const TRectF &rect = property_value_rect_.GetAt(ip);
			if (rect.Contains(Point(X, Y))){
				nn_document_.SetCurrentPropertyIndex(ip);
				if (rect.top >= default_property_height &&
					rect.bottom <= PB_Property->Height) {
					EditProperty();
				}
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
int TFo_Main::EditProperty()
{
	const NNLayer *layer = nn_document_.GetLayerByID(nn_document_.GetCurrentLayerID());
	if (layer) {
		if (nn_document_.GetCurrentPropertyIndex() >= 0 &&
			nn_document_.GetCurrentPropertyIndex() < layer->GetNumProperty() &&
			property_value_rect_.GetSize() == layer->GetNumProperty()) {

			const NNLayerProperty &property = layer->GetProperty(nn_document_.GetCurrentPropertyIndex());
			const TRectF &rect = property_value_rect_.GetAt(nn_document_.GetCurrentPropertyIndex());
			if (property.GetEditable()){
				const NNLayerComponent *component = layer->GetComponent();
				if (component) {
					const NNLayerComponentProperty *component_property = component->GetPropertyByName(property.GetName());
					switch (property.GetType()) {
					case NNLayerPropertyType::Boolean:
						if (property.GetValue() == "True") {
							for (int il = 0 ; il < nn_document_.GetNumLayer() ; il++) {
								const NNLayer &layer2 = nn_document_.GetLayer(il);
								const int property_index = layer2.GetPropertyIndexByName(property.GetName());
								if (layer2.GetSelected() && property_index >= 0) {
									nn_document_.SetLayerPropertyValue(layer2.GetLayerID(), property_index, FString("False"));
								}
							}
						}else {
							for (int il = 0 ; il < nn_document_.GetNumLayer() ; il++) {
								const NNLayer &layer2 = nn_document_.GetLayer(il);
								const int property_index = layer2.GetPropertyIndexByName(property.GetName());
								if (layer2.GetSelected() && property_index >= 0) {
									nn_document_.SetLayerPropertyValue(layer2.GetLayerID(), property_index, FString("True"));
								}
							}
						}
						break;
					case NNLayerPropertyType::Option:
						if (component_property) {
							Co_Property->Items->Clear();
							const FStringList &options = component_property->GetOptions();
							for (int i = 0 ; i < options.GetSize() ; i++) {
								Co_Property->Items->Add(options.GetAt(i).PChar());
								if (property.GetValue() == options.GetAt(i)) {
									Co_Property->ItemIndex = i;
								}
							}
							Co_Property->SetBounds(
								rect.left + PB_Property->Position->X - 1,
								rect.top + PB_Property->Position->Y - 1,
								rect.Width() / Co_Property->Scale->X + 2,
								rect.Height() / Co_Property->Scale->Y + 2);
							Co_Property->Visible = true;
							Co_Property->SetFocus();
							Co_Property->DropDown();
							current_property_index_ = nn_document_.GetCurrentPropertyIndex();
							current_property_layer_id_ = layer->GetLayerID();
						}else{
							return false;
						}
						break;
					default:
						Ed_Property->Text = property.GetValue().PChar();
						Lo_EditProperty->SetBounds(
							rect.left + PB_Property->Position->X - 1,
							rect.top + PB_Property->Position->Y - 1,
							rect.Width() + 2,
							rect.Height() + 2);
						Ed_Property->SelectAll();
						Bu_PropertyBrowse->Visible = property.GetType() == NNLayerPropertyType::File || property.GetType() == NNLayerPropertyType::Model;
						Lo_EditProperty->Visible = true;
						Ed_Property->SetFocus();
						current_property_index_ = nn_document_.GetCurrentPropertyIndex();
						current_property_layer_id_ = layer->GetLayerID();
						break;
					}
					return true;
				}
			}
		}
	}

	return false;
}
//---------------------------------------------------------------------------
void TFo_Main::ClearEditBox()
{
	Lo_EditProperty->Visible = false;
	Co_Property->Visible = false;
	Ed_NetworkName->Visible = false;
	current_property_index_ = -1;
	current_property_layer_id_ = -1;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::TV_ComponentClick(TObject *Sender)
{
	network_focuse_index_ = 0;
	ClearEditBox();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_PropertyKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	switch (Key) {
	case 27:
		ClearEditBox();
		Bu_Property->SetFocus();
		break;
	case 13:
		ApplyProperty();
		ClearEditBox();
		Bu_Property->SetFocus();
		break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_PropertyKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	//Caption = UnicodeString("Property Keydown ") + IntToStr(Key);
	ClearEditBox();
	if (nn_document_.GetCurrentLayerID() >= 0) {
		const NNLayer *layer = nn_document_.GetLayerByID(nn_document_.GetCurrentLayerID());
		if (layer) {
			switch (Key) {
			case 13:
				//Enter
			case 113:
				//F2
				EditProperty();
				break;
			case 27:
				//Esc
				Bu_Network->SetFocus();
				break;
			case 38:
				//Up
				{
					const int last = nn_document_.GetCurrentPropertyIndex();
					while (true){
						if (nn_document_.GetCurrentPropertyIndex() >= 0) {
							nn_document_.SetCurrentPropertyIndex(nn_document_.GetCurrentPropertyIndex() - 1);
						}
						if (nn_document_.GetCurrentPropertyIndex() < 0) {
							nn_document_.SetCurrentPropertyIndex(layer->GetNumProperty() - 1);
						}
						if (nn_document_.GetCurrentPropertyIndex() == last) {
							break;
						}
						if (nn_document_.GetCurrentPropertyIndex() >= 0) {
							if (nn_document_.GetLayerPropertyVisible(*layer, nn_document_.GetCurrentPropertyIndex())) {
								break;
							}
						}
					}
				}
				break;
			case 40:
				//Down
				//Up
				{
					const int last = nn_document_.GetCurrentPropertyIndex();
					while (true){
						if (nn_document_.GetCurrentPropertyIndex() < layer->GetNumProperty() - 1) {
							nn_document_.SetCurrentPropertyIndex(nn_document_.GetCurrentPropertyIndex() + 1);
						}else {
							nn_document_.SetCurrentPropertyIndex(0);
						}
						if (nn_document_.GetCurrentPropertyIndex() == last) {
							break;
						}
						if (nn_document_.GetCurrentPropertyIndex() >= 0) {
							if (nn_document_.GetLayerPropertyVisible(*layer, nn_document_.GetCurrentPropertyIndex())) {
								break;
							}
						}
					}
				}
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::FormKeyUp(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	//Caption = UnicodeString("Form Keyup ") + IntToStr(Key);
	if (TC_Default->Visible && TC_Default->ActiveTab == TI_Network) {
		if (Key == 9) {
			//Tab
			if (Shift.Contains(ssShift)) {
				network_focuse_index_ = (network_focuse_index_ + 2) % 3;
			}else {
				network_focuse_index_ = (network_focuse_index_ + 1) % 3;
			}
			switch (network_focuse_index_) {
			case 0:
				Fr_Component->Bu_List->SetFocus();
				break;
			case 1:
				Bu_Property->SetFocus();
				break;
			case 2:
				Bu_Network->SetFocus();
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIN_SelectAllClick(TObject *Sender)
{
	nn_document_.SelectAllLayer();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIN_ClearSelectionClick(TObject *Sender)
{
	nn_document_.ClearSelection();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIN_DeleteClick(TObject *Sender)
{
	nn_document_.KeepForUndo(FString("Delete"));
	nn_document_.DeleteSelected();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_StatisticsPaint(TObject *Sender, TCanvas *Canvas)
{
	statistics_rect_.Clear();

	static const int default_statistics_height = 28;
	static const int default_statistics_left_width = 160;

	TCanvas *C = PB_Statistics->Canvas;

	C->BeginScene();
	C->SetMatrix(PB_Statistics->AbsoluteMatrix);
	C->Clear(SDEEP_CONSOLE_COLOR_BACKGROUND);
	if (nn_document_.GetLocked()) {
		C->EndScene();
		return;
	}

	C->Fill->Kind = TBrushKind::Solid;
	C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
	C->Stroke->Kind = TBrushKind::Solid;
	C->Font->Size = 16;

	if (network_overview_){
		const FListP<NNStatistics> &statistics = network_overview_->GetStatistics();

		for (int is = 0 ; is < statistics.GetSize() ; is++) {
			const int y = is * default_statistics_height;
			if (network_overview_->GetCurrentStatisticsIndex() == is) {

				C->Fill->Color = SDEEP_CONSOLE_COLOR_LIGHT;
				C->FillRect(
					RectF(0, y, PB_Statistics->Width, y + default_statistics_height),
					0.0f, 0.0f, AllCorners, 1.0, TCornerType::Round);
			}
		}

		C->Stroke->Thickness = 1;
		for (int is = 0 ; is < statistics.GetSize() ; is++) {
			const int y = is * default_statistics_height;
			if (is < statistics.GetSize() - 1){
				C->DrawLine(
					Point(default_grid_text_hspace, y + default_statistics_height),
					Point(PB_Statistics->Width - default_grid_text_hspace, y + default_statistics_height),
					1.0f);
			}

			TRectF statistics_rect = RectF(0, y, default_statistics_left_width, y + default_statistics_height);
			TRectF statistics_value_rect = RectF(default_statistics_left_width, y, PB_Statistics->Width, y + default_statistics_height);

			const NNStatistics &stat_1 = statistics.GetAt(is);

			//Statistics Label
			C->Fill->Color = SDEEP_CONSOLE_COLOR_HALF;
			C->FillText(
				RectF(
					statistics_rect.left + default_grid_text_hspace,
					statistics_rect.top + default_grid_text_vspace,
					statistics_rect.right - default_grid_text_hspace,
					statistics_rect.bottom - default_grid_text_vspace),
				stat_1.GetLabel().PChar(),
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center);

			//Value
			C->Fill->Color = SDEEP_CONSOLE_COLOR_DARKEST;
			C->FillText(
				RectF(
					statistics_value_rect.left + default_grid_text_hspace,
					statistics_value_rect.top + default_grid_text_vspace,
					statistics_value_rect.right - default_grid_text_hspace,
					statistics_value_rect.bottom - default_grid_text_vspace),
				FString(stat_1.GetSum()).InsertThousandSeparator().PChar(),
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center);

			statistics_rect.right = statistics_value_rect.Right;
			statistics_rect_.Add(statistics_rect);
		}
    }

	C->EndScene();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_StatisticsMouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, float X, float Y)
{
	ClearEditBox();
	Bu_Statistics->SetFocus();
	if (Button == TMouseButton::mbLeft && network_overview_) {
		network_overview_->SetCurrentStatisticsIndex(-1);

		for (int is = 0 ; is < statistics_rect_.GetSize() ; is++) {
			const TRectF &rect = statistics_rect_.GetAt(is);
			if (rect.Contains(Point(X, Y))){
				network_overview_->SetCurrentStatisticsIndex(is);
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_StatisticsKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	//Caption = UnicodeString("Statistics Keydown ") + IntToStr(Key);
	ClearEditBox();
	switch (Key) {
	case 38:
		//Up
		if (nn_document_.GetCurrentStatisticsIndex() > 0) {
			nn_document_.SetCurrentStatisticsIndex(nn_document_.GetCurrentStatisticsIndex() - 1);
		}else {
			nn_document_.SetCurrentStatisticsIndex(nn_document_.GetStatistics().GetSize() - 1);
		}
		break;
	case 40:
		//Down
		if (nn_document_.GetCurrentStatisticsIndex() < nn_document_.GetStatistics().GetSize() - 1) {
			nn_document_.SetCurrentStatisticsIndex(nn_document_.GetCurrentStatisticsIndex() + 1);
		}else {
			nn_document_.SetCurrentStatisticsIndex(0);
		}
		break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIN_ArrangeLayersClick(TObject *Sender)
{
	nn_document_.KeepForUndo(FString("Arrange Layers"));
	nn_document_.ArrangeLayers(false);
}
//---------------------------------------------------------------------------
void TFo_Main::SetEnvironment()
{
	const FStringList &local_environment_variable = settings_.GetLocalEnvironmentVariable();
	for (int i = 0 ; i < local_environment_variable.GetSize() ; i++) {
		FString value = local_environment_variable.GetAt(i).ReplaceStr(".\\", AnsiString(ExtractFilePath(ParamStr(0))).c_str());
		FString key = value.SplitBy('=');
		SetEnvironmentVariable(
			UnicodeString(key.PChar()).c_str(),
			UnicodeString(value.PChar()).c_str());
	}
}
//---------------------------------------------------------------------------
int TFo_Main::DisplayDocumentError()
{
	ResetMonitor(false);
	if (!nn_document_.GetStructureSearchEnabled()) {
		Bu_TrainingTabClick(this);
	}
	Me_Monitor->SetFocus();
	Me_Monitor->Lines->BeginUpdate();
	Me_Monitor->Lines->Clear();

	int num_error = 0;
	nn_document_.CheckError();
	if (nn_document_.GetMaxEpoch() &&
		nn_document_.GetCommandType() != SDeepCommandType::CreateTrainingResult &&
		nn_document_.GetCommandType() != SDeepCommandType::Script){
		CheckDataSizeError();
	}
	if ((!nn_document_.GetCompleteNetwork() || nn_document_.GetErrors().GetSize() > 0) &&
		nn_document_.GetCommandType() != SDeepCommandType::Script){
		network_overview_ = &nn_document_;
		network_overview_->ClearSelection();
		current_errors_ = nn_document_.GetErrors();
		for (int ie = 0 ; ie < current_errors_.GetSize() ; ie++) {
			Me_Monitor->Lines->Add(UnicodeString("[Error ") + IntToStr(++num_error) + UnicodeString("] ") + current_errors_.GetAt(ie).GetString().PChar());
		}
	}

	Me_Monitor->Lines->EndUpdate();

	if (num_error) {
		Log(FString("training cannot be started due to an error"));
		Me_Monitor->Lines->Add(IntToStr(num_error) + " errors.");
		Me_Monitor->GoToTextEnd();
		return false;
	}
	return true;
}
//---------------------------------------------------------------------------
int TFo_Main::PrepareTraining()
{
	if (!FileExists(nn_document_.GetFileName().PChar())) {
		ShowMessage(GetTranslatedText("You must save project file before processing."));
		if (!SaveAs()){
			return false;
		}
	}

	if (!DisplayDocumentError()){
		return false;
	}

	//Add New Result
	UnicodeString base_datetime_str, datetime_str;
	base_datetime_str = datetime_str = FormatDateTime("yyyymmdd_hhnnss", Now());
	int index = 0;
	while (DirectoryExists(UnicodeString(nn_document_.GetResultPath(FString("\\")).PChar()) + datetime_str)){
		index++;
		datetime_str = base_datetime_str + UnicodeString("_") + IntToStr((int)index);
	}
	nn_document_.AddResult(FString(AnsiString(datetime_str).c_str()), FString("\\"));

	//Create Directory for Result
	UnicodeString result_dir = nn_document_.GetResultFullDir(FString("\\")).PChar();
	if (!DirectoryExists(result_dir)) {
		ForceDirectories(result_dir);
	}

	//Save BMP
	nn_document_.ClearSelection();
	nn_document_.SetCurrentStatisticsIndex(nn_document_.GetStatisticsIndexByLabel(FString("Output")));
	nn_graph_mapper_.Update(nn_document_, true);
	nn_graph_mapper_.Animation(1.0);
	SaveNetworkBitmapToFile(nn_document_, (nn_document_.GetResultFullDir(FString("\\")) + FString("\\network.png")).PChar());

	return true;
}
//---------------------------------------------------------------------------
void TFo_Main::ExecuteTraining(NNDocument &document, int resume)
{
	const int instance_id = Co_Instance->ItemIndex >= 0 && Co_Instance->ItemIndex < instance_type_.GetSize() ?
		instance_type_.GetAt(Co_Instance->ItemIndex) : 0;
	const int num_node = Co_NumNode->ItemIndex >= 0 ? StrToIntDef(Co_NumNode->Items->Strings[Co_NumNode->ItemIndex], 1): 1;
	FString mode;
	switch (nn_document_.GetCommandType()){
	case SDeepCommandType::Normal:
		if (nn_document_.GetMaxEpoch() > 0){
			mode = FString("train");
			break;
        }
	case SDeepCommandType::CreateTrainingResult:
		mode = FString("create_training_result");
		break;
	case SDeepCommandType::Profiling:
		mode = FString("profile");
		break;
	case SDeepCommandType::CompareWithCPU:
		mode = FString("compare_with_cpu");
		break;
	case SDeepCommandType::Script:
		mode = FString("script, script=") + nn_document_.GetScriptName();
		break;
	}
	Log(FString("start training : mode=") +
		mode +
		FString(", instance=")+
		FString(instance_id) +
		FString(", num_node=")+
		FString(num_node) +
		FString(", processor=")+
		settings_.ProcessorTypeStr() +
		FString(", structure_search=") +
		FString(nn_document_.GetStructureSearchEnabled()));

	switch (document.GetSDeepEngineType()) {
	case SDeepEngineType::Torch:
	case SDeepEngineType::NNabla:
		ShowAnimationIndicator(GetTranslatedText("Executing Neural Network Libraries..."));
		nn_document_.CompleteNetworks(nn_document_);
		SetEnvironment();
		if (Co_Instance->ItemIndex == 0){
			//Local Processor
			SetEnvironmentVariableW(
				UnicodeString("TRAIN_PROCESSOR_INDEX").c_str(),
				UnicodeString("").c_str());
			if (document.GetSDeepEngineType() == SDeepEngineType::NNabla) {
				sdeep_training_thread_.Add(new NNablaLocalTrainingThread(
					nn_document_.GetCurrentResultName(), nn_document_.GetResultFullDir(FString("\\")),
					this, &document, &settings_, false, false, resume));
			}
			else {
				sdeep_training_thread_.Add(new TorchLocalTrainingThread(
					nn_document_.GetCurrentResultName(), nn_document_.GetResultFullDir(FString("\\")),
					this, &document, &settings_, false, false, resume));
			}
		}else if (instance_id & 0x7f000000) {
			// SSH Remote
			const FString num = FString(instance_id & 0xffffff);
			SetEnvironmentVariableW(
				UnicodeString("TRAIN_PROCESSOR_INDEX").c_str(),
				UnicodeString(num.PChar()).c_str());
			sdeep_training_thread_.Add(new NNablaLocalTrainingThread(
				nn_document_.GetCurrentResultName(), nn_document_.GetResultFullDir(FString("\\")),
				this, &document, &settings_, false, true, resume));

		} else {
			// Neural Network Console Cloud
			sdeep_training_thread_.Add(new NNablaRemoteTrainingThread(
				nn_document_.GetCurrentResultName(), nn_document_.GetResultFullDir(FString("\\")),
				this, &document, &settings_, instance_id, num_node, resume));
		}
		break;
	default:
		SetEnvironment();
		// ShowMessage(nn_document_.GetScriptFileName().PChar());
		ScriptThread *script_thread = new ScriptThread(
			nn_document_.GetCurrentResultName(), nn_document_.GetResultFullDir(FString("\\")),
			this, &document, &settings_, false, false, resume);
		script_thread->SetOption(nn_document_.GetScriptType());
		sdeep_training_thread_.Add(script_thread);
		break;
	}

	/*
	if (TC_Left->ActiveTab == TI_TrainingLeft) {
		RefreshResultsList(false);
	}
	*/
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_ControlClick(TObject *Sender)
{
	const ControlButtonType control_button_type_bak = control_button_type_;//control_button_type_ will be changed during PrepareTraining
	switch (control_button_type_bak){

	case ControlButtonType::Run:
		{
			nn_document_.SetCommandType(SDeepCommandType::Script);
			nn_document_.SetScriptName(FString(""));
			nn_document_.SetScriptFileName(FString(""));
			nn_document_.SetScriptArgs(FString(""));

			SDeepEngineType sdeep_engine_type = nn_document_.GetSDeepEngineType();
			const NNScripts &scripts = settings_.GetEngineInfo().GetAt((int)sdeep_engine_type).GetScripts();
			if (Co_Run->ItemIndex >= 0 && Co_Run->ItemIndex < scripts.GetSize()) {
				nn_document_.SetScriptType(scripts.GetAt(Co_Run->ItemIndex).GetType());
				nn_document_.SetScriptFileName(FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()) + scripts.GetAt(Co_Run->ItemIndex).GetCLI());
				if (!PrepareTraining()){
					return;
				}

				ExecuteTraining(nn_document_);
			}
		}
		break;

	case ControlButtonType::RunTraining:
	case ControlButtonType::ScheduleTraining:
		if (Co_Training->Selected){
			//Check if train or schedule
			if (!TC_Default->Visible || nn_document_.GetLocked()){
				return;
			}
			int schedule = false;
			if (Co_Training->ItemIndex != 2){// != "Create Training Result"
				//Train, Profile, Plugin
				if (Co_Instance->ItemIndex >= 0 && Co_Instance->ItemIndex != 0){// != "Local Processor"
					//Remote
					if (Co_Instance->ItemIndex < instance_available_.GetSize() && instance_available_.GetAt(Co_Instance->ItemIndex)){
						//Available
						//run
					}else {
						//Not Available
						return;
					}
				}else {
					//Local
					if (!GetTraining(true)){
						//No local traninig jobs
						//run
					}else {
						//Local training job already exists
						schedule = true;
					}
				}
			}else {
				//Create Training Result
				if (!GetTraining(true)){
					//No local traninig jobs
					//run
				}else {
					return;
				}
			}

			nn_document_.SetScriptName(FString(""));
			nn_document_.SetScriptFileName(FString(""));
			nn_document_.SetScriptArgs(FString(""));
			switch (Co_Training->ItemIndex) {
			case 0://"Train"
				break;
			case 1://"Profile"
				nn_document_.SetCommandType(SDeepCommandType::Profiling);
				break;
			case 2:// "Create Training Result"
				nn_document_.SetCommandType(SDeepCommandType::CreateTrainingResult);
				break;
			default:// Plugin
				nn_document_.SetCommandType(SDeepCommandType::Script);
				{
					NNCPlugins training_plugins = plugins_.GetPlugins(FString("_Training"));
					for (int i = 0 ; i < training_plugins.GetSize() ; i++){
						if (Co_Training->Items->Strings[Co_Training->ItemIndex] == training_plugins.GetAt(i).name_.PChar()){
							const NNCPlugin &plugin = training_plugins.GetAt(i);
							FString args;
							if (plugin.arg_option_.GetSize()) {
								if (!ShowPluginWindow(plugin, args, NULL, NULL, NULL)) {
									return;
								}
                            }
							nn_document_.SetScriptName(plugin.name_);
							nn_document_.SetScriptFileName(
								FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()) +
								FString("libs\\plugins\\_Training\\") +
								plugin.file_name_);
							nn_document_.SetScriptArgs(args);
							break;
						}
					}
				}

				break;
			}

			if (!PrepareTraining()){
				return;
			}

			if (!schedule){
				ExecuteTraining(nn_document_);
			}else {
				ScheduleTraining();
			}
		}
		break;

	case ControlButtonType::StopAll:
		{
			//Stop training
			for (int it = sdeep_training_thread_.GetSize() - 1 ; it >= 0 ; it--){
				SDeepTrainingThread *thread = sdeep_training_thread_.GetAt(it);
				thread->SetStopped(true);
				thread->Terminate();
			}
            //Stop evaluation
			if (GetEvaluating()) {
				HideAnimationIndicator();
				sdeep_evaluation_thread_->SetStopped(true);
				sdeep_evaluation_thread_->Terminate();
			}
		}
		break;

	case ControlButtonType::StopTraining:
		{
			if (GetTraining()) {
				Log(FString("stop training"));

				HideAnimationIndicator();


				if (Fr_ResultsList->GetItemIndex() >= 0 &&
					Fr_ResultsList->GetItemIndex() < nn_document_.GetResults().GetSize()) {
					const FString &job_name = nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetDir();
					for (int it = sdeep_training_thread_.GetSize() - 1 ; it >= 0 ; it--){
						SDeepTrainingThread *thread = sdeep_training_thread_.GetAt(it);
						if (thread->GetName() == job_name){
							thread->SetStopped(true);
							thread->Terminate();
							break;
						}
					}
				}
			}
		}
		break;

	case ControlButtonType::RunEvaluation:
		{
			if (nn_document_.GetCurrentResultName() != ""){
				if (ExecuteEvaluation(nn_document_.GetCurrentResultName(), nn_document_.GetResultFullDir(FString("\\")), false)){
					ResetEvaluationResult();
					Bu_EvaluationTabClick(this);
					Me_Evaluation->SetFocus();
				}
			}
		}
		break;

	case ControlButtonType::StopEvaluation:
		{
			if (GetEvaluating()) {
				Log(FString("stop evaluation"));
				HideAnimationIndicator();
				sdeep_evaluation_thread_->SetStopped(true);
				sdeep_evaluation_thread_->Terminate();
			}
		}
		break;

	default:
		{
		}
		break;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ScheduleTraining()
{
	Log(FString("schedule training"));

	SDeepProcessThread *thread = NULL;
	switch (nn_document_.GetSDeepEngineType()) {
    case SDeepEngineType::Torch:
	case SDeepEngineType::NNabla:
		SetEnvironment();
		thread = new NNablaLocalTrainingThread(nn_document_.GetCurrentResultName(), nn_document_.GetResultFullDir(FString("\\")), this, &nn_document_, &settings_, true);
		delete thread;
		thread = NULL;
		break;
	default:
		break;
	}

	nn_document_.RefreshResults(FString("\\"));
	if (TC_Left->ActiveTab == TI_TrainingLeft) {
		RefreshResultsList(false);
		//Fr_ResultsList->SetItemIndex(nn_document_.GetCurrentResultIndex());
	}
}
//---------------------------------------------------------------------------
int TFo_Main::TrainNext()
{
	if (!sdeep_training_thread_.GetSize()){
		if (TrainScheduled()) {
			//Resume scheduled task
		}else if (nn_document_.GetStructureSearchEnabled()) {
			OpenResult();
			OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
			ContinueStructureSearch();
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
int TFo_Main::ExecuteEvaluation(const FString &name, const FString &result_dir, int auto_evaluation)
{
	if (GetEvaluating()) {
		//Evaluation process already executed
		return false;
	}

	switch (nn_document_.GetSDeepEngineType()) {
	case SDeepEngineType::Torch:
		Log(FString("start evaluation torch"));
		ShowAnimationIndicator(GetTranslatedText("Executing Neural Network Libraries..."));

		SetEnvironment();
		sdeep_evaluation_thread_ = new TorchLocalEvaluationThread(name, result_dir, this, &nn_document_, &settings_);
		sdeep_evaluation_thread_->SetAutoEvaluation(auto_evaluation);

		if (TC_Left->ActiveTab == TI_TrainingLeft) {
			RefreshResultsList(false);
		}
		break;
	case SDeepEngineType::NNabla:
		Log(FString("start evaluation"));
		ShowAnimationIndicator(GetTranslatedText("Executing Neural Network Libraries..."));

		SetEnvironment();

		sdeep_evaluation_thread_ = new NNablaLocalEvaluationThread(name, result_dir, this, &nn_document_, &settings_);
		sdeep_evaluation_thread_->SetAutoEvaluation(auto_evaluation);

		if (TC_Left->ActiveTab == TI_TrainingLeft) {
			RefreshResultsList(false);
		}
		break;
	default:
        break;
	}

	return true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::FormResize(TObject *Sender)
{
	Bu_SaveAs->Position->X = Bu_Setup->Position->X - Bu_SaveAs->Width;
	Bu_Save->Position->X = Bu_SaveAs->Position->X - Bu_Save->Width;
	Bu_ConfigTab->Position->X = Bu_Save->Position->X - Bu_ConfigTab->Width;
	Bu_DatasetTab->Position->X = Bu_ConfigTab->Position->X - Bu_DatasetTab->Width;

	Pa_Processing->Position->X = (FormLayout->Width - Pa_Processing->Width) / 2;
	Pa_Processing->Position->Y = (FormLayout->Height - Pa_Processing->Height) / 2;

	if (TC_Left->ActiveTab == TI_NetworkLeft){
		if (Fr_Component->Size->Height < Sp_NetworkLeft->MinSize){
			int height = TC_Left->Size->Height - Sp_NetworkLeft->MinSize;
			if (height < Sp_NetworkLeft->MinSize){
				height = Sp_NetworkLeft->MinSize;
			}
			Lo_Property->Size->Height = height;
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::WaitForDocumentUnlock()
{
	Ti_Animation->Enabled = false;
	while (nn_document_.GetLocked()){
		Application->HandleMessage();
		CheckSplashWindow();
		CheckAnimationIndicator();
		CheckSynchronize();
		Sleep(1);
	}
	Ti_Animation->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_DatasetOpenClick(TObject *Sender)
{
	if (Lo_Table->Visible) {
		Lo_Table->Visible = false;
		Re_DatasetTop2->Visible = false;
		Fr_DatasetFiles2->Ed_ListSearch->Text = "";
		RefreshDatasetFiles();
		Fr_DatasetFiles2->Visible = true;
		Bu_OpenDataset2->Visible = true;
		Bu_CreateDataset2->Visible = true;
		RefreshListSettings();
	}else {
		Bu_OpenDataset2->Visible = false;
		Bu_CreateDataset2->Visible = false;
		Fr_DatasetFiles2->Visible = false;
		Lo_Table->Visible = true;
		Re_DatasetTop2->Visible = true;
	}
	Invalidate();
}
//---------------------------------------------------------------------------
void TFo_Main::OpenDataset(int dataset_index, UnicodeString file_name, int update_dataset_list)
{
	if (!FileExists(file_name)) {
		Activate();
		ShowMessage(GetTranslatedText("Error : Dataset file is not found.")+ UnicodeString("\n\"") + file_name + UnicodeString("\""));
		return;
	}

	ShowAnimationIndicator(GetTranslatedText("Loading dataset..."));
	nn_document_.Lock();
	Ti_AnimationTimer(this);
	//TC_Default->Enabled = false; // it causes focus loss on Fr_DatasetFiles
	TThread::CreateAnonymousThread(
		new TDocumentIOFunc(&nn_document_, &settings_, file_name, DocumentIOType::OpenDataset, dataset_index, online_debug_mode_)
		)->Start();
	WaitForDocumentUnlock();
	dataset_internal_update_count_++;

	const CSVDataset &dataset = nn_document_.GetDataset(dataset_index);
	Log(FString("open dataset : index=") +
		FString(dataset_index) +
		FString(", num_data=") +
		FString(dataset.GetNumData()) +
		FString(", num_column=") +
		FString(dataset.GetNumColumn()));
	if (update_dataset_list){

		ListItem new_dataset;
		new_dataset.AddAttribute(FString("File Name"), FString(AnsiString(file_name).c_str()));
		new_dataset.AddAttribute(FString("Name"), FString(AnsiString(ExtractFileName(file_name)).c_str()));
		new_dataset.AddAttribute(FString("Location"), FString(AnsiString(ExtractFilePath(file_name)).c_str()));
		new_dataset.AddAttribute(FString("DateTime"), FString(AnsiString(DateTimeToStr(Now())).c_str()));
		FString description =
			FString("Num Data = ") + FString(dataset.GetNumData()) + FString("\\n") +
			FString("Num Column = ") + FString(dataset.GetNumColumn()) + FString("\\n") +
			FString("Columns = ");
		for (int ic = 0 ; ic < dataset.GetNumColumn() && ic < 32 ; ic++) {
			if (ic > 0) {
				description += FString(", ") + dataset.GetColumnName(ic);
			}else {
				description += dataset.GetColumnName(ic);
			}
		}
		if (dataset.GetNumColumn() > 32) {
			description += "...";
		}
		new_dataset.AddAttribute(FString("Description"), description);

		dataset_list_.AddListItem(FString("File Name"), new_dataset);
		dataset_list_.SaveToFile();
	}

	HideAnimationIndicator();
	if (TC_Left->ActiveTab == TI_DatasetLeft) {
		RefreshDatasetList();
	}
	if (TC_Default->ActiveTab == TI_Dataset) {
		Bu_Table->SetFocus();
	}
	Sc_TableX->Tag = 0;
	Sc_TableY->Tag = 0;
}
//---------------------------------------------------------------------------
int TFo_Main::RegisterAndOpenNewDataset(int dataset_index)
{
	if (!OD_CSV->Execute()) {
		return false;
	}

	OpenDataset(dataset_index, OD_CSV->FileName);
	return true;
}
//---------------------------------------------------------------------------
int TFo_Main::RegisterProject(UnicodeString file_name)
{
	ListItem new_project;
	new_project.AddAttribute(FString("File Name"), FString(AnsiString(file_name).c_str()));
	new_project.AddAttribute(FString("Name"), FString(AnsiString(ExtractFileName(file_name)).c_str()));
	new_project.AddAttribute(FString("Location"), FString(AnsiString(ExtractFilePath(file_name)).c_str()));
	new_project.AddAttribute(FString("DateTime"), FString(AnsiString(DateTimeToStr(Now())).c_str()));

	FString description;
	for (int id = 0 ; id < nn_document_.GetNumDataset() ; id++) {
		const NNDataset &dataset = nn_document_.GetDataset(id);
		if (dataset.GetNumData()) {
			if (description != "") {
                description += FString("\\n");
			}
			description +=
				FString("Dataset \"") + dataset.GetName() + FString("\": ") +
				FString(AnsiString(ExtractFileName(dataset.GetFileName().PChar())).c_str()) +
				FString("    (") +
				FString(dataset.GetNumData()) +
				FString(" samples, ") +
				FString(dataset.GetNumColumn()) +
				FString(" columns)");
		}
	}

	new_project.AddAttribute(FString("Description"), description);

	project_list_.AddListItem(FString("File Name"), new_project);
	project_list_.SaveToFile();

	return true;
}
//---------------------------------------------------------------------------
void DrawCSVBitmap(TCanvas *C, TRectF rect, const CSVMatrix &csv, int transpose)
{
	const int width = rect.GetWidth();
	const int height = rect.GetHeight();
	TBitmap *BMP = new TBitmap(width, height);
	TBitmapData BMPData;
	BMP->Map(TMapAccess::Write, BMPData);
	const float range =csv.GetMax() - csv.GetMin();
	float norm_coef = 0.5;
	float norm_offset = csv.GetMin();
	if (range <= 0.0f) {
		norm_offset -= 1.0;
	}else {
        norm_coef = 1.0f / range;
	}
	if (transpose) {
		for (int iy = 0; iy < height ; iy++) {
			const int col = ((__int64)iy * csv.GetNumCol()) / height;
			for (int ix = 0; ix < width ; ix++) {
				const int row = ((__int64)ix * csv.GetNumRow()) / width;
				const float normalized_value = (csv.GetValue(row, col) - norm_offset) * norm_coef;
				const int color = 0xff000000 | GetCMColor(normalized_value);
				BMPData.SetPixel(ix, iy, color);
			}
		}
	}else {
		for (int iy = 0; iy < height ; iy++) {
			const int row = ((__int64)iy * csv.GetNumRow()) / height;
			for (int ix = 0; ix < width ; ix++) {
				const int col = ((__int64)ix * csv.GetNumCol()) / width;
				const float normalized_value = (csv.GetValue(row, col) - norm_offset) * norm_coef;
				const int color = 0xff000000 | GetCMColor(normalized_value);
				BMPData.SetPixel(ix, iy, color);
			}
		}
	}
	BMP->Unmap(BMPData);
	C->DrawBitmap(BMP,
		Rect(0, 0, width, height),
		rect,
		1.0,
		true);

	delete BMP;
}
//---------------------------------------------------------------------------
void TFo_Main::DrawDataCell(TCanvas *C, TRectF cell_rect, UnicodeString data_text, FStringList &request_images, FStringList &request_csvs, FListT<CSVCacheFileType> &request_csv_types)
{
	int file = false;
	if (IsImageFile(data_text)) {
		file = true;
		request_images.Add(FString(AnsiString(data_text).c_str()));
		TBitmap *image = (TBitmap *)image_cache_list_->GetImage(AnsiString(data_text).c_str());
		if (image) {
			if (image->Width > 0 && image->Height > 0) {
				const double src_ratio = image->Width * 1.0 / image->Height;
				const double cell_rect_width = cell_rect.Width() - default_dataset_text_space * 2;
				const double cell_rect_height = cell_rect.Height() - default_dataset_text_space * 2;
				const double dest_ratio = cell_rect_width / cell_rect_height;
				TRectF bmp_rect;
				if (src_ratio > dest_ratio) {
					bmp_rect = RectF(
						cell_rect.left + default_dataset_text_space,
						cell_rect.top + default_dataset_text_space + (cell_rect_height - image->Height * (cell_rect_width * 1.0 / image->Width)) * 0.5,
						cell_rect.left + default_dataset_text_space + cell_rect_width,
						cell_rect.top + default_dataset_text_space + (cell_rect_height + image->Height * (cell_rect_width * 1.0 / image->Width)) * 0.5
						);
				}else {
					bmp_rect = RectF(
						cell_rect.left + default_dataset_text_space + (cell_rect_width - image->Width * (cell_rect_height * 1.0 / image->Height)) * 0.5,
						cell_rect.top + default_dataset_text_space,
						cell_rect.left + default_dataset_text_space + (cell_rect_width + image->Width * (cell_rect_height * 1.0 / image->Height)) * 0.5,
						cell_rect.top + default_dataset_text_space + cell_rect_height
						);
				}
				C->DrawBitmap(
					(TBitmap *)image,
					RectF(0, 0, image->Width, image->Height),
					bmp_rect,
					1.0,
					image->Width < cell_rect_width && image->Height < cell_rect_height);
				C->Fill->Color = SDEEP_CONSOLE_COLOR_TEXTSHAPE;
				C->Font->Size = 12;
				C->FillText(
					RectF(
						cell_rect.left + default_dataset_text_space,
						cell_rect.top + default_dataset_text_space + 24,
						cell_rect.right - default_dataset_text_space,
						cell_rect.bottom - default_dataset_text_space),
					UnicodeString("c, ") + IntToStr(image->Height) + UnicodeString(", ") + IntToStr(image->Width),
					false,
					1.0f,
					TFillTextFlags(),
					TTextAlign::Leading,
					TTextAlign::Leading);
			}
		}
	}else if (IsCSVFile(data_text) || IsWavFile(data_text)) {
		file = true;
		//Request CSV file
		request_csvs.Add(FString(AnsiString(data_text).c_str()));
		request_csv_types.Add(IsWavFile(data_text) ? CSVCacheFileType::Wav : CSVCacheFileType::CSV);
		TRectF rect_with_margin = TRectF(cell_rect.left + default_dataset_text_space, cell_rect.top + default_dataset_text_space, cell_rect.right - default_dataset_text_space, cell_rect.bottom - default_dataset_text_space);
		TCanvasSaveState *save_state = C->SaveState();
		C->IntersectClipRect(rect_with_margin);

		//Get CSV file
		const CSVMatrix *csv = csv_cache_list_->GetCSV(AnsiString(data_text).c_str());
		if (csv) {
			csv_graph_exists_ = true;
			const int mode = Co_CSVGraphType->ItemIndex;
			switch(mode){
			case 1:
				//Line (all)
				if (csv->GetNumCol() && csv->GetNumRow() > 1){
					//Draw graph
					C->Stroke->Thickness = 1.0f;
					C->Stroke->Color = SDEEP_CONSOLE_COLOR_BRAND;
					//Column loop
					const float width = rect_with_margin.GetWidth();
					const float total_height = rect_with_margin.GetHeight();
					const float height = total_height / csv->GetNumCol();
					for (int ic = 0 ; ic < csv->GetNumCol() ; ic++) {

						TPathData *path = new TPathData();
						int last_ix = 0;
						float min_y = 0.0f, max_y = 0.0f, last_y = 0.0f;
						//Row loop (Draw single graph)
						for (int ir = 0 ; ir < csv->GetNumRow() ; ir++) {
							const float x = rect_with_margin.left + (ir * width) / (csv->GetNumRow() - 1);
							const float y = rect_with_margin.top + (ic *total_height) / csv->GetNumCol() + (1.0f - (csv->GetValue(ir, ic) + 1.0) * 0.5) * height;
							if (ir == 0) {
								path->MoveTo(PointF(x, y));
								last_ix = (int)x;
								min_y = max_y = last_y = y;
							}else if ((int)x != last_ix || ir == csv->GetNumRow() - 1) {
								if (fabs(max_y - min_y) >= 0.01) {
									path->MoveTo(PointF(last_ix, min_y));
									path->LineTo(PointF(last_ix, max_y));
									path->MoveTo(PointF(x, y));
								}else{
									path->LineTo(PointF(x, y));
								}
								last_ix = (int)x;
								min_y = max_y = last_y = y;
							}else {
								if (y < min_y) {
									min_y = y;
								}
								if (y > max_y) {
									max_y = y;
								}
								last_y = y;
							}
						}
						C->DrawPath(path, 1.0);
						delete path;
					}
				}
				break;
			case 2:
			case 3:
				{
					//2D
					DrawCSVBitmap(C, rect_with_margin, *csv, mode == 3);
				}
				break;
			default:
				//Line (summary)
				{
					//Row size (max = graph width)
					const int draw_row_size = csv->GetNumRow() < default_dataset_col_width_ - default_dataset_text_space * 2 ? csv->GetNumRow() : default_dataset_col_width_ - default_dataset_text_space * 2;

					//Column size (max = 10)
					const int draw_col_num = csv->GetNumCol() < 10 ? csv->GetNumCol() : 10;

					//Draw graph
					C->Stroke->Thickness = 2.0f;
					//Column loop
					for (int ic = 0 ; ic < draw_col_num ; ic++) {
						//Set new color
						C->Stroke->Color = 0xff000000 + GetColor((ic + 0.25) * 1.0 / draw_col_num, 60, 160);
						TPointF last;

						//Row loop (Draw single graph)
						for (int ir = 0 ; ir < draw_row_size ; ir++) {
							TPointF current;
							current.x = draw_row_size > 1 ?
								cell_rect.left + default_dataset_text_space + ir * (default_dataset_col_width_ - default_dataset_text_space * 2) / (draw_row_size - 1) :
								cell_rect.left + default_dataset_col_width_ / 2;
							float csv_value = csv->GetValue(ir, ic);
							current.y =
								cell_rect.top + default_dataset_text_space +
								(0.5f - csv_value * 0.5f) * (default_dataset_row_height_ - default_dataset_text_space * 2);
							if (ir > 0) {
								C->DrawLine(last, current,
									draw_row_size >= csv->GetNumRow() || last.x < cell_rect.left + (default_dataset_col_width_ * 7) / 8 ?
										1.0f : (cell_rect.right - last.x) * 1.0 / (default_dataset_col_width_ / 8));
							}
							last = current;
						}
					}
				}
				break;
			}

			//Draw tensor size text
			C->Fill->Color = SDEEP_CONSOLE_COLOR_TEXTSHAPE;
			C->Font->Size = 12;
			AnsiString shape_str =
				IntToStr(csv->GetNumRow()) + UnicodeString(", ") + IntToStr(csv->GetNumCol());
			C->FillText(
				RectF(
					cell_rect.left + default_dataset_text_space,
					cell_rect.bottom - 32,
					cell_rect.right - default_dataset_text_space,
					cell_rect.bottom - default_dataset_text_space),
				shape_str,
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Leading);

			//Draw "Overload" text
			if (fabs(csv->GetMax()) > 1.0f || fabs(csv->GetMin()) > 1.0f) {
				C->Fill->Color = SDEEP_CONSOLE_COLOR_TEXTRED;
				C->Font->Size = 13;
				C->FillText(
					RectF(
						cell_rect.left + default_dataset_text_space,
						cell_rect.top + default_dataset_text_space,
						cell_rect.right - default_dataset_text_space,
						cell_rect.bottom - default_dataset_text_space),
					"Overload",
					false,
					1.0f,
					TFillTextFlags(),
					TTextAlign::Trailing,
					TTextAlign::Trailing);
			}
		}
		C->RestoreState(save_state);
	}

	//Draw Cell Text
	C->Font->Size = 16;
	if (file) {
		while (true){
			const int pos = data_text.Pos("\\");
			if (pos >= 1 && C->TextWidth(data_text) > cell_rect.GetWidth() - default_dataset_text_space * 2){
				data_text = UnicodeString("... ") + data_text.SubString(pos + 1, data_text.Length() - pos);
			}else {
				break;
			}
		}
		C->Fill->Color = SDEEP_CONSOLE_COLOR_LIGHTEST;
		C->FillText(
			RectF(
				cell_rect.left + default_dataset_text_space + 1,
				cell_rect.top + 16,
				cell_rect.right - default_dataset_text_space + 1,
				cell_rect.bottom - 28),
			data_text,
			false,
			1.0f,
			TFillTextFlags(),
			TTextAlign::Leading,
			TTextAlign::Leading);
		C->Fill->Color = SDEEP_CONSOLE_COLOR_DARK;
	}else {
		C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
	}
	C->FillText(
		RectF(
			cell_rect.left + default_dataset_text_space,
			cell_rect.top + (cell_rect.bottom - cell_rect.top > 64 ? 14 : 0),
			cell_rect.right - default_dataset_text_space,
			cell_rect.bottom - (cell_rect.bottom - cell_rect.top > 64 ? 14 : 0)),
		data_text,
		true,
		1.0f,
		TFillTextFlags(),
		TTextAlign::Leading,
		cell_rect.bottom - cell_rect.top > 64 ? TTextAlign::Leading : TTextAlign::Center);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_TablePaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_Table->Canvas;

	C->BeginScene();
	C->SetMatrix(PB_Table->AbsoluteMatrix);
	C->Clear(SDEEP_CONSOLE_COLOR_BACKGROUND);

	if (nn_document_.GetLocked()) {
		C->EndScene();
		return;
	}

	TableUIState &table_ui_state = GetCurrentTableUIState();
	const CSVDataset &dataset = GetCurrentTable();
	const int num_data = dataset.GetNumData();
	const int num_column = dataset.GetNumColumn();
	dataset_header_height_ = 48;
	dataset_header_width_ = 160;
	if (&dataset == &nn_document_.GetResultCSV(2)) {
		// others
		switch (others_table_type_) {
		case OthersTableType::SimpleTable:
			default_dataset_col_width_ = num_column > 1 ? 192 : 768;
			default_dataset_row_height_ = 48;
			dataset_header_height_ = default_dataset_text_space;
			dataset_header_width_ = default_dataset_text_space;
			break;
		case OthersTableType::Profile:
			default_dataset_col_width_ = 512;
			default_dataset_row_height_ = 48;
			dataset_header_height_ = default_dataset_text_space;
			dataset_header_width_ = default_dataset_text_space;
			break;
		default://Dataset && Default
			default_dataset_col_width_ = 256;
			default_dataset_row_height_ = 128;
		}
	}else if (&dataset == &nn_document_.GetResultCSV(1)) {
		// confusion matrix
		default_dataset_col_width_ = 128;
		default_dataset_row_height_ = 48;
	}else {
		//training, validation, evaluation result
		default_dataset_col_width_ = 256;
		default_dataset_row_height_ = 128;
	}
	UnicodeString path = ExtractFilePath(dataset.GetFileName().PChar());

	FStringList request_images;
	FStringList request_csvs;
    FListT<CSVCacheFileType> request_csv_types;
	image_cache_list_->StartUpdate();
	csv_cache_list_->StartUpdate();

	csv_graph_exists_ = false;

	if (num_data >= 1 && num_column > 0) {
		C->Fill->Kind = TBrushKind::Solid;
		C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
		C->Stroke->Kind = TBrushKind::Solid;
		C->Font->Size = 16;

		C->Stroke->Thickness = 1;

		int start_col = -1, end_col = -1;
		for (int ic = 0 ; ic < num_column ; ic++) {
			const float left_x = dataset_header_width_ + default_dataset_col_width_ * ic - table_ui_state.current_left;
			const float right_x = dataset_header_width_ + default_dataset_col_width_ * (ic + 1) - table_ui_state.current_left;
			if (right_x > 0 && left_x < PB_Table->Width) {
				if (start_col < 0) {
					start_col = ic;
				}
				end_col = ic;
			}
		}
		table_ui_state.cols_in_screen = (PB_Table->Width - dataset_header_width_) / default_dataset_col_width_ - 1;
		if (table_ui_state.cols_in_screen < 1) {
			table_ui_state.cols_in_screen = 1;
		}

		int start_row = -1, end_row = -1;
		if (start_col >= 0) for (int ir = 0 ; ir < num_data ; ir++) {
			const float top_y = dataset_header_height_ + default_dataset_row_height_ * ir - table_ui_state.current_top;
			const float bottom_y = dataset_header_height_ + default_dataset_row_height_ * (ir + 1) - table_ui_state.current_top;

			if (bottom_y > dataset_header_height_ && top_y < PB_Table->Height) {
				if (start_row < 0) {
					start_row = ir;
				}
				end_row = ir;

				for (int ic = start_col ; ic <= end_col ; ic++) {
					const TRectF cell_rect = RectF(
						dataset_header_width_ + default_dataset_col_width_ * ic - table_ui_state.current_left,
						dataset_header_height_ + default_dataset_row_height_ * ir - table_ui_state.current_top,
						dataset_header_width_ + default_dataset_col_width_ * (ic + 1) - table_ui_state.current_left,
						dataset_header_height_ + default_dataset_row_height_ * (ir + 1) - table_ui_state.current_top
						);

					UnicodeString data_text = UTF8String(dataset.GetValue(ir, ic).ReplaceStr("{SDCPATH}", AnsiString(ExtractFilePath(ParamStr(0))).c_str()).PChar());

					DrawDataCell(C, cell_rect, data_text, request_images, request_csvs, request_csv_types);
				}
			}
		}
		table_ui_state.rows_in_screen = (PB_Table->Height - dataset_header_height_) / default_dataset_row_height_ - 1;
		if (table_ui_state.rows_in_screen < 1) {
			table_ui_state.rows_in_screen = 1;
		}

		C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
		C->Stroke->Thickness = 1.0f;
		for (int ic = start_col ; ic <= end_col ; ic++) {
			C->DrawLine(
				Point(dataset_header_width_ + default_dataset_col_width_ * (ic + 1) - table_ui_state.current_left, dataset_header_height_),
				Point(dataset_header_width_ + default_dataset_col_width_ * (ic + 1) - table_ui_state.current_left, dataset_header_height_ + default_dataset_row_height_ * num_data - table_ui_state.current_top),
				1.0f);
		}

		//Cursor
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_DATACURSOR;
		C->Stroke->Thickness = 4;
		SetDatasetCursorRect();
		C->DrawRect(
			RectF(
				table_ui_state.current_cursor_rect.Left - table_ui_state.current_left + 3,
				table_ui_state.current_cursor_rect.Top - table_ui_state.current_top + 3,
				table_ui_state.current_cursor_rect.Right - table_ui_state.current_left - 3,
				table_ui_state.current_cursor_rect.Bottom - table_ui_state.current_top - 3
			),
			0.0f, 0.0f, AllCorners, 1.0, TCornerType::Round);

		//Grid and Header
		C->Fill->Color = SDEEP_CONSOLE_COLOR_BACKGROUND;
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
		C->Stroke->Thickness = 1;
		//Top
		C->FillRect(
			RectF(dataset_header_width_, -1, PB_Table->Width, dataset_header_height_), //set top to -1 to avoid display issue when zoom is not 1.0(Workaround)
			0, 0, AllCorners, 1.0f, TCornerType::Round);
		//Left
		C->FillRect(
			RectF(0, dataset_header_height_, dataset_header_width_, PB_Table->Height),
			0, 0, AllCorners, 1.0f, TCornerType::Round);
		//Right
		C->FillRect(
			RectF(PB_Table->Width - default_dataset_text_space, dataset_header_height_, PB_Table->Width, PB_Table->Height),
			0, 0, AllCorners, 1.0f, TCornerType::Round);
		C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
		C->Font->Style = TFontStyles() << TFontStyle::fsBold;
		if (start_col >= 0 && dataset_header_height_ > default_dataset_text_space) for (int ic = start_col ; ic <= end_col ; ic++) {
			const float x = dataset_header_width_ + default_dataset_col_width_ * (ic + 1) - table_ui_state.current_left;
			if (x < PB_Table->Width - 16) {
				C->DrawLine(
					PointF(x, default_grid_text_hspace),
					PointF(x, dataset_header_height_),
					1.0f);
			}

			C->FillText(
				RectF(
					dataset_header_width_ + default_dataset_col_width_ * ic + default_dataset_text_space - table_ui_state.current_left,
					0,
					dataset_header_width_ + default_dataset_col_width_ * (ic + 1) - default_dataset_text_space - table_ui_state.current_left,
					dataset_header_height_),
				dataset.GetColumnName(ic).PChar(),
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center);
		}
		if (start_row>= 0) for (int ir = start_row ; ir <= end_row ; ir++) {
			C->DrawLine(
				PointF(default_dataset_text_space, dataset_header_height_ + default_dataset_row_height_ * (ir + 1) - table_ui_state.current_top),
				PointF(dataset_header_width_ + default_dataset_col_width_ * (end_col + 1) - table_ui_state.current_left, dataset_header_height_ + default_dataset_row_height_ * (ir + 1) - table_ui_state.current_top),
				1.0f);
			/*
			C->DrawLine(
				PointF(0, dataset_header_height_ + default_dataset_row_height_ * (ir + 1) - table_ui_state.current_top),
				PointF(dataset_header_width_ + default_dataset_col_width_ * num_column - table_ui_state.current_left, dataset_header_height_ + default_dataset_row_height_ * (ir + 1) - table_ui_state.current_top),
				1.0f);
			*/
			C->FillText(
				RectF(
					default_dataset_text_space,
					dataset_header_height_ + default_dataset_row_height_ * ir - table_ui_state.current_top,
					dataset_header_width_ - default_dataset_text_space,
					dataset_header_height_ * 2 + default_dataset_row_height_ * ir - table_ui_state.current_top),
				ir < dataset.GetRowNames().GetSize() ? UnicodeString(dataset.GetRowNames().GetAt(ir).PChar()) : UnicodeString(FString(ir + 1).InsertThousandSeparator().PChar()),
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center);
		}
		C->Stroke->Thickness = 2;
		C->Fill->Color = SDEEP_CONSOLE_COLOR_BACKGROUND;
		C->FillRect(
			RectF(0, 0, dataset_header_width_, dataset_header_height_),
			0, 0, AllCorners, 1.0f, TCornerType::Round);
		C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
		if (dataset_header_width_ > default_dataset_text_space) {
			C->FillText(
				RectF(
					default_dataset_text_space,
					0,
					dataset_header_width_ - default_dataset_text_space,
					dataset_header_height_),
					!dataset.GetRowNames().GetSize() ? "Index" : dataset.GetTopLeftCell().PChar(),
					false,
					1.0f,
					TFillTextFlags(),
					TTextAlign::Leading,
					TTextAlign::Center);
		}
		C->DrawLine(
			PointF(default_dataset_text_space, dataset_header_height_),
			PointF(dataset_header_width_ + default_dataset_col_width_ * (end_col + 1) - table_ui_state.current_left, dataset_header_height_),
			1.0f);
		C->DrawLine(
			PointF(dataset_header_width_, default_grid_text_hspace),
			PointF(dataset_header_width_, dataset_header_height_ + default_dataset_row_height_ * num_data - table_ui_state.current_top),
			1.0f);
	}

	for (int i = 0 ; i < request_images.GetSize() ; i++) {
		image_cache_list_->RequestImage(AnsiString(path).c_str(), request_images.GetAt(i).PChar());
	}
	image_cache_list_->EndUpdate();
	for (int i = 0 ; i < request_csvs.GetSize() ; i++) {
		csv_cache_list_->RequestCSV(AnsiString(path).c_str(), request_csvs.GetAt(i).PChar(), request_csv_types.GetAt(i));
	}
	csv_cache_list_->EndUpdate();

	C->EndScene();
}
//---------------------------------------------------------------------------
void TFo_Main::SetDatasetCursorRect()
{
	TableUIState &table_ui_state = GetCurrentTableUIState();
	table_ui_state.cursor_rect = TRectD(
		dataset_header_width_ + default_dataset_col_width_ * table_ui_state.col_index,
		dataset_header_height_ + default_dataset_row_height_ * table_ui_state.row_index,
		dataset_header_width_ + default_dataset_col_width_ * (table_ui_state.col_index + 1),
		dataset_header_height_ + default_dataset_row_height_ * (table_ui_state.row_index + 1)
		);
}
//---------------------------------------------------------------------------
void TFo_Main::PreviewData(int clear)
{
	if (!clear) {
		SetControlVisible(Lo_DataPreview, true);
		if (settings_.GetPlayAudioFile()) {
			const CSVDataset &dataset = GetCurrentTable();
			TableUIState &table_ui_state = GetCurrentTableUIState();
			const int dataset_num_data = dataset.GetNumData();
			const int dataset_num_column = dataset.GetNumColumn();
			if (table_ui_state.row_index >= 0 && table_ui_state.row_index < dataset_num_data &&
				table_ui_state.col_index >= 0 && table_ui_state.col_index < dataset_num_column) {
				UnicodeString data_text = dataset.GetValue(table_ui_state.row_index, table_ui_state.col_index).PChar();

				if (IsWavFile(data_text)) {
					UnicodeString file_name = GetDataFileName(
						ExtractFilePath(dataset.GetFileName().PChar()),
						data_text);
					if (MP_Wav->FileName != file_name && FileExists(file_name)) {
						MP_Wav->FileName = file_name;
						MP_Wav->Play();
						return;
					}
				}
			}
		}
	}
	if (clear) {
		SetControlVisible(Lo_DataPreview, false);
		SetControlVisible(Lo_Overview, true);
	}
	MP_Wav->Clear();
}
//---------------------------------------------------------------------------
int TFo_Main::IsFile(UnicodeString file_name)
{
	return IsCSVFile(file_name) || IsImageFile(file_name) || IsWavFile(file_name);
}
//---------------------------------------------------------------------------
int TFo_Main::IsCSVFile(UnicodeString file_name)
{
	UnicodeString ext = LowerCase(ExtractFileExt(file_name));

	if (ext == ".csv") {
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
int TFo_Main::IsImageFile(UnicodeString file_name)
{
	UnicodeString ext = LowerCase(ExtractFileExt(file_name));

	if (ext == ".bmp" ||
		ext == ".jpg" ||
		ext == ".jpeg" ||
		ext == ".png" ||
		ext == ".gif" ||
		ext == ".tif") {
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
int TFo_Main::IsUnknownSizeFile(UnicodeString file_name)
{
	UnicodeString ext = LowerCase(ExtractFileExt(file_name));

	if (ext == ".npy" || ext == ".dcm") {
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
int TFo_Main::IsWavFile(UnicodeString file_name)
{
	UnicodeString ext = LowerCase(ExtractFileExt(file_name));

	if (ext == ".wav") {
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
UnicodeString TFo_Main::GetDataFileName(UnicodeString path, UnicodeString file_name)
{
	UnicodeString result;
	if (FileExists(file_name)) {
		result = file_name;
	}else if (FileExists(path + file_name)) {
		result = path + file_name;
	}else {
		return "";
	}

	result = ReplaceStr(result, "/", "\\");

	int p = result.Pos(".\\");
	if (p) {
		result =
			result.SubString(1, p - 1) +
			result.SubString(p + 2, result.Length());
	}

	return result;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_TableKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	//Caption = UnicodeString("Dataset Keydown") + IntToStr(Key);
	Sc_TableY->Tag = 0;
	const CSVDataset &dataset = GetCurrentTable();
	TableUIState &table_ui_state = GetCurrentTableUIState();
	TableUIState table_ui_state_bak = table_ui_state;
	switch (Key) {
	case 13:
		//Enter
		MIT_OpenDataClick(Sender);
		break;
	case 27:
		//Esc
        PreviewData(true);//Clear sound
        break;
	case 33:
		//PgUp
		table_ui_state.row_index -= table_ui_state.rows_in_screen;
		if (table_ui_state.row_index < 0) {
			table_ui_state.row_index = 0;
		}
		break;
	case 34:
		//PgDown
		table_ui_state.row_index += table_ui_state.rows_in_screen;
		if (table_ui_state.row_index > dataset.GetNumData() - 1) {
			table_ui_state.row_index = dataset.GetNumData() - 1;
		}
		break;
	case 35:
		//End
		table_ui_state.row_index = dataset.GetNumData() - 1;
		break;
	case 36:
		//Home
		table_ui_state.row_index = 0;
		break;
	case 37:
		//Left
		if (table_ui_state.col_index > 0) {
			table_ui_state.col_index--;
		}
		break;
	case 38:
		//Up
		if (table_ui_state.row_index > 0) {
			table_ui_state.row_index--;
		}
		break;
	case 39:
		//Right
		if (table_ui_state.col_index < dataset.GetNumColumn() - 1) {
			table_ui_state.col_index++;
		}
		break;
	case 40:
		//Down
		if (table_ui_state.row_index < dataset.GetNumData() - 1) {
			table_ui_state.row_index++;
		}
		break;
	}
	if (table_ui_state.row_index != table_ui_state_bak.row_index || table_ui_state.col_index != table_ui_state_bak.col_index) {
		SetDatasetCursorRect();
		PreviewData();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_TableMouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, float X, float Y)
{
	Sc_TableX->Tag = 0;
	Sc_TableY->Tag = 0;
	Bu_Table->SetFocus();

	const CSVDataset &dataset = GetCurrentTable();
	TableUIState &table_ui_state = GetCurrentTableUIState();

	const int dataset_num_data = dataset.GetNumData();
	const int dataset_num_column = dataset.GetNumColumn();
	int mouse_down_col = ((X + table_ui_state.current_left) - dataset_header_width_) / default_dataset_col_width_;
	int mouse_down_row = ((Y + table_ui_state.current_top) - dataset_header_height_) / default_dataset_row_height_;

	if (mouse_down_col >= 0 && mouse_down_col < dataset_num_column) {
		table_ui_state.col_index = mouse_down_col;
	}
	if (mouse_down_row >= 0 && mouse_down_row < dataset_num_data) {
		table_ui_state.row_index = mouse_down_row;
	}
	SetDatasetCursorRect();
	PreviewData();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_TableMouseWheel(TObject *Sender, TShiftState Shift,
		  int WheelDelta, bool &Handled)
{
	const CSVDataset &dataset = GetCurrentTable();
	TableUIState &table_ui_state = GetCurrentTableUIState();

	Sc_TableX->Tag = 0;
	Sc_TableY->Tag = 0;
	if (Shift.Contains(ssShift)) {
		table_ui_state.col_index -= WheelDelta / 40;
	}else {
		table_ui_state.row_index -= WheelDelta / 40;
	}
	if (table_ui_state.col_index < 0) {
		table_ui_state.col_index = 0;
	}
	if (table_ui_state.col_index > dataset.GetNumColumn() - 1) {
		table_ui_state.col_index = dataset.GetNumColumn() - 1;
	}
	if (table_ui_state.row_index < 0) {
		table_ui_state.row_index = 0;
	}
	if (table_ui_state.row_index > dataset.GetNumData() - 1) {
		table_ui_state.row_index = dataset.GetNumData() - 1;
	}
	SetDatasetCursorRect();
	PreviewData();
	Handled = true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_TableYChange(TObject *Sender)
{
	const CSVDataset &dataset = GetCurrentTable();
	TableUIState &table_ui_state = GetCurrentTableUIState();
	const int dataset_num_data = dataset.GetNumData();
	if (Sc_TableY->Tag >= 0 && dataset_num_data) {
		Sc_TableY->Tag = 1;
		const double dataset_view_size = dataset_num_data * default_dataset_row_height_;
		table_ui_state.current_top = table_ui_state.top = (Sc_TableY->Value * dataset_view_size) / Sc_TableY->Max;
		PB_Table->Repaint();
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ComponentDblClick(int index)
{
	NNLayer *new_layer = AddLayer(true);
	if (new_layer) {
		new_layer->SetInitialOffset(0, -(new_layer->GetCurrentPosition(0) + new_layer->GetCurrentPosition(2)));
	}
}
//---------------------------------------------------------------------------
void TFo_Main::DatasetsItemChanged(int index)
{
	while (nn_document_.GetLocked()){
		Sleep(10);
		Application->ProcessMessages();
	}
	if (index >= 0 && index < Fr_DatasetFiles->GetItems().GetSize()) {
		const ListItem &item = Fr_DatasetFiles->GetItem(index);
		UnicodeString file_name = item.GetAttribute(FString("File Name")).PChar();
		if (FileExists(file_name)) {
			OpenDataset(0, file_name, false);
			Fr_DatasetList->SetItemIndex(0);
			PrepareRightSubPane();
			Re_RightDatasetPreviewTop->Visible = true;
			Lo_CSVGraphType->Parent = Re_RightDatasetPreviewTop;
			Invalidate();
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::Datasets2ItemClick(int index)
{
	if (index >= 0 && index < Fr_DatasetFiles2->GetItems().GetSize()) {
		const ListItem &item = Fr_DatasetFiles2->GetItem(index);
		UnicodeString file_name = item.GetAttribute(FString("File Name")).PChar();
		if (FileExists(file_name)) {
			OpenDataset(Fr_DatasetList->GetItemIndex(), file_name);
			nn_document_.UpdateConfig();
		}
	}
	Bu_DatasetOpenClick(this);
}
//---------------------------------------------------------------------------
void TFo_Main::ShowAnimationIndicator(UnicodeString text, int show_log, int show_abort_button, int show_close_button, int auto_close, int hide_log)
{
	if (settings_.GetLogLevel() >= 2){
		Log(FString("message : str=\"") + FString(AnsiString(text + "\"").c_str()));
	}
	Me_Processing->Lines->Clear();
	if (show_log) {
		Me_Processing->Visible= true;
		Pa_Processing->Height = AI_Processing->Position->Y + AI_Processing->Height + Me_Processing->Height + 8;
	}else {
		Me_Processing->Visible= false;
		Pa_Processing->Height = AI_Processing->Position->Y + AI_Processing->Height + 8;
	}
	AI_Processing->Visible = true;
	Bu_AbortProcessing->Visible = show_abort_button;
	Bu_AbortProcessing->Enabled = show_abort_button;
	Bu_CloseProcessing->Visible = false;
	Bu_CloseProcessing->Enabled = show_close_button;
	Bu_CloseProcessing->Tag = auto_close;
	if (text != "") {
		La_Processing->Text = text;
		AnsiString log_file_name = ExtractFilePath(ParamStr(0)) + AnsiString("settings\\log.txt");
		FILE *F = fopen(log_file_name.c_str(), "at");
		if (F){
			fprintf(F, "%s\n", AnsiString(FormatDateTime("[yyyy/mm/dd hh:nn:ss]  ", Now()) + text).c_str());
			fclose(F);
			F = NULL;
		}
		La_Processing->Visible = true;
	}else {
		La_Processing->Visible = false;
	}
	Pa_Processing->BringToFront();
	animation_indicator_abort_ = false;
	animation_indicator_active_ = true;
	animation_indicator_start_time_ = timeGetTime();
    animation_indicator_hide_log_ = hide_log;
}
//---------------------------------------------------------------------------
void TFo_Main::AnimationIndicatorLog(UnicodeString log)
{
	if (!Me_Processing->Visible && !animation_indicator_hide_log_) {
		Me_Processing->Visible = true;
		Pa_Processing->Height = AI_Processing->Position->Y + AI_Processing->Height + Me_Processing->Height + 8;
	}
	Me_Processing->Lines->BeginUpdate();
	FString f_log = FString(AnsiString(log).c_str());
	f_log = f_log.ReplaceStr("\r\n", "\n");
	f_log = f_log.ReplaceStr("\n\n", "\n");

	AnsiString log_file_name = ExtractFilePath(ParamStr(0)) + AnsiString("settings\\log.txt");
	FILE *F = fopen(log_file_name.c_str(), "at");
	while (f_log != ""){
		FString s1 = f_log.SplitBy('\n');
		if (s1.Pos("\r")) {
			s1 = s1.ReplaceStr("\r", "");
			if (Me_Processing->Lines->Count) {
				if (Me_Processing->Lines->Strings[Me_Processing->Lines->Count - 1].Pos("]:") <= 0){
					Me_Processing->Lines->Delete(Me_Processing->Lines->Count - 1);
				}
			}
		}else {
			if (F && s1.PChar()){
				fprintf(F, "\t%s\n", s1.PChar());
			}
		}
		Me_Processing->Lines->Add(s1.PChar());
	}
	if (F){
		fclose(F);
		F = NULL;
	}
	/*
	while (Me_Processing->Lines->Text.Pos("\r\n\r\n")) {
		Me_Processing->Lines->Text =
			FString(AnsiString(Me_Processing->Lines->Text).c_str()).ReplaceStr("\r\n\r\n", "\n").PChar();
	}
	*/
	Me_Processing->Lines->Add("");
	Me_Processing->GoToTextEnd();
	Me_Processing->Lines->Delete(Me_Processing->Lines->Count - 1);
	Me_Processing->Lines->EndUpdate();

	if (Me_Processing->Visible){
		Me_Processing->Repaint();
	}
}
//---------------------------------------------------------------------------
void TFo_Main::CheckAnimationIndicator()
{
	if (animation_indicator_active_ && !Pa_Processing->Visible){
		if (timeGetTime() > animation_indicator_start_time_ + 500){
			//Show animation indicator after 500ms from the start of processing
			Pa_Processing->Visible = true;
			Pa_Processing->Repaint();
			Application->HandleMessage();
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::HideAnimationIndicator()
{
	LogTraceback(Me_Processing->Lines);
	animation_indicator_active_ = false;
	if (Me_Processing->Text.Pos("successfully") > 0 && Bu_CloseProcessing->Tag){
		Bu_CloseProcessing->Enabled = false;
	}
	if (Bu_CloseProcessing->Enabled){
		nn_document_.Lock();
		Bu_AbortProcessing->Visible = false;
		Bu_CloseProcessing->Visible = true;
		AI_Processing->Visible = false;
	}else {
		Pa_Processing->Visible = false;
		Me_Processing->Lines->Clear();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_AbortProcessingClick(TObject *Sender)
{
	animation_indicator_abort_ = true;
	Bu_AbortProcessing->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_CloseProcessingClick(TObject *Sender)
{
	Pa_Processing->Visible = false;
	Me_Processing->Lines->Clear();
	nn_document_.Unlock();
}
//---------------------------------------------------------------------------
void TFo_Main::CheckSplashWindow()
{
	//Splash Window
	unsigned int now = timeGetTime();
	if (Pa_Splash->Visible) {
		if (time_created_ >= now - 3500) {
			unsigned int time = now - time_created_;
			/*if (time < 200) {
				Pa_Splash->Opacity = time / 200.0;
			}else */if (time > 3200) {
				const double t = 1.0 - (time - 3200) / 300.0;
				Pa_Splash->Opacity = t * t;
			}else {
				Pa_Splash->Opacity = 1.0;
			}
			Pa_Splash->Repaint();
		}else if (Pa_Splash->Visible){
			Pa_Splash->Visible = false;
		}
	}
}
//---------------------------------------------------------------------------
int TFo_Main::CheckSave()
{
	if (nn_document_.GetChanged()) {
		int result = MessageDlg(GetTranslatedText("Project has been modified. Do you want to save changes?"), TMsgDlgType::mtWarning, mbYesNoCancel, 0);
		if (result == mrCancel) {
			return false;
		} if (result == mrYes) {
			if (!Save()){
				return false;
			}
		}
	}
	return true;
}
//---------------------------------------------------------------------------
int TFo_Main::AskAndTerminateProcess()
{
	if (GetTraining()) {
		int result = MessageDlg(GetTranslatedText("Training process is still running. Do you really want to terminate?"), TMsgDlgType::mtWarning, mbOKCancel, 0);
		if (result == mrCancel) {
			return false;
		} if (result == mrOk) {
			FreeSDeepTrainingThread();
		}
	}
	if (GetEvaluating()) {
		int result = MessageDlg(GetTranslatedText("Evaluating process is still running. Do you really want to terminate?"), TMsgDlgType::mtWarning, mbOKCancel, 0);
		if (result == mrCancel) {
			return false;
		} if (result == mrOk) {
			FreeSDeepEvaluationThread();
		}
	}
	return true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_BackClick(TObject *Sender)
{
	if (nn_document_.GetLocked()){
		return;
	}
	switch (activity_) {
	case SDeepConsoleActivity::Default:
		if (!CheckSave()){
			return;
		}
		if (!AskAndTerminateProcess()){
			return;
		}
		NewProject(SDeepEngineType(0), false);
		SetActivity(SDeepConsoleActivity::Browser);
		break;
	default:
		;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::FA_TCDefaultPositionXFinish(TObject *Sender)
{
	if (FA_TCDefaultPositionX->Inverse) {
		TC_Default->Visible = false;
		TC_Default->Align = TAlignLayout::None;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::BrowserItemChanged(int index)
{
	Lo_RightSub->Visible = false;
	Lo_Right->Visible = true;
	Sp_Right->Position->X = ClientWidth - Lo_Right->Width - Sp_Right->Width - Sp_Right->Margins->Right;
	Sp_Right->Align = TAlignLayout::None;
	Sp_Right->Align = TAlignLayout::Right;
	Lo_Table->Parent = TI_Dataset;
	Lo_CSVGraphType->Parent = Re_DatasetTop2;
	Lo_Right->Repaint();

	Fr_ProjectFiles->Visible = false;
	Bu_NewProject->Visible = false;
	Bu_NewProjectTransformers->Visible = false;
    Bu_NewProjectTorch->Visible = false;
	Bu_OpenProject->Visible = false;
	Bu_OpenDataset->Visible = false;
	Bu_CreateDataset->Visible = false;
	Fr_DatasetFiles->Visible = false;
	Fr_AISolutions->Visible = false;
	La_AISolutions->Visible = false;
	Fr_NNablaExamples->Visible = false;
	La_InteractiveDemos->Visible = false;

	switch (index){
	case 0://Projects
		RefreshProjectFiles();
		Fr_ProjectFiles->Visible = true;
		Bu_NewProject->Visible = true;
		Bu_NewProjectTransformers->Visible = true;
        Bu_NewProjectTorch->Visible = true;
		Bu_OpenProject->Visible = true;
		RefreshListSettings();
		Fr_ProjectFiles->Repaint();
		break;
	case 1://Datasets
		RefreshDatasetFiles();
		Bu_OpenDataset->Visible = true;
		Bu_CreateDataset->Visible = true;
		Fr_DatasetFiles->Visible = true;
		RefreshListSettings();
		Fr_DatasetFiles->Repaint();
		break;
	case 2://AISolutions
		RefreshAISolutionsList();
		Fr_AISolutions->Visible = true;
		La_AISolutions->Visible = true;
		RefreshListSettings();
		Fr_AISolutions->Repaint();
		break;
	case 3://NNablaExamples
		RefreshNNablaExamplesList();
		Fr_NNablaExamples->Visible = true;
		La_InteractiveDemos->Visible = true;
		RefreshListSettings();
		Fr_NNablaExamples->Repaint();
		break;
	default:
		{
			index -= 4;
			const FStringList link_collection = settings_.GetLinkCollection();
			if (index >= 0 && index < link_collection.GetSize()){
				FString s = link_collection.GetAt(index);
				s.SplitBy(",");
                OpenFile(s.PChar());
			}
		}
	}
	Invalidate();
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshProjectFiles()
{
	ListItems items;
	items = project_list_;
	items.AllocateOriginalIndex();

	if (Fr_ProjectFiles->Ed_ListSearch->Text != "") {
		//Search
		FStringList search_texts;
		search_texts.SplitStr(FString(AnsiString(Fr_ProjectFiles->Ed_ListSearch->Text).c_str()), FString(" "), FString("\""));

		for (int it = 0 ; it < search_texts.GetSize() ; it++) {
			FString search_text = search_texts.GetAt(it).ReplaceStr("\"", "");
			if (search_text != "") for (int ii = items.GetSize() - 1 ; ii >= 0 ; ii--) {
				const ListItem &list_item = items.GetAt(ii);
				if (list_item.GetRemovable()) {
					if (!list_item.Contains(search_text)) {
						items.Delete(ii);
					}
				}
			}
		}
	}

	Fr_ProjectFiles->SetItems(items);
}
//---------------------------------------------------------------------------
void TFo_Main::ProjectsItemClick(int index)
{
	if (index < 0){
		return;
	}
	const ListItem &item = Fr_ProjectFiles->GetItem(index);
	SetActivity(SDeepConsoleActivity::Default);
	{
		const FString file_name = item.GetAttribute(FString("File Name"));
		if (file_name != "") {
			//Open project file in project list
			OpenProject(file_name.PChar());
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ProjectsItemChanged(int index)
{
	if (index < 0 || index >= Fr_ProjectFiles->GetItems().GetSize()){
		return;
	}
	const ListItem &item = Fr_ProjectFiles->GetItem(index);
	{
		const FString file_name = item.GetAttribute(FString("File Name"));
		if (file_name != "") {
			//Open project file in project list
			nn_document_.LoadFromFile(file_name, FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()), false, false);
		}else {
			nn_document_.Clear();
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshDatasetFiles()
{
	ListItems items;
	items = dataset_list_;
	items.AllocateOriginalIndex();
	ListItems items2;
	items2 = items;

	if (Fr_DatasetFiles->Ed_ListSearch->Text != "") {
		//Search
		FStringList search_texts;
		search_texts.SplitStr(FString(AnsiString(Fr_DatasetFiles->Ed_ListSearch->Text).c_str()), FString(" "), FString("\""));

		for (int it = 0 ; it < search_texts.GetSize() ; it++) {
			FString search_text = search_texts.GetAt(it).ReplaceStr("\"", "");
			if (search_text != "") for (int ii = items.GetSize() - 1 ; ii >= 0 ; ii--) {
				const ListItem &list_item = items.GetAt(ii);
				if (list_item.GetRemovable()) {
					if (!list_item.Contains(search_text)) {
						items.Delete(ii);
					}
				}
			}
		}
	}
	Fr_DatasetFiles->SetItems(items);

	if (Fr_DatasetFiles2->Ed_ListSearch->Text != "") {
		//Search
		FStringList search_texts;
		search_texts.SplitStr(FString(AnsiString(Fr_DatasetFiles2->Ed_ListSearch->Text).c_str()), FString(" "), FString("\""));

		for (int it = 0 ; it < search_texts.GetSize() ; it++) {
			FString search_text = search_texts.GetAt(it).ReplaceStr("\"", "");
			if (search_text != "") for (int ii = items2.GetSize() - 1 ; ii >= 0 ; ii--) {
				const ListItem &list_item = items2.GetAt(ii);
				if (list_item.GetRemovable()) {
					if (!list_item.Contains(search_text)) {
						items2.Delete(ii);
					}
				}
			}
		}
	}
	Fr_DatasetFiles2->SetItems(items2);
}
//---------------------------------------------------------------------------
void TFo_Main::SetActivity(SDeepConsoleActivity activity)
{
	switch (activity) {
	case SDeepConsoleActivity::Browser:
		SetActiveTab(TC_Left, TI_BrowserLeft);
		SetActiveTab(TC_Default, TI_Browser);
		Fr_ProjectFiles->Ed_ListSearch->Text = "";
		Fr_DatasetFiles->Ed_ListSearch->Text = "";

		RefreshProjectFiles();
		RefreshDatasetFiles();
		break;
	case SDeepConsoleActivity::Default:
		if (TC_Left->ActiveTab == TI_BrowserLeft || TC_Default->ActiveTab == TI_Browser){
			Bu_NetworkTabClick(this);
		}
		break;
	default:
		;
	}
	TC_DefaultChange(this);
	activity_ = activity;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIL_RemoveClick(TObject *Sender)
{
	TFr_List *list = NULL;
	if (TC_Default->ActiveTab == TI_Browser && Fr_ProjectFiles->Visible) {
		list = Fr_ProjectFiles;
	}else if (TC_Default->ActiveTab == TI_Browser && Fr_DatasetFiles->Visible) {
		list = Fr_DatasetFiles;
	}else if (TC_Default->ActiveTab == TI_Dataset && Fr_DatasetFiles2->Visible) {
		list = Fr_DatasetFiles2;
	}else {
		return;
	}

	if (list->GetItemIndex() < 0 || list->GetItemIndex() >= list->GetItems().GetSize()) {
		return;
	}
	const int item_index = list->GetItem(list->GetItemIndex()).GetOriginalIndex();

	if (item_index >= 0) {
		if (list == Fr_ProjectFiles){
			if (item_index < project_list_.GetSize()) {
				project_list_.Delete(item_index);
				project_list_.SaveToFile();
			}
		}else if (list == Fr_DatasetFiles || list == Fr_DatasetFiles2){
			if (item_index < dataset_list_.GetSize()) {
				dataset_list_.Delete(item_index);
				dataset_list_.SaveToFile();
			}
		}
	}

	//Update List
	if (TC_Default->ActiveTab == TI_Browser && Fr_ProjectFiles->Visible) {
		RefreshProjectFiles();
	}else if (TC_Default->ActiveTab == TI_Browser && Fr_DatasetFiles->Visible) {
		RefreshDatasetFiles();
	}else if (TC_Default->ActiveTab == TI_Dataset && Fr_DatasetFiles2->Visible) {
		RefreshDatasetFiles();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_BatchSizeChange(TObject *Sender)
{
	const int value = StrToIntDef(Ed_BatchSize->Text, nn_document_.GetBatchSize());
	if (nn_document_.GetBatchSize() != value) {
		nn_document_.SetBatchSize(value);
		nn_document_.UpdateConfig();
	}
	RefreshConfigList();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_MaxEpochChange(TObject *Sender)
{
	const int value = StrToIntDef(Ed_MaxEpoch->Text, nn_document_.GetMaxEpoch());
	if (nn_document_.GetMaxEpoch() != value) {
		nn_document_.SetMaxEpoch(value);
		nn_document_.UpdateConfig();
	}
	RefreshConfigList();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_UpdaterParam1Change(TObject *Sender)
{
	const int tag = ((TEdit*)Sender)->Tag;
	const int optimizer_index = TI_OptimizerConfig->Tag;
	if (tag < nn_document_.GetOptimizer(optimizer_index).GetSolverParameterNum()) {
		const double value = StrToFloatDef(((TEdit*)Sender)->Text, 1.0);
		if (nn_document_.GetOptimizer(optimizer_index).GetSolverParameter(tag) != value) {
			nn_document_.GetOptimizer(optimizer_index).SetSolverParameter(tag, value);
			nn_document_.UpdateConfig();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_PropertyChangeTracking(TObject *Sender)
{
	ApplyProperty();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIL_OpenFileLocationClick(TObject *Sender)
{
	TFr_List *list = NULL;
	if (TC_Default->ActiveTab == TI_Browser && Fr_ProjectFiles->Visible) {
		list = Fr_ProjectFiles;
	}else if (TC_Default->ActiveTab == TI_Browser && Fr_DatasetFiles->Visible) {
		list = Fr_DatasetFiles;
	}else if (TC_Default->ActiveTab == TI_Dataset && Fr_DatasetFiles2->Visible) {
		list = Fr_DatasetFiles2;
	}else if (TC_Left->ActiveTab == TI_TrainingLeft){
		list = Fr_ResultsList;
	}else {
		return;
	}

	if (list->GetItemIndex() < 0 || list->GetItemIndex() >= list->GetItems().GetSize()) {
		return;
	}
	FString file_location =
		list->GetItem(list->GetItemIndex()).GetAttribute(FString("Location"));
	file_location = file_location.ReplaceStr("/", "\\");
	OpenFileLocation(file_location.PChar());
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_MonitorPaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_Monitor->Canvas;

	C->BeginScene();
	C->Stroke->Color = SDEEP_CONSOLE_COLOR_BACKGROUND;
	C->SetMatrix(PB_Monitor->AbsoluteMatrix);

	//Background
	C->Clear(SDEEP_CONSOLE_COLOR_BACKGROUND);
	DrawGraphBitmap(C, PB_Monitor->Width, PB_Monitor->Height, FString());

	C->EndScene();
}
//---------------------------------------------------------------------------
void TFo_Main::SaveGraphBitmapToFile(UnicodeString file_name, int width, int height, const FString &type)
{
	TBitmap *BMP = new TBitmap(width, height);
	TCanvas *C = BMP->Canvas;
	C->BeginScene();
	C->Clear(SDEEP_CONSOLE_COLOR_BACKGROUND);
	DrawGraphBitmap(C, BMP->Width, BMP->Height, type);
	C->EndScene();
	BMP->SaveToFile(file_name);
	delete BMP;
}
//---------------------------------------------------------------------------
#define LossToY(loss) graph_height - (RB_GraphLogScale->IsChecked && monitor_current_value_range_[2] > 0.0 && loss > 0.0 ? log(loss / monitor_current_value_range_[2]) / log(monitor_current_value_range_[3] / monitor_current_value_range_[2]) : (loss - monitor_current_value_range_[2]) / (monitor_current_value_range_[3] - monitor_current_value_range_[2])) * graph_height + monitor_vspace
#define ErrorToY(error) graph_height - (RB_GraphLogScale->IsChecked && monitor_current_value_range_[4] > 0.0 && error >0.0 ? log(error / monitor_current_value_range_[4]) / log(monitor_current_value_range_[5] / monitor_current_value_range_[4]) : (error - monitor_current_value_range_[4]) / (monitor_current_value_range_[5] - monitor_current_value_range_[4])) * graph_height + monitor_vspace
#define TOCostToX(cost) log(cost / result_current_tradeoff_value_range_[0]) / log_range * graph_width + monitor_hspace
#define TOErrorToY(error) graph_height - (RB_GraphLogScale->IsChecked && result_current_tradeoff_value_range_[2] > 0.0 && error > 0.0 ? log(error / result_current_tradeoff_value_range_[2]) / log(result_current_tradeoff_value_range_[3] / result_current_tradeoff_value_range_[2]) : (error - result_current_tradeoff_value_range_[2]) / (result_current_tradeoff_value_range_[3] - result_current_tradeoff_value_range_[2])) * graph_height + monitor_vspace
//---------------------------------------------------------------------------
void TFo_Main::DrawGraphBitmap_LearningCurve(TCanvas *C, int width, int height)
{
	static const int graph_scale_pitch = 64;

	// static const int legend_space = 32;
	// static const int legend_width = 240;
	// static const int legend_vspace = 8;
	// static const int legend_pitch = 24;

	const int graph_width = width - monitor_hspace * 2;
	const int graph_height = height - monitor_vspace * 2;

	float nearest_x[6];
	memset(nearest_x, 0, sizeof(nearest_x));
	float nearest_y[6];
	memset(nearest_y, 0, sizeof(nearest_y));
	float nearest_v[6];
	memset(nearest_v, 0, sizeof(nearest_v));

	if (graph_width > 0 && graph_height > 0) {
		// FString hint_string;
		// float hint_x = 0.0f, hint_y = 0.0f;
		// TAlignLayout hint_h_align = TAlignLayout::Center;
		// TAlignLayout hint_v_align = TAlignLayout::Bottom;

		//Learning Curve

		if (monitor_current_value_range_[1] > monitor_current_value_range_[0]) {
			//Epoch scale
			int epoch_step = 1;
			while (epoch_step * graph_width * 1.0 / (monitor_current_value_range_[1] - monitor_current_value_range_[0]) < graph_scale_pitch) {
				if (epoch_step == 2 || epoch_step == 20 || epoch_step == 200 || epoch_step == 2000) {
					epoch_step = (epoch_step / 2) * 5;
				}else {
					epoch_step *= 2;
				}
			}
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->Font->Size = 16;
			for (int ie = 0 ; ie < monitor_current_value_range_[1] ; ie += epoch_step) {
				const float x = (ie - monitor_current_value_range_[0]) * graph_width * 1.0 / (monitor_current_value_range_[1] - monitor_current_value_range_[0]) + monitor_hspace;
				float alpha = 1.0f;
				if (x > monitor_hspace + graph_width) {
					alpha = 1.0f - (float)((x - (monitor_hspace + graph_width)) / (graph_width * 1.0 / (monitor_current_value_range_[1] - monitor_current_value_range_[0])));
					if (alpha < 0.0f) {
						alpha = 0.0f;
					}
				}
				C->Stroke->Color = SDEEP_CONSOLE_COLOR_THINLINE;
				C->DrawLine(
					PointF(x, monitor_vspace),
					PointF(x, monitor_vspace + graph_height),
					alpha);
				C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
				C->DrawLine(
					PointF(x, monitor_vspace + graph_height - graph_scale_size / 2),
					PointF(x, monitor_vspace + graph_height + graph_scale_size / 2),
					alpha);
				C->FillText(
					RectF(
						x - graph_scale_pitch / 2,
						monitor_vspace + graph_height + graph_scale_size / 2,
						x + graph_scale_pitch / 2,
						monitor_vspace * 2 + graph_height),
					IntToStr(ie),
					false,
					alpha,
					TFillTextFlags(),
					TTextAlign::Center,
					TTextAlign::Leading
					);
			}

			//Loss scale
			if (monitor_current_value_range_[3] > monitor_current_value_range_[2]) {
				C->Fill->Color = SDEEP_CONSOLE_COLOR_COSTGRAPH;
				C->Font->Size = 16;
				if (RB_GraphLogScale->IsChecked) {
					int num_step = 1;
					int num_step_x = 0;
					__int64 in = 1;
					FStringList scale_text;
					FListT<float> scale_y;
					while (num_step_x < 10 && in * 0.001 < monitor_current_value_range_[3]) {
						if (in * 0.001 >= monitor_current_value_range_[2]) {
							const float y = LossToY(in * 0.001);
							C->Stroke->Color = SDEEP_CONSOLE_COLOR_COSTBG;
							C->DrawLine(
								PointF(monitor_hspace, y),
								PointF(monitor_hspace + graph_width, y),
								1.0f);
							C->Stroke->Color = SDEEP_CONSOLE_COLOR_COSTGRAPH;
							C->DrawLine(
								PointF(monitor_hspace - graph_scale_size / 2, y),
								PointF(monitor_hspace + graph_scale_size / 2, y),
								1.0f);
							scale_y.Add(y);
							scale_text.Add(FString(AnsiString(FormatFloat("0.000", in * 0.001)).c_str()));
						}
						num_step++;
						if (num_step >= 10) {
							num_step = 1;
							num_step_x++;
						}
						in = num_step * pow(10.0, num_step_x);
					}
					float last_y = 0.0f;
					for (int i = scale_text.GetSize() - 1 ; i >= 0 ; i--) {
						UnicodeString s = scale_text.GetAt(i).PChar();
						const float y = scale_y.GetAt(i);
						if (y > last_y + C->Font->Size * 1.5) {
							C->FillText(
								RectF(
									0,
									y - graph_scale_pitch / 2,
									monitor_hspace - graph_scale_size / 2,
									y + graph_scale_pitch / 2),
								s,
								false,
								1.0f,
								TFillTextFlags(),
								TTextAlign::Trailing,
								TTextAlign::Center
								);
							last_y = y;
						}
					}
				}else {
					static const double min_cost_step = 0.01;
					__int64 step = 1;
					while (step > 0 && step * min_cost_step * graph_height * 1.0 / (monitor_current_value_range_[3] - monitor_current_value_range_[2]) < graph_scale_pitch && step < 0xffffffff) {
						if (FString((double)step).SubString(1,1) == "2") {
							step = (step / 2) * 5;
						}else {
							step *= 2;
						}
					}
					const double min = monitor_current_value_range_[2] - fmod(monitor_current_value_range_[2] + step, step * min_cost_step) + step * min_cost_step;
					if (step * min_cost_step > 0 &&  step < 0xffffffff) for (double v = min ; v <= monitor_current_value_range_[3] ; v += step * min_cost_step) {
						const float y = LossToY(v);
						C->Stroke->Color = SDEEP_CONSOLE_COLOR_COSTBG;
						C->DrawLine(
							PointF(monitor_hspace, y),
							PointF(monitor_hspace + graph_width, y),
							1.0f);
						C->Stroke->Color = SDEEP_CONSOLE_COLOR_COSTGRAPH;
						C->DrawLine(
							PointF(monitor_hspace - graph_scale_size / 2, y),
							PointF(monitor_hspace + graph_scale_size / 2, y),
							1.0f);
						C->FillText(
							RectF(
								0,
								y - graph_scale_pitch / 2,
								monitor_hspace - graph_scale_size / 2,
								y + graph_scale_pitch / 2),
							FormatFloat("0.00", v),
							false,
							1.0f,
							TFillTextFlags(),
							TTextAlign::Trailing,
							TTextAlign::Center
							);
					}
				}
			}

			//Error scale
			if (monitor_current_value_range_[5] > monitor_current_value_range_[4]) {
				C->Fill->Color = SDEEP_CONSOLE_COLOR_ERRORGRAPH;
				C->Font->Size = 16;
				if (RB_GraphLogScale->IsChecked) {
					int num_step = 1;
					int num_step_x = 0;
					__int64 in = 1;
					FStringList scale_text;
					FListT<float> scale_y;
					while (num_step_x < 10 && in * 0.001 < monitor_current_value_range_[5]) {
						if (in * 0.001 >= monitor_current_value_range_[4]) {
							const float y = ErrorToY(in * 0.001);
							C->Stroke->Color = SDEEP_CONSOLE_COLOR_ERRORBG;
							C->DrawLine(
								PointF(monitor_hspace, y),
								PointF(monitor_hspace + graph_width, y),
								1.0f);
							C->Stroke->Color = SDEEP_CONSOLE_COLOR_ERRORGRAPH;
							C->DrawLine(
								PointF(monitor_hspace + graph_width - graph_scale_size / 2, y),
								PointF(monitor_hspace + graph_width + graph_scale_size / 2, y),
								1.0f);
							scale_y.Add(y);
							scale_text.Add(FString(AnsiString(FormatFloat("0.000", in * 0.001)).c_str()));
						}
						num_step++;
						if (num_step >= 10) {
							num_step = 1;
							num_step_x++;
						}
						in = num_step * pow(10.0, num_step_x);
					}
					float last_y = 0.0f;
					for (int i = scale_text.GetSize() - 1 ; i >= 0 ; i--) {
						UnicodeString s = scale_text.GetAt(i).PChar();
						const float y = scale_y.GetAt(i);
						if (y > last_y + C->Font->Size * 1.5) {
						C->FillText(
							RectF(
								monitor_hspace + graph_width + graph_scale_size / 2,
								y - graph_scale_pitch / 2,
								monitor_hspace * 2 + graph_width,
								y + graph_scale_pitch / 2),
								s,
								false,
								1.0f,
								TFillTextFlags(),
								TTextAlign::Leading,
								TTextAlign::Center
								);
							last_y = y;
						}
					}
				}else {
					static const double min_error_step = 0.001;
					__int64 step = 1;
					while (step > 0 && step * min_error_step * graph_height * 1.0 / (monitor_current_value_range_[5] - monitor_current_value_range_[4]) < graph_scale_pitch && step < 0xffffffff) {
						if (FString((double)step).SubString(1,1) == "2") {
							step = (step / 2) * 5;
						}else {
							step *= 2;
						}
					}
					const double min = monitor_current_value_range_[4] - fmod(monitor_current_value_range_[4] + step, step * min_error_step) + step * min_error_step;
					if (step * min_error_step > 0 &&  step < 0xffffffff) for (double v = min ; v <= monitor_current_value_range_[5] ; v += step * min_error_step) {
						const float y = ErrorToY(v);
						C->Stroke->Color = SDEEP_CONSOLE_COLOR_ERRORBG;
						C->DrawLine(
							PointF(monitor_hspace, y),
							PointF(monitor_hspace + graph_width, y),
							1.0f);
						C->Stroke->Color = SDEEP_CONSOLE_COLOR_ERRORGRAPH;
						C->DrawLine(
							PointF(monitor_hspace + graph_width - graph_scale_size / 2, y),
							PointF(monitor_hspace + graph_width + graph_scale_size / 2, y),
							1.0f);
						C->FillText(
							RectF(
								monitor_hspace + graph_width + graph_scale_size / 2,
								y - graph_scale_pitch / 2,
								monitor_hspace * 2 + graph_width,
								y + graph_scale_pitch / 2),
							FormatFloat("0.000", v),
							false,
							1.0f,
							TFillTextFlags(),
							TTextAlign::Leading,
							TTextAlign::Center
							);
					}
				}
			}

			TCanvasSaveState *save_state = C->SaveState();
			C->IntersectClipRect(RectF(monitor_hspace, monitor_vspace, width - monitor_hspace, height - monitor_vspace));
			for (int im = 1 ; im >= 0 ; im--) if (monitor_csv_[im].GetNumData()) {
				//Loss
				if (monitor_current_value_range_[3] > monitor_current_value_range_[2]) {
					C->Stroke->Color = im ? SDEEP_CONSOLE_COLOR_COSTBG : SDEEP_CONSOLE_COLOR_COSTGRAPH;
					C->Stroke->Thickness = monitor_csv_[im].GetNumData() > 1 ? 3 : 9;
					C->Stroke->Dash = TStrokeDash::Solid;
					TPathData *path = new TPathData();
					for (int ie = 0 ; ie < monitor_csv_[im].GetNumData() ; ie++) {
						double value = monitor_csv_[im].GetFloatValue(ie, FString("cost"));
						const float x = ((ie + 1) - monitor_current_value_range_[0]) * graph_width * 1.0 / (monitor_current_value_range_[1] - monitor_current_value_range_[0]) + monitor_hspace;
						const float y = LossToY(value);
						if (ie == 0) {
							path->MoveTo(PointF(x, y));
						}
						path->LineTo(PointF(x, y));
						if (fabs(x - result_last_mouse_x_ ) < fabs(nearest_x[im * 3] - result_last_mouse_x_)) {
							nearest_x[im * 3] = x;
							nearest_y[im * 3] = y;
							nearest_v[im * 3] = (float)value;
						}
					}
					C->DrawPath(path, 1.0);
					delete path;
				}

				//Training Error
				if (monitor_current_value_range_[5] > monitor_current_value_range_[4] &&
					(MIMV_TrainingValidation->IsChecked || MIMV_Training->IsChecked)) {
					C->Stroke->Color = im ? SDEEP_CONSOLE_COLOR_ERRORBG : SDEEP_CONSOLE_COLOR_ERRORGRAPH;
					C->Stroke->Thickness = monitor_csv_[im].GetNumData() > 1 ? 3 : 9;
					float last_y = -1.0e+10f;
					TPathData *path = new TPathData();
					for (int ie = 0 ; ie < monitor_csv_[im].GetNumData() ; ie++) {
						FString value = monitor_csv_[im].GetValue(ie, FString("train_error"));
						if (value != "") {
							const float y = ErrorToY(value.AsFloat());
							const float x = ((ie + 1) - monitor_current_value_range_[0]) * graph_width * 1.0 / (monitor_current_value_range_[1] - monitor_current_value_range_[0]) + monitor_hspace;
							if (last_y < -1.0e+9f) {
								path->MoveTo(PointF(x, y));
							}
							path->LineTo(PointF(x, y));
							last_y = y;
							if (fabs(x - result_last_mouse_x_ ) < fabs(nearest_x[im * 3 + 1] - result_last_mouse_x_)) {
								nearest_x[im * 3 + 1] = x;
								nearest_y[im * 3 + 1] = y;
								nearest_v[im * 3 + 1] = value.AsFloat();
							}
						}
					}
					C->DrawPath(path, 1.0);
					delete path;
				}

				//Validation Error
				if (monitor_current_value_range_[5] > monitor_current_value_range_[4] &&
					(MIMV_TrainingValidation->IsChecked || MIMV_Validation->IsChecked)) {
					C->Stroke->Color = im ? SDEEP_CONSOLE_COLOR_ERRORBG : SDEEP_CONSOLE_COLOR_ERRORGRAPH;
					C->Stroke->Thickness = monitor_csv_[im].GetNumData() > 1 ? 3 : 9;
					C->Stroke->Dash = TStrokeDash::Dot;
					float last_y = -1.0e+10f;
					TPathData *path = new TPathData();
					for (int ie = 0 ; ie < monitor_csv_[im].GetNumData() ; ie++) {
						FString value = monitor_csv_[im].GetValue(ie, FString("valid_error"));
						if (value != "") {
							const float y = ErrorToY(value.AsFloat());
							const float x = ((ie + 1) - monitor_current_value_range_[0]) * graph_width * 1.0 / (monitor_current_value_range_[1] - monitor_current_value_range_[0]) + monitor_hspace;
							if (last_y < -1.0e+9f) {
								path->MoveTo(PointF(x, y));
							}
							path->LineTo(PointF(x, y));
							last_y = y;
							if (fabs(x - result_last_mouse_x_ ) < fabs(nearest_x[im * 3 + 2] - result_last_mouse_x_)) {
								nearest_x[im * 3 + 2] = x;
								nearest_y[im * 3 + 2] = y;
								nearest_v[im * 3 + 2] = value.AsFloat();
							}
						}
					}
					C->DrawPath(path, 1.0);
					delete path;
				}
			}
			C->RestoreState(save_state);

			C->Stroke->Dash = TStrokeDash::Solid;
			C->Stroke->Thickness = 1;
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
			C->DrawLine(
				PointF(monitor_hspace, monitor_vspace),
				PointF(monitor_hspace, monitor_vspace + graph_height),
				1.0f);
			C->DrawLine(
				PointF(monitor_hspace, monitor_vspace + graph_height),
				PointF(monitor_hspace + graph_width, monitor_vspace + graph_height),
				1.0f);
			C->DrawLine(
				PointF(monitor_hspace + graph_width, monitor_vspace + graph_height),
				PointF(monitor_hspace + graph_width, monitor_vspace),
				1.0f);

			C->Font->Size = 16;
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			UnicodeString s = "Learning Curve";
			if (MIMV_Name->IsChecked && result_name_[0] != ""){
				s += UnicodeString(" : ") + UnicodeString(result_name_[0].PChar());
				if (result_name_[1] != "" && result_name_[0] != result_name_[1]){
					s += UnicodeString(" vs ") + UnicodeString(result_name_[1].PChar());
				}
			}
			C->FillText(
				RectF(16, 0, width, 48),
				s,
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center
				);

			C->Font->Size = 16;
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->FillText(
				RectF(monitor_hspace, height - 48, monitor_hspace + graph_width, height),
				"Epoch",
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Center,
				TTextAlign::Center
				);

			C->Font->Size = 16;
			C->Fill->Color = SDEEP_CONSOLE_COLOR_COSTGRAPH;
			C->FillText(
				RectF(0, height - 48, monitor_hspace, height),
				"Cost",
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Center,
				TTextAlign::Center
				);

			C->Font->Size = 16;
			C->Fill->Color = SDEEP_CONSOLE_COLOR_ERRORGRAPH;
			C->FillText(
				RectF(monitor_hspace + graph_width, height - 48, monitor_hspace * 2 + graph_width, height),
				"Error",
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Center,
				TTextAlign::Center
				);


			//Legend
			int legend_right = width - monitor_hspace;
			for (int i = 2 ; i >= 0 ; i--) {
				UnicodeString s = i == 0 ? "COST" : (i == 1 ? "TRAINING ERROR" : "VALIDATION ERROR");
				C->Font->Size = 13;
				int w = C->TextWidth(s);
				if (w < 60){
					w = 60;
				}

				C->Fill->Color = SDEEP_CONSOLE_COLOR_HALF;
				TRectF rect = RectF(legend_right - w,	24,	legend_right, 48);
				C->FillText(rect,
					s,
					false,
					1.0,
					TFillTextFlags(),
					TTextAlign::Leading,
					TTextAlign::Center
					);

				C->Stroke->Color = i == 0 ? SDEEP_CONSOLE_COLOR_COSTGRAPH : SDEEP_CONSOLE_COLOR_ERRORGRAPH;
				C->Stroke->Thickness = 3;
				C->Stroke->Dash = i < 2 ? TStrokeDash::Solid : TStrokeDash::Dot;
				C->DrawLine(PointF(legend_right - w, 24), PointF(legend_right - w + 48, 24), 1.0);

				legend_right -= w + 8;

				if (i > 0){
					const int checked = MIMV_TrainingValidation->IsChecked ||
						(i == 1 ? MIMV_Training->IsChecked : MIMV_Validation->IsChecked);
					monitor_checkbox_[i - 1] = RectF(legend_right - 16, 24, legend_right, 40);
					C->DrawBitmap(
						checked ? Im_Checked->Bitmap : Im_NotChecked->Bitmap,
						RectF(0, 0, Im_Checked->Bitmap->Width, Im_Checked->Bitmap->Height),
						monitor_checkbox_[i - 1],
						1.0,
						false);
                    monitor_checkbox_[i - 1].right = legend_right + w + 8;

					legend_right -= 16 + 24;
				}
			}

			/*
			//Legend
			C->Fill->Color = SDEEP_CONSOLE_COLOR_BACKGROUND;
			const int legend_items = MIMV_TrainingValidation->IsChecked ? 3 : 2;
			const int legend_height = legend_vspace * 2 + legend_pitch * legend_items;
			TRectF legend_rect =
				RectF(
					monitor_hspace + graph_width - legend_space - legend_width,
					monitor_vspace + legend_space,
					monitor_hspace + graph_width - legend_space,
					monitor_vspace + legend_space + legend_height);
			C->FillRect(
				legend_rect,
				0.0f, 0.0f, AllCorners, 0.5f, TCornerType::Round);
			C->Stroke->Thickness = 1;
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->DrawRect(
				legend_rect,
				0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			for (int i = 0 ; i < legend_items ; i++) {
				C->Stroke->Color = i == 0 ? SDEEP_CONSOLE_COLOR_COSTGRAPH : SDEEP_CONSOLE_COLOR_ERRORGRAPH;
				int item_index = i;
				if (MIMV_Validation->IsChecked && i == 1) {
					item_index = 2;
				}

				C->Stroke->Thickness = 3;
				C->Stroke->Dash = item_index < 2 ? TStrokeDash::Solid : TStrokeDash::Dash;
				TRectF legend_rect_1 =
					RectF(
						legend_rect.Left,
						legend_rect.Top + legend_vspace + legend_pitch * i,
						legend_rect.Right,
						legend_rect.Top + legend_vspace + legend_pitch * (i + 1));
				C->DrawLine(
					PointF(legend_rect_1.Left + legend_vspace * 2, (legend_rect_1.Top + legend_rect_1.Bottom) / 2),
					PointF(legend_rect_1.Left + legend_rect_1.Width () / 3, (legend_rect_1.Top + legend_rect_1.Bottom) / 2),
					1.0f);
				UnicodeString s = item_index == 0 ? "Cost" : (item_index == 1 ? "Training Error" : "Validation Error");
				C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
				C->FillText(
					RectF(
						legend_rect_1.Left + legend_rect.Width () / 3,
						legend_rect_1.Top,
						legend_rect_1.Right,
						legend_rect_1.Bottom),
					s,
					false,
					1.0,
					TFillTextFlags(),
					TTextAlign::Center,
					TTextAlign::Center
					);
			}
			*/
		}

		if (Bu_Monitor->IsFocused && result_last_mouse_y_ >= monitor_vspace && result_last_mouse_y_ < height - monitor_vspace &&
			result_last_mouse_x_ >= monitor_hspace && result_last_mouse_x_ < width - monitor_hspace && nearest_x[0] > 0) {
			C->Stroke->Thickness = 1;
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_HALF;
			C->DrawLine(
				TPointF(nearest_x[0], monitor_vspace),
				TPointF(nearest_x[0], height - monitor_vspace),
				1.0f);

			C->Stroke->Thickness = 9;
			C->Stroke->Dash = TStrokeDash::Solid;
			C->Font->Size = 16;
			const int order[6] = {3, 4, 5, 0, 1, 2};
			for (int i = 0 ; i < 6 ; i++) {
                int index = order[i];
				if (fabs(nearest_x[index] - nearest_x[0]) <= 0.01) {
					C->Stroke->Color = index < 3 ?
						(index == 0 ? SDEEP_CONSOLE_COLOR_COSTGRAPH : SDEEP_CONSOLE_COLOR_ERRORGRAPH) :
						(index == 3 ? SDEEP_CONSOLE_COLOR_COSTBG : SDEEP_CONSOLE_COLOR_ERRORBG);
					TPathData *path = new TPathData();
					path->MoveTo(PointF(nearest_x[index], nearest_y[index]));
					path->LineTo(PointF(nearest_x[index], nearest_y[index]));
					C->DrawPath(path, 1.0);
					delete path;

					C->Fill->Color = C->Stroke->Color;
					C->FillText(
						RectF(
							nearest_x[index] + 8,
							nearest_y[index] - 32,
							nearest_x[index] + 128,
							nearest_y[index]),
						FormatFloat("0.000", nearest_v[index]),
						false,
						1.0f,
						TFillTextFlags(),
						TTextAlign::Leading,
						TTextAlign::Center
						);
				}
			}
		}
		/*
		if (hint_string != "") {
			//TCanvasSaveState *save_state = C->SaveState();
			//C->IntersectClipRect(RectF(monitor_hspace, monitor_vspace, width - monitor_hspace, height - monitor_vspace));
			C->Fill->Color = SDEEP_CONSOLE_COLOR_BACKGROUND;
			int x_offset = 0;
			switch (hint_h_align) {
			case TAlignLayout::Left: x_offset = -144; break;
			case TAlignLayout::Right: x_offset = 144; break;
			default: break;
			}
			int y_offset = 0;
			switch (hint_v_align) {
			case TAlignLayout::Top: y_offset = -56; break;
			case TAlignLayout::Bottom: y_offset = 56; break;
			default: break;
			}
			TRectF hint_rect =
				RectF(
					x_offset + hint_x - 128,
					y_offset + hint_y - 40,
					x_offset + hint_x + 128,
					y_offset + hint_y + 40);
			C->FillRect(
				hint_rect,
				0.0f, 0.0f, AllCorners, 0.5f, TCornerType::Round);
			C->Stroke->Thickness = 1;
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->DrawRect(
				hint_rect,
				0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->FillText(
				hint_rect,
				hint_string.PChar(),
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Center,
				TTextAlign::Center
				);
			//C->RestoreState(save_state);
		}
		*/
	}
}
//---------------------------------------------------------------------------
void TFo_Main::DrawGraphBitmap_TradeoffGraph(TCanvas *C, int width, int height)
{
	static const int graph_scale_size = 16;
	static const int graph_scale_pitch = 64;

	// static const int legend_space = 32;
	// static const int legend_width = 240;
	// static const int legend_vspace = 8;
	// static const int legend_pitch = 24;

	const int graph_width = width - monitor_hspace - monitor_hspace_2;
	const int graph_height = height - monitor_vspace * 2;

	if (graph_width > 0 && graph_height > 0) {
		FString hint_string;
		float hint_x = 0.0f, hint_y = 0.0f;
		TAlignLayout hint_h_align = TAlignLayout::Center;
		TAlignLayout hint_v_align = TAlignLayout::Bottom;

		//Tradeoff Analysis

		if (result_current_tradeoff_value_range_[1] > result_current_tradeoff_value_range_[0]) {
			const double log_range = log(result_current_tradeoff_value_range_[1] / result_current_tradeoff_value_range_[0]);

			//NumMultiplyAdd Scale
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->Stroke->Thickness = 1;
			C->Font->Size = 16;
			int num_step = 1;
			int num_step_x = 0;
			__int64 in = 1;
			FStringList scale_text;
			FListT<float> scale_x;
			while (num_step_x < 16 && in < result_current_tradeoff_value_range_[1]) {
				if (in >= result_current_tradeoff_value_range_[0]) {
					const float x = TOCostToX(in);
					C->Stroke->Color = SDEEP_CONSOLE_COLOR_THINLINE;
					C->DrawLine(
						PointF(x, monitor_vspace),
						PointF(x, monitor_vspace + graph_height),
						1.0f);
					C->Stroke->Color = SDEEP_CONSOLE_COLOR_THINLINE;
					C->DrawLine(
						PointF(x, monitor_vspace + graph_height - graph_scale_size / 2),
						PointF(x, monitor_vspace + graph_height + graph_scale_size / 2),
						1.0f);
					UnicodeString s =
						in < 1000 ? IntToStr(in) :
							(in < 1000000 ? IntToStr(in / 1000) + UnicodeString("k") :
								(in < 1000000000 ? IntToStr(in / 1000000) + UnicodeString("M") :
									(in < 1000000000000.0 ? IntToStr(in / 1000000000) + UnicodeString("G") :
										IntToStr((__int64)(in / 1000000000000.0)) + UnicodeString("T"))));
					scale_text.Add(FString(AnsiString(s).c_str()));
					scale_x.Add(x);
				}
				num_step++;
				if (num_step >= 10) {
					num_step = 1;
					num_step_x++;
				}
				in = num_step * pow(10.0, num_step_x);
			}

			float last_x = monitor_hspace + graph_width;
			for (int i = scale_text.GetSize() - 1 ; i >= 0 ; i--) {
				UnicodeString s = scale_text.GetAt(i).PChar();
				const float x = scale_x.GetAt(i);
				const int width = C->TextWidth(s);
				if (x + width / 2 <= last_x) {
					C->FillText(
						RectF(
							x - width / 2,
							monitor_vspace + graph_height + graph_scale_size / 2,
							x + width / 2,
							monitor_vspace * 2 + graph_height),
						s,
						false,
						1.0f,
						TFillTextFlags(),
						TTextAlign::Center,
						TTextAlign::Leading
						);
					last_x = x - width / 2 - 16;
				}
			}
		}

		//Error scale
		if (result_current_tradeoff_value_range_[3] > result_current_tradeoff_value_range_[2]) {
			C->Fill->Color = SDEEP_CONSOLE_COLOR_ERRORGRAPH;
			C->Font->Size = 16;
			if (RB_GraphLogScale->IsChecked) {
				int num_step = 1;
				int num_step_x = 0;
				__int64 in = 1;
				FStringList scale_text;
				FListT<float> scale_y;
				while (num_step_x < 10 && in * 0.001 < result_current_tradeoff_value_range_[3]) {
					if (in * 0.001 >= result_current_tradeoff_value_range_[2]) {
						const float y = TOErrorToY(in * 0.001);
						if (y < monitor_vspace + graph_height){
							C->Stroke->Color = SDEEP_CONSOLE_COLOR_ERRORBG;
							C->DrawLine(
								PointF(monitor_hspace, y),
								PointF(monitor_hspace + graph_width, y),
								1.0f);
							C->Stroke->Color = SDEEP_CONSOLE_COLOR_ERRORGRAPH;
							C->DrawLine(
								PointF(monitor_hspace - graph_scale_size / 2, y),
								PointF(monitor_hspace + graph_scale_size / 2, y),
								1.0f);
						}
						scale_y.Add(y);
						scale_text.Add(FString(AnsiString(FormatFloat("0.000", in * 0.001)).c_str()));
					}
					num_step++;
					if (num_step >= 10) {
						num_step = 1;
						num_step_x++;
					}
					in = num_step * pow(10.0, num_step_x);
				}
				float last_y = 0.0f;
				for (int i = scale_text.GetSize() - 1 ; i >= 0 ; i--) {
					UnicodeString s = scale_text.GetAt(i).PChar();
					const float y = scale_y.GetAt(i);
					if (y > last_y + C->Font->Size * 1.5) {
						C->FillText(
							RectF(
								0.0f,
								y - graph_scale_pitch / 2,
								monitor_hspace - graph_scale_size / 2,
								y + graph_scale_pitch / 2),
							s,
							false,
							1.0f,
							TFillTextFlags(),
							TTextAlign::Trailing,
							TTextAlign::Center
							);
						last_y = y;
					}
				}
			}else {
				static const double min_error_step = 0.001;
				__int64 step = 1;
				while (step > 0 && step * min_error_step * graph_height * 1.0 / (result_current_tradeoff_value_range_[3] - result_current_tradeoff_value_range_[2]) < graph_scale_pitch) {
					if (FString((double)step).SubString(1,1) == "2") {
						step = (step / 2) * 5;
					}else {
						step *= 2;
					}
				}
				const double min = result_current_tradeoff_value_range_[2] - fmod(result_current_tradeoff_value_range_[2] + step, step * min_error_step) + step * min_error_step;
				if (step > 0) for (double v = min ; v <= result_current_tradeoff_value_range_[3] ; v += step * min_error_step) {
					const float y = TOErrorToY(v);
					if (y < monitor_vspace + graph_height){
						C->Stroke->Color = SDEEP_CONSOLE_COLOR_ERRORBG;
						C->DrawLine(
							PointF(monitor_hspace, y),
							PointF(monitor_hspace + graph_width, y),
							1.0f);
						C->Stroke->Color = SDEEP_CONSOLE_COLOR_ERRORGRAPH;
						C->DrawLine(
							PointF(monitor_hspace - graph_scale_size / 2, y),
							PointF(monitor_hspace + graph_scale_size / 2, y),
							1.0f);
                    }
					C->FillText(
						RectF(
							0.0f,
							y - graph_scale_pitch / 2,
							monitor_hspace - graph_scale_size / 2,
							y + graph_scale_pitch / 2),
						FormatFloat("0.000", v),
						false,
						1.0f,
						TFillTextFlags(),
						TTextAlign::Trailing,
						TTextAlign::Center
						);
				}
			}
		}

		//Draw dots
		TCanvasSaveState *save_state = C->SaveState();
		C->IntersectClipRect(RectF(monitor_hspace, monitor_vspace, monitor_hspace + graph_width, height - monitor_vspace));
		if (result_current_tradeoff_value_range_[1] > result_current_tradeoff_value_range_[0] &&
			result_current_tradeoff_value_range_[3] > result_current_tradeoff_value_range_[2]) {

			const double log_range = log(result_current_tradeoff_value_range_[1] / result_current_tradeoff_value_range_[0]);

			//Enum pareto optimal
			FListT<int> pareto_optimal;
			const int start_index =
				Co_ResultsTradeoffType->ItemIndex == 1 && nn_document_.GetCurrentResultIndex() >= 0 ?
					nn_document_.GetCurrentResultIndex() ://previous
					0;//all
			FListT<double> verr_cache;
			FListT<__int64> statistics_cache;
			FListT<int> pareto_cache;
			for (int ie = 0 ; ie < tradeoff_csv_.GetNumData() ; ie++) {
				verr_cache.Add(tradeoff_csv_.GetFloatValue(ie, FString("verr")));
				statistics_cache.Add(tradeoff_csv_.GetFloatValue(ie, result_tradeoff_h_statistics_str_));
				pareto_cache.Add(1);
			}
			if (Co_ResultsTradeoffType->ItemIndex == 1){
				for (int ie = start_index ; ie < tradeoff_csv_.GetNumData() ; ie++) if (pareto_cache.GetAt(ie)){
					const double verr = verr_cache.GetAt(ie);
					const __int64 statistics = statistics_cache.GetAt(ie);
					int pareto = verr < NAN_VALUE_THRESHOLD && statistics >= 0.0;
					if (pareto) for (int ie2 = start_index ; ie2 < tradeoff_csv_.GetNumData() ; ie2++) if (ie != ie2) {
						int check = true;
						if (!pareto_cache.GetAt(ie2)) {
							check = false;
						}
						if (check) {
							const double verr2 = verr_cache.GetAt(ie2);
							const __int64 statistics2 = statistics_cache.GetAt(ie2);
							if (verr2 < NAN_VALUE_THRESHOLD && statistics2 >= 0.0){
								if ((verr2 < verr && statistics2 < statistics) || (verr2 <= verr && statistics2 <= statistics && ie2 < ie)) {
									pareto = false;
									break;
								}else if (verr < verr2 && statistics < statistics2) {
									pareto_cache.SetAt(ie2, false);
								}
							}
						}
					}
					pareto_cache.SetAt(ie, pareto);
					if (pareto){
						pareto_optimal.Add(ie);
					}
				}
			}else {
				for (int ie = start_index ; ie < tradeoff_csv_.GetNumData() ; ie++) {
					if (tradeoff_csv_.GetValue(ie, FString("pareto")) == "1"){
						pareto_optimal.Add(ie);
					}
				}
			}
			//Sort pareto optimul
			for (int ie = 1 ; ie < pareto_optimal.GetSize() ; ie++) {
				int index = ie;
				while (index > 0){
					if (tradeoff_csv_.GetFloatValue(pareto_optimal.GetAt(index), result_tradeoff_h_statistics_str_) <
						tradeoff_csv_.GetFloatValue(pareto_optimal.GetAt(index - 1), result_tradeoff_h_statistics_str_)) {
						const int tmp = pareto_optimal.GetAt(index);
						pareto_optimal.SetAt(index, pareto_optimal.GetAt(index - 1));
						pareto_optimal.SetAt(index - 1, tmp);
						index--;
					}else {
						break;
					}
				}
			}
			//Draw pareto optimul line
			if (MIMV_TrainingValidation->IsChecked || MIMV_Validation->IsChecked){
				C->Stroke->Thickness = 2;
				C->Stroke->Color = SDEEP_CONSOLE_COLOR_PARETOLINE;
				float last_x = 0.0f, last_y = 0.0f;
				for (int ie = 0 ; ie < pareto_optimal.GetSize() ; ie++) {
					const double verr = verr_cache.GetAt(pareto_optimal.GetAt(ie));
					const __int64 statistics = statistics_cache.GetAt(pareto_optimal.GetAt(ie));
					if (verr < NAN_VALUE_THRESHOLD && statistics >= 0) {
						const float x = statistics > 0 ? TOCostToX(statistics) : monitor_hspace;
						const float y = TOErrorToY(verr);

						if (last_x > 0.0) {
							C->DrawLine(
								TPoint(last_x, last_y),
								TPoint(x, last_y),
								1.0f);
							C->DrawLine(
								TPoint(x, last_y),
								TPoint(x, y),
								1.0f);
						}else if (x > monitor_hspace){
							C->DrawLine(
								TPoint(x, monitor_vspace),
								TPoint(x, y),
								1.0f);
						}
						if (ie == pareto_optimal.GetSize() - 1) {
							C->DrawLine(
								TPoint(monitor_hspace + graph_width, y),
								TPoint(x, y),
								1.0f);
						}

						last_x = x;
						last_y = y;
					}
				}
			}

			//Plot networks
			for (int ie = start_index ; ie < tradeoff_csv_.GetNumData() ; ie++) {
				const FString pareto = tradeoff_csv_.GetValue(ie, FString("pareto"));
				const FString &dir = tradeoff_csv_.GetValue(ie, FString("dir"));
				int draw = dir == nn_document_.GetCurrentResultName();
				switch (Co_ResultsTradeoffType->ItemIndex) {
				case 2:
					draw |= pareto == "1";
					break;
				case 0:
				case 1:
				default:
					draw = true;
					break;
				}
				if (draw) {
					//C->Stroke->Color = (0xff000000 | GetColor(ie * 1.0 / tradeoff_csv_.GetNumData(), 60, 160));
					C->Stroke->Color = 0xff000000 + FadeColor(SDEEP_CONSOLE_COLOR_LINER_ERROR, SDEEP_CONSOLE_COLOR_LIGHTEST, (0.7 * ie ) / (tradeoff_csv_.GetNumData() + 1));
					const FString &terr = tradeoff_csv_.GetValue(ie, FString("terr"));
					const FString &verr = tradeoff_csv_.GetValue(ie, FString("verr"));
					const FString &statistics = tradeoff_csv_.GetValue(ie, result_tradeoff_h_statistics_str_);
					C->Stroke->Thickness = 1;
					if (dir == nn_document_.GetCurrentResultName()) {
						C->Stroke->Color = SDEEP_CONSOLE_COLOR_PRTSELECT;
					}
					C->Fill->Color = C->Stroke->Color;
					if (statistics != "") {
						int size = 4;
						const float x = TOCostToX(statistics.AsFloat());
						//Plot training error
						if (terr != "" && (MIMV_TrainingValidation->IsChecked || MIMV_Training->IsChecked)) {
							const float y = TOErrorToY(terr.AsFloat());
							C->DrawRect(
								RectF(x - size, y - size, x + size, y + size),
								0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
							if (fabs(x - result_last_mouse_x_) <= size && fabs(y - result_last_mouse_y_) <= size) {
								if (result_last_mouse_down_) {
									Fr_ResultsList->SetItemIndex(tradeoff_csv_.GetValue(ie, FString("idx")).AsInt());
								}
								hint_x = x;
								hint_y = y;
								hint_string =
									dir +
									FString("\nTraining Error = ") + terr +
									FString("\n") +	result_tradeoff_h_statistics_str_ + FString(" = ") + statistics.InsertThousandSeparator();
							}
						}
						//Plot validation error
						if (verr != "" && (MIMV_TrainingValidation->IsChecked || MIMV_Validation->IsChecked)) {
							const float y = TOErrorToY(verr.AsFloat());
							C->FillRect(
								RectF(x - size, y - size, x + size, y + size),
								0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
							if (fabs(x - result_last_mouse_x_) <= size && fabs(y - result_last_mouse_y_) <= size) {
								if (result_last_mouse_down_) {
									Fr_ResultsList->SetItemIndex(tradeoff_csv_.GetValue(ie, FString("idx")).AsInt());
								}
								hint_x = x;
								hint_y = y;
								hint_string =
									dir +
									FString("\nBest Validation Error = ") + verr +
									FString("\n") +	result_tradeoff_h_statistics_str_ + FString(" = ") + statistics.InsertThousandSeparator();
							}

							//Draw name of the network
							if (MIMV_Name->IsChecked || MIMV_Comment->IsChecked){
								C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
								int name_x = x + size;
								if (name_x + C->TextWidth(dir.PChar()) >= monitor_hspace + graph_width){
									name_x = monitor_hspace + graph_width - C->TextWidth(dir.PChar());
								}
								C->FillText(
									RectF(name_x, 0, width, y - size),
									MIMV_Name->IsChecked ? dir.PChar() : tradeoff_csv_.GetValue(ie, FString("comment")).PChar(),
									false,
									1.0f,
									TFillTextFlags(),
									TTextAlign::Leading,
									TTextAlign::Trailing
									);
							}
						}
					}
				}
			}

			//Structure Search candidate
			if (MIMV_TrainingValidation->IsChecked || MIMV_Validation->IsChecked){
				FListT<double> gain_for_sort = structure_search_candidate_gain_;
				gain_for_sort.Sort();
				const double min_gain = gain_for_sort.GetSize() > 20 ? gain_for_sort.GetAt(gain_for_sort.GetSize() - 20) : 0.0;
				C->Stroke->Color = SDEEP_CONSOLE_COLOR_LIGHTBRAND;
				C->Stroke->Thickness = 1;
				C->Fill->Color = SDEEP_CONSOLE_COLOR_LIGHTFG2;
				for (int i = 0 ; i < structure_search_candidate_myu_.GetSize() ; i++) if (i != structure_search_candidate_index_){
					if (structure_search_candidate_gain_.GetAt(i) > min_gain) {
						const float x = TOCostToX(structure_search_candidate_cost_.GetAt(i));
						const float y = TOErrorToY(exp(structure_search_candidate_myu_.GetAt(i)));
						const float y1 = TOErrorToY(exp(structure_search_candidate_myu_.GetAt(i) - structure_search_candidate_sigma_.GetAt(i)));
						const float y2 = TOErrorToY(exp(structure_search_candidate_myu_.GetAt(i) + structure_search_candidate_sigma_.GetAt(i)));
						C->DrawLine(PointF(x, y1), PointF(x, y2), 1.0);
						C->DrawLine(PointF(x - 2, y), PointF(x + 2, y), 1.0);
						/*
						C->FillText(
							RectF(x + 2, y, x + 200, y + 200),
							FormatFloat("0.0000", structure_search_candidate_gain_.GetAt(i)),
							false,
							1.0f,
							TFillTextFlags(),
							TTextAlign::Leading,
							TTextAlign::Leading
							);
						*/
					}
				}
				C->Stroke->Thickness = 1;
				C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
				if (structure_search_candidate_index_ >= 0 && structure_search_candidate_index_ < structure_search_candidate_cost_.GetSize()){
					int i = structure_search_candidate_index_;
					const float x = TOCostToX(structure_search_candidate_cost_.GetAt(i));
					const float y = TOErrorToY(exp(structure_search_candidate_myu_.GetAt(i)));
					const float y1 = TOErrorToY(exp(structure_search_candidate_myu_.GetAt(i) - structure_search_candidate_sigma_.GetAt(i)));
					const float y2 = TOErrorToY(exp(structure_search_candidate_myu_.GetAt(i) + structure_search_candidate_sigma_.GetAt(i)));
					C->Stroke->Color = SDEEP_CONSOLE_COLOR_CANDIDATE;
					C->DrawLine(PointF(x, y1), PointF(x, y2), 1.0);
					C->DrawLine(PointF(x - 4, y), PointF(x + 4, y), 1.0);
					C->FillText(
						RectF(x + 2, y, x + 200, y + 200),
						FormatFloat("0.0000", structure_search_candidate_gain_.GetAt(i)),
						false,
						1.0f,
						TFillTextFlags(),
						TTextAlign::Leading,
						TTextAlign::Leading
						);
				}else if (nn_document_.GetCurrentResultIndex() >= 0){
					const FString &s_log_myu = tradeoff_csv_.GetValue(nn_document_.GetCurrentResultIndex(), FString("expm"));
					const FString &s_log_sigma = tradeoff_csv_.GetValue(nn_document_.GetCurrentResultIndex(), FString("exps"));
					if (s_log_sigma != "") {
						const double log_myu = s_log_myu.AsFloat();
						const double log_sigma = s_log_sigma.AsFloat();
						const double cost = tradeoff_csv_.GetFloatValue(nn_document_.GetCurrentResultIndex(), result_tradeoff_h_statistics_str_);
						const float x = TOCostToX(cost);
						const float y = TOErrorToY(exp(log_myu));
						const float y1 = TOErrorToY(exp(log_myu - log_sigma));
						const float y2 = TOErrorToY(exp(log_myu + log_sigma));
						C->Stroke->Color = SDEEP_CONSOLE_COLOR_DARKEST;
						C->DrawLine(PointF(x, y1), PointF(x, y2), 1.0);
						C->DrawLine(PointF(x - 6, y), PointF(x + 6, y), 1.0);
						C->DrawLine(PointF(x - 3, y1), PointF(x + 3, y1), 1.0);
						C->DrawLine(PointF(x - 3, y2), PointF(x + 3, y2), 1.0);

						if (hint_string == "") {
							hint_x = x;
							hint_y = y;
							hint_h_align = hint_x + 288 > monitor_hspace + graph_width - 16 ? TAlignLayout::Left : TAlignLayout::Right;
							hint_v_align = hint_y + 56 > monitor_vspace + graph_height - 16 ? TAlignLayout::Top : TAlignLayout::Center;
							hint_string = FString((
									AnsiString("+1 sigma = ") +
									FormatFloat("0.000", exp(log_myu + log_sigma)) +
									AnsiString("\nmyu = ") +
									FormatFloat("0.000", exp(log_myu)) +
									AnsiString("\n-1 sigma = ") +
									FormatFloat("0.000", exp(log_myu - log_sigma))
								).c_str());

						}
					}
				}
			}
		}

		C->RestoreState(save_state);

		C->Stroke->Dash = TStrokeDash::Solid;
		C->Stroke->Thickness = 1;
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
		C->DrawLine(
			PointF(monitor_hspace, monitor_vspace),
			PointF(monitor_hspace, monitor_vspace + graph_height),
			1.0f);
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
		C->DrawLine(
			PointF(monitor_hspace, monitor_vspace + graph_height),
			PointF(monitor_hspace + graph_width, monitor_vspace + graph_height),
			1.0f);

		C->Font->Size = 16;
		C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
		C->FillText(
			RectF(16, 0, width, 48),
			"Trade-off Graph",
			false,
			1.0,
			TFillTextFlags(),
			TTextAlign::Leading,
			TTextAlign::Center
			);

		C->Font->Size = 16;
		C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
		result_tradeoff_h_statistics_rect_ = RectF(monitor_hspace, height - 48, monitor_hspace + graph_width, height);
		C->FillText(
			result_tradeoff_h_statistics_rect_,
			result_tradeoff_h_statistics_str_.PChar(),
			false,
			1.0,
			TFillTextFlags(),
			TTextAlign::Center,
			TTextAlign::Center
			);

		C->Font->Size = 16;
		C->Fill->Color = SDEEP_CONSOLE_COLOR_ERRORGRAPH;
		C->FillText(
			RectF(16, height - 48, monitor_hspace, height),
			"Error",
			false,
			1.0,
			TFillTextFlags(),
			TTextAlign::Center,
			TTextAlign::Center
			);

		//Legend
		int legend_right = monitor_hspace + graph_width;
		for (int i = 1 ; i >= 0 ; i--) {
			UnicodeString s = i == 0 ? "TRAINING ERROR" : "BEST VALIDATION ERROR";
			C->Font->Size = 13;
			int w = C->TextWidth(s);

			C->Fill->Color = SDEEP_CONSOLE_COLOR_HALF;
			TRectF rect = RectF(legend_right - w,	24,	legend_right, 48);
			C->FillText(rect,
				s,
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center
				);

			legend_right -= w + 8;

			if (i == 0){
				C->Stroke->Thickness = 1;
				C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINER_ERROR;
				C->DrawRect(
					RectF(legend_right - 8, 32, legend_right, 40),
					0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			}else {
				C->Fill->Color = SDEEP_CONSOLE_COLOR_LINER_ERROR;
				C->FillRect(
					RectF(legend_right - 8, 32, legend_right, 40),
					0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			}

			legend_right -= 8 + 24;
		}

		/*
		//Legend
		C->Fill->Color = SDEEP_CONSOLE_COLOR_BACKGROUND;
		const int legend_items = MIMV_TrainingValidation->IsChecked ? 2 : 1;
		const int legend_height = legend_vspace * 2 + legend_pitch * legend_items;
		TRectF legend_rect =
			RectF(
				monitor_hspace + graph_width - legend_space - legend_width,
				monitor_vspace + legend_space,
				monitor_hspace + graph_width - legend_space,
				monitor_vspace + legend_space + legend_height);
		C->FillRect(
			legend_rect,
			0.0f, 0.0f, AllCorners, 0.5f, TCornerType::Round);
		C->Stroke->Thickness = 1;
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
		C->DrawRect(
			legend_rect,
			0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
		for (int i = 0 ; i < legend_items ; i++) {
			const int item_index = i > 0 || !MIMV_Validation->IsChecked ? i : 1;
			C->Stroke->Thickness = 1;
			TRectF legend_rect_1 =
				RectF(
					legend_rect.Left,
					legend_rect.Top + legend_vspace + legend_pitch * i,
					legend_rect.Right,
					legend_rect.Top + legend_vspace + legend_pitch * (i + 1));
			if (item_index == 0) {
				C->Stroke->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
				C->DrawRect(
					RectF(
						legend_rect_1.Left + legend_rect_1.Width() / 4 - 4, (legend_rect_1.Top + legend_rect_1.Bottom) / 2 - 4,
						legend_rect_1.Left + legend_rect_1.Width() / 4 + 4, (legend_rect_1.Top + legend_rect_1.Bottom) / 2 + 4),
						0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			}else {
				C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
				C->FillRect(
					RectF(
						legend_rect_1.Left + legend_rect_1.Width() / 4 - 4, (legend_rect_1.Top + legend_rect_1.Bottom) / 2 - 4,
						legend_rect_1.Left + legend_rect_1.Width() / 4 + 4, (legend_rect_1.Top + legend_rect_1.Bottom) / 2 + 4),
						0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			}
			UnicodeString s = item_index == 0 ? "Training Error" : "Best Validation Error";
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->FillText(
				RectF(
					legend_rect_1.Left + legend_rect.Width () / 3,
					legend_rect_1.Top,
					legend_rect_1.Right,
					legend_rect_1.Bottom),
				s,
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Center,
				TTextAlign::Center
				);
		}
		*/

		if (hint_string != "") {
			//TCanvasSaveState *save_state = C->SaveState();
			//C->IntersectClipRect(RectF(monitor_hspace, monitor_vspace, width - monitor_hspace, height - monitor_vspace));
			C->Fill->Color = SDEEP_CONSOLE_COLOR_BACKGROUND;
			int x_offset = 0;
			switch (hint_h_align) {
			case TAlignLayout::Left: x_offset = -144; break;
			case TAlignLayout::Right: x_offset = 144; break;
			default: break;
			}
			int y_offset = 0;
			switch (hint_v_align) {
			case TAlignLayout::Top: y_offset = -56; break;
			case TAlignLayout::Bottom: y_offset = 56; break;
			default: break;
			}
			TRectF hint_rect =
				RectF(
					x_offset + hint_x - 128,
					y_offset + hint_y - 40,
					x_offset + hint_x + 128,
					y_offset + hint_y + 40);
			C->FillRect(
				hint_rect,
				0.0f, 0.0f, AllCorners, 0.5f, TCornerType::Round);
			C->Stroke->Thickness = 1;
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->DrawRect(
				hint_rect,
				0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->FillText(
				hint_rect,
				hint_string.PChar(),
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Center,
				TTextAlign::Center
				);
			//C->RestoreState(save_state);
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::DrawGraphBitmap_Weight(TCanvas *C, int width, int height)
{
	C->Font->Size = 16;
	C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
	FString name = result_weight_.GetName() != "" ? result_weight_.GetName() : FString("Weight");
	C->FillText(
		RectF(default_grid_text_hspace, 0, width - default_grid_text_hspace, default_panel_height),
		name.PChar(),
		false,
		1.0,
		TFillTextFlags(),
		TTextAlign::Leading,
		TTextAlign::Center
		);

	FListT<long long> shape = result_weight_.GetShape();
	FListT<float> modified_weight;
	double gain = 1.0;
	if (RB_MonitorNormalize->IsChecked) {
		const double min = result_weight_.GetWeight().Min();
		const double max = result_weight_.GetWeight().Max();
		if (fabs(max) > fabs(min)) {
			if (fabs(max) > 0.0) {
				gain = 1.0 / fabs(max);
			}
		}else {
			if (fabs(min) > 0.0) {
				gain = 1.0 / fabs(min);
			}
		}
	}else if (RB_MonitorGain->IsChecked && TB_MonitorGain->Value != 0) {
		gain = exp(log(100.0) * TB_MonitorGain->Value / TB_MonitorGain->Max);
	}
	if (gain != 0.0) for (int i = 0 ; i < result_weight_.GetWeight().GetSize() ; i++) {
		modified_weight.Add(result_weight_.GetWeight().GetAt(i) * gain);
	}
	const FListT<float> &weight = modified_weight.GetSize() ? modified_weight : result_weight_.GetWeight();

	if (shape.GetSize() == 0 || weight.GetSize() == 0) {
		return;
	}

	const int space = 8;

	//Extend single dim vector to matrix
	if (shape.GetSize() < 2) {
		shape.Insert(0, 1);
	}

	//RGB image
	int display_dim = 2;
	int rgb = false;
	if (shape.GetSize() >= 3) {
		if (shape.GetAt(shape.GetSize() - 3) == 3) {
			display_dim = 3;
			rgb = true;
		}
	}

	//Data size of single matrix
	int data_size = 1;
	for (int i = shape.GetSize() - 1 ; i >= 0 && i >= shape.GetSize() - display_dim ; i--) {
		data_size *= shape.GetAt(i);
	}

	//Total data num
	const int data_num = weight.GetSize() / data_size;

	//Block size
	const double weight_zoom = pow(2.0, result_weight_current_zoom_ * 0.5);
	const int data_block_size = shape.GetSize() > display_dim ? shape.GetAt(shape.GetSize() - display_dim - 1) : 1;

	const int data_width = shape.GetAt(shape.GetSize() - 1);
	const int data_height = shape.GetAt(shape.GetSize() - 2);
	const int data_width_2 = data_width > 1024 ? 1024 : data_width;
	const int data_height_2 = data_height > 1024 ? 1024 : data_height;
	const int top = default_panel_height - result_weight_current_top_, left = default_grid_text_hspace;
	const int plot_height = height - default_panel_height;

	TCanvasSaveState *save_state = C->SaveState();
	C->IntersectClipRect(RectF(0, default_panel_height , width, height));

	int x = left, y = top;

	//Weight
	for (int i = 0 ; i < data_num ; i++) {
		TRectF dest_rect = Rect(x, y, x + data_width_2 * weight_zoom, y + data_height_2 * weight_zoom);
		if ((dest_rect.top >= default_panel_height && dest_rect.top < height) ||
			(dest_rect.bottom >= default_panel_height && dest_rect.bottom < height) ||
			(dest_rect.top < default_panel_height && dest_rect.bottom >= height)) {
			TBitmap *BMP = new TBitmap(data_width_2, data_height_2);
			TBitmapData BMPData;
			BMP->Map(TMapAccess::Write, BMPData);
			const int offset = i * data_size;
			if (!rgb) {
				for (int iy = 0; iy < data_height_2 ; iy++) {
					for (int ix = 0; ix < data_width_2 ; ix++) {
						/*
						int w = weight.GetAt(ix + iy * data_width + offset) * 128.0 + 128.0;
						if (w > 255) {
							w = 255;
						}else if (w < 0) {
							w = 0;
						}
						const int color = 0xff000000 | (w << 16) | (w << 8) | w;
						*/
						/*
						const double w = weight.GetAt(ix + iy * data_width + offset);
						int color;
						if (w > 0.0) {
							int wi = w * 192.0;
							if (wi > 191) {
								wi = 191;
							}
							color = 0xff404040 + (wi << 16) - ((wi / 3) << 8) - (wi / 3);
						}else {
							int wi = -w * 192.0;
							if (wi > 191) {
								wi = 191;
							}
							color = 0xff404040 - ((wi / 3) << 16) - ((wi / 3) << 8) + wi;
						}
						*/
						const double w = weight.GetAt(ix + iy * data_width + offset);
						int color;
						if (w > 0.0) {
							int wi = w * 256.0;
							if (wi > 255) {
								wi = 255;
							}
							color = 0xffffffff - (wi << 8) - wi;
						}else {
							int wi = -w * 256.0;
							if (wi > 255) {
								wi = 255;
							}
							color = 0xffffffff - (wi << 16) - (wi << 8);
						}
						BMPData.SetPixel(ix, iy, color);
					}
				}
			}else {
				const int channel_size = data_size / 3;
				for (int iy = 0; iy < data_height_2 ; iy++) {
					for (int ix = 0; ix < data_width_2 ; ix++) {
						int r = weight.GetAt(ix + iy * data_width + offset) * 128.0 + 128.0;
						if (r > 255) {
							r = 255;
						}else if (r < 0) {
							r = 0;
						}
						int g = weight.GetAt(ix + iy * data_width + channel_size + offset) * 128.0 + 128.0;
						if (g > 255) {
							g = 255;
						}else if (g < 0) {
							g = 0;
						}
						int b = weight.GetAt(ix + iy * data_width + channel_size * 2 + offset) * 128.0 + 128.0;
						if (b > 255) {
							b = 255;
						}else if (b < 0) {
							b = 0;
						}
						const int color = 0xff000000 | (r << 16) | (g << 8) | b;
						BMPData.SetPixel(ix, iy, color);
					}
				}
			}
			BMP->Unmap(BMPData);
			C->DrawBitmap(BMP,
				Rect(0, 0, data_width_2, data_height_2),
				dest_rect,
				1.0,
				true);

			delete BMP;
		}
		x += data_width_2 * weight_zoom + space;
		if (x + data_width_2 * weight_zoom > width - default_grid_text_hspace) {
			y += data_height_2 * weight_zoom + space;
			x = default_grid_text_hspace;
		}else if ((i + 1) % data_block_size == 0) {
			y += space + (x > 0 ? data_height_2 * weight_zoom : 0);
			x = default_grid_text_hspace;
		}
	}
	if (data_width_2 != data_width || data_height_2 != data_height){
		C->Font->Size = 16;
		C->Fill->Color = SDEEP_CONSOLE_COLOR_HALF;
		C->FillText(
			RectF(default_grid_text_hspace, y, width - default_grid_text_hspace, y + default_panel_height),
			"* For large weights, up to 1,024 weights are displayed in each width and height.",
			false,
			1.0,
			TFillTextFlags(),
			TTextAlign::Leading,
			TTextAlign::Leading
			);
		y += default_panel_height;
	}

	//Contribution Ratio
	FListT<float> contribution_ratio = result_weight_.GetContributionRatio();
	static const int vspace = 32;
	static const int hspace = 64;
	static const int graph_width = 512;
	static const int graph_height = 256;
	if (contribution_ratio.GetSize() > 1) {
		if ((y >= default_panel_height && y < height) ||
			(y + vspace * 2 + graph_height >= default_panel_height && y + vspace * 2 + graph_height < height) ||
			(y < default_panel_height && y + vspace * 2 + graph_height >= height)) {
			//Title
			C->Font->Size = 16;
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->FillText(
				RectF(default_grid_text_hspace, y, width - default_grid_text_hspace, y + vspace),
				"Contribution Ratio",
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center
				);

			//Grid
			C->Stroke->Dash = TStrokeDash::Solid;
			C->Stroke->Thickness = 1;
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
			C->DrawLine(TPoint(hspace, y + vspace), TPoint(hspace, y + vspace + graph_height), 1.0);
			C->DrawLine(TPoint(hspace, y + vspace + graph_height), TPoint(hspace + graph_width, y + vspace + graph_height), 1.0);

			//Graph
			int last_x = hspace, last_y = y + vspace + graph_height;
			float sum = 0.0, last_sum = 0.0;
			TPathData *path = new TPathData();
			path->MoveTo(PointF(last_x, last_y));
			for (int i = 0 ; i < contribution_ratio.GetSize() ; i++) {
				sum += contribution_ratio.GetAt(i);
				const float x1 = hspace + (i + 1) * graph_width * 1.0f / contribution_ratio.GetSize();
				const float y1 = y + vspace + graph_height - sum * graph_height;
				int draw_dim = i == contribution_ratio.GetSize() - 1;
				if ((last_sum < 0.5f && sum >= 0.5f) ||
					(last_sum < 0.9f && sum >= 0.9f)) {
					C->Stroke->Color = SDEEP_CONSOLE_COLOR_CONTGRAPH2;
					C->Stroke->Thickness = 1;
					C->DrawLine(TPoint(hspace, y1), TPoint(x1, y1), 1.0);
					C->DrawLine(TPoint(x1, y1), TPoint(x1, y + vspace + graph_height), 1.0);

					FString ratio = FString((int)(sum * 100)) + FString("%");
					C->FillText(
						RectF(0, y1 - 16, hspace, y1 + 16),
						ratio.PChar(),
						false,
						1.0,
						TFillTextFlags(),
						TTextAlign::Center,
						TTextAlign::Center
						);

					draw_dim = true;
				}
				if (draw_dim) {
					FString dim = FString(i + 1);
					C->FillText(
						RectF(x1 - 64, y + vspace + graph_height, x1 + 64, y + vspace * 2 + graph_height),
						dim.PChar(),
						false,
						1.0,
						TFillTextFlags(),
						TTextAlign::Center,
						TTextAlign::Center
						);
				}
				path->LineTo(TPoint(x1, y1));

				last_sum = sum;
			}
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_CONTGRAPH;
			C->Stroke->Thickness = 3;
			C->Stroke->Dash = TStrokeDash::Solid;
			C->DrawPath(path, 1.0);
			delete path;

			//0 dim
			C->FillText(
				RectF(hspace - 64, y + vspace + graph_height, hspace + 64, y + vspace * 2 + graph_height),
				"0",
				false,
				1.0,
				TFillTextFlags(),
				TTextAlign::Center,
				TTextAlign::Center
				);
		}

		y += vspace * 2 + graph_height;
	}

	C->RestoreState(save_state);


	Sc_Monitor->Max = y - top + Lo_MonitorGain->Height + 16 > plot_height ? (y - top + Lo_MonitorGain->Height + 16) - plot_height : 0;
	if (result_weight_top_ > Sc_Monitor->Max) {
		result_weight_top_ = Sc_Monitor->Max;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::DrawGraphBitmap(TCanvas *C, int width, int height, FString type)
{
	if (nn_document_.GetLocked()) {
		return;
	}

	C->Stroke->Kind = TBrushKind::Solid;
	C->DrawRect(
		RectF(0, 0, width, height),
		0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);

	if ((type == "" && RB_ResultsLearningCurve->IsChecked) || type == "LearningCurve") {
		DrawGraphBitmap_LearningCurve(C, width, height);
	}else if ((type == "" && RB_ResultsTradeoffGraph->IsChecked) || type == "TradeoffGraph") {
		DrawGraphBitmap_TradeoffGraph(C, width, height);
	}else {
		DrawGraphBitmap_Weight(C, width, height);
	}

	result_last_mouse_down_ = false;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_SetupClick(TObject *Sender)
{
	/*if (FullScreen) {
		FullScreen = false;
		FormResize(Sender);
	}*/

	nn_document_.Lock();
	Fo_Setup = new TFo_Setup(this);
	Fo_Setup->SetLang(Lang->Lang);
	Fo_Setup->SetSDeepConsoleGlobalSettings(&settings_, setting_file_);
	Fo_Setup->ShowModal();
	if (Fo_Setup->apply_) {
		settings_.SaveToIni(setting_file_);
		SetCSVGraphType();
	}
	Fo_Setup->Release();
	Log(FString("setup : processor=") + settings_.ProcessorTypeStr());

	nn_document_.Unlock();
	OpenInstanceList();
	UpdateInstanceList();
	Translate();
	Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	if (activity_ == SDeepConsoleActivity::Default && !auth_failed) {
		if (!CheckSave()){
			CanClose = false;
		}
		if (CanClose && !AskAndTerminateProcess()){
			CanClose = false;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Co_PropertyKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	switch (Key) {
	case 27:
		ClearEditBox();
		Bu_Property->SetFocus();
		break;
	case 13:
		ClearEditBox();
		Bu_Property->SetFocus();
		Key = 0;
		break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Co_PropertyChange(TObject *Sender)
{
	ApplyProperty();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	if (nn_document_.GetLocked()){
		return;
	}

	//La_Test->Text = UnicodeString("Form Keydown") + IntToStr(Key);
	//Me_Debug->Lines->Add(UnicodeString("Form Keydown") + IntToStr(Key));
	switch (Key){
	case 49:
		// Ctrl + 1
		if (Bu_NetworkTab->Visible) {
            Bu_NetworkTabClick(Sender);
		}
        break;
	case 50:
		// Ctrl + 2
		if (Bu_TrainingTab->Visible) {
			Bu_TrainingTabClick(Sender);
		}
		break;
	case 51:
		// Ctrl + 3
		if (Bu_EvaluationTab->Visible) {
			Bu_EvaluationTabClick(Sender);
		}
		break;
	case 52:
		// Ctrl + 4
		if (Bu_DatasetTab->Visible) {
			Bu_DatasetTabClick(Sender);
		}
		break;
	case 53:
		// Ctrl + 5
		if (Bu_ConfigTab->Visible) {
			Bu_ConfigTabClick(Sender);
		}
		break;
	case 93:
		// Context Menu
		if (TC_Left->ActiveTab == TI_BrowserLeft){
			if (Fr_ProjectFiles->Visible && Fr_ProjectFiles->Bu_Menu->Visible){
				Fr_ProjectFiles->Bu_MenuClick(Sender);
			}else if (Fr_DatasetFiles->Visible && Fr_DatasetFiles->Bu_Menu->Visible){
				Fr_DatasetFiles->Bu_MenuClick(Sender);
			}
		}else if (activity_ == SDeepConsoleActivity::Default && TC_Default->Visible){
			if (TC_Default->ActiveTab == TI_Network && Bu_NetworkMenu->Visible){
				Bu_NetworkMenuClick(Sender);
			}else if (TC_Left->ActiveTab == TI_TrainingLeft && Fr_ResultsList->Bu_Menu->Visible){
				Fr_ResultsList->Bu_MenuClick(Sender);
			}else if (TC_Default->ActiveTab == TI_Dataset && Fr_DatasetFiles2->Visible && Fr_DatasetFiles2->Bu_Menu->Visible){
				Fr_DatasetFiles2->Bu_MenuClick(Sender);
			}else if (TC_Left->ActiveTab == TI_DatasetLeft && Fr_DatasetList->Bu_Menu->Visible){
				Fr_DatasetList->Bu_MenuClick(Sender);
			}else if (TC_Left->ActiveTab == TI_ConfigLeft && Fr_ConfigList->Bu_Menu->Visible){
				Fr_ConfigList->Bu_MenuClick(Sender);
			}
		}
		break;
	case 112:
		//F1 : Help
		if (Shift.Contains(ssCtrl)) {
			if (Bu_Help->Visible && Bu_Help->Enabled) {
				Bu_HelpClick(Sender);
			}
		}else {
			OpenManual();
		}
		break;
	case 114:
		//F3 : Search
		if (activity_ == SDeepConsoleActivity::Default && TC_Default->Visible){
			if (TC_Default->ActiveTab == TI_Network && Bu_NetworkMenu->Visible){
				if (Shift.Contains(ssShift)) {
					SearchLayer(-1);
				}else {
					SearchLayer(1);
				}
			}
		}
		break;
	case 121:
		//F10 : Setup
		setup_request_ = true;
		break;
	case 122:
		//F11 : Full Screen
		Bu_FullScreenClick(Sender);
		break;
	case 123:
		//F12 screen cursor on/off
		screen_cursor_on_ = !screen_cursor_on_;
		break;
	case 107://Ctrl + +
	case 187://Ctrl + +
		if (Shift.Contains(ssCtrl)) {
			if (Shift.Contains(ssShift)) {
				if (settings_.GetGUIScale() < 5){
					settings_.SetGUIScale(settings_.GetGUIScale() + 1);
					SetScale(settings_.GetGUIScale());
				}
			}else if (activity_ == SDeepConsoleActivity::Default && TC_Default->Visible && TC_Default->ActiveTab == TI_Network){
				MINZ_ZoomInClick(Sender);
			}
		}
		break;
	case 109://Ctrl + -
	case 189://Ctrl + -
		if (Shift.Contains(ssCtrl)) {
			if (Shift.Contains(ssShift)) {
				if (settings_.GetGUIScale() > -5){
					settings_.SetGUIScale(settings_.GetGUIScale() - 1);
					SetScale(settings_.GetGUIScale());
				}
			}else if (activity_ == SDeepConsoleActivity::Default && TC_Default->Visible && TC_Default->ActiveTab == TI_Network){
				MINZ_ZoomOutClick(Sender);
			}
		}
		break;
	default:
		if (TC_Default->ActiveTab == TI_Browser){
			if (Fr_ProjectFiles->Visible) {
				switch (Key) {
				case 70:
					//Ctrl+F : Search
					if (Fr_ProjectFiles->Ed_ListSearch->Visible) {
						Fr_ProjectFiles->Ed_ListSearch->SetFocus();
						Fr_ProjectFiles->Ed_ListSearch->SelectAll();
					}
					break;
				case 78:
					//Ctrl+N : New
					if (Bu_NewProject->Visible && Bu_NewProject->Enabled){
						Bu_NewProjectTorchClick(Sender);
					}
					break;
				case 79:
					//Ctrl+O : Open
					if (Bu_OpenProject->Visible && Bu_OpenProject->Enabled){
						Bu_OpenProjectClick(Sender);
					}
					break;
				}
			}else if (Fr_DatasetFiles->Visible){
				switch (Key) {
				case 70:
					//Ctrl+F : Search
					if (Fr_DatasetFiles->Ed_ListSearch->Visible) {
						Fr_DatasetFiles->Ed_ListSearch->SetFocus();
						Fr_DatasetFiles->Ed_ListSearch->SelectAll();
					}
					break;
				case 78:
					//Ctrl+N : New
					if (Bu_CreateDataset->Visible && Bu_CreateDataset->Enabled){
						Bu_CreateDatasetClick(Sender);
					}
					break;
				case 79:
					//Ctrl+O : Open
					if (Bu_OpenDataset->Visible && Bu_OpenDataset->Enabled){
						Bu_OpenDatasetClick(Sender);
					}
					break;
				}
			}
		}else if (activity_ == SDeepConsoleActivity::Default && TC_Default->Visible) {
			switch (Key) {
			case 70:
				//Ctrl+F : Search
				if (TC_Default->ActiveTab == TI_Network) {
					if (Fr_Component->Bu_List->IsFocused) {
						Fr_Component->Ed_ListSearch->SetFocus();
						Fr_Component->Ed_ListSearch->SelectAll();
					}else {
						Ed_NetworkSearch->SetFocus();
						Ed_NetworkSearch->SelectAll();
                    }
				}else if (TC_Default->ActiveTab == TI_Dataset && Fr_DatasetFiles2->Visible && Fr_DatasetFiles2->Ed_ListSearch->Visible) {
					Fr_DatasetFiles2->Ed_ListSearch->SetFocus();
					Fr_DatasetFiles2->Ed_ListSearch->SelectAll();
				}else if (Fr_Component->Ed_ListSearch->Visible) {
					Fr_Component->Ed_ListSearch->SetFocus();
					Fr_Component->Ed_ListSearch->SelectAll();
				}
				break;
			case 78:
				//Ctrl+N : New
				if (TC_Default->ActiveTab == TI_Dataset && Fr_DatasetFiles2->Visible && Bu_CreateDataset2->Visible && Bu_CreateDataset2->Enabled){
					Bu_CreateDatasetClick(Sender);
				}
				break;
			case 79:
				//Ctrl+O : Open
				if (TC_Default->ActiveTab == TI_Dataset){
					if (Fr_DatasetFiles2->Visible){
						Bu_OpenDataset2Click(Sender);
					}else if (Bu_DatasetOpen->Visible && Bu_DatasetOpen->Enabled){
						Bu_DatasetOpenClick(Sender);
					}
				}
				break;
			case 83:
				if (Bu_Save->Enabled) {
					//Ctrl+S : Save
					if (Shift.Contains(ssCtrl)) {
						Save();
					}
				}
				break;
			case 87:
				if (Bu_Back->Enabled && Bu_Back->Visible) {
					//Ctrl+W : Close Windows
					if (Shift.Contains(ssCtrl)) {
						Bu_BackClick(Sender);
					}
				}
				break;
			case 89:
			case 90:
				//Ctrl+Y, Ctrl+Z : Redo, Undo
				if (Shift.Contains(ssCtrl)) {
					if (Key == 89 || Shift.Contains(ssShift)) {
						//Ctrl+Y or Ctrl+Shift+Z : Redo
						if (Bu_Redo->Visible && Bu_Redo->Enabled) {
							Bu_RedoClick(Sender);
						}
					}else if (!Shift.Contains(ssShift)){
						//Ctrl+Z : Undo
						if (Bu_Undo->Visible && Bu_Undo->Enabled) {
							Bu_UndoClick(Sender);
						}
					}
				}
				break;
			case 116:
				//F5
				if (Shift.Contains(ssShift)) {
					if (RR_ControlLight->Visible && TC_Default->ActiveTab == TI_Results) {
						control_button_type_ = ControlButtonType::StopTraining;
						Bu_ControlClick(Sender);
					}
				}else {
					if (RR_ControlDark->Visible && TC_Default->ActiveTab == TI_Network) {
						control_button_type_ = ControlButtonType::RunTraining;
						Bu_ControlClick(Sender);
					}
				}
				break;
			case 117:
				//F6
				if (Shift.Contains(ssShift)) {
					if (RR_ControlLight->Visible && TC_Default->ActiveTab == TI_Evaluation) {
						control_button_type_ = ControlButtonType::StopEvaluation;
						Bu_ControlClick(Sender);
					}
				}else {
					if (RR_ControlDark->Visible && TC_Default->ActiveTab == TI_Results) {
						control_button_type_ = ControlButtonType::RunEvaluation;
						Bu_ControlClick(Sender);
					}
				}
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::OpenManual()
{
	UnicodeString lang_u = settings_.GetLanguage() == "auto" ? default_lang_ : UnicodeString(settings_.GetLanguage().PChar());
	UnicodeString file_name = ExtractFilePath(ParamStr(0)) + "manual_" + lang_u + ".pdf";
	if (FileExists(file_name)) {
		OpenFile(AnsiString(file_name).c_str());
	}else {
		OpenFile(AnsiString(ExtractFilePath(ParamStr(0)) + "manual.pdf").c_str());
    }
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIM_OpenResultLocationClick(TObject *Sender)
{
	UnicodeString result_dir = GetResultDir();
	if (RB_ResultsWeight->IsChecked) {
		result_dir += "/net";
	}
	if (DirectoryExists(result_dir)) {
		OpenFileLocation(AnsiString(result_dir).c_str());
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ResetMonitor(int all)
{
	nn_document_.SetCurrentResultIndex(-1);
	current_errors_.Clear();

	monitor_csv_[0].Clear();
	if (all) {
		monitor_csv_[1].Clear();
	}
	tradeoff_csv_.Clear();
	result_name_[0] = "";
	if (all) {
		result_name_[1] = "";
	}
	result_network_.Clear();
	result_network_name_ = "";
	Me_Monitor->Lines->Clear();

	network_overview_ = NULL;
	network_overview_update_count_ = 0;
	result_graph_mapper_.Clear();
	result_weight_.Clear();

	PB_TrainingEpoch->Value = 0;
	La_MonitorEpoch->Text = "(- / -)";

	Ed_MonitorTimeElapsed->Text = "--:--:--:--";
	Ed_MonitorTimeRemaining->Text = "--:--:--:--";
	Ed_MonitorTimeTotal->Text = "--:--:--:--";

	Fr_ResultsList->SetItemIndex(-1);
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshMonitorTimeFromLog()
{
	TFormatSettings *format_settings = new TFormatSettings();
	format_settings->DateSeparator = '-';
	format_settings->TimeSeparator = ':';
	format_settings->ShortDateFormat = "yyyy-mm-dd";
	format_settings->ShortTimeFormat = "hh:nn:ss,zzz";

	UnicodeString elapsed_str = "--:--:--:--";
	UnicodeString remaining_str = "--:--:--:--";
	UnicodeString total_str = "--:--:--:--";

	TDateTime start, epoch_start, end, epoch_end;
	start = end = epoch_end = Now();
	int epoch_start_enabled = false, start_enabled = false;
	int epoch_end_enabled = false, end_enabled = false;
	int epoch = 0, max_epoch = 0;

	for (int il = 0 ; il < Me_Monitor->Lines->Count ; il++) {
		UnicodeString s = Me_Monitor->Lines->Strings[il];
		if (s.SubString(5, 1) == "-" && s.SubString(8, 1) == "-" && s.SubString(11, 1) == " " &&
			s.SubString(14, 1) == ":" && s.SubString(17, 1) == ":" && s.SubString(20, 1) == ",") {
			TDateTime date_time = StrToDateTime(s.SubString(1, 23), *format_settings);
			if (!start_enabled) {
				start_enabled = true;
				start = date_time;
			}

			int pos = s.Pos("epoch ");
			if (pos >= 24) {
				if (s.Pos("begin") >= 24) {
					//2015-05-30 17:13:06,719 [sdeep][INFO] : Training epoch 1 of 30 begin
					FString fs = FString(AnsiString(s).c_str());
					fs.SplitBy("of ");
					max_epoch = fs.SplitBy(" begin").AsInt();
					epoch_start_enabled = true;
					epoch_start = date_time;
				}else {
					FString epoch_str = FString(AnsiString(s.SubString(pos + 6, s.Length() - (pos + 6) + 1)).c_str());
					epoch_str = epoch_str.SplitBy(" ");
					epoch = epoch_str.AsInt();

					epoch_end_enabled = true;
					epoch_end = date_time;
				}
			}
			end_enabled = true;
			end = date_time;
		}
	}

	int training = false;
	if (Fr_ResultsList->GetItemIndex() >= 0 && Fr_ResultsList->GetItemIndex() <  nn_document_.GetResults().GetSize()) {
		training |= GetTraining(nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetDir());
	}
	if (start_enabled && end_enabled) {
		if (!training) {
			int elapsed_sec = (int)((double)(end - start) * 24 * 60 * 60);
			UnicodeString s = IntToStr(elapsed_sec / (24 * 60 * 60));
			if (s.Length() < 2) {
				s = UnicodeString("0") + s;
			}
			s +=
				UnicodeString(":") +
				FormatDateTime("hh:mm:ss", elapsed_sec * 1.0 / (24 * 60 * 60));
			elapsed_str = s;
			remaining_str = "00:00:00:00";
			total_str = elapsed_str;
		}else {
			{
				int elapsed_sec = (int)((double)(Now() - start) * 24 * 60 * 60);
				UnicodeString s = IntToStr(elapsed_sec / (24 * 60 * 60));
				if (s.Length() < 2) {
					s = UnicodeString("0") + s;
				}
				s +=
					UnicodeString(":") +
					FormatDateTime("hh:mm:ss", elapsed_sec * 1.0 / (24 * 60 * 60));
				elapsed_str = s;
			}

			if (epoch_start_enabled && epoch_end_enabled && epoch) {
				const double time_per_epoch = ((double)(epoch_end - epoch_start) / epoch);
				TDateTime estimated_end_time = epoch_start + time_per_epoch * max_epoch;

				{
					int remaining_sec = estimated_end_time > Now() ? (int)((double)(estimated_end_time - Now()) * 24 * 60 * 60) : 0;
					//int remaining_sec = estimated_end_time > epoch_end ? (int)((double)(estimated_end_time - epoch_end) * 24 * 60 * 60) : 0;
					UnicodeString s = IntToStr(remaining_sec / (24 * 60 * 60));
					if (s.Length() < 2) {
						s = UnicodeString("0") + s;
					}
					s +=
						UnicodeString(":") +
						FormatDateTime("hh:mm:ss", remaining_sec * 1.0 / (24 * 60 * 60));
					remaining_str = s;
				}

				{
					int total_sec = (int)((double)(estimated_end_time - start) * 24 * 60 * 60);
					//int total_sec = (int)((double)(estimated_end_time - epoch_start) * 24 * 60 * 60);
					UnicodeString s = IntToStr(total_sec / (24 * 60 * 60));
					if (s.Length() < 2) {
						s = UnicodeString("0") + s;
					}
					s +=
						UnicodeString(":") +
						FormatDateTime("hh:mm:ss", total_sec * 1.0 / (24 * 60 * 60));
					total_str = s;
				}
			}
		}
	}

	if (Ed_MonitorTimeElapsed->Text != elapsed_str) {
		Ed_MonitorTimeElapsed->Text = elapsed_str;
	}
	if (Ed_MonitorTimeRemaining->Text != remaining_str) {
		Ed_MonitorTimeRemaining->Text = remaining_str;
	}
	if (Ed_MonitorTimeTotal->Text != total_str) {
		Ed_MonitorTimeTotal->Text = total_str;
	}

	UnicodeString epoch_text =
		UnicodeString("(") +
		(max_epoch > 0 ? IntToStr((int)epoch) : UnicodeString("-")) +
		UnicodeString(" / ") +
		(max_epoch > 0 ? IntToStr((int)max_epoch) : UnicodeString("-")) +
		UnicodeString(")");

	if (max_epoch == 0) {
		max_epoch = 1;
	}
	if (progress_training_max_ != max_epoch) {
		progress_training_max_ = max_epoch;
	}
	if (progress_training_current_ != epoch) {
		progress_training_current_ = epoch;
	}
	if (La_MonitorEpoch->Text != epoch_text) {
		La_MonitorEpoch->Text = epoch_text;
	}
	delete format_settings;
}
//---------------------------------------------------------------------------
void TFo_Main::ResetEvaluationResult()
{
	nn_document_.ClearResultCSV(0);//Clear Evaluation Result
	nn_document_.ClearResultCSV(1);//Clear Confusion Matrix
	nn_document_.ClearResultCSV(2);//Clear Error Analysis

	Me_Evaluation->Lines->Clear();

	PB_EvaluationData->Value = 0;
	La_EvaluationData->Text = "(- / -)";

	Ed_EvaluationTimeElapsed->Text = "--:--:--:--";
	Ed_EvaluationTimeRemaining->Text = "--:--:--:--";
	Ed_EvaluationTimeTotal->Text = "--:--:--:--";
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshEvaluationTimeFromLog()
{
	TFormatSettings *format_settings = new TFormatSettings();
	format_settings->DateSeparator = '-';
	format_settings->TimeSeparator = ':';
	format_settings->ShortDateFormat = "yyyy-mm-dd";
	format_settings->ShortTimeFormat = "hh:nn:ss,zzz";

	UnicodeString elapsed_str = "--:--:--:--";
	UnicodeString remaining_str = "--:--:--:--";
	UnicodeString total_str = "--:--:--:--";

	TDateTime start, data_start, end, data_end;
	start = end = data_end = Now();
	int data_start_enabled = false, start_enabled = false;
	int data_end_enabled = false, end_enabled = false;
	int data = 0, max_data = 0;

	for (int il = 0 ; il < Me_Evaluation->Lines->Count ; il++) {
		UnicodeString s = Me_Evaluation->Lines->Strings[il];
		if (s.SubString(5, 1) == "-" && s.SubString(8, 1) == "-" && s.SubString(11, 1) == " " &&
			s.SubString(14, 1) == ":" && s.SubString(17, 1) == ":" && s.SubString(20, 1) == ",") {
			TDateTime date_time = StrToDateTime(s.SubString(1, 23), *format_settings);

			if (!start_enabled) {
				start_enabled = true;
				start = date_time;
			}

			int pos = s.Pos("data ");
			if (pos > 24) {
				FString data_str = FString(AnsiString(s.SubString(pos + 5, s.Length() - pos + 5 + 1)).c_str());
				data = data_str.SplitBy(" / ").AsInt();
				max_data = data_str.AsInt();

				data_end_enabled = true;
				data_end = date_time;
				if (!data_start_enabled) {
					data_start_enabled = true;
					data_start = date_time;
				}
			}
			end_enabled = true;
			end = date_time;
		}
	}

	int evaluating = false;
	if (sdeep_evaluation_thread_ && Fr_ResultsList->GetItemIndex() >= 0) {
		evaluating = sdeep_evaluation_thread_->GetName() == nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetDir();
	}
	if (start_enabled && end_enabled) {
		if (!evaluating) {
			int elapsed_sec = (int)((double)(end - start) * 24 * 60 * 60);
			UnicodeString s = IntToStr(elapsed_sec / (24 * 60 * 60));
			if (s.Length() < 2) {
				s = UnicodeString("0") + s;
			}
			s +=
				UnicodeString(":") +
				FormatDateTime("hh:mm:ss", elapsed_sec * 1.0 / (24 * 60 * 60));
			elapsed_str = s;
			remaining_str = "00:00:00:00";
			total_str = elapsed_str;
		}else {
			{
				int elapsed_sec = (int)((double)(Now() - start) * 24 * 60 * 60);
				UnicodeString s = IntToStr(elapsed_sec / (24 * 60 * 60));
				if (s.Length() < 2) {
					s = UnicodeString("0") + s;
				}
				s +=
					UnicodeString(":") +
					FormatDateTime("hh:mm:ss", elapsed_sec * 1.0 / (24 * 60 * 60));
				elapsed_str = s;
			}

			if (data_start_enabled && data_end_enabled && data) {
				const double time_per_data = ((double)(data_end - data_start) / data);
				TDateTime estimated_end_time = data_start + time_per_data * max_data;

				{
					int remaining_sec = estimated_end_time > Now() ? (int)((double)(estimated_end_time - Now()) * 24 * 60 * 60) : 0;
					UnicodeString s = IntToStr(remaining_sec / (24 * 60 * 60));
					if (s.Length() < 2) {
						s = UnicodeString("0") + s;
					}
					s +=
						UnicodeString(":") +
						FormatDateTime("hh:mm:ss", remaining_sec * 1.0 / (24 * 60 * 60));
					remaining_str = s;
				}

				{
					int total_sec = (int)((double)(estimated_end_time - start) * 24 * 60 * 60);
					UnicodeString s = IntToStr(total_sec / (24 * 60 * 60));
					if (s.Length() < 2) {
						s = UnicodeString("0") + s;
					}
					s +=
						UnicodeString(":") +
						FormatDateTime("hh:mm:ss", total_sec * 1.0 / (24 * 60 * 60));
					total_str = s;
				}
			}
		}
	}

	if (Ed_EvaluationTimeElapsed->Text != elapsed_str) {
		Ed_EvaluationTimeElapsed->Text = elapsed_str;
	}
	if (Ed_EvaluationTimeRemaining->Text != remaining_str) {
		Ed_EvaluationTimeRemaining->Text = remaining_str;
	}
	if (Ed_EvaluationTimeTotal->Text != total_str) {
		Ed_EvaluationTimeTotal->Text = total_str;
	}

	UnicodeString data_text =
		UnicodeString("(") +
		(max_data > 0 ? IntToStr((int)data) : UnicodeString("-")) +
		UnicodeString(" / ") +
		(max_data > 0 ? IntToStr((int)max_data) : UnicodeString("-")) +
		UnicodeString(")");

	if (max_data == 0) {
		max_data = 1;
	}
	if (progress_evaluation_max_ != max_data) {
		progress_evaluation_max_ = max_data;
	}
	if (progress_evaluation_current_ != data) {
		progress_evaluation_current_ = data;
	}
	if (La_EvaluationData->Text != data_text) {
		La_EvaluationData->Text = data_text;
	}
	delete format_settings;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_UndoClick(TObject *Sender)
{
	ClearEditBox();
	nn_document_.Undo(0);
	Bu_Network->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Bu_RedoClick(TObject *Sender)
{
	ClearEditBox();
	nn_document_.Redo(0);
	Bu_Network->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_UndoClick(TObject *Sender)
{
	Bu_UndoClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIN_InverseSelectionClick(TObject *Sender)
{
	nn_document_.InverseLayerSelection();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_CutClick(TObject *Sender)
{
	nn_document_.KeepForUndo(FString("Cut"));
	SetClipboardText(nn_document_.Cut());
	Bu_Network->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_CopyClick(TObject *Sender)
{
	SetClipboardText(nn_document_.Copy());
	Bu_Network->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_PasteClick(TObject *Sender)
{
	if (TPlatformServices::Current->SupportsPlatformService(__uuidof(IFMXExtendedClipboardService))) {
		_di_IFMXExtendedClipboardService ClipboardService = TPlatformServices::Current->GetPlatformService(__uuidof(IFMXExtendedClipboardService));
		if (ClipboardService->HasText()){//Memory leak
			UnicodeString clipboard_str = ClipboardService->GetText();
			if (clipboard_str.SubString(1, 16) == "[Network_Global]") {
				nn_document_.KeepForUndo(FString("Paste"));
				nn_document_.Paste(FString(AnsiString(clipboard_str).c_str()));
			}
		}
	}
	Bu_Network->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_TableYExit(TObject *Sender)
{
	Sc_TableY->Tag = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_NetworkYChange(TObject *Sender)
{
	if (Sc_NetworkY->Tag >= 0) {
		Sc_NetworkY->Tag = 1;
		network_current_top_ = network_top_ = (Sc_NetworkY->Value * nn_document_.GetNetworkViewSizeY()) / Sc_NetworkY->Max;
		PB_Network->Repaint();
		PB_NetworkOverview->Repaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_NetworkYExit(TObject *Sender)
{
	Sc_NetworkY->Tag = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_NetworkXChange(TObject *Sender)
{
	if (Sc_NetworkX->Tag >= 0) {
		Sc_NetworkX->Tag = 1;
		network_current_left_ = network_left_ = (Sc_NetworkX->Value * nn_document_.GetNetworkViewSizeX()) / Sc_NetworkY->Max;
		PB_Network->Repaint();
		PB_NetworkOverview->Repaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_NetworkXExit(TObject *Sender)
{
	Sc_NetworkX->Tag = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_TableXChange(TObject *Sender)
{
	const CSVDataset &dataset = GetCurrentTable();
	TableUIState &table_ui_state = GetCurrentTableUIState();
	const int dataset_num_column = dataset.GetNumColumn();
	if (Sc_TableX->Tag >= 0 && dataset_num_column) {
		Sc_TableX->Tag = 1;
		const double dataset_view_size = dataset_num_column * default_dataset_col_width_;
		table_ui_state.current_left = table_ui_state.left = (Sc_TableX->Value * dataset_view_size) / Sc_TableX->Max;
		PB_Table->Repaint();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_NetworkMouseWheel(TObject *Sender, TShiftState Shift,
		  int WheelDelta, bool &Handled)
{
	ClearEditBox();
	Bu_Network->SetFocus();
	network_focuse_index_ = 1;

	Sc_NetworkX->Tag = 0;
	Sc_NetworkY->Tag = 0;

	nn_document_.SetCurrentLayerID(-1);
	nn_document_.SetCurrentLinkID(-1);

	if (Shift.Contains(ssCtrl)) {
		//Zoom
		const double last_zoom = network_zoom_;
		network_zoom_ += WheelDelta / 120;
		network_left_ = network_last_x_ -
			(network_last_x_ - network_left_) / pow(2.0, (network_zoom_ - last_zoom) * 0.5);
		network_top_ = network_last_y_ -
			(network_last_y_ - network_top_) / pow(2.0, (network_zoom_ - last_zoom) * 0.5);
	}else {
		//Scroll
		if (Shift.Contains(ssShift)) {
			//Scroll X
			network_left_ -= WheelDelta / pow(2.0, network_zoom_ * 0.5);
		}else {
			//Scroll Y
			network_top_ -= WheelDelta / pow(2.0, network_zoom_ * 0.5);
		}
	}
	if (network_zoom_ > 6) {
		network_zoom_ = 6;
	}
	if (network_zoom_ < -8) {
		network_zoom_ = -8;
	}
	if (network_top_ > nn_document_.GetNetworkViewSizeY() - PB_Network->Height / pow(2.0, network_zoom_ * 0.5)) {
		network_top_ = nn_document_.GetNetworkViewSizeY() - PB_Network->Height / pow(2.0, network_zoom_ * 0.5);
	}
	if (network_top_ < 0.0) {
		network_top_ = 0.0;
	}
	if (network_left_ > nn_document_.GetNetworkViewSizeX() - PB_Network->Width / pow(2.0, network_zoom_ * 0.5)) {
		network_left_ = nn_document_.GetNetworkViewSizeX() - PB_Network->Width / pow(2.0, network_zoom_ * 0.5);
	}
	if (network_left_ < 0.0) {
		network_left_ = 0.0;
	}
	Handled = true;
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::MIT_OpenDataClick(TObject *Sender)
{
	const CSVDataset &dataset = GetCurrentTable();
	TableUIState &table_ui_state = GetCurrentTableUIState();
	const int dataset_num_data = dataset.GetNumData();
	const int dataset_num_column = dataset.GetNumColumn();
	if (table_ui_state.row_index >= 0 && table_ui_state.row_index < dataset_num_data &&
		table_ui_state.col_index >= 0 && table_ui_state.col_index < dataset_num_column) {
		UnicodeString data_text = dataset.GetValue(table_ui_state.row_index, table_ui_state.col_index).PChar();

		if (IsFile(data_text)) {
			UnicodeString file_name = GetDataFileName(
				ExtractFilePath(dataset.GetFileName().PChar()),
				data_text);
			if (file_name != "") {
				OpenFile(AnsiString(file_name).c_str());
			}
		}else if (&dataset == &nn_document_.GetResultCSV(0) || (&dataset == &nn_document_.GetResultCSV(2) && MIT_Sort->Enabled)){
			//Output result
			if (StrToFloatDef(data_text, NAN_VALUE) < NAN_VALUE_THRESHOLD){
				//Is value
				SortOutputResultByValue(table_ui_state.col_index);
			}
		}else if (&dataset == &nn_document_.GetResultCSV(1)){
			//Confusion Matrix
			FString label_tmp = dataset.GetRowName(table_ui_state.row_index);
			FString label = label_tmp.SplitBy("=");
			label = label.SplitBy(":");
			FString result_tmp = dataset.GetColumnName(table_ui_state.col_index);
			FString result = result_tmp.SplitBy("=");
			result = result.SplitBy(":");
			if (result.Pos("__")){
                result_tmp = result;
				result = result_tmp.SplitBy("__");
			}
			if (result == label + FString("'")){
				const int label_index = label_tmp.AsInt();
				const int result_index = result_tmp.AsInt();
				float binary_threshold = 0.5f;
				FString threshold_text = dataset.GetTopLeftCell();
				if (threshold_text.SubString(1,3) == "TH:") {
					binary_threshold = threshold_text.SubString(4, threshold_text.Length() - 3).AsFloat();
				}
				ErrorAnalysis(label, result, label_index, result_index, binary_threshold);
			}
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::SortOutputResultByValue(int col_index)
{
	const CSVDataset &dataset = GetCurrentTable();

	FListT<std::pair<float,int>> paired;
	for (int i = 0 ; i < dataset.GetNumData(); i++) {
		paired.Add(std::pair<float,int>(dataset.GetFloatValue(i, col_index), i));
	}
	paired.Sort();
	FListT<float> value;
	FListT<int> index;
	for (int i = 0 ; i < paired.GetSize(); i++) {
		value.Add(paired.GetAt(i).first);
		index.Add(paired.GetAt(i).second);
	}

/*
	FListT<float> value;
	FListT<int> index;
	for (int i = 0 ; i < dataset.GetNumData(); i++){
		value.Add(dataset.GetFloatValue(i, col_index));
		index.Add(i);
	}
	value.SortWith(index);
*/

	CSVDataset sorted_dataset;
	sorted_dataset.SetColumnNames(dataset.GetColumnNames());
	FStringList row_names;
	for (int id = 0 ; id < index.GetSize(); id++){
		const int original_index = index.GetAt(id);
		if (original_index < dataset.GetRowNames().GetSize()){
			row_names.Add(dataset.GetRowNames().GetAt(original_index));
		}else {
			row_names.Add(FString(original_index + 1));
		}
		sorted_dataset.SetData(id, dataset.GetData(original_index));
	}
	sorted_dataset.SetTopLeftCell(FString("Index"));
	sorted_dataset.SetRowNames(row_names);
	sorted_dataset.SetFileName(dataset.GetFileName());
	nn_document_.SetResultCSV(2, sorted_dataset);
	others_table_type_ = OthersTableType::Dataset;
	Co_EvaluationOthers->ItemIndex = -1;
	RB_EvaluationOthers->IsChecked = true;
	RB_EvaluationOutputResultChange(this);
}
//---------------------------------------------------------------------------
void TFo_Main::ErrorAnalysis(const FString &label, const FString &result, int label_index, int result_index, double binary_threshold)
{
	//Error Analysis
	//ShowMessage((label + FString("(") + label_tmp + FString(",") + result_tmp + FString(")")).PChar());

	const CSVDataset &output_result = nn_document_.GetResultCSV(0);
	FListT<int> label_indices, result_indices;
	CreateOutputIndexSeries(label, false, &label_indices);
	CreateOutputIndexSeries(result, true, &result_indices, binary_threshold);
	if (label_indices.GetSize() == result_indices.GetSize() && label_indices.GetSize() == output_result.GetNumData()){
		CSVDataset error_analysis;
		error_analysis.SetColumnNames(output_result.GetColumnNames());
		FStringList row_names;
		for (int id = 0 ; id < output_result.GetNumData() ; id++){
			if (label_indices.GetAt(id) == label_index && result_indices.GetAt(id) == result_index){
				error_analysis.SetData(row_names.GetSize(), output_result.GetData(id));
				row_names.Add(FString(id + 1));
			}
		}
		error_analysis.SetTopLeftCell(FString("Index"));
		error_analysis.SetRowNames(row_names);
		error_analysis.SetFileName(output_result.GetFileName());
		nn_document_.SetResultCSV(2, error_analysis);
		others_table_type_ = OthersTableType::Dataset;
		Co_EvaluationOthers->ItemIndex = -1;
		RB_EvaluationOthers->IsChecked = true;
		RB_EvaluationOutputResultChange(this);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Me_MonitorDblClick(TObject *Sender)
{
	FString line = FString(AnsiString(Me_Monitor->Lines->Strings[Me_Monitor->CaretPosition.Line]).c_str());
	while (line != ""){
		FString file_name = line.SplitBy('\"');
		if (file_name != "." && (FileExists(file_name.PChar()) || DirectoryExists(file_name.PChar()))) {
			OpenFileLocation(file_name.PChar());
			return;
		}
	}

	JumpToError(true);
}
//---------------------------------------------------------------------------
int TFo_Main::JumpToError(int edit)
{
	if (Me_Monitor->CaretPosition.Line >= Me_Monitor->Lines->Count || !network_overview_) {
		return false;
	}
	FString line = FString(AnsiString(Me_Monitor->Lines->Strings[Me_Monitor->CaretPosition.Line]).c_str());
	line.SplitBy("[Error ");
	if (line == "") {
		return false;
	}
	const int error_index = line.SplitBy("] ").AsInt() - 1;

	//Network Error
	network_overview_->ClearSelection();
	if (error_index >= 0 && error_index < current_errors_.GetSize()) {
		const NNError &error = current_errors_.GetAt(error_index);

		if (error.GetNetworkName() != "") {
			if (edit) {
				nn_document_.PrepareUndoInAdvance();
				FString network_name = nn_document_.GetName();
				nn_document_.SetActiveStructure(error.GetNetworkName());
				if (network_overview_structure_index_ >= 0){
					network_overview_structure_index_ = nn_document_.GetActiveStructureIndex();
				}
				if (network_name != nn_document_.GetName()) {
					nn_document_.KeepPreparedForUndo(FString("Change active network"));
				}

				nn_document_.ClearSelection();
			}
			int layer_id = network_overview_->GetLayerIDByName(error.GetLayerName());
			if (network_overview_->GetName() == error.GetNetworkName() && layer_id >= 0) {
				network_overview_->SetCurrentLayerID(layer_id);
				network_overview_->SetLayerSelected(layer_id, true, false);
				if (edit) {
					network_overview_->SetCurrentLayerID(layer_id);
					network_overview_->SetLayerSelected(layer_id, true, false);
				}
			}
			if (edit) {
				Bu_NetworkTabClick(this);
				if (layer_id >= 0 && error.GetPropertyName() != "" && error.GetPropertyName() != "Name") {
					const NNLayer *layer = network_overview_->GetLayerByID(layer_id);
					if (layer) {
						const int property_index = layer->GetPropertyIndexByName(error.GetPropertyName());
						network_overview_->SetCurrentPropertyIndex(property_index);
						ClearEditBox();
						Bu_Property->SetFocus();
						network_focuse_index_ = 2;
						PB_Property->Repaint();
						Application->ProcessMessages();
						EditProperty();//need property paint before edit property
						return true;
					}
				}
				Bu_Network->SetFocus();
				network_focuse_index_ = 1;
			}
			return true;
        }
	}


	//Dataset error
	if (line.Pos("Dataset") == 1){
		if (edit) {
			line.SplitBy('\"');
			const FString dataset_name = line.SplitBy('\"');
			const int dataset_index = nn_document_.GetDatasetIndexByName(dataset_name);
			if (dataset_index >= 0) {
				Bu_DatasetTabClick(this);
				Fr_DatasetList->SetItemIndex(dataset_index);
				DatasetItemChanged(dataset_index);
			}
		}
		return true;
	}

	//Configuration Error
	if (line.Pos("Batch size is larger than")){
		if (edit) {
			Bu_ConfigTabClick(this);
			Fr_ConfigList->SetItemIndex(0);
			Ed_BatchSize->SetFocus();
			Ed_BatchSize->SelectAll();
		}
		return true;
	}

	//Configuration Error (Optimizer)
	if (line.Pos("Optimizer") == 1){
		if (edit) {
			line.SplitBy('\"');
			const FString optimizer_name = line.SplitBy('\"');
			const int optimizer_index = nn_document_.GetOptimizerIndexByName(optimizer_name);
			if (optimizer_index >= 0) {
				Bu_ConfigTabClick(this);
				Fr_ConfigList->SetItemIndex(optimizer_index + 1);
				RefreshConfigTab(optimizer_index + 1);
				if (line.Pos("Network")) {
					Ed_OptimizerNetwork->SetFocus();
					Ed_OptimizerNetwork->SelectAll();
				}else if (line.Pos("Dataset")) {
					Ed_OptimizerDataset->SetFocus();
					Ed_OptimizerDataset->SelectAll();
				}
			}
		}
		return true;
	}

	//Configuration Error (Monitor)
	if (line.Pos("Monitor") == 1){
		if (edit) {
			line.SplitBy('\"');
			const FString monitor_name = line.SplitBy('\"');
			const int monitor_index = nn_document_.GetMonitorIndexByName(monitor_name);
			if (monitor_index >= 0) {
				Bu_ConfigTabClick(this);
				Fr_ConfigList->SetItemIndex(monitor_index + 1 + nn_document_.GetNumOptimizer());
				RefreshConfigTab(monitor_index + 1 + nn_document_.GetNumOptimizer());
				if (line.Pos("Network")) {
					Ed_MonitorNetwork->SetFocus();
					Ed_MonitorNetwork->SelectAll();
				}else if (line.Pos("Dataset")) {
					Ed_MonitorDataset->SetFocus();
					Ed_MonitorDataset->SelectAll();
				}
			}
		}
		return true;
	}

	//Configuration Error (Executor)
	if (line.Pos("Executor") == 1){
		if (edit) {
			line.SplitBy('\"');
			const FString executor_name = line.SplitBy('\"');
			const int executor_index = nn_document_.GetExecutorIndexByName(executor_name);
			if (executor_index >= 0) {
				Bu_ConfigTabClick(this);
				Fr_ConfigList->SetItemIndex(executor_index + 1 + nn_document_.GetNumOptimizer() + nn_document_.GetNumMonitor());
				RefreshConfigTab(executor_index + 1 + nn_document_.GetNumOptimizer() + nn_document_.GetNumMonitor());
				if (line.Pos("Network")) {
					Ed_ExecutorNetwork->SetFocus();
					Ed_ExecutorNetwork->SelectAll();
				}else if (line.Pos("Dataset")) {
					Ed_ExecutorDataset->SetFocus();
					Ed_ExecutorDataset->SelectAll();
				}
			}
		}
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::TC_DefaultChange(TObject *Sender)
{
	network_overview_ = &nn_document_;
	if (TC_Default->ActiveTab == TI_Browser){
		if (Fr_ProjectFiles->Visible){
			Fr_ProjectFiles->Bu_List->SetFocus();
		}else if (Fr_DatasetFiles->Visible){
			Fr_DatasetFiles->Bu_List->SetFocus();
		}
	}else if (TC_Default->ActiveTab == TI_Network){
		// EDIT Tab
		Bu_Network->SetFocus();
		Co_Training->Tag = 1;
		Co_Training->ItemIndex = 0;
		Co_Training->Tag = 0;
		UpdateInstanceList();
	}else if (TC_Default->ActiveTab == TI_Dataset){
		Lo_Table->Parent = TI_Dataset;
		Lo_CSVGraphType->Parent = Re_DatasetTop2;
        Lo_CSVGraphType->Position->X = Ch_DatasetImageNormalization->Position->X + Ch_DatasetImageNormalization->Width + Ch_DatasetImageNormalization->Margins->Left;
		Fr_DatasetFiles2->Visible = false;
		Bu_OpenDataset2->Visible = false;
		Bu_CreateDataset2->Visible = false;
		Re_DatasetTop2->Visible = true;
		Lo_Table->Visible = true;
		RefreshDatasetList();
		Bu_Table->SetFocus();
	}else if (TC_Default->ActiveTab == TI_Config){
		Fr_ConfigList->SetItemIndex(0);
		Fr_ConfigList->Bu_List->SetFocus();
		RefreshConfigList();
		RefreshConfigTab(0);
	}else if (TC_Default->ActiveTab == TI_Results || TC_Default->ActiveTab == TI_Evaluation) {
		RefreshResultsList(true);
		Fr_ResultsList->SetItemIndex(nn_document_.GetCurrentResultIndex());
		if (nn_document_.GetCurrentResultIndex() >= 0){
			network_overview_ = NULL;
			OpenResult();
			OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
		}
		if (TC_Default->ActiveTab == TI_Results) {
			if (RB_ResultsWeight->IsChecked){
				RB_ResultsLearningCurve->IsChecked = true;
			}
		}else if (TC_Default->ActiveTab == TI_Evaluation) {
			Lo_Table->Parent = Lo_EvaluationDataset;
			Lo_CSVGraphType->Position->X = Co_EvaluationOthers->Position->X + Co_EvaluationOthers->Width + Lo_CSVGraphType->Margins->Left;
			Lo_CSVGraphType->Parent = Re_EvaluationTop2;
			Lo_Table->Visible = true;
		}
		Co_HStatistics->Visible = false;
		Fr_ResultsList->Bu_List->SetFocus();
	}else if (TC_Default->ActiveTab == TI_Debug) {
		Me_Debug->SetFocus();
	}

	RefreshListSettings();
	Ti_AnimationTimer(Sender);
}
//---------------------------------------------------------------------------
int TFo_Main::OpenNetworkFromIni(NNStructures &networks, UnicodeString network_file_name)
{
    networks.Clear();
	networks.SetNNLayerComponents(nn_document_.GetNNLayerComponents());
	networks.SetNNStructureSearchPolicies(nn_document_.GetNNStructureSearchPolicies());
	if (FileExists(network_file_name + ".ini")) {
		FFastIni network_ini = FFastIni(AnsiString(network_file_name + ".ini").c_str());
		int result = NNDocument::IniToNetworks(network_ini, networks, false);
		return result;
	}
	return false;
}
//---------------------------------------------------------------------------
void TFo_Main::OpenResultsNNtxt(AnsiString result_network_file_name, NNDocument &nn_document, int finalize)
{
	FStringList error_str;
	((NNStructures*)&nn_document)->Clear();
	NntxtToDocument(result_network_file_name.c_str(), nn_document, error_str);//Parameter layers have been created independently at this moment
	if (finalize) for (int in = 0 ; in < nn_document.GetNumStructure() ; in++) {
		nn_document.SetActiveStructure(in);
		nn_document.RefreshDisplayValue();
		nn_document.EmbedParameters();// Embed parameter layers into side connector of neural network layers (affine, conv. etc.)
		nn_document.ArrangeLayers();
	}
}
//---------------------------------------------------------------------------
void TFo_Main::OpenResult(int result_index, int for_comparison)
{
	if (result_index < 0) {
		result_index = nn_document_.GetCurrentResultIndex();
	}
	monitor_csv_[for_comparison].Clear();
	result_name_[for_comparison] = "";
	Co_HStatistics->Visible = false;
	Co_HStatistics->Tag = 1;
	if (!for_comparison) {
		result_weight_.Clear();
		Co_HStatistics->Items->Clear();
	}
	if (result_index < 0) {
		if (!for_comparison) {
			result_network_.Clear();
			result_network_name_ = "";
			Me_Monitor->Lines->Clear();
			RefreshMonitorTimeFromLog();
		}
		Co_HStatistics->Tag = 0;
		return;
	}

	UnicodeString result_dir = GetResultDir(result_index);
	if (DirectoryExists(result_dir)) {
		RefreshOtherFilesInResultDir(result_index, FString(""));
		UnicodeString monitoring_report_file_name = result_dir + UnicodeString("\\monitoring_report.yml");
		if (FileExists(monitoring_report_file_name)) {
			MonitoringReportYAMLToCSV(AnsiString(monitoring_report_file_name).c_str(), monitor_csv_[for_comparison]);
		}

		if (!for_comparison) {
			struct stat stat_buf;
			if (stat(AnsiString(monitoring_report_file_name).c_str(), &stat_buf) == 0) {
				monitor_date_ = stat_buf.st_mtime;
			}
			monitor_update_request_ = false;
			if (TC_Default->ActiveTab == TI_Results || TC_Default->ActiveTab == TI_Evaluation) {
				// Open network configuration
				AnsiString network_file_name = result_dir + UnicodeString("\\network");
				if (result_network_name_ != nn_document_.GetResultName(result_index)){
					const int last_network_index = result_network_.GetActiveStructureIndex();
					//OpenNetworkFromIni(result_network_, network_file_name);
					result_network_.LoadFromFile(FString((network_file_name + ".ini").c_str()), FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()), false, false);

					// Open results.nntxt if it exists
					AnsiString result_network_file_name = result_dir + UnicodeString("\\results.nntxt");
					if (FileExists(result_network_file_name)) {
						OpenResultsNNtxt(result_network_file_name, result_network_);
                    }

					if (last_network_index >= 0 && last_network_index < result_network_.GetNumStructure()){
						result_network_.SetActiveStructure(last_network_index);
						if (network_overview_structure_index_ >= 0){
							network_overview_structure_index_ = last_network_index;
						}
					}
					result_network_name_ = nn_document_.GetResultName(result_index);
                    network_overview_update_count_ = 0;
                }
				network_overview_ = &result_network_;
			}

			log_file_name_ = result_dir + UnicodeString("\\log.txt");
			if (FileExists(log_file_name_)) {
				Me_Monitor->Lines->BeginUpdate();
				int opened = false;
				for (int it = 0 ; it < sdeep_training_thread_.GetSize() ; it++) {
					SDeepTrainingThread *thread = sdeep_training_thread_.GetAt(it);
					if (nn_document_.GetCurrentResultName() == thread->GetName()){
						Me_Monitor->Lines->Clear();
						Me_Monitor->Text = thread->GetLog().GetText().PChar();
						Me_Monitor->Lines->Add("");
						opened = true;
					}
				}
				if (!opened) {
					try{

						Me_Monitor->Lines->LoadFromFile(log_file_name_);
					}
					catch (...) {}
                }
				if (Visible && WindowState != TWindowState::wsMinimized && TC_Default->Visible && TC_Default->ActiveTab == TI_Results && Me_Monitor->Visible && Me_Monitor->Text != "") {
					try {
						Me_Monitor->GoToTextEnd();
					}
					catch (...) {}
				}
				Me_Monitor->Lines->EndUpdate();
			}else {
				Me_Monitor->Lines->Clear();
			}
			PB_MonitorPerformance->Repaint();

			if (result_index < nn_document_.GetResults().GetSize()){
				const NNTrainingResult &result = nn_document_.GetResult(result_index);
				const int num_statistics = result.GetNumStatistics();
				for (int is = 0 ; is < num_statistics ; is++){
					const FString &s = result.GetStatisticsLabel(is);
					Co_HStatistics->Items->Add(s.PChar());
					if (s == result_tradeoff_h_statistics_str_){
						Co_HStatistics->ItemIndex = is;
					}
				}
			}
			/*
			if (Co_HStatistics->ItemIndex < 0){
				result_tradeoff_h_statistics_str_ = FString("CostMultiplyAdd");
				Co_HStatistics->ItemIndex = Co_HStatistics->Items->IndexOf(result_tradeoff_h_statistics_str_.PChar());
				assert(Co_HStatistics->ItemIndex >= 0);
				nn_document_.SetParetoOptimalStatisticsLabel(result_tradeoff_h_statistics_str_);
			}
            */

			RefreshMonitorTimeFromLog();
			result_name_[0] = nn_document_.GetResultName(result_index);
		}else {
			result_name_[1] = nn_document_.GetResultName(result_index);
		}
	}

	Co_HStatistics->Tag = 0;
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshOtherFilesInResultDir(int result_index, FString active_file_name)
{
	if (active_file_name == "" && Co_EvaluationOthers->ItemIndex >= 0){

		active_file_name = FString(AnsiString(Co_EvaluationOthers->Items->Strings[Co_EvaluationOthers->ItemIndex]).c_str());
	}
	Co_EvaluationOthers->Items->BeginUpdate();
	Co_EvaluationOthers->Items->Clear();

	AnsiString result_dir = GetResultDir(result_index);
	FStringList result_files = EnumFiles(result_dir.c_str());

	for (int i = 0 ; i < result_files.GetSize() ; i++){
		const FString &file_name = result_files.GetAt(i);
		UnicodeString ext = ExtractFileExt(file_name.PChar()).LowerCase();
		if (ext == ".csv"){
			if (file_name == "output_result.csv" || file_name.SubString(1, 16) == "confusion_matrix"){
				continue;
			}
		}else if (ext == ".png"){
			if (file_name == "network.png"){
				continue;
			}
		}else {
			continue;
		}
		Co_EvaluationOthers->Items->Add(file_name.PChar());
		if (active_file_name == file_name){
			Co_EvaluationOthers->ItemIndex = Co_EvaluationOthers->Items->Count - 1;
		}
	}
    Co_EvaluationOthers->Items->Add("");// To fix the problem of incorrect size
	Co_EvaluationOthers->Items->EndUpdate();
}
//---------------------------------------------------------------------------
void TFo_Main::OpenEvaluationResult(int need_reload, int need_result)
{
	if (nn_document_.GetCurrentResultIndex() < 0) {
		nn_document_.ClearResultCSV(0);//Clear Evaluation Dataset
		nn_document_.ClearResultCSV(1);//Clear Confusion Matrix
		nn_document_.ClearResultCSV(2);//Clear Error Analysis
		Me_Evaluation->Lines->Clear();
		RefreshEvaluationTimeFromLog();
		if (TC_Default->ActiveTab == TI_Evaluation) {
			PB_Table->Repaint();
		}

		return;
	}

	UnicodeString result_dir = GetResultDir();
	if (DirectoryExists(result_dir)) {
		log_file_name_ = result_dir + UnicodeString("\\evaluation_log.txt");
		if (FileExists(log_file_name_)) {
				Me_Evaluation->Lines->BeginUpdate();
				int opened = false;
				if (sdeep_evaluation_thread_) if (nn_document_.GetCurrentResultName() == sdeep_evaluation_thread_->GetName()){
					Me_Evaluation->Lines->Clear();
					Me_Evaluation->Text = sdeep_evaluation_thread_->GetLog().GetText().PChar();
					Me_Evaluation->Lines->Add("");
					opened = true;
				}
				if (!opened) {
					try{

						Me_Evaluation->Lines->LoadFromFile(log_file_name_);
					}
					catch (...) {}
				}
				if (Visible && WindowState != TWindowState::wsMinimized && TC_Default->Visible && TC_Default->ActiveTab == TI_Evaluation && Me_Evaluation->Visible && Me_Evaluation->Text != "") {
					try {
						Me_Evaluation->GoToTextEnd();
					}
					catch (...) {}
				}
				Me_Evaluation->Lines->EndUpdate();
		}else {
			Me_Evaluation->Lines->Clear();
		}

		RefreshEvaluationTimeFromLog();

		if (GetEvaluating()) {
			if (Fr_ResultsList->GetItemIndex() >= 0 &&
				Fr_ResultsList->GetItemIndex() < nn_document_.GetResults().GetSize()) {
				if (sdeep_evaluation_thread_->GetName() == nn_document_.GetResult(Fr_ResultsList->GetItemIndex()).GetDir()){
					need_result = false;
				}
			}
		}
		if (need_result) {
			//Open output result dataset
			FString file_name = FString(AnsiString(result_dir + UnicodeString("\\output_result.csv")).c_str());
			if (need_reload || nn_document_.GetResultCSV(2).GetFileName() != file_name){
				if (FileExists(file_name.PChar())) {
					ShowAnimationIndicator(GetTranslatedText("Loading output result..."));
					nn_document_.Lock();
                    Ti_AnimationTimer(this);
					TThread::CreateAnonymousThread(
						new TDocumentIOFunc(&nn_document_, &settings_, file_name.PChar(), DocumentIOType::OpenResultCSV, 0, online_debug_mode_)
						)->Start();
					WaitForDocumentUnlock();
					HideAnimationIndicator();
				}else {
					nn_document_.ClearResultCSV(0);
				}
			}

			//Update modal name for confusion matrix
			const UnicodeString cm_modal_bak = Co_EvaluationConfusionMatrixModal->ItemIndex >= 0 ? Co_EvaluationConfusionMatrixModal->Items->Strings[Co_EvaluationConfusionMatrixModal->ItemIndex] : UnicodeString("");
			const CSVDataset &dataset = nn_document_.GetResultCSV(0);
			const FStringList &col_names = dataset.GetColumnNames();
			FStringList modal_names_for_cm;
			FStringList modal_name_pairs_for_cm;
			if (dataset.GetNumData()) {
				for (int ic = 0 ; ic < col_names.GetSize() ; ic++) {
					if (!IsFile(dataset.GetValue(0, ic).PChar())) {
						FString modal = col_names.GetAt(ic);
						modal = modal.SplitBy("__");
						modal = modal.SplitBy(':');
						if (modal_names_for_cm.IndexOf(modal) < 0) {
							FString last;
							for (int ic2 = ic + 1 ; ic2 < col_names.GetSize() ; ic2++) {
								FString modal2 = col_names.GetAt(ic2);
								modal2 = modal2.SplitBy("__");
								modal2 = modal2.SplitBy(':');
								if (modal2 != last && modal2.SubString(1, modal.Length() + 1) == modal + "'") {
									modal_names_for_cm.Add(modal);
									modal_name_pairs_for_cm.Add(modal + FString(" - ") + modal2);
								}
								last = modal2;
							}
						}
					}
				}
			}
			Co_EvaluationConfusionMatrixModal->Tag = 1;
			Co_EvaluationConfusionMatrixModal->Items->BeginUpdate();
			Co_EvaluationConfusionMatrixModal->Items->Clear();
			for (int i = 0 ; i < modal_name_pairs_for_cm.GetSize() ; i++) {
				Co_EvaluationConfusionMatrixModal->Items->Add(modal_name_pairs_for_cm.GetAt(i).PChar());
				if (cm_modal_bak == modal_name_pairs_for_cm.GetAt(i).PChar()) {
					Co_EvaluationConfusionMatrixModal->ItemIndex = i;
				}
			}
			if (modal_names_for_cm.GetSize() && Co_EvaluationConfusionMatrixModal->ItemIndex < 0) {
				Co_EvaluationConfusionMatrixModal->ItemIndex = 0;
			}
			Co_EvaluationConfusionMatrixModal->Items->EndUpdate();
			Co_EvaluationConfusionMatrixModal->Tag = 0;

			UpdateConfusionMatrix();
		}else {
			nn_document_.ClearResultCSV(0);//Clear Evaluation Result
			nn_document_.ClearResultCSV(1);//Clear Confusion Matrix
		}
		nn_document_.ClearResultCSV(2);//Clear Error Analysis
		if (RB_EvaluationOthers->IsChecked){
			RB_EvaluationOutputResult->IsChecked = true;
		}
	}
	if (TC_Default->ActiveTab == TI_Evaluation) {
		PB_Table->Repaint();
	}
}
//---------------------------------------------------------------------------
int TFo_Main::SearchVariableInDataset(const FString &variable_name, int &dim_size, int &col_start_index, int &num_category, int &by_index)
{
	const CSVDataset &dataset = nn_document_.GetResultCSV(0);
	const FStringList &col_names = dataset.GetColumnNames();

	dim_size = 0;
	col_start_index = -1;
	num_category = 0;
	by_index = false;

	double min = 0.0, max = 0.0;
	for (int ic = 0 ; ic < col_names.GetSize() && !animation_indicator_abort_ ; ic++) {
		FString v_name = col_names.GetAt(ic);
		v_name = v_name.SplitBy("__");
		v_name = v_name.SplitBy(':');
		if (v_name == variable_name) {
			dim_size++;
			if (col_start_index < 0) {
				col_start_index = ic;
			}
			dataset.GetColumnMinMax(ic, min, max);
		}

		CheckAnimationIndicator();
		Application->HandleMessage();
	}
	if (dim_size && col_start_index >= 0 && !animation_indicator_abort_) {
		FString cols, rows;
		if (dim_size == 1) {
			if (min >= 0.0 && max <= 10000){
				//label by index
				by_index = true;
				num_category = (int)max + 1;
			}else {
				return false;
			}
		}else {
			num_category = dim_size;
		}
		return true;
	}else {
		return false;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::CreateOutputIndexSeries(const FString &variable_name, int result, FListT<int> *indices, double binary_threshold)
{
	indices->Clear();

	const CSVDataset &dataset = nn_document_.GetResultCSV(0);

	int dim_size = 0;
	int col_start_index = -1;
	int num_category = 0;
    int by_index = false;
	if (SearchVariableInDataset(variable_name, dim_size, col_start_index, num_category, by_index)){
		for (int id = 0 ; id < dataset.GetNumData() && !animation_indicator_abort_ ; id++){
			int max_index = -1;
			if (by_index) {
				max_index =
					result ?
						(int)(dataset.GetFloatValue(id, col_start_index) + (1.0 - binary_threshold)) : // 0.5 for thrshold = 0.5
						dataset.GetIntValue(id, col_start_index);
			}else {
				double max_score = -(double)FLT_MAX;
				for (int ic = 0 ; ic < num_category ; ic++) {
					const double score = dataset.GetFloatValue(id, ic + col_start_index);
					if (score > max_score) {
						max_index = ic;
						max_score = score;
					}
				}
			}
			indices->Add(max_index);
			if (id % 100 == 0){
				CheckAnimationIndicator();
				Application->HandleMessage();
			}
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::UpdateConfusionMatrix(int force, double binary_threshold)
{
	UnicodeString result_dir = GetResultDir();
	const CSVDataset &dataset = nn_document_.GetResultCSV(0);
	const FStringList &col_names = dataset.GetColumnNames();

	//Update Confusion Matrix
	CSVDataset confusion_matrix;
	if (Co_EvaluationConfusionMatrixModal->ItemIndex >= 0) {
		int confusion_matrix_created = false;

		FString s = FString(Co_EvaluationConfusionMatrixModal->ItemIndex >= 0 ? AnsiString(Co_EvaluationConfusionMatrixModal->Items->Strings[Co_EvaluationConfusionMatrixModal->ItemIndex]).c_str() : "");
		FString cm_modal_name = s.SplitBy(" - ");
		FString cm_result_modal_name = s;
		FString confusion_matrix_file_name = FString(AnsiString(result_dir).c_str()) + FString("\\confusion_matrix_") + cm_modal_name + FString(".csv");
		int load_result = !force ? confusion_matrix.LoadFromFile(confusion_matrix_file_name.PChar()) : false;// cannot use read only mode because of different column num
		load_result &= confusion_matrix.GetNumData() >= 8 && (confusion_matrix.GetNumData() >= confusion_matrix.GetNumColumn() + 5);
		if (load_result && confusion_matrix.GetNumColumn() > 1){
			load_result &= confusion_matrix.GetColumnName(1) != "";
		}
		if (!FileExists(confusion_matrix_file_name.PChar()) || !load_result){
			confusion_matrix.Clear();

			ShowAnimationIndicator(GetTranslatedText("Creating confusion matrix..."), false, true);
			nn_document_.Lock();
			Ti_AnimationTimer(this);
			confusion_matrix_created = true;

			//Get indices of label and result
			FListT<int> label_index, result_index;
			CreateOutputIndexSeries(cm_modal_name, false, &label_index);
			CreateOutputIndexSeries(cm_result_modal_name, true, &result_index, binary_threshold);

			int dim_size = 0, result_dim_size = 0;
			int modal_start_index = -1, result_modal_start_index = -1;
			int by_index = false, result_by_index = false;
			int num_category = 0, result_num_category = 0;
			if (SearchVariableInDataset(cm_modal_name, dim_size, modal_start_index, num_category, by_index) &&
				SearchVariableInDataset(cm_result_modal_name, result_dim_size, result_modal_start_index, result_num_category, result_by_index) &&
				result_num_category <= num_category){

				FString cols, rows;
				if (dim_size == 1) {
					FString s = col_names.GetAt(modal_start_index);
                    FString col_name = s.SplitBy(";");

					for (int ic = 0 ; ic < num_category ; ic++) {
						if (ic > 0) {
							rows += FString(",");
						}
						rows += col_name + FString("=") + FString(ic);
						if (s != "") {
							rows += FString("(") + s.SplitBy(";") + FString(")");
						}
					}
				}else {
					for (int ic = 0 ; ic < num_category ; ic++) {
						if (ic > 0) {
							rows += FString(",");
						}
						rows += col_names.GetAt(modal_start_index + ic);
					}
				}
				if (result_dim_size == 1) {
					for (int ic = 0 ; ic < num_category ; ic++) {
						if (ic > 0) {
							cols += FString(",");
						}
						cols += col_names.GetAt(result_modal_start_index) + FString("=") + FString(ic);
					}
				}else {
					FString s = col_names.GetAt(modal_start_index);
                    s.SplitBy(";");

					for (int ic = 0 ; ic < num_category ; ic++) {
						if (ic > 0) {
							cols += FString(",");
						}
						if (result_modal_start_index + ic < col_names.GetSize()){
							cols += col_names.GetAt(result_modal_start_index + ic);
							if (s != "") {
								cols += FString("(") + s.SplitBy(";") + FString(")");
							}
						}else {
							cols += "-";
							animation_indicator_abort_ = true;//failed
						}
					}
				}
				if (fabs(binary_threshold - 0.5) >= DBL_EPSILON) {
					confusion_matrix.SetTopLeftCell(FString("TH:") + FString(binary_threshold));
				}
				confusion_matrix.SetColumnNames(cols + FString(",Recall"));
				confusion_matrix.SetRowNames(rows + FString(",Precision,F-Measures,,Accuracy,Avg.Precision,Avg.Recall,Avg.F-Measures"));

                //Count
				int *count = new int[num_category * num_category];
				memset(count, 0, sizeof(int) * num_category * num_category);
				if (label_index.GetSize() == dataset.GetNumData() && result_index.GetSize() == dataset.GetNumData()){
					for (int id = 0 ; id < dataset.GetNumData() && !animation_indicator_abort_ ; id++){
						const int max_index = label_index.GetAt(id);
						const int result_max_index = result_index.GetAt(id);
						if (max_index >= 0 && result_max_index >= 0 && max_index < num_category && result_max_index < num_category) {
							count[max_index * num_category + result_max_index]++;
						}else {
							//ShowMessage((FString(max_index) + FString(",") + FString(result_max_index) + FString(",") + FString(num_category)).PChar());
						}
					}
				}

				if (!animation_indicator_abort_){
					//Accuracy
					int sum_all = 0;
					int sum_correct = 0;
					for (int ic = 0 ; ic < num_category ; ic++) {
						for (int ic2 = 0 ; ic2 < num_category ; ic2++) {
							confusion_matrix.SetValue(ic, ic2, FString(count[ic * num_category + ic2]));
							sum_all += count[ic * num_category + ic2];
							if (ic == ic2) {
								sum_correct += count[ic * num_category + ic2];
							}
						}
					}
					//Recall
					double sum_recall = 0.0;
					for (int ic = 0 ; ic < num_category ; ic++) {
						int sum = 0;
						for (int ic2 = 0 ; ic2 < num_category ; ic2++) {
							sum += count[ic * num_category + ic2];
						}
						if (sum == 0) {
							sum = 1;
						}
						const double recall = count[ic * num_category + ic] * 1.0 / sum;
						confusion_matrix.SetValue(ic, num_category, FString(recall).SubString(1, 6));
						sum_recall += recall;
					}
					//Precision
					double sum_precision = 0.0;
					for (int ic2 = 0 ; ic2 < num_category ; ic2++) {
						int sum = 0;
						for (int ic = 0 ; ic < num_category ; ic++) {
							sum += count[ic * num_category + ic2];
						}
						if (sum == 0) {
							sum = 1;
						}
						const double precision = count[ic2 * num_category + ic2] * 1.0 / sum;
						confusion_matrix.SetValue(num_category, ic2, FString(precision).SubString(1, 6));
						sum_precision += precision;
					}
					//FMs
					double sum_fms = 0.0;
					for (int ic2 = 0 ; ic2 < num_category ; ic2++) {
						const double precision = confusion_matrix.GetFloatValue(num_category, ic2);
						const double recall = confusion_matrix.GetFloatValue(ic2, num_category);
						double fms = 0.0;
						if (precision + recall > 0.0) {
							fms = 2 * precision * recall / (precision + recall);
						}
						confusion_matrix.SetValue(num_category + 1, ic2, FString(fms).SubString(1, 6));
						sum_fms += fms;
					}

					//Accuracy
					if (sum_all == 0) {
						sum_all = 1;
					}
					confusion_matrix.SetValue(num_category + 3, 0, FString(sum_correct * 1.0 / sum_all).SubString(1, 6));

					//Average
					confusion_matrix.SetValue(num_category + 4, 0, FString(sum_precision / num_category).SubString(1, 6));
					confusion_matrix.SetValue(num_category + 5, 0, FString(sum_recall / num_category).SubString(1, 6));
					confusion_matrix.SetValue(num_category + 6, 0, FString(sum_fms / num_category).SubString(1, 6));

					confusion_matrix.SaveToFile(confusion_matrix_file_name.PChar());
				}
				delete[] count;
			}
		}else {//if (load_result)
			FString rows;
			for (int ir = 0 ; ir < confusion_matrix.GetNumData() ; ir++){
				if (ir > 0){
					rows += FString(",");
				}
				rows += confusion_matrix.GetValue(ir, 0);
			}
			confusion_matrix.SetTopLeftCell(confusion_matrix.GetColumnName(0));
			confusion_matrix.DeleteCol(0);
			confusion_matrix.SetRowNames(rows);
		}

		if (confusion_matrix_created){
			HideAnimationIndicator();
			nn_document_.Unlock();
		}
	}
	nn_document_.SetResultCSV(1, confusion_matrix);
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshDatasetList()
{
	if (TC_Left->ActiveTab != TI_DatasetLeft) {
		return;
	}

	Fr_DatasetList->SetMenu(PM_DatasetList);
	ListItems items;

	for (int i = 0 ; i < nn_document_.GetNumDataset() ; i++) {
		const NNDataset &dataset = nn_document_.GetDataset(i);
		ListItem &list_item = items.Add();
		list_item.SetRenamable(true);
		list_item.SetOriginalIndex(i);
		list_item.AddAttribute(FString("Name"), dataset.GetName());
		FString description =
			FString(AnsiString(ExtractFileName(dataset.GetFileName().PChar())).c_str()) +
			FString("\nNum Data = ") + FString(dataset.GetNumData()) +
			FString(", Num Column = ") + FString(dataset.GetNumColumn());
		description +=
			FString("\nShuffle = ") + FString(dataset.GetShuffle() ? "Yes" : "No") +
			FString(", Cache = ") + FString(dataset.GetEnableCache() ? "Yes" : "No") +
			FString(", Normalize = ") + FString(dataset.GetImageNormalization() ? "Yes" : "No");
		list_item.AddAttribute(FString("Description"), description);
		list_item.AddAttribute(FString("Main"), FString(nn_document_.GetMainDatasetIndex() == i ? "MAIN" : ""));
	}

	Fr_DatasetList->SetItems(items);
	if (Fr_DatasetList->GetItemIndex() < 0 && nn_document_.GetNumDataset()) {
		Fr_DatasetList->SetItemIndex(0);
	}
	Invalidate();
}
//---------------------------------------------------------------------------
void TFo_Main::DatasetItemChanged(int index)
{
	Bu_DatasetTabClick(this);//back to the dataset table from check consistency or dataset plugin results
	if (!Lo_Table->Visible){
        Bu_DatasetOpenClick(this);
	}
	dataset_internal_update_count_++;
}
//---------------------------------------------------------------------------
void TFo_Main::DatasetItemRename(int index, FString text)
{
	if (text == ""){
		return;
	}
	static const FString allowed_chars = FString("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-");
	for (int i = 1 ; i <= text.Length() ; i++){
		if (allowed_chars.Pos(text.SubString(i, 1)) <= 0){
			return;
		}
	}

	if (index >= 0 && index < nn_document_.GetNumDataset() && text != "") {
		FString name = text;
		int found = false;
		for (int i = 0; i < nn_document_.GetNumDataset() && !found ; i++) if (i != index){
			if (nn_document_.GetDataset(i).GetName() == name) {
				found = true;
			}
		}
		if (found) {
			return;
		}
		NNDataset &dataset = nn_document_.GetDataset_(index);
		if (dataset.GetName() != name) {
			dataset.SetName(name);
			nn_document_.UpdateConfig();
			RefreshDatasetList();
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshConfigList()
{
	if (TC_Left->ActiveTab != TI_ConfigLeft) {
		return;
	}

	Fr_ConfigList->SetMenu(PM_ConfigList);
	config_type_.Clear();
	ListItems items;

	//Global Config
	{
		config_type_.Add(ConfigType::Global);
		ListItem &list_item = items.Add();
		list_item.SetRenamable(false);
		list_item.SetOriginalIndex(0);
		list_item.AddAttribute(FString("Name"), FString("Global Config"));
		list_item.AddAttribute(
			FString("Description"),
			FString("Max Epoch = ") + FString(nn_document_.GetMaxEpoch()) +
			FString("\nBatch Size = ") + FString(nn_document_.GetBatchSize())
			);
	}

	//Optimizer
	for (int io = 0 ; io < nn_document_.GetNumOptimizer() ; io++) {
		config_type_.Add(ConfigType::Optimizer);
		const NNOptimizer &optimizer = nn_document_.GetOptimizer(io);
		ListItem &list_item = items.Add();
		list_item.SetRenamable(true);
		list_item.SetOriginalIndex(io);
		list_item.AddAttribute(FString("Name"), optimizer.GetOptimizerName());
		FString description =
			FString("Network = ") + optimizer.GetNetworkName() +
			FString("\nDataset = ") + optimizer.GetDatasetNames().GetText(',') +
			FString("\nUpdater = ") + optimizer.GetSolverName();
		/*
		for (int ip = 0 ; ip < optimizer.GetSolverParameterNum() ; ip++) {
			description +=
				FString(", ") +
				optimizer.GetSolverParameterName(ip) +
				FString(" = ") +
				FString(optimizer.GetSolverParameter(ip));
		}
		*/
		list_item.AddAttribute(
			FString("Description"),
			description
			);
		list_item.AddAttribute(
			FString("Type"),
			FString("OPTIMIZER")
			);
	}

	//Monitor
	for (int im = 0 ; im < nn_document_.GetNumMonitor() ; im++) {
		config_type_.Add(ConfigType::Monitor);
		const NNMonitor &monitor = nn_document_.GetMonitor(im);
		ListItem &list_item = items.Add();
		list_item.SetRenamable(false);
		list_item.SetOriginalIndex(im);
		list_item.AddAttribute(FString("Name"), monitor.GetMonitorName());
		FString description =
			FString("Network = ") + monitor.GetNetworkName() +
			FString("\nDataset = ") + monitor.GetDatasetNames().GetText(',');
		list_item.AddAttribute(
			FString("Description"),
			description
			);
		list_item.AddAttribute(
			FString("Type"),
			FString("MONITOR")
			);
	}

	//Executor
	for (int ie = 0 ; ie < nn_document_.GetNumExecutor() ; ie++) {
		config_type_.Add(ConfigType::Executor);
		const NNExecutor &executor = nn_document_.GetExecutor(ie);
		ListItem &list_item = items.Add();
		list_item.SetRenamable(true);
		list_item.SetOriginalIndex(ie);
		list_item.AddAttribute(FString("Name"), executor.GetExecutorName());
		FString description =
			FString("Network = ") + executor.GetNetworkName() +
			FString("\nDataset = ") + executor.GetDatasetNames().GetText(',');
		list_item.AddAttribute(
			FString("Description"),
			description
			);
		list_item.AddAttribute(
			FString("Type"),
			FString("EXECUTOR")
			);
	}

	Fr_ConfigList->SetItems(items);
	if (Fr_ConfigList->GetItemIndex() < 0) {
		Fr_ConfigList->SetItemIndex(0);
	}
	Invalidate();
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshConfigTab(int index)
{
	if (index < 0 || index >=config_type_.GetSize()) {
		return;
	}

	ConfigType config_type = config_type_.GetAt(index);
	switch (config_type) {
	case ConfigType::Global:
		SetActiveTab(TC_Config, TI_GlobalConfig);
		break;
	case ConfigType::Optimizer:
		SetActiveTab(TC_Config, TI_OptimizerConfig);
		TI_OptimizerConfig->Tag = Fr_ConfigList->GetItem(index).GetOriginalIndex();
		break;
	case ConfigType::Monitor:
		SetActiveTab(TC_Config, TI_MonitorConfig);
		TI_MonitorConfig->Tag = Fr_ConfigList->GetItem(index).GetOriginalIndex();
		break;
	case ConfigType::Executor:
		SetActiveTab(TC_Config, TI_ExecutorConfig);
		TI_ExecutorConfig->Tag = Fr_ConfigList->GetItem(index).GetOriginalIndex();
		break;
	default:
		;
	}
	Ti_AnimationTimer(this);
	Invalidate();
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshResultsList(int reset_position)
{
	if (TC_Left->ActiveTab != TI_TrainingLeft) {
		return;
	}
	const FListP<NNTrainingResult> &results = nn_document_.GetResults();

	// list_name_ = UnicodeString("Results (") + UnicodeString(FString(results.GetSize()).InsertThousandSeparator().PChar()) + UnicodeString(")");
	ListItems items;
	FStringList stat_names;

	for (int i = 0 ; i < results.GetSize() ; i++) {
		const NNTrainingResult &result = results.GetAt(i);

		ListItem &list_item = items.Add();
		list_item.SetOriginalIndex(i);
		list_item.AddAttribute(FString("Name"), result.GetDir());
		list_item.AddAttribute(FString("Comment"), result.GetComment());
		list_item.AddAttribute(FString("*PieGraph"), FString(result.GetLastEpoch()) + FString("/") + FString(result.GetMaxEpoch()));
		list_item.AddAttribute(FString("Location"), nn_document_.GetResultPath(FString("\\")) + result.GetDir() + FString("\\"));

		FString pareto_str;
		FString ongoing_str;
		FString scheduled_str;
		FString error_str;
		FString caption;
		FString content;
		FString evaluated_str;
		const int training = GetTraining(result.GetDir());
		if (training) {
			switch (nn_document_.GetSDeepEngineType()) {
			case SDeepEngineType::NNabla:
            case SDeepEngineType::Torch:
				ongoing_str += "TRAINING";
				break;
			default:
				ongoing_str += "PROCESSING";
			}
			list_item.AddAttribute(FString("Training"), FString("True"));
			list_item.AddAttribute(FString("Incomplete"), FString("True"));
		}else if (result.GetSchedule()) {
			scheduled_str += "SCHEDULED";
		}else if (!result.GetComplete()) {
			error_str += "INCOMPLETE";
			list_item.AddAttribute(FString("Incomplete"), FString("True"));
		}
		if (result.GetTraining() >= 0.0 && result.GetValidation() < NAN_VALUE_THRESHOLD) {
			caption += FString("Training\nValidation\n");
			content +=
				FString(AnsiString(FormatFloat("0.000000", result.GetTraining())).c_str()) + FString("\n") +
				FString(AnsiString(FormatFloat("0.000000", result.GetValidation())).c_str()) + FString("\n");
		}
		if (result.GetBestEpoch() > 0 && result.GetBestValidation() < NAN_VALUE_THRESHOLD) {
			caption += FString("Best Validation\n");
			content +=
				FString(AnsiString(FormatFloat("0.000000", result.GetBestValidation())).c_str()) +
				FString(" @ epoch ") +
				FString(result.GetBestEpoch()) +
				FString("\n");
		}
		const int statistics_index = result.GetStatisticsIndex(result_tradeoff_h_statistics_str_);
		if (statistics_index >= 0) if (result.GetStatisticsValue(statistics_index) >= 0.0) {
			caption += result_tradeoff_h_statistics_str_ + FString("\n");
			content += FString(result.GetStatisticsValue(statistics_index)).InsertThousandSeparator() + FString("\n");
		}
		if (ongoing_str == "" && error_str == "" && scheduled_str == "" && result.GetParetoOptimal()) {
			pareto_str = "PARETO OPTIMAL";
		}

		int evaluating = false;
		if (sdeep_evaluation_thread_) {
			evaluating = sdeep_evaluation_thread_->GetName() == result.GetDir();
		}
		if (evaluating) {
			ongoing_str += "EVALUATING";
			pareto_str = "";
		}
		if (result.GetEvaluated()){
			evaluated_str += "EVALUATED";
			list_item.AddAttribute(FString("*CheckMark"), FString("true"));
		}
		list_item.AddAttribute(FString("Pareto"), pareto_str);
		list_item.AddAttribute(FString("Ongoing"), ongoing_str);
		list_item.AddAttribute(FString("Scheduled"), scheduled_str);
		list_item.AddAttribute(FString("Error"), error_str);
		list_item.AddAttribute(FString("Caption"), caption);
		list_item.AddAttribute(FString("Content"), content);
		list_item.AddAttribute(FString("Evaluated"), evaluated_str);

		list_item.SetRenamable(!training && !evaluating);

		for (int is = 0 ; is < result.GetNumStatistics() ; is++){
			if (stat_names.IndexOf(result.GetStatisticsLabel(is)) < 0){
				stat_names.Add(result.GetStatisticsLabel(is));
			}
		}
	}
	Fr_ResultsList->SetItems(items);

	//Update tradeoff_csv_
	{
		tradeoff_csv_.Clear();
		FString col_names = FString("idx,dir,terr,verr,pareto,expm,exps,comment");
		for (int is = 0 ; is < stat_names.GetSize() ; is++){
			col_names += FString(",") + stat_names.GetAt(is);
		}
		tradeoff_csv_.SetColumnNames(col_names);
		const FListP<NNTrainingResult> &results = nn_document_.GetResults();
		for (int ir = 0 ; ir < results.GetSize() ; ir++) {
			const NNTrainingResult &result = results.GetAt(ir);
			tradeoff_csv_.SetValue(ir, 0, FString(ir));
			tradeoff_csv_.SetValue(ir, 1, FString(result.GetDir()));
			if (result.GetTraining() < NAN_VALUE_THRESHOLD) {
				tradeoff_csv_.SetValue(ir, 2, FString(AnsiString(FormatFloat("0.000000", result.GetTraining())).c_str()));
			}
			if (result.GetValidation() < NAN_VALUE_THRESHOLD) {
				tradeoff_csv_.SetValue(ir, 3, FString(AnsiString(FormatFloat("0.000000", result.GetBestValidation())).c_str()));
			}
			tradeoff_csv_.SetValue(ir, 4, FString(result.GetParetoOptimal()));

			double log_myu = 0.0, log_sigma = 0.0;
			result.GetStructureSearchExpectiation(log_myu, log_sigma);
			if (log_sigma > 0.0) {
				tradeoff_csv_.SetValue(ir, 5, FString(log_myu));
				tradeoff_csv_.SetValue(ir, 6, FString(log_sigma));
			}
			for (int is = 0 ; is < result.GetNumStatistics() ; is++){
				if (result.GetStatisticsValue(is) > 0){
					const int index = stat_names.IndexOf(result.GetStatisticsLabel(is));
					tradeoff_csv_.SetValue(ir, 8 + index, FString(result.GetStatisticsValue(is)));
				}
			}
			tradeoff_csv_.SetValue(ir, 7, result.GetComment());
		}
	}

	if (reset_position) {
		Fr_ResultsList->SetItemIndex(nn_document_.GetCurrentResultIndex());
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ResultsListDblClick(int index)
{
	MIL_OpenFileLocationClick(this);
}
//---------------------------------------------------------------------------
void TFo_Main::ResultsListDelete(int index)
{
	Ti_AnimationTimer(this);
	if (MILR_DeleteFromDisk->Enabled){
		MILR_DeleteFromDiskClick(this);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_DeleteFromDiskClick(TObject *Sender)
{
	if (Fr_ResultsList->GetItemIndex() >= 0 && Fr_ResultsList->GetItemIndex() < Fr_ResultsList->GetItems().GetSize()) {
		const ListItem &item = Fr_ResultsList->GetItem(Fr_ResultsList->GetItemIndex());
		FString name =
			item.GetAttribute(FString("Name"));
		FString file_location =
			item.GetAttribute(FString("Location"));
		int result = MessageDlg(
			/*
			UnicodeString("Delete th\"") +
			UnicodeString(file_location.PChar()) +
			UnicodeString("\" from disk.\n") +
			*/
			UnicodeString(
				StringReplace(GetTranslatedText("Do you really want to delete (file) from disk?"),
				"(file)", UnicodeString("\"") + UnicodeString(name.PChar()) + UnicodeString("\""), TReplaceFlags() << rfReplaceAll)),
			TMsgDlgType::mtWarning, mbOKCancel, 0);
		if (result == mrOk) {
			DeleteDir(file_location.SubString(1, file_location.Length() - 1).PChar());//delete last \\

			nn_document_.RefreshResults(FString("\\"));
			Fr_ResultsList->SetItemIndex(nn_document_.GetCurrentResultIndex());
			RefreshResultsList(false);
			OpenResult();
			OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ch_StructureSearchEnabledChange(TObject *Sender)
{
	if (nn_document_.GetStructureSearchEnabled() != Ch_StructureSearchEnabled->IsChecked) {
		nn_document_.SetStructureSearchEnabled(Ch_StructureSearchEnabled->IsChecked);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_DeleteAllIncompleteResultsFromDiskClick(TObject *Sender)
{
	int result = MessageDlg(
		GetTranslatedText("Do you really want to delete all imcomplete results from disk?"),
		TMsgDlgType::mtWarning, mbOKCancel, 0);
	if (result == mrOk) {
		ShowAnimationIndicator(GetTranslatedText("Delete all incomplete results..."), true);
		for (int i = 0 ; i < Fr_ResultsList->GetItems().GetSize() ; i++) {
			if (Fr_ResultsList->GetItem(i).GetAttribute(FString("Incomplete")) == "True" &&
				Fr_ResultsList->GetItem(i).GetAttribute(FString("Training")) != "True") {
				FString file_location =
					Fr_ResultsList->GetItem(i).GetAttribute(FString("Location"));
				file_location = file_location.SubString(1, file_location.Length() - 1).PChar();//delete last os.sep
				AnimationIndicatorLog(UnicodeString("Deleting ... ") + ExtractFileName(file_location.PChar()));
				DeleteDir(file_location.PChar());
			}
		}

		nn_document_.RefreshResults(FString("\\"));
		Fr_ResultsList->SetItemIndex(nn_document_.GetCurrentResultIndex());
		RefreshResultsList(false);
		OpenResult();
		OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
		HideAnimationIndicator();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_OpenLearningCurveForComparisonClick(TObject *Sender)
{
	OpenResult(-1, true);
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MILR_ClearLearningCurveForComparisonClick(TObject *Sender)
{
	monitor_csv_[1].Clear();
	result_name_[1] = "";
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::MIN_SaveNetworkBitmapAsClick(TObject *Sender)
{
	if (!SD_Image->Execute()) {
		return;
	}
	SaveNetworkBitmapToFile(nn_document_, SD_Image->FileName);
}
//---------------------------------------------------------------------------
void TFo_Main::SaveNetworkBitmapToFile(NNDocument &nn_document, UnicodeString file_name)
{
	static const int max_size = 3200;
	double zoom =
		nn_document.GetNetworkViewSizeX() > nn_document.GetNetworkViewSizeY() ?
		max_size * 1.0 / nn_document.GetNetworkViewSizeX() :
		max_size * 1.0 / nn_document.GetNetworkViewSizeY();
	if (zoom > 1.0) {
		zoom = 1.0;
	}
	TBitmap *BMP = new TBitmap(nn_document.GetNetworkViewSizeX() * zoom, nn_document.GetNetworkViewSizeY() * zoom);
	TCanvas *C = BMP->Canvas;
	C->BeginScene();
	C->Clear(0xffffffff);
	NNGraphMapper nn_graph_mapper;
	nn_graph_mapper.Update(nn_document, true);
	nn_graph_mapper.Animation(1.0);
	DrawNetworkBitmap(C, BMP->Width, BMP->Height, nn_graph_mapper, 0.0, 0.0, zoom);
	C->EndScene();
	BMP->SaveToFile(file_name);
	delete BMP;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkOverviewPaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_NetworkOverview->Canvas;
	C->BeginScene();
	C->SetMatrix(PB_NetworkOverview->AbsoluteMatrix);
	C->Clear(SDEEP_CONSOLE_COLOR_LIGHTEST);

	DrawNetworkBitmap(
		C,
		PB_NetworkOverview->Width, PB_NetworkOverview->Height,
		result_graph_mapper_,
		0.0, 0.0,
		network_overview_current_zoom_);

	if (TC_Default->ActiveTab == TI_Network){
		TRectF rect = TRectF(
			network_current_left_ * network_overview_current_zoom_,
			network_current_top_ * network_overview_current_zoom_,
			(network_current_left_ + PB_Network->Width * pow(2.0, -network_current_zoom_ * 0.5)) * network_overview_current_zoom_,
			(network_current_top_ + PB_Network->Height * pow(2.0, -network_current_zoom_ * 0.5)) * network_overview_current_zoom_);

		if (fabs(rect.Left) >= 2.0 ||
			fabs(rect.Top) >= 2.0 ||
			rect.Right < PB_NetworkOverview->Width ||
			rect.Bottom < PB_NetworkOverview->Height){
			const float cover_ratio =
				(rect.Width() > PB_NetworkOverview->Width ? 1.0f : rect.Width() / PB_NetworkOverview->Width) *
				(rect.Height() > PB_NetworkOverview->Height ? 1.0f : rect.Height() / PB_NetworkOverview->Height);
			const float alpha = cover_ratio < 1.0f ? 0.125 * (1.0 - cover_ratio) : 0.0f;
			C->Fill->Color = (((int)(alpha * 255)) << 24) | (SDEEP_CONSOLE_COLOR_BRAND & 0xffffff);
			C->Fill->Kind = TBrushKind::Solid;
			const float alpha2 = cover_ratio < 1.0f ? 1.0f * (1.0 - cover_ratio) : 0.0f;
			C->Stroke->Color = (((int)(alpha2 * 255)) << 24) | (SDEEP_CONSOLE_COLOR_BRAND & 0xffffff);
			C->Stroke->Kind = TBrushKind::Solid;

			C->FillRect(rect, 0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			C->DrawRect(rect, 0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
		}
	}

	if (network_overview_ &&
		Bu_Overview->IsFocused &&
		network_overview_current_zoom_ < 1.0 &&
        network_overview_last_x_ >= 0 && network_overview_last_y_ >= 0 &&
		network_overview_last_x_ < network_overview_->GetNetworkViewSizeX() && network_overview_last_y_ < network_overview_->GetNetworkViewSizeY()) {
		const int w = 360, h = 160;
		TBitmap *BMP = new TBitmap(w, h);
		TCanvas *BMPC = BMP->Canvas;
		BMPC->BeginScene();
		BMPC->Clear(0xffffffff);
		DrawNetworkBitmap(BMPC, BMP->Width, BMP->Height, result_graph_mapper_, network_overview_last_x_ - w / 2, network_overview_last_y_ - h / 2, 1.0);
		BMPC->EndScene();

		const int x = network_overview_last_x_ * network_overview_current_zoom_;
		const int y = network_overview_last_y_ * network_overview_current_zoom_;
		C->DrawBitmap(
			BMP,
			RectF(0, 0, w, h),
			RectF(x - w/2, y - h/2, x + w/2, y + h/2),
			1.0f,
			false);
		delete BMP;

		const TRectF rect = RectF(x - w/2, y - h/2, x + w/2, y + h/2);
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_LIGHTFG2;
		C->Stroke->Thickness = 0.5;
		C->DrawRect(rect, 0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
	}

	C->EndScene();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIRN_OpenInNetworkTabClick(TObject *Sender)
{
	if (OpenNetworkFromResult(false)) {
		Bu_NetworkTabClick(this);
	}
}

//---------------------------------------------------------------------------
void __fastcall TFo_Main::Me_MonitorClick(TObject *Sender)
{
	JumpToError(false);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Me_MonitorMouseWheel(TObject *Sender, TShiftState Shift,
		  int WheelDelta, bool &Handled)
{
	JumpToError(false);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Me_MonitorKeyUp(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	JumpToError(false);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::FA_HintFinish(TObject *Sender)
{
	if (FA_Hint->Inverse) {
		Pa_Hint->Visible = false;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_DatasetFileNameDblClick(TObject *Sender)
{
	if (FileExists(Ed_DatasetFileName->Text)) {
		OpenFile(AnsiString(Ed_DatasetFileName->Text).c_str());
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkOverviewDblClick(TObject *Sender)
{
	if (Bu_NetworkTab->Visible && network_overview_){
		if (network_overview_->GetNumLayer() && (TC_Default->ActiveTab == TI_Results || TC_Default->ActiveTab == TI_Evaluation)) {
			if (network_overview_->GetCurrentLayerID() >= 0 && TC_Default->ActiveTab == TI_Results) {
				MIRN_PlotWeightClick(Sender);
				return;
			}
			if (nn_document_.GetCurrentResultIndex() >= 0) {
				int result = MessageDlg(GetTranslatedText("Open this network to edit?"), TMsgDlgType::mtConfirmation, mbOKCancel, 0);
				if (result == mrOk) {
					MIRN_OpenInNetworkTabClick(Sender);
				}
				return;
			}
		}
		Bu_NetworkTabClick(this);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_FullScreenClick(TObject *Sender)
{
	WindowState = TWindowState::wsNormal;
	FullScreen = !FullScreen;
	FormResize(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_MonitorClick(TObject *Sender)
{
	/*
	training_result_graph_mode_ =
		training_result_graph_mode_ == TrainingResultGraphMode::LearningCurve ?
			TrainingResultGraphMode::TradeOffAnalysis :
			TrainingResultGraphMode::LearningCurve;
	PB_Monitor->Repaint();
	*/
}
//---------------------------------------------------------------------------

FString DatasetPosStr(FString dataset_name, int col, int row)
{
	return
        FString("dataset = ") +
		dataset_name +
		FString(", col = ") +
		FString(col) +
		(row >= 0 ?
			FString(", row = ") +
			FString(row) :
			FString(""));
}
//---------------------------------------------------------------------------
int TFo_Main::GetDatasetVariable(const NNDataset &dataset, FStringList &variable_names, FListP<FListT<int> > &sizes, FListT<int> &types, FStringList &errors)
{
	int num_error = errors.GetSize();
	if (dataset.GetNumData()){
		//Column loop
		for (int ic = 0 ; ic < dataset.GetNumColumn() ; ic ++) {
			FString header = dataset.GetColumnName(ic);
			FString s = header;
			s = s.SplitBy(':');
			FString variable_name = s.SplitBy("__");
			FString dim_index = s;
			if (variable_name == "") {
				errors.Add(
					FString("Empty variable name at ") +
					DatasetPosStr(dataset.GetName(), ic, 0));
			}

			int variable_index = variable_names.IndexOf(variable_name);
			if (variable_index < 0) {
				//Set Training variable_names
				variable_index = variable_names.GetSize();
				variable_names.Add(variable_name);
			}

			int type = 0;
			FListT<int> size;
			GetDataSizeAndType(dataset, 0, ic, size, type, errors);
			if (dim_index != ""){
				if (type >= 2){
					size.Insert(0, 1);
				}
				type++;
			}

			if (variable_index < sizes.GetSize()){
				sizes.GetAt(variable_index).SetAt(0, sizes.GetAt(variable_index).GetAt(0) + 1);
			}else {
				sizes.Add(size);
				types.Add(type);
			}

		}
	}
	return errors.GetSize() == num_error;
}
//---------------------------------------------------------------------------
int TFo_Main::CheckDatasetConsistencyWithNetwork(const NNDataset &dataset, const NNStructure &structure, FStringList &errors)
{
	FStringList variable_names;
	FListP<FListT<int> > sizes;
	FStringList sizestrs;
	FListT<int> types;
	GetDatasetVariable(dataset, variable_names, sizes, types, errors);

	for (int iv = 0 ; iv < sizes.GetSize() ; iv++){
        const FListT<int> &size_array = sizes.GetAt(iv);
		FString size;
		for (int id = 0 ; id < size_array.GetSize() ; id++){
			if (id > 0){
				size += FString(", ");
			}
			if (size_array.GetAt(id) == 0){
				size += FString("c");
			}else if (size_array.GetAt(id) == -1){
				size += FString("?");
			}else {
				size += FString(size_array.GetAt(id));
			}
		}
		sizestrs.Add(size);
	}

	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);
		const FString variable_name = layer.GetDisplayPropertyValue(FString("Dataset"));
		int variable_index = variable_names.IndexOf(variable_name);
		if (variable_name != "" && variable_index >= 0) {
			const FString size_property =
				layer.GetPropertyIndexByName("Input") >= 0 ?
				FString("Input") :
				FString("Size");
			FString size_str =
				layer.GetDisplayPropertyValue(size_property).ReplaceStr(",", ", ");
			FString dataset_size = sizestrs.GetAt(variable_index);

			if (size_str != "" && dataset_size != "?") {
				const int c_pos = dataset_size.Pos("c");
				if (c_pos && size_str.Length() >= c_pos) {
					size_str =
						size_str.SubString(1, c_pos - 1) +
						FString("c") +
						size_str.SubString(c_pos + 1, size_str.Length() - c_pos);
				}
				if (size_str != dataset_size) {
					errors.Add(
						FString("Different data size of \"") +
						variable_name +
						FString("\" between dataset \"") +
						dataset.GetName() +
						FString("\" and network \"") +
                        structure.GetName() +
						FString("\"")
						);
					errors.Add(
						FString("    Dataset = ") +
						sizestrs.GetAt(variable_index) +
						FString(", Network (") +
						structure.GetName() +
						FString("->") +
						layer.GetDisplayPropertyValue(FString("Name")) +
						FString("->") +
						size_property +
						FString(") = ") +
						size_str
						);
				}
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------------
int TFo_Main::CheckDataSizeErrorSub(FString network_name, const FStringList &dataset_names)
{
	FStringList errors;

	if (network_name == "MainValidation" || network_name == "MainRuntime"){
		network_name = "Main";
	}
	int structure_index = nn_document_.GetStructureIndexByName(network_name);
	if (structure_index >= 0){
		for (int id = 0 ; id < dataset_names.GetSize() ; id++){
			int dataset_index = nn_document_.GetDatasetIndexByName(dataset_names.GetAt(id));
			if (dataset_index >= 0){
				CheckDatasetConsistencyWithNetwork(
					nn_document_.GetDataset(dataset_index),
					nn_document_.GetStructure(structure_index),
					errors);
			}
		}
	}

	for (int ie = 0 ; ie < errors.GetSize() ; ie++){
		NNError &new_error = nn_document_.AddError();
		new_error.SetNetworkName(network_name);
		new_error.SetString(errors.GetAt(ie));
	}

	return errors.GetSize() == 0;
}
//---------------------------------------------------------------------------
int TFo_Main::CheckDataSizeError()
{
	// Optimizer
	for (int io = 0 ; io < nn_document_.GetNumOptimizer() ; io++){
		const NNOptimizer &optimizer = nn_document_.GetOptimizer(io);
		CheckDataSizeErrorSub(optimizer.GetNetworkName(), optimizer.GetDatasetNames());
	}
	// Monitor
	for (int im = 0 ; im < nn_document_.GetNumMonitor() ; im++){
		const NNMonitor &monitor = nn_document_.GetMonitor(im);
		CheckDataSizeErrorSub(monitor.GetNetworkName(), monitor.GetDatasetNames());
	}
	// Executor
	for (int ie = 0 ; ie < nn_document_.GetNumExecutor() ; ie++){
		const NNExecutor &executor = nn_document_.GetExecutor(ie);
		CheckDataSizeErrorSub(executor.GetNetworkName(), executor.GetDatasetNames());
	}
	return true;
}
//---------------------------------------------------------------------------
int TFo_Main::GetDataSizeAndType(const NNDataset &dataset, int row_index, int col_index, FListT<int> &size, int &type, FStringList &errors)
{
	int num_error = errors.GetSize();

	const FString &cell_str = dataset.GetValue(row_index, col_index);
	type = 0;//Value Type
	if (IsImageFile(cell_str.PChar())) {
		type = 2;//Image Type
		ImageCache image;
		image.SetFileName(cell_str.PChar());
		image.SetDatasetPath(AnsiString(ExtractFilePath(dataset.GetFileName().PChar())).c_str());
		image.SetRequest(true);
		image.LoadImage();
		if (!image.GetEnabled()) {
			errors.Add(
				FString("Failed to load image at ") +
				DatasetPosStr(dataset.GetName(), col_index, row_index + 1));
			errors.Add(
				FString("    File = \"") +
				dataset.GetFileName() +
				FString("\""));
		}else {
			size.Add(0);
			TBitmap *bmp = (TBitmap*)image.GetImage();
			size.Add(bmp->Height);
			size.Add(bmp->Width);
		}
	}else if (IsCSVFile(cell_str.PChar()) || IsWavFile(cell_str.PChar())) {
		type = 4;//CSV Type
		CSVCache csv;
		csv.SetFileName(cell_str.PChar());
		csv.SetDatasetPath(AnsiString(ExtractFilePath(dataset.GetFileName().PChar())).c_str());
		csv.SetCSVCacheFileType(IsWavFile(cell_str.PChar()) ? CSVCacheFileType::Wav :  CSVCacheFileType::CSV);
		csv.SetRequest(true);
		csv.LoadCSV();
		if (!csv.GetEnabled()) {
			errors.Add(
				FString("Failed to load CSV at ") +
				DatasetPosStr(dataset.GetName(), col_index, row_index + 1));
			errors.Add(
				FString("    File = \"") +
				dataset.GetFileName() +
				FString("\""));
		}else {
			size.Add(csv.GetCSV()->GetNumRow());
			size.Add(csv.GetCSV()->GetNumCol());
		}
	}else if (IsUnknownSizeFile(cell_str.PChar())){
    	size.Add(-1);
    }else {
		size.Add(1);
	}
	return errors.GetSize() == num_error;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIT_CheckConsistencyClick(TObject *Sender)
{
	Log(FString("check consistency"));
	ShowAnimationIndicator(GetTranslatedText("Checking dataset consistency..."), true, true);
	nn_document_.Lock();

	FStringList messages;
	FStringList errors;

	/*
	if (nn_document_.GetDataset(0).GetNumColumn() && nn_document_.GetDataset(1).GetNumColumn()) {
		if (nn_document_.GetDataset(0).GetNumColumn() != nn_document_.GetDataset(1).GetNumColumn()) {
			errors.Add(FString("Number of columns are different between training and validation dataset"));
			errors.Add(
				FString("    Training = ") + FString(nn_document_.GetDataset(0).GetNumColumn()) +
				FString(", Validation = ") + FString(nn_document_.GetDataset(1).GetNumColumn()));
		}
	}
	messages.Add(FString("Num Column : ") + FString(nn_document_.GetDataset(0).GetNumColumn()));
	*/

	static const FString data_type_str[6] = {
	FString("Value"),
	FString("Vector"),
	FString("Image"),
	FString("Images"),
	FString("CSV"),
	FString("CSVs"),
	};

	FStringList modal_names;
	FStringList sizes;
	FListT<int> types;
	unsigned int last = timeGetTime();
	if (errors.GetSize() == 0) {
		//dataset loop
		for (int id = 0 ; id < nn_document_.GetNumDataset() && !animation_indicator_abort_; id ++) {
			const NNDataset &dataset = nn_document_.GetDataset(id);
			messages.Add(FString("Num Data (") + dataset.GetName() + FString(") : ") + FString(dataset.GetNumData()));


			//Column loop
			for (int ic = 0 ; ic < dataset.GetNumColumn() && !animation_indicator_abort_ ; ic ++) {
				FString modal_name = dataset.GetColumnName(ic);
				if (modal_name.SubString(1, 1) != "#") {//not comment col
					FString modal_name_only = modal_name;
					modal_name_only = modal_name_only.SplitBy(':');
					modal_name_only = modal_name_only.SplitBy("__");
					if (modal_name_only == "") {
						errors.Add(
							FString("Empty variable name at ") +
							DatasetPosStr(dataset.GetName(), ic, 0));
					}

					int modal_index = modal_names.IndexOf(modal_name);
					if (modal_index < 0) {
						//Set Training modal_names
						modal_index = modal_names.GetSize();
						modal_names.Add(modal_name);
					}

					AnimationIndicatorLog(UnicodeString((FString("Checking ") + dataset.GetName() + FString("->") + modal_name + FString(" ...")).PChar()));

					//Row(Data) loop
					for (int ir = 0 ; ir < dataset.GetNumData() && !animation_indicator_abort_ ; ir ++) {
						int type = 0;
						FListT<int> size_array;
						GetDataSizeAndType(dataset, ir, ic, size_array, type, errors);
						FString size;
						for (int id = 0 ; id < size_array.GetSize() ; id++){
							if (id > 0){
								size += FString(", ");
							}
							if (size_array.GetAt(id) == 0){
								size += FString("c");
							}else if (size_array.GetAt(id) == -1){
								size += FString("?");
							}else {
								size += FString(size_array.GetAt(id));
							}
						}

						//Check type
						int type_consistent = true;
						if (types.GetSize() <= modal_index) {
							//Set Type of this column with type of 2nd row
							types.Add(type);
						}else if (types.GetAt(modal_index) != type) {
							type_consistent = false;
							errors.Add(
								FString("Inconsistent data type at ") +
								DatasetPosStr(dataset.GetName(), ic, ir + 1));
							errors.Add(
								FString("    Expected = ") +
								data_type_str[types.GetAt(modal_index)] +
								FString(", Actual = ") +
								data_type_str[type]);
							types.SetAt(modal_index, type);
						}

						//Check size
						if (type_consistent) {
							if (sizes.GetSize() <= modal_index) {
								//Set Size of this column with size of 2nd row
								sizes.Add(size);
							}else if (sizes.GetAt(modal_index) != size) {
								errors.Add(
									FString("Inconsistent data size at ") +
									DatasetPosStr(dataset.GetName(), ic, ir + 1));
								errors.Add(
									FString("    Expected = ") +
									sizes.GetAt(modal_index) +
									FString(", Actual = ") +
									size);
								sizes.GetAt(modal_index) = size;
							}
						}
						if (timeGetTime() > last + 10000){
							AnimationIndicatorLog(
								UnicodeString((
										FString("(") + FString(ir) + FString(" / ") + FString(dataset.GetNumData()) + FString(")")
									).PChar()));
							last = timeGetTime();
						}
						Application->ProcessMessages();
					}
					AnimationIndicatorLog(
						UnicodeString((
								FString("(") + FString(dataset.GetNumData()) + FString(" / ") + FString(dataset.GetNumData()) + FString(")")
							).PChar()));
                }
			}
		}
	}

	if (!animation_indicator_abort_) {
		if (sizes.GetSize() == types.GetSize() && sizes.GetSize() == modal_names.GetSize()) {
			int dim_index = 0;
			FString last_modal_name;
			for (int ic = 0 ; ic < modal_names.GetSize() ; ic++) {
				FString modal_name = modal_names.GetAt(ic);
				if (modal_name.Pos(":")) {
					//Remove label name
					modal_name = modal_name.SplitBy(':');
				}
				if (modal_name.Pos("__")) {
					//Remove index
					FString modal_name_tmp = modal_name.SplitBy("__");
					int dim_actual = modal_name.AsInt();
					modal_name = modal_name_tmp;
					if (modal_name != last_modal_name) {
						dim_index = 0;
					}else {
						dim_index++;
					}

					if (dim_index != dim_actual) {
						errors.Add(
							FString("Inconsistent dim index in ") +
							FString(modal_name));
						errors.Add(
							FString("    Expected = ") +
							FString(dim_index) +
							FString(", Actual = ") +
							FString(dim_actual));
					}
				}

				modal_names.GetAt(ic) = modal_name;
				last_modal_name = modal_name;
			}

			for (int ic = 0 ; ic < modal_names.GetSize() - 1 ; ic++) {
				if (modal_names.GetAt(ic) == modal_names.GetAt(ic + 1) && types.GetAt(ic) / 2 == types.GetAt(ic + 1) / 2) {
					if (types.GetAt(ic) < 2){
						//Vector
						const int size = sizes.GetAt(ic).AsInt() + sizes.GetAt(ic + 1).AsInt();
						sizes.GetAt(ic) = FString(size);
					}else {
						//Image or CSV
						if (types.GetAt(ic) % 2){
							FString size_str = sizes.GetAt(ic);
							const int size = size_str.SplitBy(", ").AsInt() + 1;
							sizes.GetAt(ic) = FString(size) + FString(", ") + size_str;
						}else {
							sizes.GetAt(ic) = FString("2, ") + sizes.GetAt(ic);
						}
					}

					types.SetAt(ic, (types.GetAt(ic) / 2) * 2 + 1);
					modal_names.Delete(ic + 1);
					types.Delete(ic + 1);
					sizes.Delete(ic + 1);
					ic--;
				}
			}

			FString modal_names_str = FString("Name");
			FString types_str = FString("Type");
			FString size_str = FString("Size");
			for (int ic = 0 ; ic < modal_names.GetSize() ; ic++) {
				modal_names_str += FString("\t") + modal_names.GetAt(ic);

				if (modal_names.IndexOf(modal_names.GetAt(ic)) < ic) {
					errors.Add(FString("Duplicated modal name \"") + modal_names.GetAt(ic) + FString("\""));
				}

				types_str += FString("\t") + data_type_str[types.GetAt(ic)];
				size_str += FString("\t") + sizes.GetAt(ic);
			}
			messages.Add(FString(""));
			messages.Add(FString("Num Modal : ") + FString(modal_names.GetSize()));
			messages.Add(modal_names_str);
			messages.Add(types_str);
			messages.Add(size_str);
		}else {
			errors.Add(FString("Unknown error."));
		}

		/*
		//Check size consistensy with network
		for (int in = 0 ; in < nn_document_.GetNumStructure() ; in++) {
			const NNStructure &structure = nn_document_.GetStructure(in);
			for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
				const NNLayer &layer = structure.GetLayer(il);
				const FString variable_name = layer.GetDisplayPropertyValue(FString("Dataset"));
				int variable_index = modal_names.IndexOf(variable_name);
				if (variable_name != "" && variable_index >= 0) {
					const FString size_property =
						layer.GetPropertyIndexByName("Input") >= 0 ?
						FString("Input") :
						FString("Size");
					FString size_str =
						layer.GetDisplayPropertyValue(size_property).ReplaceStr(",", ", ");

					if (size_str != "") {
						FString dataset_size = sizes.GetAt(variable_index);
						const int c_pos = dataset_size.Pos("c");
						if (c_pos && size_str.Length() >= c_pos) {
							size_str =
								size_str.SubString(1, c_pos - 1) +
								FString("c") +
								size_str.SubString(c_pos + 1, size_str.Length() - c_pos);
						}
						if (size_str != dataset_size) {
							errors.Add(
								FString("Different data size of \"") + variable_name + FString("\" between Dataset and Network Graph"));
							errors.Add(
								FString("    Dataset = ") +
								sizes.GetAt(variable_index) +
								FString(", Network (") +
								layer.GetDisplayPropertyValue(FString("Name")) +
								FString("->") +
								size_property +
								FString(") = ") +
								size_str
								);
						}
					}
				}
			}
		}
		*/
	}

	nn_document_.Unlock();
	HideAnimationIndicator();

	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	Me_Debug->Lines->Clear();
	for (int ie = 0 ;ie < messages.GetSize() ; ie++) {
		Me_Debug->Lines->Add(messages.GetAt(ie).PChar());
	}

	Me_Debug->Lines->Add("");
	if (errors.GetSize() == 0 && !animation_indicator_abort_) {
		Me_Debug->Lines->Add("No inconsistency were found.");
	}else {
		for (int ie = 0 ;ie < errors.GetSize() ; ie++) {
			Me_Debug->Lines->Add(errors.GetAt(ie).PChar());
		}
	}
}
//---------------------------------------------------------------------------



void __fastcall TFo_Main::PB_MonitorMouseMove(TObject *Sender, TShiftState Shift,
		  float X, float Y)
{
	result_last_mouse_x_ = X;
	result_last_mouse_y_ = Y;
	if (Bu_Monitor->IsFocused) {
		PB_Monitor->Repaint();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_MonitorMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, float X, float Y)
{
	Sc_Monitor->Tag = 0;
	Fr_ResultsList->Bu_List->SetFocus();
	Fr_ResultsList->Sc_ListY->Tag = 0;//Allow scroll
	result_last_mouse_down_ = true;

	if (RB_ResultsLearningCurve->IsChecked){
		if (monitor_checkbox_[0].Contains(TPointF(X, Y))){
			if (MIMV_TrainingValidation->IsChecked){
				MIMV_Validation->IsChecked = true;
			}else if (MIMV_Validation->IsChecked){
				MIMV_TrainingValidation->IsChecked = true;
			}
			monitor_csv_[0].Update();
		}
		if (monitor_checkbox_[1].Contains(TPointF(X, Y))){
			if (MIMV_TrainingValidation->IsChecked){
				MIMV_Training->IsChecked = true;
			}else if (MIMV_Training->IsChecked){
				MIMV_TrainingValidation->IsChecked = true;
			}
			monitor_csv_[0].Update();
		}
	}
	if (RB_ResultsTradeoffGraph->IsChecked){
		if (result_tradeoff_h_statistics_rect_.Contains(TPointF(X,Y))){
			const double cx = (result_tradeoff_h_statistics_rect_.left + result_tradeoff_h_statistics_rect_.right) * 0.5;
			const double cy = (result_tradeoff_h_statistics_rect_.top + result_tradeoff_h_statistics_rect_.bottom) * 0.5;
			Co_HStatistics->Position->X = cx - Co_HStatistics->Width * 0.5;
			Co_HStatistics->Position->Y = cy - Co_HStatistics->Height * 0.5;
			Co_HStatistics->Visible = true;
			Co_HStatistics->SetFocus();
			Co_HStatistics->DropDown();
		}else {
			Co_HStatistics->Visible = false;
		}
	}
    Bu_Monitor->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIN_ImportCaffePrototxtClick(TObject *Sender)
{
	if (OD_Prototxt->Execute()) {
		Bu_NetworkTabClick(this);

		nn_graph_mapper_.Clear();
		Log(FString("import : ext=.prototxt"));
		nn_document_.KeepForUndo(FString("Import prototxt (Caffe)"));
		FStringList error_str;
		if (nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla) {
			CaffeProtoTxtToNNStructure_NNabla(AnsiString(OD_Prototxt->FileName).c_str(), nn_document_, error_str);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MINE_ExportCaffePrototxtClick(TObject *Sender)
{
	if (SD_Prototxt->Execute()) {
		Bu_NetworkTabClick(this);

		Log(FString("export : ext=.prototxt"));

		FStringList error_str;
		if (nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla) {
			NNStructureToCaffeProtoTxt_NNabla(nn_document_, AnsiString(SD_Prototxt->FileName).c_str(), error_str);
		}
		if (error_str.GetSize() == 0) {
			OpenFile(AnsiString(SD_Prototxt->FileName).c_str());
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_LearningRateMultiplierChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	double value = StrToFloatDef(Ed_LearningRateMultiplier->Text, nn_document_.GetOptimizer(optimizer_index).GetLearningRateMultiplier());
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateMultiplier() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateMultiplier(value);
        nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
int TFo_Main::GetExecuting() const
{
	return GetTraining() || GetEvaluating();
}
//---------------------------------------------------------------------------
int TFo_Main::GetTraining(int local_only) const
{
	// True if there is an active training thread.

	if (local_only){
		for (int it = 0 ; it < sdeep_training_thread_.GetSize() ; it++){
			if (!sdeep_training_thread_.GetAt(it)->GetRemote()){
				return true;
			}
		}
		return false;
	}else {
		return sdeep_training_thread_.GetSize() > 0;
	}
}
//---------------------------------------------------------------------------
int TFo_Main::GetTraining(const FString &job_name) const
{
	int result = false;
	for (int it = 0 ; it < sdeep_training_thread_.GetSize() && !result ; it++){
		SDeepTrainingThread *thread = sdeep_training_thread_.GetAt(it);
		result |= thread->GetName() == job_name;
	}
    return result;
}
//---------------------------------------------------------------------------
int TFo_Main::GetEvaluating() const
{
	return sdeep_evaluation_thread_ != NULL;
}
//---------------------------------------------------------------------------
int TFo_Main::GetEvaluating(const FString &job_name) const
{
    return sdeep_evaluation_thread_ ? sdeep_evaluation_thread_->GetName() == job_name : false;
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::MIT_OpenFileLocationClick(TObject *Sender)
{
	const CSVDataset &dataset = GetCurrentTable();
	TableUIState &table_ui_state = GetCurrentTableUIState();
	const int dataset_num_data = dataset.GetNumData();
	const int dataset_num_column = dataset.GetNumColumn();
	if (table_ui_state.row_index >= 0 && table_ui_state.row_index < dataset_num_data &&
		table_ui_state.col_index >= 0 && table_ui_state.col_index < dataset_num_column) {
		UnicodeString data_text = dataset.GetValue(table_ui_state.row_index, table_ui_state.col_index).PChar();

		if (IsFile(data_text)) {
			UnicodeString file_name = GetDataFileName(
				ExtractFilePath(dataset.GetFileName().PChar()),
				data_text);
			if (file_name != "") {
				OpenFileLocation(AnsiString(ExtractFilePath(file_name)).c_str());
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Co_EvaluationConfusionMatrixModalChange(TObject *Sender)
{
	if (Co_EvaluationConfusionMatrixModal->Tag == 0) {
		OpenEvaluationResult(false, true);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Me_EvaluationDblClick(TObject *Sender)
{
	FString line = FString(AnsiString(Me_Evaluation->Lines->Strings[Me_Evaluation->CaretPosition.Line]).c_str());
	while (line != ""){
		FString file_name = line.SplitBy('\"');
		if (FileExists(file_name.PChar()) || DirectoryExists(file_name.PChar())) {
			OpenFileLocation(file_name.PChar());
			return;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::RB_EvaluationOutputResultChange(TObject *Sender)
{
	Sc_TableX->Tag = 0;
	Sc_TableY->Tag = 0;
	image_cache_list_->ClearCache();
	csv_cache_list_->ClearCache();
	PB_Table->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_StructureSearchConstraintValidationMinChange(TObject *Sender)
{
	const double value = StrToFloatDef(Ed_StructureSearchConstraintValidationMin->Text, -1.0);
	if (nn_document_.GetStructureSearchConstraint(FString("Validation"), 0) != value) {
		nn_document_.SetStructureSearchConstraint(FString("Validation"), 0,	value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_StructureSearchConstraintValidationMaxChange(TObject *Sender)
{
	const double value = StrToFloatDef(Ed_StructureSearchConstraintValidationMax->Text, -1.0);
	if (nn_document_.GetStructureSearchConstraint(FString("Validation"), 1) != value) {
		nn_document_.SetStructureSearchConstraint(FString("Validation"), 1,	value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_StructureSearchConstraintCostMultiplyAddMinChange(TObject *Sender)
{
	const double value = StrToFloatDef(Ed_StructureSearchConstraintCostMultiplyAddMin->Text, -1.0);
	if (nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 0) != value) {
		nn_document_.SetStructureSearchConstraint(FString("CostMultiplyAdd"), 0,	value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_StructureSearchConstraintCostMultiplyAddMaxChange(TObject *Sender)
{
	const double value = StrToFloatDef(Ed_StructureSearchConstraintCostMultiplyAddMax->Text, -1.0);
	if (nn_document_.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 1) != value) {
		nn_document_.SetStructureSearchConstraint(FString("CostMultiplyAdd"), 1,	value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
int TFo_Main::OpenSDNX(FString sdnx_file_name, int load_weight)
{
	int result = true;
	FString sdnx_file_name_wo_ext = sdnx_file_name.SubString(1, sdnx_file_name.Length() - 5);
	FString sdnx_file_extract_dest_dir = sdnx_file_name_wo_ext + FString(".files");
	ExtractNetwork(sdnx_file_name_wo_ext, sdnx_file_extract_dest_dir, true);
	AnsiString network_file_name = (sdnx_file_extract_dest_dir + FString("\\ffnn")).PChar();
	if (FileExists(network_file_name + ".xml")) {
		result = OpenNetworkFromIni(nn_document_, network_file_name);
		if (result) {
			nn_graph_mapper_.Clear();
		}
		if (!load_weight) {
			for (int il = 0 ; il < nn_document_.GetNumLayer() ; il++) {
				const NNLayer &layer = nn_document_.GetLayer(il);
				for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
					const NNLayerProperty &property = layer.GetProperty(ip);
					if (property.GetType() == NNLayerPropertyType::File) {
                    	nn_document_.SetLayerPropertyValue(layer.GetLayerID(), ip, FString(""));
					}
				}
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------
void TFo_Main::SetNNablaWeight(FString result_dir, NNDocument &document)
{
	FStringList result_files = EnumFiles((result_dir + FString("\\net\\")).PChar());
	for (int it = 0 ; it < result_files.GetSize() ; it++){
		FString s = result_files.GetAt(it);
		if (s.SubString(s.Length() - 3, 4).ToLower() != ".txt"){
			result_files.Delete(it);
			it--;
		}else {
			FString param_name, normalized;//normalized is for sorting
			while (s != ""){
				FString s1 = s.SplitBy('[');
				param_name += s1;
				normalized += s1;
				if (s != ""){
					FString digit = s.SplitBy(']');
					normalized +=
						FString("[0000000").SubString(1, 8 - digit.Length() + 1) +
						digit +
						FString("]");
				}
			}
			param_name = param_name.SubString(1, param_name.Length() - 4).ReplaceStr("~", "/").ReplaceStr("%5B","[").ReplaceStr("%5D","]");//remove ".txt" and restore "[" and "]"
			result_files.GetAt(it) =
				param_name + FString(",") + normalized + FString(",") + result_dir + FString("\\net\\") +result_files.GetAt(it);
		}
	}

	result_files.Sort();

	FStringList weight_scopes;
	for (int it = 0 ; it < result_files.GetSize() ; it++){
		weight_scopes.Add(result_files.GetAt(it).SplitBy(','));//set param_name to weight_scopes
		result_files.GetAt(it).SplitBy(',');//remove normalized name (only file name will remain)
	}

	if (online_debug_mode_){
		TI_Debug->Visible = true;
		SetActiveTab(TC_Default, TI_Debug);
		Me_Debug->Lines->Clear();
		Me_Debug->Lines->Add(weight_scopes.GetText().PChar());
		Me_Debug->Lines->Add(result_files.GetText().PChar());
	}

	document.FillParameterName(weight_scopes, result_files);
}
//---------------------------------------------------------------------------
void TFo_Main::SetTorchWeight(FString result_dir, NNDocument &document)
{
	FStringList result_files = EnumFiles((result_dir + FString("\\net\\")).PChar());
	for (int it = 0 ; it < result_files.GetSize() ; it++){
		FString s = result_files.GetAt(it);
		if (s.SubString(s.Length() - 3, 4).ToLower() != ".txt"){
			result_files.Delete(it);
			it--;
		}else {
			FString param_name, normalized;//normalized is for sorting
			while (s != ""){
				FString s1 = s.SplitBy('[');
				param_name += s1;
				normalized += s1;
				if (s != ""){
					FString digit = s.SplitBy(']');
					normalized +=
						FString("[0000000").SubString(1, 8 - digit.Length() + 1) +
						digit +
						FString("]");
				}
			}
			param_name = param_name.SubString(1, param_name.Length() - 4).ReplaceStr("~", "/").ReplaceStr("%5B","[").ReplaceStr("%5D","]");//remove ".txt" and restore "[" and "]"
			result_files.GetAt(it) =
				param_name + FString(",") + normalized + FString(",") + result_dir + FString("\\net\\") +result_files.GetAt(it);
		}
	}

	result_files.Sort();

	FStringList weight_scopes;
	for (int it = 0 ; it < result_files.GetSize() ; it++){
		weight_scopes.Add(result_files.GetAt(it).SplitBy(','));//set param_name to weight_scopes
		result_files.GetAt(it).SplitBy(',');//remove normalized name (only file name will remain)
	}

	if (online_debug_mode_){
		TI_Debug->Visible = true;
		SetActiveTab(TC_Default, TI_Debug);
		Me_Debug->Lines->Clear();
		Me_Debug->Lines->Add(weight_scopes.GetText().PChar());
		Me_Debug->Lines->Add(result_files.GetText().PChar());
	}

	document.FillParameterName(weight_scopes, result_files);
}
//---------------------------------------------------------------------------
int TFo_Main::OpenNetworkFromResult(int load_weight)
{
	if (nn_document_.GetCurrentResultIndex() < 0) {
		return false;
	}

	int result = true;
	nn_document_.KeepForUndo(FString("Open Network"));
	AnsiString network_file_name = GetResultDir() + UnicodeString("\\network");
	result &= OpenNetworkFromIni(nn_document_, network_file_name);
	if (result) {
		nn_graph_mapper_.Clear();
	}
	// Open results.nntxt if it exists
	AnsiString result_network_file_name = GetResultDir() + UnicodeString("\\results.nntxt");
	if (FileExists(result_network_file_name)) {
		OpenResultsNNtxt(result_network_file_name, nn_document_, false);
	}

	if (load_weight) {//LoadWeight OpenWeight
		FString result_dir = FString(AnsiString(GetResultDir()).c_str());
		if (nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla) {
			//NNabla
			if (FileExists((result_dir + FString("\\results.nnp")).PChar())){
				ExtractNetwork(result_dir + FString("\\results"), result_dir + FString("\\net"), true);
			}else {
				ExtractNetwork(result_dir + FString("\\parameters"), result_dir + FString("\\net"), true);
			}
			SetNNablaWeight(result_dir, nn_document_);
		}
	}

	for (int in = 0 ; in < nn_document_.GetNumStructure() ; in++) {
		nn_document_.SetActiveStructure(in);
		nn_document_.RefreshDisplayValue();
		nn_document_.EmbedParameters();// Embed parameter layers into side connector of neural network layers (affine, conv. etc.)
		nn_document_.ArrangeLayers();
	}
	return result;
}
//---------------------------------------------------------------------------
void TFo_Main::ExtractNetwork(FString net_file_name_no_ext, FString output_dir, int overwrite)
{
	ShowAnimationIndicator(GetTranslatedText("Loading weight..."), nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla);

	nn_document_.Lock();
    Ti_AnimationTimer(this);
	SetEnvironment();
	switch (nn_document_.GetSDeepEngineType()) {
	case SDeepEngineType::NNabla:
		TThread::CreateAnonymousThread(
			new TDocumentIOFunc(
				&nn_document_, &settings_,
				(net_file_name_no_ext + FString(",") + output_dir).PChar(),
				DocumentIOType::ExtractWeight_NNabla,
				overwrite,
				online_debug_mode_)
			)->Start();
		break;
    case SDeepEngineType::Torch:
        TThread::CreateAnonymousThread(
            new TDocumentIOFunc(
                &nn_document_, &settings_,
                (net_file_name_no_ext + FString(".nnpt") + FString(",") + output_dir).PChar(),
                DocumentIOType::ExtractNnpt,
                overwrite,
                online_debug_mode_)
            )->Start();
        break;
	default:
		nn_document_.Unlock();
		break;
	}
	WaitForDocumentUnlock();

	HideAnimationIndicator();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_RetrainNotInPlaceClick(TObject *Sender)
{
	if (nn_document_.GetCurrentResultIndex() < 0) {
		return;
	}
	OpenNetworkFromResult(false);

	Ch_StructureSearchEnabled->IsChecked = false;

	control_button_type_ = ControlButtonType::RunTraining;
	Bu_ControlClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_RetrainInPlaceClick(TObject *Sender)
{
	if (nn_document_.GetCurrentResultIndex() < 0) {
		return;
	}
	if (GetTraining(nn_document_.GetResult(nn_document_.GetCurrentResultIndex()).GetDir())){
		return;
	}

	int result = MessageDlg(
		GetTranslatedText("You are going to retrain the network. The previous result will be overwritten. Are you sure you want to proceed?"),
		TMsgDlgType::mtWarning, mbOKCancel, 0);
	if (result == mrCancel) {
		return;
	}

	UnicodeString result_dir = GetResultDir();
	if (DirectoryExists(result_dir)) {
		UnicodeString result_file_name = result_dir + UnicodeString("\\result.ini");
		FFastIni ini = FFastIni(AnsiString(result_file_name).c_str());
		ini.Write<int>("result", "schedule", 1);
	}

	nn_document_.RefreshResults(FString("\\"));
	RefreshResultsList(false);

	Ch_StructureSearchEnabled->IsChecked = false;
	TrainScheduled();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_RetrainAllClick(TObject *Sender)
{
	if (nn_document_.GetCurrentResultIndex() < 0) {
		return;
	}

	int result = MessageDlg(
		GetTranslatedText("You are going to retrain all networks in the list. All previous results will be overwritten. It may take a very long time. Are you sure you want to proceed?"),
		TMsgDlgType::mtWarning, mbOKCancel, 0);
	if (result == mrCancel) {
		return;
	}

	for (int ir = 0 ; ir < nn_document_.GetResults(). GetSize() ; ir++) {
		nn_document_.SetCurrentResultIndex(ir);
		if (GetTraining(nn_document_.GetResult(ir).GetDir())){
			continue;
		}
		UnicodeString result_dir = GetResultDir();
		if (DirectoryExists(result_dir)) {
			UnicodeString result_file_name = result_dir + UnicodeString("\\result.ini");
			FFastIni ini = FFastIni(AnsiString(result_file_name).c_str());
			ini.Write<int>("result", "schedule", 1);
		}
	}

	nn_document_.RefreshResults(FString("\\"));
	RefreshResultsList(false);

	Ch_StructureSearchEnabled->IsChecked = false;
	TrainScheduled();
}
//---------------------------------------------------------------------------
int TFo_Main::TrainScheduled()
{
	if (GetTraining()) {
    	return false;
	}
	int scheduled_index = -1;
    int schedule_type = 0;
	for (int ir = nn_document_.GetResults().GetSize() - 1 ; ir >= 0 ; ir--) {
		schedule_type = nn_document_.GetResults().GetAt(ir).GetSchedule();
		if (schedule_type) {
			scheduled_index = ir;
			break;
		}
	}
	if (scheduled_index < 0) {
		return false;
	}

	nn_document_.SetCurrentResultIndex(scheduled_index);
	UnicodeString result_dir = GetResultDir();
	if (!DirectoryExists(result_dir)) {
		return false;
	}

	UnicodeString result_file_name = result_dir + UnicodeString("\\network.ini");
	if (!FileExists(result_file_name)) {
		return false;
	}
	nn_document_tmp_.LoadFromFile(FString(AnsiString(result_file_name).c_str()), FString(""), false, true, true);
	nn_document_tmp_.SetFileName(nn_document_.GetFileName());

	nn_document_.SetCurrentResultIndex(scheduled_index);
	ExecuteTraining(nn_document_tmp_, schedule_type == 2);

    return true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIM_SaveCSVAsClick(TObject *Sender)
{
	if (!SD_CSV->Execute()) {
		return;
	}

	if (RB_ResultsLearningCurve->IsChecked) {
		monitor_csv_[0].SaveToFile(AnsiString(SD_CSV->FileName).c_str());
	}else if (RB_ResultsTradeoffGraph->IsChecked) {
		tradeoff_csv_.SaveToFile(AnsiString(SD_CSV->FileName).c_str());
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIT_SaveCSVAsClick(TObject *Sender)
{
	if (!SD_CSV->Execute()) {
		return;
	}

	const CSVDataset &dataset = GetCurrentTable();
	dataset.SaveToFile_(AnsiString(SD_CSV->FileName).c_str());
}
//---------------------------------------------------------------------------



void __fastcall TFo_Main::Ch_EarlyStoppingChange(TObject *Sender)
{
	if (nn_document_.GetEarlyStopping() != Ch_EarlyStopping->IsChecked) {
		nn_document_.SetEarlyStopping(Ch_EarlyStopping->IsChecked);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_TimeLimitChange(TObject *Sender)
{
	FString s = FString(AnsiString(Ed_TimeLimit->Text).c_str());
	const int days = s.SplitBy(':').AsInt();
	const int hours = s.SplitBy(':').AsInt();
	const int minutes = s.SplitBy(':').AsInt();
	const int seconds = s.AsInt();
	const double value =
		days > 0 || hours > 0 || minutes > 0 || seconds > 0 ?
		days + (hours + (minutes + seconds * (1.0 / 60.0)) * (1.0 / 60.0)) * (1.0 / 24.0) + 0.00000001 :
		0.0;
	if (nn_document_.GetTimeLimit() != value) {
		nn_document_.SetTimeLimit(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Bu_PropertyBrowseClick(TObject *Sender)
{
	if (nn_document_.GetCurrentLayerID() != current_property_layer_id_) {
		return;
	}

	if (nn_document_.GetCurrentPropertyIndex() != current_property_index_) {
		return;
	}

	if (Lo_EditProperty->Visible || (Co_Property->Visible && Co_Property->Selected)) {
		const NNLayer *layer = nn_document_.GetLayerByID(current_property_layer_id_);
		if (layer) {
			const NNLayerProperty &property = layer->GetProperty(current_property_index_);
			switch (property.GetType()){
			case NNLayerPropertyType::Model:
				{
					/*
					Fo_HuggingFaceModelHub = new TFo_HuggingFaceModelHub(this);
					Fo_HuggingFaceModelHub->SetLang(Lang->Lang);
					Fo_HuggingFaceModelHub->SetTask(layer->GetType().PChar());
					Fo_HuggingFaceModelHub->ShowModal();
					if (Fo_HuggingFaceModelHub->ModalResult == mrOk){
						Ed_Property->Text = Fo_HuggingFaceModelHub->GetResult();
					}
					Fo_HuggingFaceModelHub->Release();
					*/
					OpenFile((FString("https://huggingface.co/models?pipeline_tag=") + layer->GetType()).PChar());
				}
				break;

			case NNLayerPropertyType::File:
				if (OD_File->Execute()) {
					Ed_Property->Text = OD_File->FileName;
				}
				break;
			default:
				break;
			}
		}
	}

}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestNaiveNet2VecClick(TObject *Sender)
{
	TI_Debug->Visible = true;

	FListT<float> result;
	FStringList result_caption;
	NNStructureToVector(nn_document_, result, result_caption, nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla);

	Me_Debug->Lines->Clear();
	Me_Debug->Lines->Add(IntToStr(result.GetSize()) + UnicodeString(" attributes."));
	for (int i = 0 ; i < result.GetSize() ; i++) {
		Me_Debug->Lines->Add((result_caption.GetAt(i) + FString(" : ") + FString(result.GetAt(i))).PChar());
	}
	SetActiveTab(TC_Default, TI_Debug);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_CreateNaiveNet2VecCSVfromLearningResultsClick(TObject *Sender)
{
	if (!SD_CSV->Execute()) {
		return;
	}

	Bu_TrainingTabClick(this);
	Application->ProcessMessages();

	//Prepare CSV
	FStringList rdb, tdb;

	//Result loop
	for (int ir = 0 ; ir < nn_document_.GetResults().GetSize() ; ir++) {
		nn_document_.SetCurrentResultIndex(ir);
		Fr_ResultsList->SetItemIndex(ir);
		Application->ProcessMessages();

		const NNTrainingResult &training_result = nn_document_.GetResult(ir);
		if (training_result.GetComplete()){
			OpenNetworkFromResult(false);

			FListT<float> result;
			FStringList result_caption;
			NNStructureToVector(nn_document_, result, result_caption, nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla);

			//Header
			if (!tdb.GetSize()) {
				tdb.Add(FString("Index,Error"));
				FString s = FString("Index");
				for (int id = 0 ; id < result_caption.GetSize() ; id++) {
					s += FString(",") + result_caption.GetAt(id);
				}
				rdb.Add(s);
			}

			//Data
			tdb.Add(FString(ir) + FString(",") + FString(training_result.GetValidation()));
			FString s = FString(ir);
			for (int id = 0 ; id < result.GetSize() ; id++) {
				s += FString(",") + FString(result.GetAt(id));
			}
			rdb.Add(s);
		}
	}

	FString file_name = FString(AnsiString(SD_CSV->FileName).c_str());
	rdb.SaveToFile((file_name.SubString(1, file_name.Length() - 4) + FString(".rdb.csv")).PChar());
	tdb.SaveToFile((file_name.SubString(1, file_name.Length() - 4) + FString(".tdb.csv")).PChar());
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_DebugPaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_Debug->Canvas;
	C->BeginScene();
	C->SetMatrix(PB_Debug->AbsoluteMatrix);
	//C->Clear(0xff000000);
	if (debug_bmp_) {
		C->DrawBitmap(
			debug_bmp_,
			RectF(debug_bmp_->Width, debug_bmp_->Height, 0, 0),
			RectF(debug_bmp_->Width, debug_bmp_->Height, 0, 0),
			1.0f, true);
	}
	C->EndScene();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestGaussianProcessImplementationClick(TObject *Sender)
{
	//Setup
	static const int num_training_data = 8;
	static const double value_range = 1.0;
	static const int dim = 1;
	const int plot_size = PB_Debug->Width;

	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	Me_Debug->Lines->Clear();
	ClearDebugBMP();
	debug_bmp_ = new TBitmap(plot_size, plot_size);

	TCanvas *C = debug_bmp_->Canvas;
	C->BeginScene();
	C->Clear(0xff404040);

	GaussianProcess_GaussianKernel gp;
	gp.SetGaussianKernelSigma(10.0);
	gp.SetBeta(400.0);

	//Create training data
	for (int i = 0 ; i < num_training_data ; i++) {
		FListT<double> x;
		x.Add(rand() % 1000 * 0.001);
		for (int id = 1 ; id < dim ; id++) {
			//x.Add(rand() % 1000 * 0.001);
			x.Add(x.GetAt(0) + rand() % 100 * 0.001 - 0.05);
		}
		const double y = (sin(x.GetAt(0) * 2 * 3.1415926) + rand() % 100 * 0.001 - 0.05) * value_range;
		gp.AddData(x, y);

		C->Stroke->Color = 0xffffffff;
		C->Stroke->Thickness = 5;
		C->DrawLine(
			TPointF(x.GetAt(0) * plot_size, y / value_range * plot_size * 0.25 + plot_size / 2),
			TPointF(x.GetAt(0) * plot_size, y / value_range * plot_size * 0.25 + plot_size / 2),
			1.0f);
	}

	//Train
	gp.Train();

	//Tuning
	Me_Debug->Lines->Add("beta\tsigma\tbefore\tafter");
	for (int i = 0 ; i < 1000 ; i++) {
		Me_Debug->Lines->Add(
			FormatFloat("0.0", gp.GetBeta()) + UnicodeString("\t") +
			FormatFloat("0.00000", gp.GetGaussianKernelSigma()) + UnicodeString("\t") +
			FormatFloat("0.00000", gp.EvaluateLogLikelihood()) + UnicodeString("\t") +
			FormatFloat("0.00000", gp.SearchHyperParameter()));
	}

	//Predict
	FListT<double> last_x;
	double last_y_myu = 0.0;//, last_y_sigma = 0.0;
	for (int ix = 0 ; ix < plot_size ; ix++) {
		FListT<double> x;
		double y_myu = 0.0, y_sigma = 0.0;
		x.Add(ix * 1.0 / plot_size);
		for (int id = 1 ; id < dim ; id++) {
			//x.Add(rand() % 1000 * 0.001);
			x.Add(x.GetAt(0) + rand() % 100 * 0.001 - 0.05);
		}
		gp.Predict(x, y_myu, y_sigma);

		C->Stroke->Thickness = 1;
		C->Stroke->Color = 0x8000ff00;
		C->DrawLine(
			TPointF(x.GetAt(0) * plot_size, (y_myu - y_sigma * 3) / value_range * plot_size * 0.25 + plot_size / 2),
			TPointF(x.GetAt(0) * plot_size, (y_myu + y_sigma * 3) / value_range * plot_size * 0.25 + plot_size / 2),
			1.0f);
		if (ix > 0){
			C->Stroke->Color = 0xff00ff00;
			C->DrawLine(
				TPointF(last_x.GetAt(0) * plot_size, last_y_myu / value_range * plot_size * 0.25 + plot_size / 2),
				TPointF(x.GetAt(0) * plot_size, y_myu / value_range * plot_size * 0.25 + plot_size / 2),
				1.0f);

		}

		last_x = x;
		last_y_myu = y_myu;
		//last_y_sigma = y_sigma;
	}

	C->EndScene();

	PB_Debug->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestSimulatedStructureSearchWithNaiveNet2VecClick(TObject *Sender)
{
	//Setup
	int num_repeat = 1000;
	int num_search_epoch = 30;
	//double error_gain = 1.0;
	//double error_offset = -1.0;
	double exploration_sigma = 1.0;
	double gaussian_kernel_sigma = 10.0;
	double gaussian_process_beta = 4.0;
	int effective_col_only = 1;


	FFastIni ini("");
	FStringList sl;
	for (int i = 0 ; i < Me_Debug->Lines->Count ; i++) {
		sl.Add(FString(AnsiString(Me_Debug->Lines->Strings[i]).c_str()));
	}
	ini.SetStringList(sl);
	num_repeat = ini.ReadInteger("Setup", "num repeat", num_repeat);
	num_search_epoch = ini.ReadInteger("Setup", "num search epoch", num_search_epoch);
	//error_gain = ini.ReadDouble("Setup", "error gain", error_gain);
	//error_offset = ini.ReadDouble("Setup", "error offset", error_offset);
	exploration_sigma = ini.ReadDouble("Setup", "exploration sigma", exploration_sigma);
	gaussian_kernel_sigma = ini.ReadDouble("Setup", "gaussian kernel sigma", gaussian_kernel_sigma);
	gaussian_process_beta = ini.ReadDouble("Setup", "gaussian process beta", gaussian_process_beta);
	effective_col_only = ini.ReadInteger("Setup", "effective col only", effective_col_only);

	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	Me_Debug->Lines->Clear();
	Me_Debug->Lines->Add(UnicodeString("[Setup]"));
	Me_Debug->Lines->Add(UnicodeString("num repeat=") + IntToStr(num_repeat));
	Me_Debug->Lines->Add(UnicodeString("num search epoch=") + IntToStr(num_search_epoch));
	//Me_Debug->Lines->Add(UnicodeString("error gain=") + FloatToStr(error_gain));
	//Me_Debug->Lines->Add(UnicodeString("error offset=") + FloatToStr(error_offset));
	Me_Debug->Lines->Add(UnicodeString("exploration sigma=") + FloatToStr(exploration_sigma));
	Me_Debug->Lines->Add(UnicodeString("gaussian kernel sigma=") + FloatToStr(gaussian_kernel_sigma));
	Me_Debug->Lines->Add(UnicodeString("gaussian process beta=") + FloatToStr(gaussian_process_beta));
	Me_Debug->Lines->Add(UnicodeString("effective col only=") + FloatToStr(effective_col_only));


	//Open dataset
	OD_CSV->Title = "Open .tdb.csv";
	FString tdb_file_name = ini.ReadString("Setup", "tdb file name", FString(""));
	if (tdb_file_name == "") {
		if (!OD_CSV->Execute()) {
			OD_CSV->Title = "";
			return;
		}
		tdb_file_name = FString(AnsiString(OD_CSV->FileName).c_str());
	}
	CSVDataset tdb;
	tdb.LoadFromFile(tdb_file_name.PChar(), true);
	Me_Debug->Lines->Add(UnicodeString("tdb file name=") + tdb_file_name.PChar());

	OD_CSV->Title = "Open .rdb.csv";
	FString rdb_file_name = ini.ReadString("Setup", "rdb file name", FString(""));
	if (rdb_file_name == "") {
		if (!OD_CSV->Execute()) {
			OD_CSV->Title = "";
			return;
		}
		rdb_file_name = FString(AnsiString(OD_CSV->FileName).c_str());
	}
	CSVDataset rdb;
	rdb.LoadFromFile(rdb_file_name.PChar(), true);
	Me_Debug->Lines->Add(UnicodeString("rdb file name=") + rdb_file_name.PChar());

	Me_Debug->Lines->Add(UnicodeString("[Dataset]"));
	Me_Debug->Lines->Add(UnicodeString("tdb num data : ") + IntToStr(tdb.GetNumData()));
	Me_Debug->Lines->Add(UnicodeString("tdb num column : ") + IntToStr(tdb.GetNumColumn()));
	Me_Debug->Lines->Add(UnicodeString("rdb num data : ") + IntToStr(rdb.GetNumData()));
	Me_Debug->Lines->Add(UnicodeString("rdb num column : ") + IntToStr(rdb.GetNumColumn()));
	if (effective_col_only) for (int ic = rdb.GetNumColumn() - 1 ; ic >= 0 ; ic--) {
		double max = 0.0, min = 0.0;
		rdb.GetColumnMinMax(ic, min, max);
		if (min == max) {
			rdb.DeleteCol(ic);
		}
	}
	Me_Debug->Lines->Add(UnicodeString("rdb num effective column : ") + IntToStr(rdb.GetNumColumn()));

	OD_CSV->Title = "";
	Application->ProcessMessages();

	if (tdb.GetNumData() != rdb.GetNumData()) {
		ShowMessage("Different data size between tdb and rdb.");
		return;
	}

	if (tdb.GetNumData() < num_search_epoch) {
		ShowMessage("Not enough data in tdb and rdb.");
		return;
	}

	if (tdb.GetNumColumn() != 2) {
		ShowMessage("Column num of tdb must be two (data id and error).");
		return;
	}

	CSVDataset adb;
	adb = rdb;
	for (int i = 0 ; i < tdb.GetNumData() ; i++) {
		adb.SetValue(i, 0, tdb.GetValue(i, 1));
	}
	adb.SaveToFile((rdb.GetFileName().SubString(1, rdb.GetFileName().Length() - 8) + FString(".adb.csv")).PChar());

	//Baseline
	FListT<double> baseline_result;
	for (int ir = 0 ; ir < num_repeat ; ir++) {
		//Reset candidates
		FListT<int> candidates;
		for (int i = 0 ; i < tdb.GetNumData() ; i++) {
			candidates.Add(i);
		}

		double best_error = 1.0e+304;
		for (int ie = 0 ; ie < num_search_epoch ; ie++) {
			const int candidate_index = ie == 0 ? 0 : rand() % candidates.GetSize();
			const int index = candidates.GetAt(candidate_index);
			candidates.Delete(candidate_index);

			const double error = tdb.GetValue(index, 1).AsFloat();
			if (error < best_error) {
				best_error = error;
			}
			if (baseline_result.GetSize() <= ie) {
				baseline_result.Add(best_error);
			}else {
				baseline_result.SetAt(ie, baseline_result.GetAt(ie) + best_error);
			}
		}
	}

	//Bayesian search
	FListT<double> bayesian_result;
	FListT<double> best_myu; best_myu.Add(0.0);
	FListT<double> best_possible; best_possible.Add(0.0);
	FListT<double> actual;
	FListT<double> beta, sigma;
	FListT<double> initial, final;
	Me_Debug->Lines->Add(UnicodeString("[Result]"));
	Me_Debug->Lines->Add("\trand\tsearch\tbest\tpossible\tactual\tbeta\tsigma\tinit\tfinal");
	{// for (int ir = 0 ; ir < num_repeat ; ir++) {
		//Reset candidates
		FListT<int> candidates;
		for (int i = 0 ; i < tdb.GetNumData() ; i++) {
			candidates.Add(i);
		}

		GaussianProcess_GaussianKernel gp;
		gp.SetGaussianKernelSigma(gaussian_kernel_sigma);
		gp.SetBeta(gaussian_process_beta);

		double best_error = 1.0e+304;
		for (int ie = 0 ; ie < num_search_epoch ; ie++) {
			int candidate_index = 0;
			if (ie > 0){
				double best_possible_error = 1.0e+304;
				double best_myu_error = 0.0;
				for (int ic = 0 ; ic < candidates.GetSize() ; ic++) {
					const int index = candidates.GetAt(ic);
					FListT<double> x;
					double y_myu = 0.0, y_sigma = 0.0;
					for (int id = 1 ; id < rdb.GetNumColumn() ; id++) {
						x.Add(rdb.GetValue(index, id).AsFloat());
					}
					gp.Predict(x, y_myu, y_sigma);
					const double possible_error = y_myu - y_sigma * exploration_sigma;
					if (possible_error < best_possible_error) {
						candidate_index = ic;
						best_possible_error = possible_error;
						best_myu_error = y_myu;
					}
				}
				best_myu.Add(best_myu_error);
				best_possible.Add(best_possible_error);
			}

			const int index = candidates.GetAt(candidate_index);
			candidates.Delete(candidate_index);

			const double error = tdb.GetValue(index, 1).AsFloat();
			if (error < best_error) {
				best_error = error;
			}
			if (bayesian_result.GetSize() <= ie) {
				bayesian_result.Add(best_error);
			}else {
				bayesian_result.SetAt(ie, bayesian_result.GetAt(ie) + best_error);
			}

			{
				FListT<double> x;
				for (int id = 1 ; id < rdb.GetNumColumn() ; id++) {
					x.Add(rdb.GetFloatValue(index, id));
				}
				const double y = tdb.GetFloatValue(index, 1);
				gp.AddData(x, y);
				gp.SetBeta(gaussian_process_beta);//Always start from initial beta
				gp.Train();
				const double ll = gp.EvaluateLogLikelihood();
				initial.Add(ll);

				//Tuning
				for (int i = 0 ; i < 100 ; i++) {
					gp.SearchHyperParameter();
					/*
					Me_Debug->Lines->Add(
						FormatFloat("0.0", gp.GetBeta()) + UnicodeString("\t") +
						FormatFloat("0.00000", gp.GetGaussianKernelSigma()) + UnicodeString("\t") +
						FormatFloat("0.00000", gp.EvaluateLikelihood()) + UnicodeString("\t") +
						FormatFloat("0.00000", gp.SearchHyperParameter()));
					*/
				}
				const double ll_after = gp.EvaluateLogLikelihood();
				final.Add(ll_after);
				actual.Add(y);
				beta.Add(gp.GetBeta());
				sigma.Add(gp.GetGaussianKernelSigma());
			}
			Me_Debug->Lines->Add(
				IntToStr(ie) +
				UnicodeString("\t") +
				FormatFloat("0.0000", baseline_result.GetAt(ie) / num_repeat) +
				UnicodeString("\t") +
				FormatFloat("0.0000", bayesian_result.GetAt(ie)) +
				UnicodeString("\t") +
				FormatFloat("0.0000", best_myu.GetAt(ie)) +
				UnicodeString("\t") +
				FormatFloat("0.0000", best_possible.GetAt(ie)) +
				UnicodeString("\t") +
				FormatFloat("0.0000", actual.GetAt(ie)) +
				UnicodeString("\t") +
				FormatFloat("0.0000", beta.GetAt(ie)) +
				UnicodeString("\t") +
				FormatFloat("0.0000", sigma.GetAt(ie)) +
				UnicodeString("\t") +
				FormatFloat("0.0000", initial.GetAt(ie)) +
				UnicodeString("\t") +
				FormatFloat("0.0000", final.GetAt(ie)));
			Application->ProcessMessages();
		}

	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Co_StructureSearchMethodChange(TObject *Sender)
{
	if (nn_document_.GetStructureSearchMethod() != (StructureSearchMethod)Co_StructureSearchMethod->ItemIndex) {
		nn_document_.SetStructureSearchMethod((StructureSearchMethod)Co_StructureSearchMethod->ItemIndex);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestGaussianProcessClosedTestClick(TObject *Sender)
{
	//Setup
	double gaussian_kernel_sigma = 1.0;
	double gaussian_process_beta = 4.0;

	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	Me_Debug->Lines->Clear();

	//Open dataset
	if (!OD_CSV->Execute()) {
		OD_CSV->Title = "";
		return;
	}
	CSVDataset tdb;
	tdb.LoadFromFile(AnsiString(OD_CSV->FileName).c_str(), true);
	Me_Debug->Lines->Add(UnicodeString("tdb file name=") + tdb.GetFileName().PChar());

	Me_Debug->Lines->Add(UnicodeString("[Dataset]"));
	Me_Debug->Lines->Add(UnicodeString("num data : ") + IntToStr(tdb.GetNumData()));
	Me_Debug->Lines->Add(UnicodeString("num column : ") + IntToStr(tdb.GetNumColumn()));

	Application->ProcessMessages();

	GaussianProcess_GaussianKernel gp;
	gp.SetGaussianKernelSigma(gaussian_kernel_sigma);
	gp.SetBeta(gaussian_process_beta);

	double mean = 0.0, variance = 1.0;
	tdb.GetColumnMeanVariance(0, mean, variance);
	Me_Debug->Lines->Add(UnicodeString("mean : ") + FloatToStr(mean));
	Me_Debug->Lines->Add(UnicodeString("variance : ") + FloatToStr(variance));
	for (int i = 0 ; i < tdb.GetNumData() ; i++) {
		FListT<double> x;
		for (int id = 1 ; id < tdb.GetNumColumn() ; id++) {
			x.Add(tdb.GetFloatValue(i, id));
		}
		const double y = tdb.GetFloatValue(i, 0);
		gp.AddData(x, y);
	}

	gp.Train();
	Me_Debug->Lines->Add(UnicodeString("offset(-mean) : ") + FloatToStr(gp.GetYOffset()));
	Me_Debug->Lines->Add(UnicodeString("gain(1.0/sqrt(var)) : ") + FloatToStr(gp.GetYGain()));
	//Tuning
	//*
	Me_Debug->Lines->Add(UnicodeString("[Tuning]"));
	Me_Debug->Lines->Add("t\tlog_likelihood\trmse\tbeta\tsigma");
	for (int i = 0 ; i < 100 ; i++) {
		Me_Debug->Lines->Add(
			IntToStr(i + 1) +
			UnicodeString("\t") +
			FormatFloat("0.0000", gp.SearchHyperParameter()) +
			UnicodeString("\t") +
			FormatFloat("0.0000", sqrt(gp.EvaluateTrainingMSE())) +
			UnicodeString("\t") +
			FormatFloat("0.0000", gp.GetBeta()) +
			UnicodeString("\t") +
			FormatFloat("0.0000", gp.GetGaussianKernelSigma()));
		Application->ProcessMessages();
	}
	//*/

	Me_Debug->Lines->Add(UnicodeString("[Result]"));
	Me_Debug->Lines->Add("truth\tprediction\tsigma");
	for (int i = 0 ; i < tdb.GetNumData() ; i++) {
		FListT<double> x;
		for (int id = 1 ; id < tdb.GetNumColumn() ; id++) {
			x.Add(tdb.GetFloatValue(i, id));
		}
		double y_myu = 0.0, y_sigma = 0.0;
		gp.Predict(x, y_myu, y_sigma);

		Me_Debug->Lines->Add(
			FormatFloat("0.0000", tdb.GetFloatValue(i, 0)) +
			UnicodeString("\t") +
			FormatFloat("0.0000", y_myu) +
			UnicodeString("\t") +
			FormatFloat("0.0000", y_sigma));
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestGaussianClick(TObject *Sender)
{
	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	Me_Debug->Lines->Clear();
	Me_Debug->Lines->Add(
		UnicodeString("p(myu = 0, var = 1.0, left = -1.0, right = 1.0) = ") +
		FloatToStr(p(0, 1.0, -1.0, 1.0))
		);
	Me_Debug->Lines->Add(
		UnicodeString("p(myu = 0, var = 1.0, left = -2.0, right = 2.0) = ") +
		FloatToStr(p(0, 1.0, -2.0, 2.0))
		);
	Me_Debug->Lines->Add(
		UnicodeString("p(myu = 0, var = 1.0, left = -3.0, right = 3.0) = ") +
		FloatToStr(p(0, 1.0, -3.0, 3.0))
		);
	Me_Debug->Lines->Add(
		UnicodeString("p(myu = 0, var = 1.0, left = -4.0, right = 4.0) = ") +
		FloatToStr(p(0, 1.0, -4.0, 4.0))
		);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIM_SaveGraphBMPAsClick(TObject *Sender)
{
	if (!SD_Image->Execute()) {
		return;
	}
	if (RB_ResultsWeight->IsChecked) {
		SaveGraphBitmapToFile(SD_Image->FileName, PB_Monitor->Width, PB_Monitor->Height);
	}else {
		SaveGraphBitmapToFile(SD_Image->FileName);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIMV_TrainingValidationClick(TObject *Sender)
{
	((TMenuItem*)Sender)->IsChecked = true;

	//Update max and min of graph and redraw graph
	monitor_csv_[0].Update();
	tradeoff_csv_.Update();
	Ti_AnimationTimer(Sender);
	PB_Monitor->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestSaveGraphBitmapsClick(TObject *Sender)
{
	if (!SD_Image->Execute()) {
    	return;
	}

	UnicodeString ext = ExtractFileExt(SD_Image->FileName);
	UnicodeString file_name = SD_Image->FileName.SubString(1, SD_Image->FileName.Length() - ext.Length());

	Bu_TrainingTabClick(this);
	Application->ProcessMessages();

	//Prepare CSV
	FStringList rdb, tdb;

	//Result loop
	for (int ir = 0 ; ir < nn_document_.GetResults().GetSize() ; ir++) {
		nn_document_.SetCurrentResultIndex(ir);
		Fr_ResultsList->SetItemIndex(ir);
		Application->ProcessMessages();
		SaveGraphBitmapToFile(
			file_name + UnicodeString("_") + FormatFloat("0000", ir) + ext);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestExportTradeoffCSVForEvaluationClick(TObject *Sender)
{
	if (!nn_document_.GetResults().GetSize()) {
		ShowMessage("No result to export");
	}

	if (!SD_CSV->Execute()) {
		return;
	}

	Bu_TrainingTabClick(this);
	RB_ResultsTradeoffGraph->IsChecked = true;
	Application->ProcessMessages();

	//Prepare CSV
	FStringList csv;
	csv.Add(FString("index,time(h),min_err,cost004,cost005,cost006"));

	//Result loop
	NNTrainingResult &first_result = nn_document_.GetResults().GetAt(0);
	const FString &first_result_dir = first_result.GetDir();
	TDateTime start_time =
		EncodeDate(
			first_result_dir.SubString(1, 4).AsInt(),
			first_result_dir.SubString(5, 2).AsInt(),
			first_result_dir.SubString(7, 2).AsInt()) +
		EncodeTime(
			first_result_dir.SubString(10, 2).AsInt(),
			first_result_dir.SubString(12, 2).AsInt(),
			first_result_dir.SubString(14, 2).AsInt(),
			0);

	double min_err = 1.0e+308;
	double cost[3];
	memset(cost, 0, sizeof(cost));
	for (int ir = 0 ; ir < nn_document_.GetResults().GetSize() ; ir++) {
		nn_document_.SetCurrentResultIndex(ir);
		Fr_ResultsList->SetItemIndex(ir);
		Application->ProcessMessages();

		NNTrainingResult &result = nn_document_.GetResults().GetAt(ir);
		const FString &result_dir = result.GetDir();
		TDateTime current_time =
			EncodeDate(
				result_dir.SubString(1, 4).AsInt(),
				result_dir.SubString(5, 2).AsInt(),
				result_dir.SubString(7, 2).AsInt()) +
			EncodeTime(
				result_dir.SubString(10, 2).AsInt(),
				result_dir.SubString(12, 2).AsInt(),
				result_dir.SubString(14, 2).AsInt(),
				0);

		if (result.GetComplete()) {
			if (result.GetValidation() < min_err) {
				min_err = result.GetValidation();
			}
			for (int ic = 0 ; ic < 3 ; ic++) {
				if (result.GetValidation() < 0.04 + ic * 0.01) {
					if (cost[ic] <= 0.0 || result.GetCostMultiplyAdd() < cost[ic]) {
						cost[ic] = result.GetCostMultiplyAdd();
					}
				}
			}
		}

		csv.Add(
			FString(ir) + //index
			FString(",") +
			FString((double)(current_time - start_time) * 24.0) + //time(h)
			FString(",") +
			FString(min_err) +
			FString(",") +
			(cost[0] > 0.0 ? FString(cost[0]) : FString("")) +
			FString(",") +
			(cost[1] > 0.0 ? FString(cost[1]) : FString("")) +
			FString(",") +
			(cost[2] > 0.0 ? FString(cost[2]) : FString("")));

	}
	csv.SaveToFile(AnsiString(SD_CSV->FileName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MINZ_DefaultClick(TObject *Sender)
{
	network_zoom_ = 0;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MINZ_ZoomInClick(TObject *Sender)
{
	network_zoom_++;
	if (network_zoom_ > 6) {
		network_zoom_ = 6;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MINZ_ZoomOutClick(TObject *Sender)
{
	network_zoom_--;
	if (network_zoom_ < -8) {
		network_zoom_ = -8;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_MonitorSolveClick(TObject *Sender)
{
	if (training_monitor_solve_dir_ != ""){
		DeleteDir(AnsiString(training_monitor_solve_dir_).c_str());
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_MonitorMouseWheel(TObject *Sender, TShiftState Shift,
          int WheelDelta, bool &Handled)
{
	Sc_Monitor->Tag = 0;
	//Zoom
	if (RB_ResultsLearningCurve->IsChecked) {
		monitor_zoom_ += WheelDelta / 120.0;
		if (monitor_zoom_ > 8) {
			monitor_zoom_ = 8;
		}
		if (monitor_zoom_ < 0) {
			monitor_zoom_ = 0;
		}
		monitor_csv_[0].Update();
	}else if (RB_ResultsTradeoffGraph->IsChecked) {
		result_tradeoff_zoom_ += WheelDelta / 120.0;
		if (result_tradeoff_zoom_ > 16) {
			result_tradeoff_zoom_ = 16;
		}
		if (result_tradeoff_zoom_ < 0) {
			result_tradeoff_zoom_ = 0;
		}
		tradeoff_csv_.Update();
	}else {
		if (Shift.Contains(ssCtrl)) {
			result_weight_zoom_ += WheelDelta / 120.0;
			if (result_weight_zoom_ > 8) {
				result_weight_zoom_ = 8;
			}
			if (result_weight_zoom_ < 0) {
				result_weight_zoom_ = 0;
			}
		}else {
			result_weight_top_ -= WheelDelta;
			if (result_weight_top_ > Sc_Monitor->Max) {
				result_weight_top_ = Sc_Monitor->Max;
			}
			if (result_weight_top_ < 0.0) {
				result_weight_top_ = 0.0;
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIMZ_DefaultClick(TObject *Sender)
{
	if (RB_ResultsLearningCurve->IsChecked) {
		monitor_zoom_ = 0;
		monitor_csv_[0].Update();
	}else if (RB_ResultsTradeoffGraph->IsChecked) {
		result_tradeoff_zoom_ = 0;
		tradeoff_csv_.Update();
	}else if (RB_ResultsWeight->IsChecked) {
		result_weight_zoom_ = 4;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIMZ_ZoomInClick(TObject *Sender)
{
	if (RB_ResultsLearningCurve->IsChecked) {
		monitor_zoom_ ++;
		if (monitor_zoom_ > 8) {
			monitor_zoom_ = 8;
		}
		monitor_csv_[0].Update();
	}else if (RB_ResultsTradeoffGraph->IsChecked) {
		result_tradeoff_zoom_ ++;
		if (result_tradeoff_zoom_ > 16) {
			result_tradeoff_zoom_ = 16;
		}
		tradeoff_csv_.Update();
	}else if (RB_ResultsWeight->IsChecked) {
		result_weight_zoom_ ++;
		if (result_weight_zoom_ > 8) {
			result_weight_zoom_ = 8;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIMZ_ZoomOutClick(TObject *Sender)
{
	if (RB_ResultsLearningCurve->IsChecked) {
		monitor_zoom_ --;
		if (monitor_zoom_ < 0) {
			monitor_zoom_ = 0;
		}
		monitor_csv_[0].Update();
	}else if (RB_ResultsTradeoffGraph->IsChecked) {
		result_tradeoff_zoom_ --;
		if (result_tradeoff_zoom_ < 0) {
			result_tradeoff_zoom_ = 0;
		}
		tradeoff_csv_.Update();
	}else if (RB_ResultsWeight->IsChecked) {
		result_weight_zoom_ --;
		if (result_weight_zoom_ < 0) {
			result_weight_zoom_ = 0;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestContinueStructureSearchClick(TObject *Sender)
{
	ContinueStructureSearch();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_StructureSearchObjectiveChange(TObject *Sender)
{
	if (nn_document_.GetStructureSearchObjective() != (StructureSearchObjective)Co_StructureSearchObjective->ItemIndex) {
		nn_document_.SetStructureSearchObjective((StructureSearchObjective)Co_StructureSearchObjective->ItemIndex);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Me_MonitorKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	if (Key == 13) {
		Key = 0;
		Me_MonitorDblClick(Sender);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_OnlineDebugClick(TObject *Sender)
{
	online_debug_mode_ = true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIRN_OpenInNetworkTabWithWeightClick(TObject *Sender)
{
	if (OpenNetworkFromResult(true)) {
		Bu_NetworkTabClick(this);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_NetworkOverviewMouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, float X, float Y)
{
	if (!network_overview_){
		return;
	}

	Sc_NetworkX->Tag = 0;
	Sc_NetworkY->Tag = 0;
	Bu_Overview->SetFocus();

	X /= network_overview_current_zoom_;
	Y /= network_overview_current_zoom_;

	if (Button == TMouseButton::mbLeft) {
		const int last_id = network_overview_->GetCurrentLayerID();
		network_overview_->ClearSelection();
		SetCaptured((_di_IControl)*PB_NetworkOverview);
		const FListP<NNGraphItem> &graph = result_graph_mapper_.GetGraph();
		for (int i = graph.GetSize() - 1 ; i >= 0 ; i--) {
			const NNGraphItem &item = graph.GetAt(i);
			if (item.can_focus) {
				if (item.Contains(X, Y)){
					network_overview_->SetLayerSelected(item.id, true, false);
					network_overview_->SetCurrentLayerID(item.id);
					if (item.id != last_id) {
						result_weight_.Clear();
						result_weight_top_ = 0.0;
						Sc_Monitor->Max = 0;
					}
					break;
				}
			}
		}
	}
	PB_Monitor->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkOverviewMouseMove(TObject *Sender, TShiftState Shift,
		  float X, float Y)
{
	if (!network_overview_){
		return;
	}

	network_overview_last_x_ = X / network_overview_current_zoom_;
	network_overview_last_y_ = Y / network_overview_current_zoom_;

	if (Bu_Overview->IsFocused &&
		network_overview_current_zoom_ <= 1.0 &&
		network_overview_last_x_ < network_overview_->GetNetworkViewSizeX() && network_overview_last_y_ < network_overview_->GetNetworkViewSizeY()) {
        result_graph_mapper_.RequestRepaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_MonitorChange(TObject *Sender)
{
	if (Sc_Monitor->Tag >= 0) {
		Sc_Monitor->Tag = 1;
		result_weight_top_ = result_weight_current_top_ = Sc_Monitor->Value;
		PB_Monitor->Repaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_MonitorExit(TObject *Sender)
{
	Sc_Monitor->Tag = 0;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::RB_MonitorNormalizeClick(TObject *Sender)
{
	PB_Monitor->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::TB_MonitorGainChange(TObject *Sender)
{
	RB_MonitorGain->IsChecked = true;
	La_MonitorGain->Text = FormatFloat("0.00", exp(log(100.0) * TB_MonitorGain->Value / TB_MonitorGain->Max)) + " x";
	RB_MonitorNormalizeClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIRN_PlotWeightClick(TObject *Sender)
{
	if (!network_overview_){
		return;
	}

	result_weight_.Clear();
	result_weight_top_ = 0.0;
	Sc_Monitor->Max = 0;
	const NNLayer *layer = network_overview_->GetLayerByID(network_overview_->GetCurrentLayerID());
	if (layer) {
		int load_weight = false;

		if (nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla) {
			if (layer->GetType() == "Parameter") {
				load_weight = true;
			}else {
				int weight_property_index = layer->GetPropertyIndexByName("W.Initializer");
				if (weight_property_index >= 0) {
					load_weight = network_overview_->GetLayerPropertyVisible(*layer, weight_property_index);
				}
			}
		}
		FString result_dir = FString(AnsiString(GetResultDir()).c_str());
		if (load_weight) {
			AnsiString weight_file_name =
					GetResultDir() +
					UnicodeString("\\net\\");
			if (nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla) {
				//NNabla
				if (FileExists((result_dir + FString("\\results.nnp")).PChar())){
					ExtractNetwork(
						result_dir + FString("\\results"),
						result_dir + FString("\\net"), false);
				}else {
					ExtractNetwork(
						result_dir + FString("\\parameters"),
						result_dir + FString("\\net"), false);
				}
				if (layer->GetType() == "Parameter") {
					weight_file_name +=
						UnicodeString(layer->GetPropertyValue(FString("Name")).PChar()) +
						UnicodeString(".txt");
				}else {
					const NNLayerComponent *component = layer->GetComponent();
					weight_file_name +=
						UnicodeString(layer->GetDisplayPropertyValue(FString("ParameterScope")).PChar()) +
						UnicodeString("~") +
						UnicodeString(component->GetParameterScope().PChar()) +
						UnicodeString("~W.txt");
				}
			}
			if (result_weight_.Load(weight_file_name.c_str())){
				result_weight_.SetName(layer->GetPropertyValue(FString("Name")) + FString("->Weight"));
				result_weight_.RemoveSingleSizeDim();

				if (layer->GetType() == "Affine") {
					const int out_shape_property_index = layer->GetPropertyIndexByName("OutShape");
					if (out_shape_property_index >= 0) {
						const NNLayerProperty &out_shape_property = layer->GetProperty(out_shape_property_index);
						for (int i = 0 ; i < out_shape_property.GetIntArray().GetSize() ; i++){
							result_weight_.TransposeRight();
						}
					}
				}

				result_weight_.CalcContributionRatio();

				RB_ResultsWeight->IsChecked = true;
			}
		}
		PB_Monitor->Repaint();
	}
}
//---------------------------------------------------------------------------
int TFo_Main::DebugMessage(UnicodeString s, int test_pass)
{
	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	if (s != "") {
		UnicodeString pre_str;
		switch (test_pass){
		case 0: pre_str = "*   FAILED : "; break;
		case 1: pre_str = "    PASS   : "; break;
		case 2: pre_str = "    "; break;
		default:
			break;
		}
		s = pre_str + s;
		Me_Debug->Lines->Add(DateTimeToStr(Now()) + UnicodeString(" ") + s);

		AnsiString log_file_name = ExtractFilePath(ParamStr(0)) + AnsiString("settings\\log.txt");
		FILE *F = fopen(log_file_name.c_str(), "at");
		if (F){
			fprintf(F, "%s\n", AnsiString(FormatDateTime("[yyyy/mm/dd hh:nn:ss]  ", Now()) + s).c_str());
			fclose(F);
			F = NULL;
		}
	}else {
		Me_Debug->Lines->Add("");
	}
	Me_Debug->GoToTextEnd();
	Me_Debug->Repaint();
	Application->ProcessMessages();

	return test_pass;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestAllClick(TObject *Sender)
{
    //TODO: Torch Support
	AnsiString sdc_path = AnsiString(ExtractFilePath(ParamStr(0)));

	int test_count = 0;
	int error_count = 0;
	//Test sample project

	FStringList test_projects;
	test_projects.LoadFromFile((sdc_path + "settings\\test_projects\\projects.txt").c_str());

	FStringList result;

	for (int ip = 0 ; ip < test_projects.GetSize() && !closed_ ; ip++) if (test_projects.GetAt(ip) != "") {
		UnicodeString sample_project_files_path = sdc_path + AnsiString(test_projects.GetAt(ip).PChar()) + ".files";
		DeleteDir(AnsiString(sample_project_files_path).c_str());

		UnicodeString sample_project_file_name = sdc_path + AnsiString(test_projects.GetAt(ip).PChar()) + ".sdcproj";
		DebugMessage(UnicodeString("Test sample project : ") + sample_project_file_name, 2);
		test_count++;
		if (!DebugMessage(UnicodeString("load " + sample_project_file_name), OpenProject(sample_project_file_name))){
			error_count++;
		}
		for (int i = 0 ; i < nn_document_.GetNumDataset() ; i++) {
			nn_document_.GetDataset_(i).SetEnableCache(true); //Don't create new cache file while testing
		}
		if (Sender == MI_TestAll) {
			nn_document_.SetMaxEpoch(2);
		}
		const int nnabla = nn_document_.GetSDeepEngineType() == SDeepEngineType::NNabla;

		if (!closed_){
			control_button_type_ = ControlButtonType::RunTraining;
			Bu_ControlClick(Sender);
		}
		while (GetTraining()){
			Sleep(1);
			Application->ProcessMessages();
		}
		for (int i = 0 ; i < 10 ; i++) {
			Sleep(100);
			Application->ProcessMessages();
		}
		test_count++;
		if (!DebugMessage(UnicodeString("training"), Me_Monitor->Text.Pos("Training Completed") > 0)){
			error_count++;
		}
		const int result_index = nn_document_.GetCurrentResultIndex();
		if (result_index >= 0) {
			result.Add(FString((AnsiString(test_projects.GetAt(ip).PChar()) + "\t" +FloatToStr(nn_document_.GetResult(result_index).GetBestValidation())).c_str()));
		}


		const FString full_path = nn_document_.GetResult(nn_document_.GetCurrentResultIndex()).GetFullPath();

		if (!closed_){
			control_button_type_ = ControlButtonType::RunEvaluation;
			Bu_ControlClick(Sender);
		}
		while (sdeep_evaluation_thread_){
			Sleep(1);
			Application->ProcessMessages();
		}
		for (int i = 0 ; i < 10 ; i++) {
			Sleep(100);
			Application->ProcessMessages();
		}
		test_count++;
		if (!DebugMessage(UnicodeString("evaluation"),
						  Me_Evaluation->Text.Pos("Forward Calculation Completed") > 0 ||
						  Me_Evaluation->Text.Pos("Forward Completed") > 0)){
			error_count++;
		}

		if (!closed_){
			if (nnabla) {
				MIRN_OpenInNetworkTabWithWeightClick(Sender);
				UnicodeString generated_weight_dir = (full_path + FString("net")).PChar();
				const int success = DirectoryExists(generated_weight_dir) && TestCheckIfAllLayerHasInitialWeight();
				test_count++;
				if (!DebugMessage(UnicodeString("load weight"), success)){
					error_count++;
				}
			}
        }

		//Delete generated data
		DeleteDir((full_path + FString("net")).PChar());

		if (DirectoryExists((full_path + FString("0_0000")).PChar())) {
			//Auto Encoder
			DeleteDir((full_path + FString("0_0000")).PChar());
			DeleteFile((full_path + FString("output_result.csv")).PChar());
			DeleteFile((full_path + FString("evaluation_log.txt")).PChar());
		}
		if (DirectoryExists((full_path + FString("1_0000")).PChar())) {
			//RNN, etc
			DeleteDir((full_path + FString("1_0000")).PChar());
		}

		DebugMessage("");
	}
	DebugMessage(IntToStr(error_count) + UnicodeString("/") + IntToStr(test_count) + " error(s).", 2);

	if (Sender == MI_TestAllFull) {
        result.Add();
		for (int i = 0; i < Me_Debug->Lines->Count ; i++) {
			result.Add(FString(AnsiString(Me_Debug->Lines->Strings[i]).c_str()));
		}
		AnsiString fn = ExtractFilePath(ParamStr(0)) + "test.log";
		result.SaveToFile(fn.c_str());
    }

}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MI_DemoForMovieClick(TObject *Sender)
{
	AnsiString sdc_path = AnsiString(ExtractFilePath(ParamStr(0)));

	FStringList demo_projects;
	demo_projects.LoadFromFile((sdc_path + "dev_utils\\demo_projects.txt").c_str());
	FListP<NNStructure> structures;
	for (int ip = 0 ; ip < demo_projects.GetSize() ; ip++) if (demo_projects.GetAt(ip) != "") {
		FString demo_project_file_name = FString(sdc_path.c_str()) + demo_projects.GetAt(ip) + FString(".sdcproj");
		nn_document_.LoadFromFile(demo_project_file_name, FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()), false, false);
		structures.Add(nn_document_);
	}

	Ti_Animation->Enabled = false;

	double span = 30;
	double coef = 0.2;
	int frame_index = 0;
	for (int ip = 0 ; ip < structures.GetSize() ; ip++){
		*dynamic_cast<NNStructure*>(&nn_document_) = structures.GetAt(ip);
		nn_graph_mapper_.Update(nn_document_, true);

		for (int i = 0 ; i < span ; i++){
			nn_graph_mapper_.Animation(coef);
			PB_Network->Repaint();
			Application->HandleMessage();
			TBitmap *BMP = new TBitmap(1920, 1080);
			TCanvas *C = BMP->Canvas;
			C->BeginScene();
			C->Clear(0xffffffff);//white
			DrawNetworkBitmap(C, BMP->Width, BMP->Height, nn_graph_mapper_, 0.0, 0.0, 1.0);
			C->EndScene();
			BMP->SaveToFile((sdc_path + AnsiString("dev_utils\\movie\\") + IntToStr(frame_index++) + ".png").c_str());
			delete BMP;
		}
		span -= 1.0;
		coef = 1.0 - exp(log(0.001) / span);
	}
	Ti_Animation->Enabled = true;
}
//---------------------------------------------------------------------------
int TFo_Main::TestCheckIfAllLayerHasInitialWeight()
{
	int result = true;
	for (int il = 0 ; il < nn_document_.GetNumLayer() ; il++) {
		const NNLayer &layer = nn_document_.GetLayer(il);
		for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
			const NNLayerProperty &property = layer.GetProperty(ip);
			if (property.GetName().Pos(FString(".File"))) {
				if (nn_document_.GetLayerPropertyVisible(layer, ip)) {
                    int result_1 = true;
					const FStringList repeat_extensions = nn_document_.GetRepeatExtensions(layer, true);
					FString value = property.GetValue();
					for (int ir = 0; ir < repeat_extensions.GetSize() ; ir++) {
						FString value_1 = value.SplitBy(';');
						result_1 &= FileExists(value_1.PChar());
						const FString &ext = repeat_extensions.GetAt(ir).ReplaceStr("/", "~");
						if (ext != "") {
							result_1 &= value_1.Pos(ext) > 0;
						}
					}
					result_1 &= value == "";

					if (!result_1) {
						// This check will be failed if there is a loop in the unit
						DebugMessage((layer.GetDisplayPropertyValue(FString("Name")) + FString("_") + property.GetName() + FString(" is not loaded.")).PChar());
					}
				}
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_EvaluationSolveClick(TObject *Sender)
{
	if (evaluation_solve_dir_ != ""){
		DeleteDir(AnsiString(evaluation_solve_dir_).c_str());
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_BenchmarkClick(TObject *Sender)
{
	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	Me_Debug->Lines->Clear();

	unsigned int start = timeGetTime();
	FString s;
	static const int count = 100;

	Me_Debug->Lines->Add(UnicodeString("NetToStr"));
	for (int i = 0 ; i < count ; i++) {
		Me_Debug->Lines->Add(IntToStr(i + 1));
		s = nn_document_.NetworkToString(nn_document_);
		Application->ProcessMessages();
	}
	unsigned int read_start = timeGetTime();
	Me_Debug->Lines->Add(UnicodeString("StrToNet"));
	for (int i = 0 ; i < count ; i++) {
		Me_Debug->Lines->Add(IntToStr(i + 1));
		NNStructure network;
		nn_document_.StringToNetwork(s, network);
		Application->ProcessMessages();
	}
	unsigned int end = timeGetTime();
	Me_Debug->Lines->Add(UnicodeString("NetToStr(ms)\t") + IntToStr((int)(read_start - start)));
	Me_Debug->Lines->Add(UnicodeString("StrToNet(ms)\t") + IntToStr((int)(end - read_start)));
}
//---------------------------------------------------------------------------
double gaussian_rand()
{
	double l = -6.0;
	for (int i = 0 ; i < 12 ; i++) {
		l += rand() * 1.0 / RAND_MAX;
	}
	return l;
}
void __fastcall TFo_Main::MI_RandomZGenerationClick(TObject *Sender)
{
	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	Me_Debug->Lines->Clear();

	static const int dim = 64;
	static const int num_rand = 500;
	static const int interval = 30;
	static const int gauss = true;
	float z[dim], next_z[dim];

	for (int id = 0 ; id < dim ; id++) {
		if (gauss) {
			z[id] = gaussian_rand();
			next_z[id] = gaussian_rand();
		}else {
			z[id] = rand() * 1.0 / RAND_MAX;
			next_z[id] = rand() * 1.0 / RAND_MAX;
		}
	}

	for (int i = 0 ; i < num_rand ; i++) {
		for (int it = 0 ; it < interval ; it++) {
			UnicodeString s;
			for (int id = 0 ; id < dim ; id++) {
				const double pos = it * 1.0 / interval;
				if (id > 0) {
					s += ",";
				}
				s += FloatToStr(z[id] * (1.0 - pos) + next_z[id] * pos);
			}
			Me_Debug->Lines->Add(s);
		}

		memcpy(z, next_z, sizeof(z));
		for (int id = 0 ; id < dim ; id++) {
			if (gauss) {
				next_z[id] = gaussian_rand();
			}else {
				next_z[id] = rand() * 1.0 / RAND_MAX;
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ch_SaveBestChange(TObject *Sender)
{
	if (nn_document_.GetSaveBest() != Ch_SaveBest->IsChecked) {
		nn_document_.SetSaveBest(Ch_SaveBest->IsChecked);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ProcessMessages()
{
	static unsigned __int64 last = 0;
	unsigned __int64 time = timeGetTime();
	if (last < time - 250) {
		last = time;
		Application->ProcessMessages();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_PropertyDblClick(TObject *Sender)
{
	//Open File if property type is NNLayerPropertyType::File
	if (nn_document_.GetCurrentLayerID() != current_property_layer_id_) {
		return;
	}

	if (nn_document_.GetCurrentPropertyIndex() != current_property_index_) {
		return;
	}

	if (Lo_EditProperty->Visible || (Co_Property->Visible && Co_Property->Selected)) {
		const NNLayer *layer = nn_document_.GetLayerByID(current_property_layer_id_);
		if (layer) {
			const NNLayerProperty &property = layer->GetProperty(current_property_index_);
			if (property.GetType() == NNLayerPropertyType::File) {
				OpenFile(property.GetValue().PChar());
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_RemoveDeadLinksClick(TObject *Sender)
{
	TFr_List *list = NULL;
	if (TC_Default->ActiveTab == TI_Browser && Fr_ProjectFiles->Visible) {
		list = Fr_ProjectFiles;
	}else if (TC_Default->ActiveTab == TI_Browser && Fr_DatasetFiles->Visible) {
		list = Fr_DatasetFiles;
	}else if (TC_Default->ActiveTab == TI_Dataset && Fr_DatasetFiles2->Visible) {
		list = Fr_DatasetFiles2;
	}else {
		return;
	}

	FStringList dead_links;
	FListT<int> dead_link_indices;
	for (int i = 0; i < list->GetItems().GetSize() ; i++) {
		if (list->GetItem(i).GetRemovable()) {
			FString file_name =
				list->GetItem(i).GetAttribute(FString("Location")) +
				list->GetItem(i).GetAttribute(FString("Name"));
			if (!FileExists(file_name.PChar())) {
				dead_links.Add(file_name);
				dead_link_indices.Add(list->GetItem(i).GetOriginalIndex());
			}
		}
	}

	if (!dead_link_indices.GetSize()) {
		ShowMessage(GetTranslatedText("No dead links are found."));
	}else {
		int result = MessageDlg(IntToStr(dead_link_indices.GetSize()) + GetTranslatedText(" dead links are found. Are you sure you want to proceed?"), TMsgDlgType::mtWarning, mbYesNoCancel, 0);
		if (result == mrCancel) {
			return;
		} if (result == mrYes) {
			for (int i = dead_link_indices.GetSize() - 1 ; i >= 0 ; i--) {
				int index = dead_link_indices.GetAt(i);
				if (list == Fr_ProjectFiles){
					project_list_.Delete(index);
				}else if (list == Fr_DatasetFiles || list == Fr_DatasetFiles2){
					dataset_list_.Delete(index);
				}
			}
		}
		if (list == Fr_ProjectFiles){
			project_list_.SaveToFile();
			RefreshProjectFiles();
		}else if (list == Fr_DatasetFiles || list == Fr_DatasetFiles2){
			dataset_list_.SaveToFile();
			RefreshDatasetFiles();
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::SetScale(int gui_scale)
{
	const float scale = settings_.GUIScaleToGUIScaleFactor(gui_scale);
	Fo_Main->FormLayout->Scale->X = scale;
	Fo_Main->FormLayout->Scale->Y = scale;
	Fo_Main->FormLayout->Align = TAlignLayout::None;
	Fo_Main->FormLayout->Align = TAlignLayout::Client;
	Fo_Main->TC_Default->Align = TAlignLayout::None;
	Fo_Main->TC_Default->Align = TAlignLayout::Client;
	Fo_Main->FormResize(this);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_TestDummyImageDatasetClick(TObject *Sender)
{
	if (!SD_CSV->Execute()){
		return;
	}

	static const int data_width = 64, data_height = 64;

	FStringList csv;
	csv.Add(FString("x:in,y:out"));
	UnicodeString data_path = SD_CSV->FileName + UnicodeString(".image\\");
	for (int i = 0 ; i < 40000 ; i++) {
		UnicodeString in_image_file_name = data_path + UnicodeString("In_") + IntToStr(i) + UnicodeString(".png");
		if (i == 0) {
			ForceDirectories(ExtractFileDir(in_image_file_name));
		}
		FString text;
		for (int it = 0; it < 3; it++){
			text += FString("ABCDEFGHIJKLMNOPQRSTUVWXYZ").SubString(rand() % 26 + 1, 1);
		}
		const int text_pos_x = rand() % 20 + 4;
		const int text_pos_y = rand() % 20 + 8;
		const int text_size = rand() % 8 + 16;
		{
			TBitmap *BMP = new TBitmap(data_width, data_height);
			//BMP->PixelFormat = TPixelFormat::L;
			TBitmapData BMPData;
			BMP->Map(TMapAccess::Write, BMPData);
			double current_value = rand() * 1.0 / RAND_MAX;
			double next_value = rand() * 1.0 / RAND_MAX;
			const int offset = rand() % 33 - 16;
			int length = rand() % 32 + 1;
			int pos = 0;
			for (int iy = 0; iy < data_height ; iy++) {
				for (int ix = 0; ix < data_width ; ix++) {
					float value = next_value * (pos * 1.0 / length) + current_value * (1.0 - pos * 1.0 / length);
					const int ii = (int)(120 + offset + value * 16);
					const int color = 0xff000000 | (ii << 16) | (ii << 8) | ii;
					BMPData.SetPixel(ix, iy, color);
					pos++;
					if (pos >= length) {
						current_value = next_value;
						pos = 0;
						next_value = rand() * 1.0 / RAND_MAX;
						length = rand() % 32 + 1;
					}
				}
			}
			BMP->Unmap(BMPData);
			BMP->Canvas->BeginScene();
			BMP->Canvas->Font->Size = text_size;
			for (int it = 0; it < 2 ; it++) {
				const int ii = (int)(120 + rand() % 64);
				const int color = 0xff000000 | (ii << 16) | (ii << 8) | ii;
				BMP->Canvas->Fill->Color = color;
				BMP->Canvas->Fill->Kind = TBrushKind::Solid;

				BMP->Canvas->FillText(
					Rect(text_pos_x + (1 - it) * (rand() % 2 + 1), text_pos_y + (1 - it) * (rand() % 2 + 1), data_width, data_height),
					text.PChar(),
					false,
					0.8f,
					TFillTextFlags(),
					TTextAlign::Leading,
					TTextAlign::Leading);
			};
			BMP->Canvas->EndScene();
			BMP->SaveToFile(in_image_file_name);
			delete BMP;
		}
		UnicodeString out_image_file_name = data_path + UnicodeString("Out_") + IntToStr(i) + UnicodeString(".png");
		{
			TBitmap *BMP = new TBitmap(data_width, data_height);
			BMP->Canvas->BeginScene();
            BMP->Canvas->Clear(0xff000000);
			BMP->Canvas->Font->Size = text_size;
			BMP->Canvas->Fill->Color = 0xffffffff;
			BMP->Canvas->Fill->Kind = TBrushKind::Solid;

			BMP->Canvas->FillText(
				Rect(text_pos_x, text_pos_y, data_width, data_height),
				text.PChar(),
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Leading);
			BMP->Canvas->EndScene();
			BMP->SaveToFile(out_image_file_name);
			delete BMP;
		}
		csv.Add(FString(AnsiString(UnicodeString("\"") + in_image_file_name + UnicodeString("\",\"") + out_image_file_name + UnicodeString("\"")).c_str()));
	}
	csv.SaveToFile(AnsiString(SD_CSV->FileName).c_str());
}
//---------------------------------------------------------------------------
void TFo_Main::TrainingResultRename(int index, FString name)
{
	const ListItem &list_item = Fr_ResultsList->GetItem(index);
	if (list_item.GetAttributeValue(0) == name) {
		return;
	}

	bool renamed = true;
	static const FString chars = FString("¥\\/:*?\"<>|=");
	for (int i = 1 ; i <= chars.Length() ; i++){
		if (name.Pos(chars.SubString(i, 1)) > 0){
			renamed = false;
			break;
		}
	}

	//Rename
	if (renamed) {
		FString original_name = nn_document_.GetResultPath(FString("\\")) + list_item.GetAttributeValue(0);
		FString dest_name = nn_document_.GetResultPath(FString("\\")) + name;
		renamed = RenameDir(original_name.PChar(), dest_name.PChar());
	}
	if (renamed) {
		nn_document_.RefreshResults(FString("\\"), false);
		RefreshResultsList(false);
		OpenResult();
		OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
	}else if (!renamed) {
		ShowMessage(FString(
			GetTranslatedFString("Cannot rename") + FString(" \"") +
			list_item.GetAttributeValue(0) +
			FString("\" to \"") +
			name + FString("\".")
			).PChar());
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_RenameClick(TObject *Sender)
{
	TFr_List *list = NULL;
	if (TC_Default->ActiveTab == TI_Browser && Fr_ProjectFiles->Visible) {
		list = Fr_ProjectFiles;
	}else if ((TC_Default->ActiveTab == TI_Browser || TC_Default->ActiveTab == TI_Dataset) &&
			  Fr_DatasetFiles->Visible) {
		list = Fr_DatasetFiles;
	}else if (TC_Left->ActiveTab == TI_TrainingLeft){
		list = Fr_ResultsList;
	}else {
		return;
	}

	list->StartRenameListItem(list->GetItemIndex());
	Invalidate();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIN_ImportSDCPRJClick(TObject *Sender)
{
	if (!CheckSave()){
		return;
	}
	if (!AskAndTerminateProcess()){
		return;
	}
	if (OD_Project->Execute()){
		nn_document_.LoadFromFile(
			FString(AnsiString(OD_Project->FileName).c_str()),
			FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()),
			true);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MINE_PythonCodeNNablaClick(TObject *Sender)
{
	if (SetClipboardText(GetNNablaPythonCode(nn_document_).GetText())){
		ShowMessage(GetTranslatedText("The generated code was copied to the clipboard."));
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkTabPaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_NetworkTab->Canvas;
	C->BeginScene();
	C->SetMatrix(PB_NetworkTab->AbsoluteMatrix);
	C->Clear(SDEEP_CONSOLE_COLOR_BACKGROUND);

	//Add button
	C->Stroke->Color = SDEEP_CONSOLE_COLOR_UIFG;
	C->Stroke->Thickness = 1;
	TRectF rect = network_tab_add_rect_;
	if (network_tab_index_2_ == nn_document_.GetNumStructure()){
		C->Fill->Color = SDEEP_CONSOLE_COLOR_UIBG;
		C->Fill->Kind = TBrushKind::Solid;
		C->FillRect(rect, 2.0f, 2.0f, AllCorners, 1.0f, TCornerType::Round);
	}

	//Draw add button
	TRect text_rect = Rect(rect.left, 0, rect.right, PB_NetworkTab->Height - 3);
	C->Fill->Color = network_tab_index_2_ == nn_document_.GetNumStructure() ? SDEEP_CONSOLE_COLOR_FOREGROUND : SDEEP_CONSOLE_COLOR_HALF;
	C->Fill->Kind = TBrushKind::Solid;
	C->Font->Size = 16;
	C->Font->Style = TFontStyles() << TFontStyle::fsBold;
	C->FillText(
		text_rect,
		"+",
		false,
		1.0f,
		TFillTextFlags(),
		TTextAlign::Center,
		TTextAlign::Center
		);

	network_tab_delete_rect_.Clear();
	for (int i = 0; i < nn_document_.GetNumStructure() && i < network_tab_rect_.GetSize() ; i++) {
		//Draw delete button
		if (i == nn_document_.GetActiveStructureIndex()){
			TRect delete_rect = Rect(network_tab_rect_.GetAt(i).right - PB_NetworkTab->Height + 20, 12, network_tab_rect_.GetAt(i).right - 12, PB_NetworkTab->Height - 15);
			network_tab_delete_rect_.Add(delete_rect);
		}else {
			TRect delete_rect = Rect(-1, -1, -1, -1);
			network_tab_delete_rect_.Add(delete_rect);
		}
	}

	for (int i = 0; i < nn_document_.GetNumStructure() ; i++) {
		//Draw active structure at last
		const int index = i < nn_document_.GetActiveStructureIndex() ? i :
			(i < nn_document_.GetNumStructure() - 1 ? i + 1 : nn_document_.GetActiveStructureIndex());
		if (index < nn_document_.GetNumStructure() && index < network_tab_rect_.GetSize()){
			const NNStructure &structure = nn_document_.GetStructure(index);
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_UIFG;
			C->Stroke->Thickness = 1;
			TRectF rect = network_tab_rect_.GetAt(index);
			if (network_tab_index_2_ == index) {
				C->Fill->Color = SDEEP_CONSOLE_COLOR_UIBG;
				C->Fill->Kind = TBrushKind::Solid;
				C->FillRect(rect, 0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			}

			//Draw tab name
			TRect text_rect = Rect(rect.left + 16, 0, rect.right - 16 - (index == nn_document_.GetActiveStructureIndex()) * (PB_NetworkTab->Height - 32), PB_NetworkTab->Height);
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			//C->Fill->Color = index == nn_document_.GetActiveStructureIndex() ? SDEEP_CONSOLE_COLOR_UICOLORFG : SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->Fill->Kind = TBrushKind::Solid;
			C->Font->Size = 16;
			//C->Font->Style = structure.GetName() == "main" ? TFontStyles() << TFontStyle::fsBold : TFontStyles();
			C->Font->Style = TFontStyles();
			C->FillText(
				text_rect,
				structure.GetName().PChar(),
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center
				);

			//Draw delete button
			if (index == nn_document_.GetActiveStructureIndex()){
				C->Fill->Color = (network_tab_index_2_ == index && network_tab_delete_focus_) ? SDEEP_CONSOLE_COLOR_FOREGROUND : SDEEP_CONSOLE_COLOR_HALF;
				C->Font->Style = TFontStyles() << TFontStyle::fsBold;
				C->FillText(
					network_tab_delete_rect_.GetAt(index),
					"x",
					false,
					1.0f,
					TFillTextFlags(),
					TTextAlign::Center,
					TTextAlign::Center
					);
			}

			//Draw selection rect
			if (index == nn_document_.GetActiveStructureIndex()){
				TRect selection_rect = Rect(rect.left, rect.bottom - 4, rect.right, rect.bottom);
				C->Fill->Color = SDEEP_CONSOLE_COLOR_BRAND;
				//	(network_tab_index_2_ == index ? SDEEP_CONSOLE_COLOR_UIFG : SDEEP_CONSOLE_COLOR_UIBG);
				C->FillRect(selection_rect, 0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			}
		}
	}

	C->EndScene();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_UpdateIntervalChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const int value = StrToIntDef(Ed_UpdateInterval->Text, nn_document_.GetOptimizer(optimizer_index).GetUpdateInterval());
	if (nn_document_.GetOptimizer(optimizer_index).GetUpdateInterval() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetUpdateInterval(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_LRUpdateIntervalChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const int value = StrToFloatDef(Ed_LRUpdateInterval->Text, nn_document_.GetOptimizer(optimizer_index).GetLearningRateUpdateInterval());
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateUpdateInterval() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateUpdateInterval(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Co_UpdaterChange(TObject *Sender)
{
	if (Co_Updater->ItemIndex >= 0) {
		const int optimizer_index = TI_OptimizerConfig->Tag;
		const FString value = FString(AnsiString(Co_Updater->Items->Strings[Co_Updater->ItemIndex]).c_str());
		if (nn_document_.GetOptimizer(optimizer_index).GetSolverName() != value) {
			nn_document_.GetOptimizer(optimizer_index).SetSolver(value);
			RefreshConfigList();
			nn_document_.UpdateConfig();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_WeightDecayChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const double value = StrToFloatDef(Ed_WeightDecay->Text, nn_document_.GetOptimizer(optimizer_index).GetWeightDecay());
	if (nn_document_.GetOptimizer(optimizer_index).GetWeightDecay() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetWeightDecay(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_NetworkTabMouseMove(TObject *Sender, TShiftState Shift,
		  float X, float Y)
{
	if (PB_NetworkTab->Tag){
		PB_NetworkTab->Tag = PB_NetworkTab->Tag + 1;//Mouse moved
		if (drag_network_tab_ && nn_document_.GetActiveStructureIndex() < network_tab_rect_.GetSize()) {
			drag_network_tab_x_shift_ = X - drag_network_tab_start_x_;
			TRectF rect = network_tab_rect_.GetAt(0);
			if (rect.Width() * nn_document_.GetActiveStructureIndex() + drag_network_tab_x_shift_ < 0){
				drag_network_tab_x_shift_ = -rect.Width() * nn_document_.GetActiveStructureIndex();
			}
			if (rect.Width() * nn_document_.GetActiveStructureIndex() + drag_network_tab_x_shift_ > rect.Width() * (nn_document_.GetNumStructure() - 1)){
				drag_network_tab_x_shift_ = rect.Width() * (nn_document_.GetNumStructure() - 1) - rect.Width() * nn_document_.GetActiveStructureIndex();
			}
		}
	}else {
		if (network_tab_rect_.GetSize() == nn_document_.GetNumStructure() &&
			network_tab_delete_rect_.GetSize() == nn_document_.GetNumStructure()){
			network_tab_index_2_ = -1;
			network_tab_delete_focus_ = false;
			for (int i = 0; i < network_tab_rect_.GetSize() && i < network_tab_delete_rect_.GetSize() ; i++) {
				if (network_tab_rect_.GetAt(i).Contains(PointF(X, Y))){
					network_tab_index_2_ = i;
					if (network_tab_delete_rect_.GetAt(i).Contains(PointF(X, Y))){
						network_tab_delete_focus_ = true;
					}
					break;
				}
			}
			if (network_tab_add_rect_.Contains(PointF(X,Y))) {
				network_tab_index_2_ = nn_document_.GetNumStructure();
			}

			PB_NetworkTab->Repaint();
        }
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_ForceCompleteClick(TObject *Sender)
{
	if (nn_document_.GetCurrentResultIndex() < 0) {
		return;
	}

	int result = MessageDlg(
		GetTranslatedText("You are going to make this result complete. Are you sure you want to proceed?"),
		TMsgDlgType::mtWarning, mbOKCancel, 0);
	if (result == mrCancel) {
		return;
	}


	UnicodeString result_dir = GetResultDir();
	if (DirectoryExists(result_dir)) {
		UnicodeString result_file_name = result_dir + UnicodeString("\\result.ini");
		FFastIni ini = FFastIni(AnsiString(result_file_name).c_str());
		ini.Write<int>("result", "complete", 1);
	}

	const int last_result_index = nn_document_.GetCurrentResultIndex();
	nn_document_.RefreshResults(FString("\\"));

	if (TC_Left->ActiveTab == TI_TrainingLeft) {
		if (TC_Default->Visible) {
			RefreshResultsList(false);
			OpenResult();
			OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
		}
		if (last_result_index != nn_document_.GetCurrentResultIndex()) {
			Fr_ResultsList->SetItemIndex(nn_document_.GetCurrentResultIndex());
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_NumEvaluationsChange(TObject *Sender)
{
	const int executor_index = TI_ExecutorConfig->Tag;
	const int value = StrToIntDef(Ed_NumEvaluations->Text, 1);
	if (nn_document_.GetExecutor(executor_index).GetNumEvaluations() != value) {
		nn_document_.GetExecutor(executor_index).SetNumEvaluations(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MI_CompareWithCPUClick(TObject *Sender)
{
	nn_document_.SetCommandType(SDeepCommandType::CompareWithCPU);
	Bu_ControlClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Fr_DatasetListBu_ListKeyDown(TObject *Sender, WORD &Key,
		  System::WideChar &KeyChar, TShiftState Shift)
{
	Fr_DatasetList->Bu_ListKeyDown(Sender, Key, KeyChar, Shift);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ch_DatasetMainClick(TObject *Sender)
{
	const int dataset_index = Fr_DatasetList->GetItemIndex();
	if (dataset_index >= 0  && dataset_index < nn_document_.GetNumDataset() && nn_document_.GetMainDatasetIndex() != dataset_index) {
		nn_document_.SetMainDatasetIndex(dataset_index);
		nn_document_.UpdateConfig();
		RefreshDatasetList();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIDL_DeleteDatasetClick(TObject *Sender)
{
	if (nn_document_.DeleteDataset(Fr_DatasetList->GetItemIndex())){
		nn_document_.UpdateConfig();
		RefreshDatasetList();
	}
}
//---------------------------------------------------------------------------
void TFo_Main::DatasetItemDelete(int index)
{
	MIDL_DeleteDatasetClick(this);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIDL_AddDatasetClick(TObject *Sender)
{
	nn_document_.AddDataset();
	nn_document_.UpdateConfig();
	RefreshDatasetList();
	Fr_DatasetList->SetItemIndex(nn_document_.GetNumDataset() - 1);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIDL_RenameDatasetClick(TObject *Sender)
{
	Fr_DatasetList->StartRenameListItem(Fr_DatasetList->GetItemIndex());
	Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Fr_DatasetListSc_ListYChange(TObject *Sender)
{
	Fr_DatasetList->Sc_ListYChange(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_OptimizerNetworkChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const FString value = FString(AnsiString(Ed_OptimizerNetwork->Text).c_str());
	if (nn_document_.GetOptimizer(optimizer_index).GetNetworkName() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetNetworkName(value);
		nn_document_.UpdateConfig();
		RefreshConfigList();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_OptimizerDatasetChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	FStringList dataset_names;
	dataset_names.SplitStr(FString(AnsiString(Ed_OptimizerDataset->Text).c_str()), FString(","), FString(""));
	if (nn_document_.GetOptimizer(optimizer_index).GetDatasetNames() != dataset_names) {
		nn_document_.GetOptimizer(optimizer_index).SetDatasetNames(dataset_names);
		nn_document_.UpdateConfig();
		RefreshConfigList();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_MonitorNetworkChange(TObject *Sender)
{
	const int monitor_index = TI_MonitorConfig->Tag;
	const FString value = FString(AnsiString(Ed_MonitorNetwork->Text).c_str());
	if (nn_document_.GetMonitor(monitor_index).GetNetworkName() != value) {
		nn_document_.GetMonitor(monitor_index).SetNetworkName(value);
		nn_document_.UpdateConfig();
		RefreshConfigList();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_MonitorDatasetChange(TObject *Sender)
{
	const int monitor_index = TI_MonitorConfig->Tag;
	FStringList dataset_names;
	dataset_names.SplitStr(FString(AnsiString(Ed_MonitorDataset->Text).c_str()), FString(","), FString(""));
	if (nn_document_.GetMonitor(monitor_index).GetDatasetNames() != dataset_names) {
		nn_document_.GetMonitor(monitor_index).SetDatasetNames(dataset_names);
		RefreshConfigList();
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_ExecutorNetworkChange(TObject *Sender)
{
	const int executor_index = TI_ExecutorConfig->Tag;
	const FString value = FString(AnsiString(Ed_ExecutorNetwork->Text).c_str());
	if (nn_document_.GetExecutor(executor_index).GetNetworkName() != value) {
		nn_document_.GetExecutor(executor_index).SetNetworkName(value);
        nn_document_.UpdateConfig();
		RefreshConfigList();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_ExecutorDatasetChange(TObject *Sender)
{
	const int executor_index = TI_ExecutorConfig->Tag;
	FStringList dataset_names;
	dataset_names.SplitStr(FString(AnsiString(Ed_ExecutorDataset->Text).c_str()), FString(","), FString(""));
	if (nn_document_.GetExecutor(executor_index).GetDatasetNames() != dataset_names) {
		nn_document_.GetExecutor(executor_index).SetDatasetNames(dataset_names);
		nn_document_.UpdateConfig();
		RefreshConfigList();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MICL_RenameClick(TObject *Sender)
{
	Fr_ConfigList->StartRenameListItem(Fr_ConfigList->GetItemIndex());
	Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MICL_AddOptimizerClick(TObject *Sender)
{
	Fr_ConfigList->SetFocus();
	nn_document_.AddOptimizer();
	nn_document_.UpdateConfig();
	RefreshConfigList();
	int item_index = 0;
	for (int i = 0 ; i < config_type_.GetSize() ; i++) {
		if (config_type_.GetAt(i) == ConfigType::Optimizer) {
			item_index = i;
		}

	}
	if (item_index >= 0) {
		Fr_ConfigList->SetItemIndex(item_index);
		RefreshConfigTab(item_index);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MICL_AddMonitorClick(TObject *Sender)
{
	Fr_ConfigList->SetFocus();
	nn_document_.AddMonitor_();
	nn_document_.UpdateConfig();
	RefreshConfigList();
	int item_index = 0;
	for (int i = 0 ; i < config_type_.GetSize() ; i++) {
		if (config_type_.GetAt(i) == ConfigType::Monitor) {
			item_index = i;
		}

	}
	if (item_index >= 0) {
		Fr_ConfigList->SetItemIndex(item_index);
		RefreshConfigTab(item_index);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MICL_AddExecutorClick(TObject *Sender)
{
	Fr_ConfigList->SetFocus();
	nn_document_.AddExecutor();
	nn_document_.UpdateConfig();
	RefreshConfigList();
	int item_index = 0;
	for (int i = 0 ; i < config_type_.GetSize() ; i++) {
		if (config_type_.GetAt(i) == ConfigType::Executor) {
			item_index = i;
		}

	}
	if (item_index >= 0) {
		Fr_ConfigList->SetItemIndex(item_index);
		RefreshConfigTab(item_index);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MICL_DeleteClick(TObject *Sender)
{
	Fr_ConfigList->SetFocus();
	int item_index = Fr_ConfigList->GetItemIndex();
	if (item_index < 0 || item_index >= config_type_.GetSize()) {
		return;
	}
	const ListItem &item = Fr_ConfigList->GetItem(item_index);
	const int index = item.GetOriginalIndex();
	switch (config_type_.GetAt(item_index)) {
	case ConfigType::Global:
		break;
	case ConfigType::Optimizer:
		nn_document_.DeleteOptimizer(index);
		break;
	case ConfigType::Monitor:
		// nn_document_.DeleteMonitor_(index);
		break;
	case ConfigType::Executor:
		nn_document_.DeleteExecutor(index);
		break;
	default:
		;
	}
	nn_document_.UpdateConfig();
	RefreshConfigList();
	RefreshConfigTab(Fr_ConfigList->GetItemIndex());
}
//---------------------------------------------------------------------------
void TFo_Main::ConfigItemDelete(int index)
{
	MICL_DeleteClick(this);
}
//---------------------------------------------------------------------------
void TFo_Main::ConfigItemRename(int item_index, FString name)
{
	if (name == ""){
		return;
	}
	static const FString allowed_chars = FString("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-");
	for (int i = 1 ; i <= name.Length() ; i++){
		if (allowed_chars.Pos(name.SubString(i, 1)) <= 0){
			return;
		}
	}

	if (item_index < 0 || item_index >= config_type_.GetSize()) {
		return;
	}
	const ListItem &item = Fr_ConfigList->GetItem(item_index);
	const int index = item.GetOriginalIndex();
	switch (config_type_.GetAt(item_index)) {
	case ConfigType::Global:
		break;
	case ConfigType::Optimizer:
		if (nn_document_.GetOptimizerIndexByName(name) < 0){
			if (nn_document_.GetOptimizer(index).GetOptimizerName() != name) {
				nn_document_.GetOptimizer(index).SetOptimizerName(name);
				nn_document_.UpdateConfig();
			}
		}
		break;
	case ConfigType::Monitor:
		//nn_document_.GetMonitor(index).SetMonitorName(name);
		break;
	case ConfigType::Executor:
		if (nn_document_.GetExecutorIndexByName(name) < 0){
			if (nn_document_.GetExecutor(index).GetExecutorName() != name) {
				nn_document_.GetExecutor(index).SetExecutorName(name);
				nn_document_.UpdateConfig();
			}
		}
		break;
	default:
		;
	}
	RefreshConfigList();
	RefreshConfigTab(Fr_ConfigList->GetItemIndex());
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ch_ExecutorNeedBackPropagationChange(TObject *Sender)
{
	const int executor_index = TI_ExecutorConfig->Tag;
	if (nn_document_.GetExecutor(executor_index).GetNeedBackPropagation() != Ch_ExecutorNeedBackPropagation->IsChecked) {
		nn_document_.GetExecutor(executor_index).SetNeedBackPropagation(Ch_ExecutorNeedBackPropagation->IsChecked);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ch_DatasetShuffleChange(TObject *Sender)
{
	const int dataset_index = Fr_DatasetList->GetItemIndex();
	if (dataset_index >= 0  && dataset_index < nn_document_.GetNumDataset()) {
		NNDataset &dataset = nn_document_.GetDataset_(dataset_index);
		if (dataset.GetShuffle() != Ch_DatasetShuffle->IsChecked) {
			dataset.SetShuffle(Ch_DatasetShuffle->IsChecked);
			nn_document_.UpdateConfig();
			RefreshDatasetList();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ch_DatasetEnableCacheChange(TObject *Sender)
{
	const int dataset_index = Fr_DatasetList->GetItemIndex();
	if (dataset_index >= 0  && dataset_index < nn_document_.GetNumDataset()) {
		NNDataset &dataset = nn_document_.GetDataset_(dataset_index);
		if (dataset.GetEnableCache() != Ch_DatasetEnableCache->IsChecked) {
			dataset.SetEnableCache(Ch_DatasetEnableCache->IsChecked);
			nn_document_.UpdateConfig();
			RefreshDatasetList();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ch_DatasetImageNormalizationChange(TObject *Sender)
{
	const int dataset_index = Fr_DatasetList->GetItemIndex();
	if (dataset_index >= 0  && dataset_index < nn_document_.GetNumDataset()) {
		NNDataset &dataset = nn_document_.GetDataset_(dataset_index);
		if (dataset.GetImageNormalization() != Ch_DatasetImageNormalization->IsChecked) {
			dataset.SetImageNormalization(Ch_DatasetImageNormalization->IsChecked);
			nn_document_.UpdateConfig();
			RefreshDatasetList();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::RB_ExecutorMeanChange(TObject *Sender)
{
	if (RB_ExecutorMean->IsChecked) {
		const int executor_index = TI_ExecutorConfig->Tag;
		if (nn_document_.GetExecutor(executor_index).GetRepeatEvaluationType() != 0) {
			nn_document_.GetExecutor(executor_index).SetRepeatEvaluationType(0);
			nn_document_.UpdateConfig();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::RB_ExecutorStdChange(TObject *Sender)
{
	if (RB_ExecutorStd->IsChecked) {
		const int executor_index = TI_ExecutorConfig->Tag;
		if (nn_document_.GetExecutor(executor_index).GetRepeatEvaluationType() != 2) {
			nn_document_.GetExecutor(executor_index).SetRepeatEvaluationType(2);
			nn_document_.UpdateConfig();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::RB_ExecutorLastChange(TObject *Sender)
{
	if (RB_ExecutorLast->IsChecked) {
		const int executor_index = TI_ExecutorConfig->Tag;
		if (nn_document_.GetExecutor(executor_index).GetRepeatEvaluationType() != 1) {
			nn_document_.GetExecutor(executor_index).SetRepeatEvaluationType(1);
			nn_document_.UpdateConfig();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_PropertyExit(TObject *Sender)
{
	Sc_Property->Tag = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Sc_PropertyChange(TObject *Sender)
{
	if (Sc_Property->Tag >= 0) {
		Sc_Property->Tag = 1;
		property_current_top_ = property_top_ = (Sc_Property->Value * visible_property_num_ * default_property_height) / Sc_Property->Max;
		ClearEditBox();
		Bu_Property->SetFocus();
		PB_Property->Repaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_CancelAllScheduledTaskClick(TObject *Sender)
{
	for (int ir = 0 ; ir < nn_document_.GetResults(). GetSize() ; ir++) {
		nn_document_.SetCurrentResultIndex(ir);
		UnicodeString result_dir = GetResultDir();
		if (DirectoryExists(result_dir)) {
			UnicodeString result_file_name = result_dir + UnicodeString("\\result.ini");
			FFastIni ini = FFastIni(AnsiString(result_file_name).c_str());
			ini.Write<int>("result", "schedule", 0);
		}
	}

	nn_document_.RefreshResults(FString("\\"));
	RefreshResultsList(false);
	OpenResult();
	OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILR_ResumeScheduledTaskClick(TObject *Sender)
{
	Ch_StructureSearchEnabled->IsChecked = false;
	TrainScheduled();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MILR_CancelScheduleClick(TObject *Sender)
{
	if (nn_document_.GetCurrentResultIndex() < 0) {
		return;
	}

	UnicodeString result_dir = GetResultDir();
	if (DirectoryExists(result_dir)) {
		UnicodeString result_file_name = result_dir + UnicodeString("\\result.ini");
		FFastIni ini = FFastIni(AnsiString(result_file_name).c_str());
		ini.Write<int>("result", "schedule", 0);
	}

	nn_document_.RefreshResults(FString("\\"));
	RefreshResultsList(false);
	OpenResult();
	OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
}
//---------------------------------------------------------------------------
void TFo_Main::CheckDatasetRequirement()
{
	int exist = true;
	for (int id = 0 ; id < nn_document_.GetNumDataset() ; id++){
		exist &= nn_document_.GetDataset(id).GetLoaded();
	}
	if (exist) {
        return;
	}

	FStringList sl;
	sl.SplitStr(nn_document_.GetDescription(), "\\n");
	for (int i = 0; i < sl.GetSize() ; i++) {
		const FString &s = sl.GetAt(i);
		if (s.Pos("dataset-require=") == 1) {
			FString s1 = s;
			s1.SplitBy("dataset-require=");

			//Search dataset plugin
			for (int i = 0 ; i < plugins_.GetSize() ; i++){
				const NNCPlugin &plugin = plugins_.GetAt(i);
				if (plugin.name_ == s1 && plugin.file_name_.Pos(FString("_Pre_Process\\_Download_or_Generate_Dataset")) == 1){
					//Backup dataset name
					FStringList sl;
					for (int id = 0 ; id < nn_document_.GetNumDataset() ; id++){
						sl.Add(nn_document_.GetDataset(id).GetFileName());
					}

					TMenuItem *mi = new TMenuItem(this);
					mi->Tag = i;
					PluginClick(mi);
					delete mi;

					//Reload dataset
					for (int id = 0 ; id < nn_document_.GetNumDataset() ; id++){
						if (FileExists(sl.GetAt(id).PChar())) {
							int count = 0;
							while (count < 3) {
								if (nn_document_.LoadDatasetFromFile(id, sl.GetAt(id))){
									break;
									Sleep(1000);
                                    count++;
								}
								count++;
							}
                        }
					}

					break;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_NetworkTabClick(TObject *Sender)
{
	if (nn_document_.GetLocked()){
		return;
	}
	SetActiveTab(TC_Left, TI_NetworkLeft);
	SetActiveTab(TC_Default, TI_Network);
    Fr_Component->Ed_ListSearch->Text = "";
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_DatasetTabClick(TObject *Sender)
{
	if (nn_document_.GetLocked()){
		return;
	}
	show_dataset_plugin_result_ = false;
	SetActiveTab(TC_Left, TI_DatasetLeft);
	SetActiveTab(TC_Default, TI_Dataset);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_ConfigTabClick(TObject *Sender)
{
	if (nn_document_.GetLocked()){
		return;
	}
	SetActiveTab(TC_Left, TI_ConfigLeft);
	SetActiveTab(TC_Default, TI_Config);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_TrainingTabClick(TObject *Sender)
{
	if (nn_document_.GetLocked()){
		return;
	}
	SetActiveTab(TC_Left, TI_TrainingLeft);
	SetActiveTab(TC_Default, TI_Results);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_EvaluationTabClick(TObject *Sender)
{
	if (nn_document_.GetLocked()){
		return;
	}
	SetActiveTab(TC_Left, TI_TrainingLeft);
	SetActiveTab(TC_Default, TI_Evaluation);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::RB_GraphLinearScaleChange(TObject *Sender)
{
	//Update max and min of graph and redraw graph
	monitor_csv_[0].Update();
	tradeoff_csv_.Update();
	Ti_AnimationTimer(Sender);
	PB_Monitor->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Pa_TasksResize(TObject *Sender)
{
	PB_TrainingEpoch->Width = Pa_Tasks->Width > 32 ? Pa_Tasks->Width - 32 : 1;
	PB_EvaluationData->Width = Pa_Tasks->Width > 32 ? Pa_Tasks->Width - 32 : 1;
}
//---------------------------------------------------------------------------
void TFo_Main::ComponentMouseDown(TMouseButton Button,
		  TShiftState Shift, float X, float Y)
{
	if (Button == TMouseButton::mbLeft) {
		//La_Test->Text = "Item Mouse Down";
		SetCaptured((_di_IControl)*(TPaintBox*)Fr_Component->PB_List);
		component_drag_mode_ = ComponentDragMode::BeforeAddComponent;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ComponentMouseMove(TMouseButton Button, TShiftState Shift,
		  float X, float Y)
{
	if (component_drag_mode_ != ComponentDragMode::None) {
		TPointF screen_pos = ((TPaintBox*)Fr_Component->PB_List)->LocalToAbsolute(PointF(X, Y));
		TPointF graph_pos = PB_Network->AbsoluteToLocal(screen_pos);
		TPointF network_pos = graph_pos;
		network_pos.x /= pow(2.0, network_current_zoom_ * 0.5);
		network_pos.y /= pow(2.0, network_current_zoom_ * 0.5);
		network_pos.x += network_current_left_;
		network_pos.y += network_current_top_;
		//La_Test->Text = UnicodeString("Item Dragging X:") + IntToStr((int)network_pos.x) + UnicodeString(" Y: ")+IntToStr((int)network_pos.y);

		switch (component_drag_mode_) {
		case ComponentDragMode::BeforeAddComponent:
			if (graph_pos.x >= 0 && graph_pos.y >= 0 && graph_pos.x < PB_Network->Width && graph_pos.y < PB_Network->Height) {
				if (AddLayer(false, network_pos.x, network_pos.y, false)) {
					component_drag_mode_ = ComponentDragMode::AfterAddComponent;
					nn_document_.StartDrag();
					nn_document_.Drag(0.0f, 0.0f);
				}
			}
			break;
		case ComponentDragMode::AfterAddComponent:
			nn_document_.Drag(network_pos.x - component_last_drag_pos_.x, network_pos.y - component_last_drag_pos_.y);
			break;
		default:
			break;
		}
		component_last_drag_pos_ = network_pos;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::ComponentMouseUp(TMouseButton Button,
		  TShiftState Shift, float X, float Y)
{
	if (Button == TMouseButton::mbLeft) {
		//La_Test->Text = "";
		if (component_drag_mode_ == ComponentDragMode::AfterAddComponent) {
			nn_document_.EndDrag();
            Bu_Network->SetFocus();
		}
		component_drag_mode_ = ComponentDragMode::None;
		SetCaptured(NULL);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Re_DatasetTopResize(TObject *Sender)
{
	Bu_DatasetOpen->Position->X = Re_DatasetTop->Width - Bu_DatasetOpen->Width;
	Ed_DatasetFileName->Width = Re_DatasetTop->Width - Ed_DatasetFileName->Position->X - Bu_DatasetOpen->Width;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_NetworkResize(TObject *Sender)
{
	Sc_NetworkX->Width = PB_Network->Width;
	Sc_NetworkX->Position->Y = PB_Network->Height - Sc_NetworkX->Height;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PB_TableResize(TObject *Sender)
{
	Sc_TableX->Width = PB_Table->Width;
	Sc_TableX->Position->Y = PB_Table->Height - Sc_TableX->Height;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_NetworkMenuClick(TObject *Sender)
{
	TPointF absolute_pos = Bu_NetworkMenu->LocalToAbsolute(TPointF(0, Bu_NetworkMenu->Height));
	TPointF screen_pos = ClientToScreen(absolute_pos);
	PM_Network->Popup(screen_pos.X, screen_pos.Y);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_NewProjectClick(TObject *Sender)
{
	Log(FString("new project"));
	SetActivity(SDeepConsoleActivity::Default);
	Bu_NetworkTabClick(this);
	NewProject(SDeepEngineType::NNabla);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_OpenProjectClick(TObject *Sender)
{
	SetActivity(SDeepConsoleActivity::Default);
	Bu_NetworkTabClick(this);
	NewProject(SDeepEngineType::NNabla);
	if (OD_Project->Execute()) {
		OpenProject(OD_Project->FileName);
		TC_DefaultChange(Sender);//to set network_overview_
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_OpenDatasetClick(TObject *Sender)
{
	if (!RegisterAndOpenNewDataset(0)){
		return;
	}
	RefreshDatasetFiles();
	Fr_DatasetFiles->SetItemIndex(0);
	DatasetsItemChanged(0);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_OpenDataset2Click(TObject *Sender)
{
	if (!RegisterAndOpenNewDataset(Fr_DatasetList->GetItemIndex())){
		return;
	}
	Bu_DatasetOpenClick(this);
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Fr_ProjectFilesResize(TObject *Sender)
{
	RefreshListSettings();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Fr_DatasetFilesResize(TObject *Sender)
{
	RefreshListSettings();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Fr_DatasetFiles2Resize(TObject *Sender)
{
	RefreshListSettings();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::TC_LeftResize(TObject *Sender)
{
	RefreshListSettings();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::RB_ResultsLearningCurveChange(TObject *Sender)
{
	Ti_AnimationTimer(this);
	Co_HStatistics->Visible = false;
	PB_Monitor->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Bu_CreateDatasetClick(TObject *Sender)
{
	TPointF absolute_pos = Bu_CreateDataset->LocalToAbsolute(TPointF(0, Bu_CreateDataset->Height));
	TPointF screen_pos = ClientToScreen(absolute_pos);
	PM_CreateDataset->Popup(screen_pos.X, screen_pos.Y);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MICD_ImageClassificationClick(TObject *Sender)
{
	/*
	if (FullScreen) {
		FullScreen = false;
		FormResize(Sender);
	}*/

	nn_document_.Lock();
	Fo_CreateDataset = new TFo_CreateDataset(this);
	Fo_CreateDataset->SetLang(Lang->Lang);
	Fo_CreateDataset->ShowModal();
	if (Fo_CreateDataset->ok_) {
		ShowAnimationIndicator(GetTranslatedText("Creating dataset..."), true, true, true);

		UnicodeString option_str =
			UnicodeString(" create_image_classification_dataset") +
			UnicodeString(" -i \"") + Fo_CreateDataset->Ed_SourceDir->Text + UnicodeString("\"") +
			UnicodeString(" -o \"") + Fo_CreateDataset->Ed_OutputDir->Text + UnicodeString("\"") +
			UnicodeString(" -c ") + UnicodeString(Fo_CreateDataset->RB_Monochrome->IsChecked ? "1" : "3") +
			UnicodeString(" -g ") + Fo_CreateDataset->Ed_OutputHeight->Text +
			UnicodeString(" -w ") + Fo_CreateDataset->Ed_OutputWidth->Text +
			UnicodeString(" -m ") + UnicodeString(Fo_CreateDataset->RB_Trimming->IsChecked ? "trimming" : (Fo_CreateDataset->RB_Padding->IsChecked ? "padding" : "resize")) +
			UnicodeString(" -s ") + UnicodeString(Fo_CreateDataset->Ch_Shuffle->IsChecked ? "true" : "false") +
			UnicodeString(" -f1 \"") + Fo_CreateDataset->Ed_OutputFileName1->Text + UnicodeString("\"") +
			UnicodeString(" -r1 ") + Fo_CreateDataset->Ed_OutputFileRatio1->Text +
			UnicodeString(" -f2 \"") + Fo_CreateDataset->Ed_OutputFileName2->Text + UnicodeString("\"") +
			UnicodeString(" -r2 ") + Fo_CreateDataset->Ed_OutputFileRatio2->Text;

		Log(FString("create dataset : ch=") +
			FString(Fo_CreateDataset->RB_Monochrome->IsChecked ? 1 : 3) +
			FString(", h=") +
			FString(AnsiString(Fo_CreateDataset->Ed_OutputHeight->Text).c_str()) +
			FString(", w=") +
			FString(AnsiString(Fo_CreateDataset->Ed_OutputWidth->Text).c_str()) +
			FString(", m=") +
			FString(Fo_CreateDataset->RB_Trimming->IsChecked ? "trimming" : (Fo_CreateDataset->RB_Padding->IsChecked ? "padding" : "resize")) +
			FString(", s=") +
			FString(Fo_CreateDataset->Ch_Shuffle->IsChecked ? "true" : "false") +
			FString(", r1=") +
			FString(AnsiString(Fo_CreateDataset->Ed_OutputFileRatio1->Text).c_str()) +
			FString(", r2=") +
			FString(AnsiString(Fo_CreateDataset->Ed_OutputFileRatio1->Text).c_str()));

		SetEnvironment();
		TThread::CreateAnonymousThread(
			new TDocumentIOFunc(&nn_document_, &settings_,
			option_str,
			 DocumentIOType::CreateDataset, 0, online_debug_mode_)
			)->Start();
		WaitForDocumentUnlock();

		HideAnimationIndicator();

		int dataset_index = 0;
		if (TC_Default->ActiveTab != TI_Browser){
			dataset_index = Fr_DatasetList->GetItemIndex();
			TC_DefaultChange(this);
		}
		//int error = false;
		if (StrToIntDef(Fo_CreateDataset->Ed_OutputFileRatio2->Text, -1) > 0 && !animation_indicator_abort_){
			UnicodeString file_name = Fo_CreateDataset->Ed_OutputDir->Text + "\\" + Fo_CreateDataset->Ed_OutputFileName2->Text;
			if (FileExists(file_name)){
				OpenDataset(dataset_index, file_name, true);
				RefreshDatasetFiles();
				if (TC_Default->ActiveTab == TI_Browser){
					Fr_DatasetFiles->SetItemIndex(0);
					DatasetsItemChanged(0);
				}
			}else {
				//error = true;
			}
		}
		if (StrToIntDef(Fo_CreateDataset->Ed_OutputFileRatio1->Text, -1) > 0 && !animation_indicator_abort_){
			UnicodeString file_name = Fo_CreateDataset->Ed_OutputDir->Text + "\\" + Fo_CreateDataset->Ed_OutputFileName1->Text;
			if (FileExists(file_name)){
				OpenDataset(dataset_index, file_name, true);
				RefreshDatasetFiles();
				if (TC_Default->ActiveTab == TI_Browser){
					Fr_DatasetFiles->SetItemIndex(0);
					DatasetsItemChanged(0);
				}
			}else {
				//error = true;
			}
		}
		/*
		if (error){
			ShowMessage("Error : Failed to create dataset.");
		}
		*/
	}else {
		nn_document_.Unlock();
	}
	Fo_CreateDataset->Release();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Lo_PropertyResize(TObject *Sender)
{
	ClearEditBox();
	Bu_Help->Position->X = Lo_Property->Width - Bu_Help->Width - 8;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::FormDestroy(TObject *Sender)
{
	Ti_Animation->Enabled = false;
	Application->ProcessMessages();

	Log(FString("exit"));
	FreeSDeepTrainingThread();
	FreeSDeepEvaluationThread();

	if (csv_cache_list_){
		csv_cache_list_->Terminate();
		csv_cache_list_->WaitFor();
		delete csv_cache_list_;
		csv_cache_list_ = NULL;
	}

	if (image_cache_list_){
		image_cache_list_->Terminate();
		image_cache_list_->WaitFor();
		delete image_cache_list_;
		image_cache_list_ = NULL;
	}

	if (setting_file_){
		delete setting_file_;
		setting_file_ = NULL;
	}

	ClearDebugBMP();

	const unsigned int time = timeGetTime();
	while (annonymous_thread_count && timeGetTime() < time + 5000) {
		Sleep(10);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MI_CompleteTestClick(TObject *Sender)
{
	//Build complete network with parameter layer
	NNStructures &structures = nn_document_;

	nn_document_.KeepForUndo(FString("Complete Test"));
	FlattenCurrentStructure(nn_document_, structures, true);
	nn_document_.ClearSelection();
	nn_document_.ArrangeLayers();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MI_FillParameterNameTestClick(TObject *Sender)
{
	NNStructures structures;
	structures = nn_document_;
	FStringList parameter_names, parameter_file_names;
	for (int is = 0 ; is < structures.GetNumStructure() ; is++){
		structures.SetActiveStructure(is);
		if (nn_document_.IsStructureUsed(structures.GetName())){
			FlattenCurrentStructure(nn_document_, structures, true);

			for (int il = 0 ; il < structures.GetNumLayer(); il++) {
				const NNLayer &layer = structures.GetLayer(il);
				if (layer.GetType() == "Parameter" || layer.GetType() == "WorkingMemory") {
					// Parameter layer
					FStringList repeat_extensions = structures.GetRepeatExtensions(layer, true);
					for (int ire = 0; ire < repeat_extensions.GetSize() ; ire++) {
						if (parameter_names.IndexOf(repeat_extensions.GetAt(ire)) < 0){
							parameter_names.Add(repeat_extensions.GetAt(ire));
							parameter_file_names.Add(FString("test/") + repeat_extensions.GetAt(ire).ReplaceStr("/", "~"));
						}
					}
				}
			}
		}
	}

	TI_Debug->Visible = true;
	SetActiveTab(TC_Default, TI_Debug);
	Me_Debug->Lines->Clear();
	Me_Debug->Lines->Add(parameter_file_names.GetText().PChar());

	nn_document_.FillParameterName(parameter_names, parameter_file_names);
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MI_LoadAndSaveAllProjectsClick(TObject *Sender)
{
	for (int i = 0 ; i < project_list_.GetSize() ; i++){
		FString file_name = project_list_.GetAt(i).GetAttribute(FString("File Name"));
		nn_document_.LoadFromFile(file_name, FString("{SDCPATH}"));
		nn_document_.SaveToFile(file_name);
		ShowMessage(file_name.PChar());
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MI_CreateStructureSearchCSVFileClick(TObject *Sender)
{
	FStringList sl;
	const FListP<NNTrainingResult> &results = nn_document_.GetResults();
	for (int i = 0 ; i < results.GetSize() ; i++){
		const NNTrainingResult &result = results.GetAt(i);
		sl.Add(
			result.GetFullPath() + FString("network.ini") +
			FString(",") +
			FString(result.GetBestValidation()) +
			FString(",") +
			FString(result.GetCostMultiplyAdd()));
	}
    sl.SaveToFile(AnsiString(ExtractFilePath(ParamStr(0)) + "for_structure_search_cli_test.csv").c_str());
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::Me_ProjectDescriptionChangeTracking(TObject *Sender)
{
	FString description = FString(AnsiString(Me_ProjectDescription->Text).c_str());
	description = description.ReplaceStr("\\", "\\\\");
	description = description.ReplaceStr("\r\n", "\\n");
	description = description.ReplaceStr("\t", "\\t");
	if (nn_document_.GetDescription() != description) {
		nn_document_.SetDescription(description);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MILR_RefreshClick(TObject *Sender)
{
	nn_document_.RefreshResults(FString("\\"));

	if (TC_Left->ActiveTab == TI_TrainingLeft) {
		RefreshResultsList(false);
		OpenResult();
		OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIN_ImportNnpNntxtClick(TObject *Sender)
{
	if (Sender == MIN_ImportNnpNntxt) {
		if (!CheckSave()){
			return;
		}
		if (!AskAndTerminateProcess()){
			return;
		}
    }
	if (OD_NnpNntxt->Execute()) {
        int active_structure_index_bak = nn_document_.GetActiveStructureIndex();
		Bu_NetworkTabClick(this);

		UnicodeString ext = LowerCase(ExtractFileExt(OD_NnpNntxt->FileName));
		Log(FString("import : ext=") + FString(AnsiString(ext).c_str()));
		nn_graph_mapper_.Clear();
		nn_document_.KeepForUndo(FString("Import"));
		FStringList error_str;

		NNDocument &document = Sender == MIN_ImportNnpNntxt ? nn_document_ : nn_document_tmp_;
		if (Sender == MIN_ImportNnpNntxt) {
			NewProject(SDeepEngineType::NNabla, false);
		}else {
			nn_document_tmp_.Clear(SDeepEngineType::NNabla);
        }

		UnicodeString result_dir, nntxt_file_name;
		if (ext == ".nntxt"){
			nntxt_file_name = OD_NnpNntxt->FileName;
		}else {
			ShowAnimationIndicator(GetTranslatedText("Importing..."), true, false, true);
			Ti_AnimationTimer(this);
			nn_document_.Lock();//Lock current document for waiting thread
			Ti_AnimationTimer(this);
			SetEnvironment();
			TThread::CreateAnonymousThread(
				new TDocumentIOFunc(&nn_document_, &settings_, OD_NnpNntxt->FileName, DocumentIOType::Import, 0, online_debug_mode_)
				)->Start();
			WaitForDocumentUnlock();

			HideAnimationIndicator();

			result_dir = OD_NnpNntxt->FileName.SubString(1, OD_NnpNntxt->FileName.Length() - ext.Length()) + ".files";
			nntxt_file_name = result_dir + "\\network.nntxt";
			if (!FileExists(nntxt_file_name)) {
				nntxt_file_name = result_dir + "\\network.prototxt";
			}
		}

		NntxtToDocument(AnsiString(nntxt_file_name).c_str(), document, error_str);//Parameter layers have been created independently at this moment
		if (result_dir != ""){
			SetNNablaWeight(FString(AnsiString(result_dir).c_str()), document);//Assign parameter files to parameter layers
		}

		for (int in = 0 ; in < document.GetNumStructure() ; in++) {
			document.SetActiveStructure(in);
			document.RefreshDisplayValue();
			document.EmbedParameters();// Embed parameter layers into side connector of neural network layers (affine, conv. etc.)
			document.ArrangeLayers();
		}

		if (Sender != MIN_ImportNnpNntxt) {
			// Import as Unit
			UnicodeString nnp_name = LowerCase(ExtractFileName(OD_NnpNntxt->FileName));
			nnp_name = nnp_name.SubString(1, nnp_name.Length() - ext.Length());
			FString nnp_namef = document.EscapeNameStr(FString(AnsiString(nnp_name).c_str()));

			//Escape nnp_name
			int index = 1;
			while (true) {
				int found = false;
				for (int in = 0 ; in < document.GetNumStructure() ; in++) {
					if (nn_document_.GetStructureIndexByName(nnp_namef + FString("_") + document.GetStructure(in).GetName()) >= 0) {
						found = true;
						nnp_namef = FString(AnsiString(nnp_name).c_str()) + FString("_") + FString(index++);
						break;
					}
				}
				if (!found) {
					break;
				}
			}

			// Add Networks
			for (int in = 0 ; in < document.GetNumStructure() ; in++) {
				document.SetActiveStructure(in);
				nn_document_.AddStructure();
				RemoveLossFunctions(document.GetStructure_(in), true);

				nn_document_.GetStructure_(nn_document_.GetNumStructure() - 1) = document.GetStructure(in);
				const FString structure_name = nnp_namef + FString("_") + nn_document_.GetName();
				nn_document_.SetName(structure_name);

				if (in == 0) {
					//Add Unit layer
					nn_document_.SetActiveStructure(active_structure_index_bak);
					const int unit_component_index = nn_document_.GetNNLayerComponents()->GetComponentIndexByType(FString("Unit"));
					if (unit_component_index >= 0) {
						NNLayer &new_layer = nn_document_.AddLayer(unit_component_index, true);
						new_layer.SetPropertyValue(FString("Network"), structure_name);
						nn_document_.GlobalEscapeLayerName(new_layer.GetLayerID());
						nn_document_.StartDrag();
						nn_document_.EndDrag();
					}
				}
			}
			nn_document_tmp_.Clear();
		}

		if (Sender == MIN_ImportNnpNntxt) {
			nn_document_.SetActiveStructure(0);
			if (network_overview_structure_index_ >= 0){
				network_overview_structure_index_ = active_structure_index_bak;
			}
		}else {
			nn_document_.SetActiveStructure(active_structure_index_bak);
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MIN_ImportNnptClick(TObject *Sender)
{
	if (Sender == MIN_ImportNnpt) {
		if (!CheckSave()){
			return;
		}
		if (!AskAndTerminateProcess()){
			return;
		}
    }
	if (OD_Nnpt->Execute()) {
		int active_structure_index_bak = nn_document_.GetActiveStructureIndex();
		Bu_NetworkTabClick(this);
        UnicodeString result_dir, nnpt_file_name, net_file_name_no_ext;
        nnpt_file_name = OD_Nnpt->FileName;

        net_file_name_no_ext = nnpt_file_name.SubString(1, nnpt_file_name.Length() - ExtractFileExt(nnpt_file_name).Length());
        result_dir = net_file_name_no_ext + ".files";

        ExtractNetwork(FString(AnsiString(net_file_name_no_ext).c_str()) , FString(AnsiString(result_dir).c_str()) + FString("\\net"), false);


		UnicodeString ext = LowerCase(ExtractFileExt(OD_Nnpt->FileName));
		Log(FString("import : ext=") + FString(AnsiString(ext).c_str()));
		nn_graph_mapper_.Clear();
		nn_document_.KeepForUndo(FString("Import"));
		FStringList error_str;

		NNDocument &document = Sender == MIN_ImportNnpt ? nn_document_ : nn_document_tmp_;
		FString project_file_name = FString(AnsiString(result_dir).c_str()) + FString("\\config.sdcproj");

		if (Sender == MIN_ImportNnpt) {
			NewProject(SDeepEngineType::Torch, false);
            nn_document_.LoadFromFile(project_file_name, FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()), false, false);
            SetTorchWeight(FString(AnsiString(result_dir).c_str()), document);//Assign parameter files to parameter layers
		}else {
			nn_document_tmp_.Clear(SDeepEngineType::Torch);
        }

		for (int in = 0 ; in < document.GetNumStructure() ; in++) {
			document.SetActiveStructure(in);
			//RemoveTorchLossFunctions(document.GetStructure_(in), true);
			document.RefreshDisplayValue();
			document.EmbedParameters();// Embed parameter layers into side connector of neural network layers (affine, conv. etc.)
			document.ArrangeLayers();
		}

		if (Sender != MIN_ImportNnpt) {
			// Import as Unit
			UnicodeString nnpt_name = LowerCase(ExtractFileName(OD_Nnpt->FileName));
			nnpt_name = nnpt_name.SubString(1, nnpt_name.Length() - ext.Length());
			FString nnpt_namef = document.EscapeNameStr(FString(AnsiString(nnpt_name).c_str()));

			//Escape nnp_name
			int index = 1;
			while (true) {
				int found = false;
				for (int in = 0 ; in < document.GetNumStructure() ; in++) {
					if (nn_document_.GetStructureIndexByName(nnpt_namef + FString("_") + document.GetStructure(in).GetName()) >= 0) {
						found = true;
						nnpt_namef = FString(AnsiString(nnpt_name).c_str()) + FString("_") + FString(index++);
						break;
					}
				}
				if (!found) {
					break;
				}
			}

			// Add Networks
			for (int in = 0 ; in < document.GetNumStructure() ; in++) {
				document.SetActiveStructure(in);
				nn_document_.AddStructure();
				document.LoadFromFile(project_file_name, FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()), false, false);
				RemoveTorchLossFunctions(document.GetStructure_(in), true);
				SetTorchWeight(FString(AnsiString(result_dir).c_str()), document);//Assign parameter files to parameter layers
				nn_document_.GetStructure_(nn_document_.GetNumStructure() - 1) = document.GetStructure(in);
				const FString structure_name = nnpt_namef + FString("_") + nn_document_.GetName();
				nn_document_.SetName(structure_name);

				if (in == 0) {
					//Add Unit layer
					nn_document_.SetActiveStructure(active_structure_index_bak);
					const int unit_component_index = nn_document_.GetNNLayerComponents()->GetComponentIndexByType(FString("Unit"));
					if (unit_component_index >= 0) {
						NNLayer &new_layer = nn_document_.AddLayer(unit_component_index, true);
						new_layer.SetPropertyValue(FString("Network"), structure_name);
						nn_document_.GlobalEscapeLayerName(new_layer.GetLayerID());
						nn_document_.StartDrag();
						nn_document_.EndDrag();
					}
				}
			}
            for (int in = 0 ; in < nn_document_.GetNumStructure() ; in++) {
                nn_document_.SetActiveStructure(in);
                nn_document_.RefreshDisplayValue();
                nn_document_.EmbedParameters();// Embed parameter layers into side connector of neural network layers (affine, conv. etc.)
                nn_document_.ArrangeLayers();
            }
			nn_document_tmp_.Clear();
		}

		if (Sender == MIN_ImportNnpt) {
			nn_document_.SetActiveStructure(0);
			if (network_overview_structure_index_ >= 0){
				network_overview_structure_index_ = active_structure_index_bak;
			}
		}else {
			nn_document_.SetActiveStructure(active_structure_index_bak);
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILRE_ReportClick(TObject *Sender)
{
	const int option = ((TMenuItem*)Sender)->Tag;
    const int network_only = Sender == MINE_pptx || Sender == MINE_html;

	UnicodeString report_type, ext;
	switch (option){
	case 1:
		report_type = "html";
		ext = ".html";
		break;
	default:
		report_type = "pptx";
		ext = ".pptx";
	}

	ShowAnimationIndicator(
        StringReplace(
			GetTranslatedText("Exporting (type) report..."),
			"(type)",
			report_type,
			TReplaceFlags() << rfReplaceAll));

	UnicodeString result_dir;
	UnicodeString work_dir;
	UnicodeString ini_file_name;
	if (!network_only){
		//Open project file from result
		result_dir = GetResultDir();
		if (!DirectoryExists(result_dir)) {
			HideAnimationIndicator();
			return;
		}

		UnicodeString project_file_name = result_dir + UnicodeString("\\network.ini");
		if (!FileExists(project_file_name)) {
			HideAnimationIndicator();
			return;
		}
		if (!nn_document_tmp_.LoadFromFile(FString(AnsiString(project_file_name).c_str()))){
			HideAnimationIndicator();
			return;
		}

		//Prepare report
		work_dir = result_dir + UnicodeString("\\export_report");
		if (!DirectoryExists(work_dir)){
			ForceDirectories(work_dir);
		}

		ini_file_name = work_dir + UnicodeString("\\report.ini");

		//Save all
		nn_document_tmp_.SaveToFile(FString(AnsiString(ini_file_name).c_str()), true);
	}else {
		work_dir = ExtractFilePath(ParamStr(0)) + UnicodeString("tmp\\") + report_type;
		if (!DirectoryExists(work_dir)){
			ForceDirectories(work_dir);
		}
		ini_file_name = work_dir + UnicodeString("\\report.ini");
		nn_document_.SaveToFile(FString(AnsiString(ini_file_name).c_str()), true, true);
		if (!nn_document_tmp_.LoadFromFile(FString(AnsiString(ini_file_name).c_str()))){
			HideAnimationIndicator();
			return;
		}
	}

	//Additional information for report
	FFastIni ini = FFastIni(AnsiString(ini_file_name).c_str());

	//Global information
	UnicodeString project_name = ExtractFileName(UnicodeString(nn_document_.GetFileName().ReplaceStr(".sdcproj", "").PChar()));
	if (project_name == ""){
        project_name = "New project";
	}
	ini.Write<FString>("Global", "ProjectName", FString(AnsiString(project_name).c_str()));

	//Networks
	FListT<int> used_structure_index = nn_document_tmp_.GetUsedStructureIndex();
	FStringList layer_types;
	for (int in = 0 ; in < nn_document_tmp_.GetNumStructure() ; in++){
		nn_document_tmp_.SetActiveStructure(in);
		UnicodeString network_graph_file_name = UnicodeString("network_") + IntToStr(in) + UnicodeString(".png");
		SaveNetworkBitmapToFile(nn_document_tmp_, work_dir + UnicodeString("\\") + network_graph_file_name);

		FString prefix =
			(nn_document_tmp_.GetNumStructure() > 1 ? nn_document_tmp_.GetName() + FString("_") : FString(""));
			FString("Network_Global");
		FString global_section = prefix + FString("Network_Global");
		ini.Write<FString>(global_section, FString("Image"), FString(AnsiString(network_graph_file_name).c_str()));

		if (used_structure_index.IndexOf(in) < 0){
			//do not display statistics if the structure is not used.
			FString structure_section = prefix + FString("Statistics");
			ini.Write<int>(structure_section, FString("NumStatistics"), 0);
		}

		for (int il = 0 ; il < nn_document_tmp_.GetNumLayer() ; il++){
			layer_types.Add(nn_document_tmp_.GetLayer(il).GetType());
		}
	}

	if (!network_only){
		//Learning curve
		UnicodeString learning_cutve_file_name = UnicodeString("learning_curve.png");

		//To draw learning curve and finish its animation
		if (TC_Default->ActiveTab != TI_Results){
			SetActiveTab(TC_Default, TI_Results);
			RB_ResultsLearningCurve->IsChecked = true;
			Ti_AnimationTimer(Sender);
		}
		memcpy(monitor_current_value_range_, monitor_value_range_, sizeof(monitor_value_range_));

		SaveGraphBitmapToFile(work_dir + UnicodeString("\\") + learning_cutve_file_name, 1280, 563, FString("LearningCurve"));
		ini.Write<FString>("Result", "LearningCurve_Image", FString(AnsiString(learning_cutve_file_name).c_str()));

		//Confusion matrix
		FStringList result_files = EnumFiles(AnsiString(result_dir).c_str());
		FStringList confusion_matrix;
		for (int i = 0 ; i < result_files.GetSize() ; i++){
			if (result_files.GetAt(i).Pos("confusion_matrix_") == 1){
				confusion_matrix.Add(result_files.GetAt(i));
			}
		}
		ini.Write<int>("Result", "NumConfusionMatrix", confusion_matrix.GetSize());
		for (int i = 0 ; i < confusion_matrix.GetSize() ; i++){
			ini.Write<FString>(FString("Result"), FString("ConfusionMatrix_") + FString(i), FString("..\\") + confusion_matrix.GetAt(i));
		}
	}else {
		//Delete dataset, optiimizer, monitor and executor
		ini.Write<int>(FString("Dataset"), FString("Num"), 0);
		ini.Write<int>(FString("Config"), FString("NumOptimizer"), 0);
		ini.Write<int>(FString("Config"), FString("NumMonitor"), 0);
		ini.Write<int>(FString("Config"), FString("NumExecutor"), 0);
	}

	//references
	const NNLayerComponents *components = nn_document_tmp_.GetNNLayerComponents();
	layer_types.SortAndDeleteDuplex();
	FStringList reference_name;
	FStringList reference;
	for (int i = 0 ; i < layer_types.GetSize() ; i++){
		const NNLayerComponent *component = components->GetComponentByType(layer_types.GetAt(i));
		if (component){
			if (component->GetReference() != ""){
				reference_name.Add(layer_types.GetAt(i));
				reference.Add(component->GetReference());
			}
		}
	}
	if (!network_only){
		for (int i = 0 ; i < nn_document_tmp_.GetNumOptimizer() ; i++){
			const NNOptimizer &optimizer = nn_document_tmp_.GetOptimizer(i);
			const int solver_index = nn_document_tmp_.GetSolverComponents()->GetSolverComponentIndexByName(optimizer.GetSolverName());
			if (solver_index >= 0){
				const SolverComponent &component = nn_document_tmp_.GetSolverComponents()->GetAt(i);
				if (component.GetReference() != "" && reference_name.IndexOf(optimizer.GetSolverName()) < 0){
					reference_name.Add(optimizer.GetSolverName());
					reference.Add(component.GetReference());
				}
			}
		}
	}
	ini.Write<int>("Reference", "NumReference", reference.GetSize());
	for (int i = 0 ; i < reference.GetSize() ; i++){
		ini.Write<FString>(FString("Reference"), FString("ReferenceLayer_") + FString(i), reference_name.GetAt(i));
		ini.Write<FString>(FString("Reference"), FString("Reference_") + FString(i), reference.GetAt(i));
	}

	//Start pptx genaration
	ini.Flush();
	nn_document_.Lock();//Lock current document for waiting thread
	Ti_AnimationTimer(this);
	SetEnvironment();
	TThread::CreateAnonymousThread(
		new TDocumentIOFunc(&nn_document_, &settings_, ini_file_name, DocumentIOType::ExportReport, option, online_debug_mode_)
		)->Start();
	WaitForDocumentUnlock();

	HideAnimationIndicator();

	UnicodeString pptx_file_name = work_dir + UnicodeString("\\report") + ext;
	if (FileExists(pptx_file_name)){
		OpenFile(AnsiString(pptx_file_name).c_str());
	}else {
		ShowMessage(GetTranslatedText("Error : Failed to create file") + UnicodeString(" (") + report_type + UnicodeString(")."));
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Bu_HelpClick(TObject *Sender)
{
	if (nn_document_.GetCurrentLayerID() >= 0) {
		const NNLayer *layer = nn_document_.GetLayerByID(nn_document_.GetCurrentLayerID());
		UnicodeString lang_u = settings_.GetLanguage() == "auto" ? default_lang_ : UnicodeString(settings_.GetLanguage().PChar());
		FString lang;
		if (lang_u == "ja") {
            lang = FString("-ja");
		}
		OpenFile((FString("https://support.dl.sony.com/docs") + lang + FString("/layer_reference/#") + layer->GetType()).PChar());
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_LearningRateSchedulerChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	FString scheduler = FString(AnsiString(Co_LearningRateScheduler->Items->Strings[Co_LearningRateScheduler->ItemIndex]).c_str());
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateScheduler() != scheduler) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateScheduler(scheduler);
        nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_TypeConfigChange(TObject *Sender)
{
	if (nn_document_.GetTypeConfig() != (TypeConfig)Co_TypeConfig->ItemIndex) {
		nn_document_.SetTypeConfig((TypeConfig)Co_TypeConfig->ItemIndex);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_MonitorIntervalChangeTracking(TObject *Sender)
{
    const int value = StrToIntDef(Ed_MonitorInterval->Text, nn_document_.GetMonitorInterval());
	if (nn_document_.GetMonitorInterval() != value) {
		nn_document_.SetMonitorInterval(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_LRUpdateIntervalUnitChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateUpdateIntervalUnit() != Co_LRUpdateIntervalUnit->ItemIndex) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateUpdateIntervalUnit(Co_LRUpdateIntervalUnit->ItemIndex);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::MIMV_NameClick(TObject *Sender)
{
	((TMenuItem*)Sender)->IsChecked = true;

	//Update max and min of graph and redraw graph
	monitor_csv_[0].Update();
	tradeoff_csv_.Update();
	Ti_AnimationTimer(Sender);
	PB_Monitor->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_PolynomialSchedulerPowerChangeTracking(TObject *Sender)

{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const double value = StrToFloatDef(Ed_PolynomialSchedulerPower->Text, nn_document_.GetOptimizer(optimizer_index).GetLearningRatePower());
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRatePower() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRatePower(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_StepSchedulerGammaChangeTracking(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const double value = StrToFloatDef(Ed_StepSchedulerGamma->Text, nn_document_.GetOptimizer(optimizer_index).GetLearningRateMultiplier());
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateMultiplier() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateMultiplier(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_StepSchedulerIntervalUnitChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateUpdateIntervalUnit() != Co_StepSchedulerIntervalUnit->ItemIndex) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateUpdateIntervalUnit(Co_StepSchedulerIntervalUnit->ItemIndex);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_StepSchedulerStepsChangeTracking(TObject *Sender)

{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	FString learning_rate_steps_str = FString(AnsiString(Ed_StepSchedulerSteps->Text).c_str());
	FListT<int> learning_rate_steps;
	while (learning_rate_steps_str != ""){
		learning_rate_steps.Add(learning_rate_steps_str.SplitBy(",").AsInt());
	}
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateSteps() != learning_rate_steps) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateSteps(learning_rate_steps);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MILRE_NNPClick(TObject *Sender)
{
	//Open project file from result
	UnicodeString result_dir = GetResultDir();
	if (!DirectoryExists(result_dir)) {
		return;
	}

	//option = 0:NNP, 1:NNB, 2:ONNX, 3:pb, 4:ckpt, 5:meta 6:tflite
	const int option = ((TMenuItem*)Sender)->Tag;

	AnsiString ext;
	switch (option){
	case 0:
		ext = ".nnp";
		break;
	case 1:
		ext = ".nnb";
		break;
	case 2:
		ext = ".onnx";
		break;
	case 3:
		ext = "saved_model";
		break;
	case 4:
		ext = ".ckpt";
		break;
	case 5:
		ext = ".meta";
		break;
	case 6:
		ext = ".tflite";
		break;
	}
	Log(FString("export : ext=") + FString(ext.c_str()));
	ShowAnimationIndicator(
		StringReplace(
			GetTranslatedText("Exporting (ext)..."),
			"(ext)",
			ext,
			TReplaceFlags() << rfReplaceAll),
		true, false, true);
	Ti_AnimationTimer(this);
	nn_document_.Lock();//Lock current document for waiting thread
	Ti_AnimationTimer(this);
	SetEnvironment();
	TThread::CreateAnonymousThread(
		new TDocumentIOFunc(&nn_document_, &settings_, result_dir, DocumentIOType::Export, option, online_debug_mode_)
		)->Start();
	WaitForDocumentUnlock();

	HideAnimationIndicator();

	MIL_OpenFileLocationClick(Sender);
}
//---------------------------------------------------------------------------



void __fastcall TFo_Main::PMS_CopyClick(TObject *Sender)
{
	//Copy statistics to clipboard

	const FListP<NNStatistics> &statistics = network_overview_->GetStatistics();
	FStringList statistics_str;
	for (int is = 0 ; is < statistics.GetSize() ; is++) {
		const NNStatistics &stat_1 = statistics.GetAt(is);
		statistics_str.Add(stat_1.GetLabel() + FString("\t") + FString(stat_1.GetSum()).InsertThousandSeparator());
	}
	SetClipboardText(statistics_str.GetText());
}
//---------------------------------------------------------------------------
int TFo_Main::SetClipboardText(const FString &data)
{
	if (TPlatformServices::Current->SupportsPlatformService(__uuidof(IFMXExtendedClipboardService))) {
		_di_IFMXExtendedClipboardService ClipboardService = TPlatformServices::Current->GetPlatformService(__uuidof(IFMXExtendedClipboardService));
		ClipboardService->SetText(UnicodeString(data.PChar()));
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIT_CommandPromptClick(TObject *Sender)
{
	ChDir(ExtractFileDir(ParamStr(0)));
	RunApp("cmd", "/K \"libs\\scripts\\env.bat");
}
//---------------------------------------------------------------------------
FString TFo_Main::GetCurrentFileLocation()
{
	TFr_List *list = NULL;
	if (TC_Default->ActiveTab == TI_Browser && Fr_ProjectFiles->Visible) {
		list = Fr_ProjectFiles;
	}else if (TC_Default->ActiveTab == TI_Browser && Fr_DatasetFiles->Visible) {
		list = Fr_DatasetFiles;
	}else if (TC_Default->ActiveTab == TI_Dataset && Fr_DatasetFiles2->Visible) {
		list = Fr_DatasetFiles2;
	}else if (TC_Left->ActiveTab == TI_TrainingLeft){
		list = Fr_ResultsList;
	}else if (TC_Default->ActiveTab == TI_Browser && Fr_NNablaExamples->Visible) {
		return FString(AnsiString(ExtractFilePath(ParamStr(0)) + "temp\\nnabla-examples\\").c_str());
	}else if (TC_Default->ActiveTab == TI_Browser && Fr_AISolutions->Visible) {
		return FString(AnsiString(ExtractFilePath(ParamStr(0)) + "temp\\ai-solutions\\").c_str());
	}else {
		return FString("");
	}

	if (list->GetItemIndex() < 0 || list->GetItemIndex() >= list->GetItems().GetSize()) {
		return FString("");
	}
	return list->GetItem(list->GetItemIndex()).GetAttribute(FString("Location"));
}

//---------------------------------------------------------------------------
FString TFo_Main::GetCurrentImageFileName()
{
	const CSVDataset &dataset = GetCurrentTable();
	TableUIState &table_ui_state = GetCurrentTableUIState();
	const int dataset_num_data = dataset.GetNumData();
	const int dataset_num_column = dataset.GetNumColumn();
	if (table_ui_state.row_index >= 0 && table_ui_state.row_index < dataset_num_data &&
		table_ui_state.col_index >= 0 && table_ui_state.col_index < dataset_num_column) {
		UnicodeString data_text = dataset.GetValue(table_ui_state.row_index, table_ui_state.col_index).PChar();

		if (IsImageFile(data_text)) {
			return FString(AnsiString(data_text).c_str());
		}
	}
	return FString("");
}

//---------------------------------------------------------------------------
void __fastcall TFo_Main::MILRM_CommandPromptClick(TObject *Sender)
{
	AnsiString dir = GetCurrentFileLocation().PChar();
	if (dir == ""){
		dir = ExtractFileDir(ParamStr(0));
	}
	ChDir(dir);
	RunApp("cmd", (FString("/K \"") + FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()) + FString("libs\\scripts\\env.bat\"")).PChar());
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Bu_UnitizeClick(TObject *Sender)
{
	Fo_CreateUnit = new TFo_CreateUnit(this);
	Fo_CreateUnit->SetLang(Lang->Lang);
	Fo_CreateUnit->ShowModal();
	int result = Fo_CreateUnit->create_;
	UnicodeString network_name = Fo_CreateUnit->Ed_NetworkName->Text;
	Fo_CreateUnit->Release();

	if (!result){
		return;
	}

	nn_document_.KeepForUndo(FString("Create Unit"));
	nn_document_.Unitize(FString(AnsiString(network_name).c_str()));
}
//---------------------------------------------------------------------------
int TFo_Main::RegisterDatasetCSVs(FString out_dir)
{
	if (out_dir == "") {
		return false;
	}
	int csv_found = false;
	FStringList files = EnumFiles((out_dir + FString("\\")).PChar());
	if (files.GetSize() <= 2 || files.GetSize() > 100){
		// (. and ..) or too many files
		return false;
	}
	for (int i = 0 ; i < files.GetSize() ; i++){
		const FString &s = files.GetAt(i);
		if (s.SubString(s.Length() - 3, 4).ToLower() == ".csv"){
			//Register csv
			Ti_AnimationTimer(this);
			OpenDataset(0, (out_dir + FString("\\") + s).PChar());
			csv_found = true;
		}
	}

	if (!csv_found){
		for (int i = 0 ; i < files.GetSize() ; i++){
			const FString &s = files.GetAt(i);
			if (s != "." && s != ".."){
				struct stat st;
				if (stat((out_dir + FString("\\") + s).PChar(), &st) == -1 ) {
					continue;
				}
				if ((st.st_mode & S_IFMT) != S_IFDIR ) {
					continue;
				}
				csv_found |= RegisterDatasetCSVs(out_dir + FString("\\") + s);
			}
		}
	}
	return csv_found;
}

//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_HStatisticsExit(TObject *Sender)
{
	Co_HStatistics->Visible = false;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_HStatisticsChange(TObject *Sender)
{
	if (Co_HStatistics->Tag != 0){
		return;
	}
	if (Co_HStatistics->ItemIndex >= 0){
		result_tradeoff_h_statistics_str_ = FString(AnsiString(Co_HStatistics->Items->Strings[Co_HStatistics->ItemIndex]).c_str());
		nn_document_.SetParetoOptimalStatisticsLabel(result_tradeoff_h_statistics_str_);
		nn_document_.RefreshResults(FString("\\"));
	}
	Co_HStatistics->Visible = false;
	tradeoff_csv_.Update();//To refresh graph
	RefreshResultsList(false);
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_OptimizerRangeFromChangeTracking(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const int value = StrToIntDef(Ed_OptimizerRangeFrom->Text, -1);
	if (nn_document_.GetOptimizer(optimizer_index).GetRangeFrom() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetRangeFrom(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Ed_OptimizerRangeToChangeTracking(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const int value = StrToIntDef(Ed_OptimizerRangeTo->Text, -1);
	if (nn_document_.GetOptimizer(optimizer_index).GetRangeTo() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetRangeTo(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Co_OptimizerRangeUnitChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	if (nn_document_.GetOptimizer(optimizer_index).GetRangeUnit() != Co_OptimizerRangeUnit->ItemIndex) {
		nn_document_.GetOptimizer(optimizer_index).SetRangeUnit(Co_OptimizerRangeUnit->ItemIndex);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Lo_OverviewResize(TObject *Sender)
{
	Bu_OverviewNext->Position->X = Lo_Overview->Width - Bu_OverviewNext->Width;
	Bu_OverviewPrevious->Position->X = Bu_OverviewNext->Position->X - Bu_OverviewPrevious->Width;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Bu_OverviewPreviousClick(TObject *Sender)
{
	network_overview_structure_index_ =
		(network_overview_structure_index_ + 1 +
			(Sender == Bu_OverviewPrevious ?
				network_overview_->GetNumStructure() :
				1)
		) % (network_overview_->GetNumStructure() + 1) - 1;
	if (network_overview_structure_index_ >= 0 && network_overview_structure_index_ != network_overview_->GetActiveStructureIndex()){
		if (network_overview_ == &nn_document_){
			nn_document_.KeepForUndo(FString("Change active network"));
		}
		network_overview_->SetActiveStructure(network_overview_structure_index_);
	}
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::Ed_LearningRateWarmupLengthChangeTracking(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	const int value = StrToIntDef(Ed_LearningRateWarmupLength->Text, nn_document_.GetOptimizer(optimizer_index).GetLearningRateWarmupLength());
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateWarmupLength() != value) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateWarmupLength(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_LearningRateWarmupLengthUnitChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateWarmupLengthUnit() != Co_LearningRateWarmupLengthUnit->ItemIndex) {
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateWarmupLengthUnit(Co_LearningRateWarmupLengthUnit->ItemIndex);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::Ch_LearningRateWarmupChange(TObject *Sender)
{
	const int optimizer_index = TI_OptimizerConfig->Tag;
	FString scheduler =
		Ch_LearningRateWarmup->IsChecked ?
			FString("Linear") :
			FString("None");
	if (nn_document_.GetOptimizer(optimizer_index).GetLearningRateWarmupScheduler() != scheduler){
		nn_document_.GetOptimizer(optimizer_index).SetLearningRateWarmupScheduler(scheduler);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIDL_MoveUpClick(TObject *Sender)
{
	if (Fr_DatasetList->GetItemIndex() > 0){
		nn_document_.SwapDataset(Fr_DatasetList->GetItemIndex(), Fr_DatasetList->GetItemIndex() - 1);
		nn_document_.UpdateConfig();
		Fr_DatasetList->SetItemIndex(Fr_DatasetList->GetItemIndex() - 1);
		RefreshDatasetList();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIDL_MoveDownClick(TObject *Sender)
{
	if (Fr_DatasetList->GetItemIndex() >= 0 && Fr_DatasetList->GetItemIndex() < nn_document_.GetNumDataset() - 1){
		nn_document_.SwapDataset(Fr_DatasetList->GetItemIndex(), Fr_DatasetList->GetItemIndex() + 1);
		nn_document_.UpdateConfig();
		Fr_DatasetList->SetItemIndex(Fr_DatasetList->GetItemIndex() + 1);
		RefreshDatasetList();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MICL_MoveUpClick(TObject *Sender)
{
	Fr_ConfigList->SetFocus();
	int item_index = Fr_ConfigList->GetItemIndex();
	if (item_index < 0 || item_index >= config_type_.GetSize()) {
		return;
	}
	const ListItem &item = Fr_ConfigList->GetItem(item_index);
	const int index = item.GetOriginalIndex();
	if (index > 0) switch (config_type_.GetAt(item_index)) {
	case ConfigType::Global:
		break;
	case ConfigType::Optimizer:
		nn_document_.SwapOptimizer(index, index - 1);
		Fr_ConfigList->SetItemIndex(Fr_ConfigList->GetItemIndex() - 1);
		nn_document_.UpdateConfig();
		break;
	case ConfigType::Monitor:
		break;
	case ConfigType::Executor:
		nn_document_.SwapExecutor(index, index - 1);
		Fr_ConfigList->SetItemIndex(Fr_ConfigList->GetItemIndex() - 1);
		nn_document_.UpdateConfig();
		break;
	default:
		;
	}
	RefreshConfigList();
	RefreshConfigTab(Fr_ConfigList->GetItemIndex());
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MICL_MoveDownClick(TObject *Sender)
{
	Fr_ConfigList->SetFocus();
	int item_index = Fr_ConfigList->GetItemIndex();
	if (item_index < 0 || item_index >= config_type_.GetSize()) {
		return;
	}
	const ListItem &item = Fr_ConfigList->GetItem(item_index);
	const int index = item.GetOriginalIndex();
	switch (config_type_.GetAt(item_index)) {
	case ConfigType::Global:
		break;
	case ConfigType::Optimizer:
		if (index < nn_document_.GetNumOptimizer() - 1){
			nn_document_.SwapOptimizer(index, index + 1);
			Fr_ConfigList->SetItemIndex(Fr_ConfigList->GetItemIndex() + 1);
			nn_document_.UpdateConfig();
		}
		break;
	case ConfigType::Monitor:
		break;
	case ConfigType::Executor:
		if (index < nn_document_.GetNumExecutor() - 1){
			nn_document_.SwapExecutor(index, index + 1);
			Fr_ConfigList->SetItemIndex(Fr_ConfigList->GetItemIndex() + 1);
			nn_document_.UpdateConfig();
		}
		break;
	default:
		;
	}
	RefreshConfigList();
	RefreshConfigTab(Fr_ConfigList->GetItemIndex());
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_NetworkTabMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, float X, float Y)
{
	if (Button != TMouseButton::mbLeft){
		return;
	}
	SetCaptured(NULL);
	PB_Network->PopupMenu = PM_Network;
	if (drag_network_tab_){
		drag_network_tab_ = false;
		if (network_tab_rect_.GetSize()){
			TRectF rect = network_tab_rect_.GetAt(0);
			const int tab_width = rect.Width();
			const int drag_network_index =
				(tab_width * nn_document_.GetActiveStructureIndex() + drag_network_tab_x_shift_ + tab_width / 2) / tab_width;
			if (drag_network_index != nn_document_.GetActiveStructureIndex() &&
				drag_network_index >= 0 && drag_network_index < nn_document_.GetNumStructure() &&
				network_tab_names_.GetSize() == nn_document_.GetNumStructure() && network_tab_rect_.GetSize() == nn_document_.GetNumStructure()){
				network_tab_index_2_ = drag_network_index;

				FString tmp_s = network_tab_names_.GetAt(nn_document_.GetActiveStructureIndex());
				network_tab_names_.Delete(nn_document_.GetActiveStructureIndex());
				network_tab_names_.Insert(drag_network_index, tmp_s);

				TRectF tmp_r = network_tab_rect_.GetAt(nn_document_.GetActiveStructureIndex());
				network_tab_rect_.Delete(nn_document_.GetActiveStructureIndex());
				network_tab_rect_.Insert(drag_network_index, tmp_r);

				nn_document_.MoveStructure(drag_network_index);
				PB_NetworkTab->Tag = 0;
				return;
			}
		}
	}
	if (PB_NetworkTab->Tag == 0 || PB_NetworkTab->Tag >= 4){//Effective mouse down
		PB_NetworkTab->Tag = 0;
		return;
	}
	PB_NetworkTab->Tag = 0;
	ClearEditBox();
	Ed_NetworkName->Visible = false;
	if (network_tab_index_2_ >= 0 && network_tab_index_2_ < nn_document_.GetNumStructure()) {
		if (network_tab_delete_focus_) {
			nn_document_.KeepForUndo(FString("Delete network"));
			nn_document_.DeleteStructure(network_tab_index_2_);
			network_tab_index_2_ = -1;
		}else if (network_tab_index_2_ < network_tab_rect_.GetSize()) {
			//Rename
			TRectF rect = network_tab_rect_.GetAt(network_tab_index_2_);
			Ed_NetworkName->SetBounds(rect.Left, rect.top, rect.Width(), rect.Height());
			Ed_NetworkName->Text = nn_document_.GetName().PChar();
			Ed_NetworkName->Tag = network_tab_index_2_;
			Ed_NetworkName->Visible = true;
			Ed_NetworkName->SelectAll();
			Ed_NetworkName->SetFocus();
		}
	}else if (network_tab_index_2_ >= 0){
		nn_document_.KeepForUndo(FString("Add network"));
		nn_document_.AddStructure();
		network_tab_index_2_ = -1;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_NetworkTabMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, float X, float Y)
{
	if (Button == TMouseButton::mbLeft){
        Bu_NetworkTab2->SetFocus();
		PB_NetworkTabMouseMove(Sender, Shift, X, Y);
		drag_network_tab_ = false;
		if (network_tab_index_2_ >= 0 &&
			network_tab_index_2_ != nn_document_.GetActiveStructureIndex() &&
			network_tab_index_2_ < nn_document_.GetNumStructure()){
			nn_document_.KeepForUndo(FString("Change active network"));
			nn_document_.SetActiveStructure(network_tab_index_2_);
			if (network_overview_structure_index_ >= 0){
				network_overview_structure_index_ = network_tab_index_2_;
			}
			PB_NetworkTab->Tag = 4;//Mouse down (only drag and drop are allowed)
		}else {
			PB_NetworkTab->Tag = 1;//Mouse down
		}
		SetCaptured((_di_IControl)*PB_NetworkTab);
		PB_Network->PopupMenu = NULL;
		if (network_tab_index_2_ >= 0 &&
			network_tab_index_2_ < nn_document_.GetNumStructure()){
			//Drag
			drag_network_tab_ = true;
			drag_network_tab_start_x_ = X;
			drag_network_tab_x_shift_ = 0;
		}
	}else {
        PB_NetworkTab->Tag = 0;
		drag_network_tab_ = false;
	}
}
//---------------------------------------------------------------------------



void __fastcall TFo_Main::Ed_DatasetFileNameClick(TObject *Sender)
{
	if (Ed_DatasetFileName->Text == "(Not Set)"){
		Bu_DatasetOpenClick(Sender);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::FormMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
		  bool &Handled)
{
	if (Shift.Contains(ssShift) && Shift.Contains(ssCtrl)) {
		//Zoom
		if (WheelDelta > 0){
			//Zoom In
			if (settings_.GetGUIScale() < 5){
				settings_.SetGUIScale(settings_.GetGUIScale() + 1);
				SetScale(settings_.GetGUIScale());
				Handled = true;
			}
		}else {
			//Zoom Out
			if (settings_.GetGUIScale() > -5){
				settings_.SetGUIScale(settings_.GetGUIScale() - 1);
				SetScale(settings_.GetGUIScale());
			}
		}
		Handled = true;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::SetActiveTab(TTabControl *TC, TTabItem *TI)
{
	for (int it = 0 ; it < TC->TabCount ; it++){
		if (TC->Tabs[it] == TI){
			TC->Tabs[it]->Visible = true;
			TC->ActiveTab = TI;
		}else {
			TC->Tabs[it]->Visible = false;
		}
	}
    PreviewData(true);// Clear data preview
}
//---------------------------------------------------------------------------
void EnumPluginFiles(const FString &path, const FString dir, FStringList &files)
{
	FStringList result_files = EnumFiles((path + dir).PChar());
	FString sub_path = dir != "" ? dir + FString("\\") : FString("");
	for (int it = 0 ; it < result_files.GetSize() ; it++){
		FString s = result_files.GetAt(it);
		if (s.SubString(s.Length() - 2, 3).ToLower() != ".py"){
			if (s.SubString(1, 1) == "_" && DirectoryExists((path + dir + FString("\\") + s).PChar())){
				EnumPluginFiles(path, sub_path + s, files);
			}
		}else {
			if (dir != "") {
				files.Add(sub_path + s);
			}else {
				files.Add(s);
			}
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::CreatePluginMenu(TMenuItem *menu_parent, FString plugin_dir, int include_history)
{
	// Delete Children
	for (int i = menu_parent->ItemsCount - 1 ; i >= 0 ; i--) {
		menu_parent->Items[i]->Release();
	}

	NNCPlugins plugins = plugins_.GetPlugins(plugin_dir);

	// Add history
	int added_history_num = 0;
	for (int ih = 0 ; ih < plugin_history_.GetSize() ; ih++){
		for (int i = 0 ; i < plugins.GetSize() ; i++){
			if (plugins.GetAt(i).name_ == plugin_history_.GetAt(ih)){
				TMenuItem *mi = new TMenuItem(NULL);
				mi->Text = plugins.GetAt(i).name_.PChar();
				mi->OnClick = PluginClick;
				mi->Tag = plugins.GetAt(i).original_index_;
				menu_parent->AddObject(mi);
				added_history_num++;
				break;
			}
		}
	}

	// Add separator
	if (added_history_num) {
		TMenuItem *mi = new TMenuItem(NULL);
		mi->Text = "-";
		menu_parent->AddObject(mi);
	}

	// Add Plugin Items
	for (int i = 0 ; i < plugins.GetSize() ; i++){
		if (plugins.GetAt(i).name_ != ""){
			TMenuItem *mi = new TMenuItem(NULL);
			mi->Text = plugins.GetAt(i).name_.PChar();
			mi->OnClick = PluginClick;
			mi->Tag = plugins.GetAt(i).original_index_;

			TMenuItem *parent = menu_parent;
			FString plugin_dir = plugins.GetAt(i).file_name_;
			while (plugin_dir.Pos(FString("\\")) >  0){
				FString dir = plugin_dir.SplitBy("\\");
				dir = dir.SubString(2, dir.Length() - 1).ReplaceStr("_", " ");
				if (parent->ItemsCount) {
					if (((TMenuItem*)parent->Items[parent->ItemsCount - 1])->Text == dir.PChar()){
						parent = (TMenuItem*)parent->Items[parent->ItemsCount - 1];
						continue;
					}
				}
				TMenuItem *mi = new TMenuItem(NULL);
				mi->Text = dir.PChar();
				parent->AddObject(mi);
				parent = mi;
			}
			parent->AddObject(mi);
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshPlugins()
{
	FString plugin_ini_file_name = FString(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\plugins.ini").c_str());

	//Load from file
	plugins_.LoadFromIni(plugin_ini_file_name);

	//Enum plugin files

	FString plugin_path = FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()) + FString("libs\\plugins\\");

	FStringList result_files;
	EnumPluginFiles(plugin_path, FString(""), result_files);

	//Update plugins
	SetEnvironment();
	int updated = false;
	for (int it = 0 ; it < result_files.GetSize() ; it++){
		NNCPlugin plugin;
		plugin.file_name_ = result_files.GetAt(it);
		int plugin_index = plugins_.IndexOf(plugin);

		//update if the plugin is not found in the plugin list
		int update = plugin_index < 0;
		if (update){
			//Add new plugin
			plugin_index = plugins_.GetSize();
			plugins_.Add(plugin);
		}
		plugins_.GetAt(plugin_index).found_ = true;

		if (!update){
			struct stat stat_buf;
			if (stat((plugin_path + plugins_.GetAt(plugin_index).file_name_).PChar(), &stat_buf) == 0) {
				//update if the plugin was modified
				update = stat_buf.st_mtime > plugins_.GetAt(plugin_index).date_modified_;
			}
		}

		if (update){
			plugins_.GetAt(plugin_index).need_update_ = true;
			updated = true;
		}
	}

	updated |= plugins_.RemoveNotFoundPlugins();

	if (updated){
		//Save to file
		plugins_.SaveToIni(plugin_ini_file_name);
		ShowAnimationIndicator(GetTranslatedText("Updating plugins..."), true, true, true);

		//Update
		nn_document_.Lock();
		Ti_AnimationTimer(this);
		SetEnvironment();
		ChDir(ExtractFileDir(ParamStr(0)));
		TThread::CreateAnonymousThread(
			new TDocumentIOFunc(&nn_document_, &settings_,
			UnicodeString("\"") + ExtractFilePath(ParamStr(0)) + "libs\\scripts\\plugin.py\"",
			 DocumentIOType::PythonCall, 0, online_debug_mode_)
			)->Start();
		WaitForDocumentUnlock();
		HideAnimationIndicator();

		//Load from python
		plugins_.LoadFromIni(plugin_ini_file_name);
	}
	plugin_history_.LoadFromFile(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\plugin_history.txt").c_str());
	RefreshPluginMenus(true);
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshPluginMenus(int all)
{
	//Add Plugin Menu Item (Evaluation Tab)
	CreatePluginMenu(MIT_Plugin, FString("_Post_Process"), true);

	if (all) {
		//Add Plugin Menu Item (Create Dataset)
		{
			TMenuItem *mi = new TMenuItem(this);
			CreatePluginMenu(mi, FString("_Pre_Process"));
			while (mi->ItemsCount){
				PM_CreateDataset->AddObject(mi->Items[0]);
			}
			delete mi;
		}

		//Add Plugin Menu Item (Dataset)
		CreatePluginMenu(MIT_PluginDataset, FString("_Dataset"));

		//Add Plugin List Item (Training)
		NNCPlugins training_plugins = plugins_.GetPlugins(FString("_Training"));
		for (int i = 0 ; i < training_plugins.GetSize() ; i++){
			if (training_plugins.GetAt(i).name_ != ""){
				Co_Training->Items->Add(training_plugins.GetAt(i).name_.PChar());
			}
		}
    }
}
//---------------------------------------------------------------------------
int TFo_Main::ShowPluginWindow(const NNCPlugin& plugin, FString &args, FString *dir, FString *output_file_name, FStringList *dataset_files)
{
	const FString file_location = GetCurrentFileLocation();

	Fo_Plugin = new TFo_Plugin(this);
	Fo_Plugin->SetLang(Lang->Lang);
	Fo_Plugin->PrepareGUI(plugin);
	FString current_image_file_name = GetCurrentImageFileName();
	if (current_image_file_name != ""){
		Fo_Plugin->SetImageArgument(current_image_file_name);
	}
	int training_dataset_index = nn_document_.GetDatasetIndexByName(FString("Training"));
	int validation_dataset_index = nn_document_.GetDatasetIndexByName(FString("Validation"));
	if (training_dataset_index < 0 && nn_document_.GetNumDataset()) {
		training_dataset_index = 0;
	}
	if (validation_dataset_index < 0 && training_dataset_index != 1 && nn_document_.GetNumDataset() > 1) {
		training_dataset_index = 1;
	}
	Fo_Plugin->SetDefaultArgument(FString("path to training dataset csv file (csv)"), nn_document_.GetDataset(training_dataset_index).GetFileName());
	Fo_Plugin->SetDefaultArgument(FString("path to validation dataset csv file (csv)"), nn_document_.GetDataset(validation_dataset_index).GetFileName());
	Fo_Plugin->SetDefaultArgument(FString("path to dataset csv file (csv)"), GetCurrentTable().GetFileName());
	Fo_Plugin->SetBaseDir(file_location.PChar());
	if (plugin.file_name_.Pos(FString("_Pre_Process\\_Download_or_Generate_Dataset")) == 1) {
		Fo_Plugin->SetSampleDatasetDir(ExtractFilePath(ParamStr(0)) + "samples\\sample_dataset");
    }
	Fo_Plugin->ShowModal();

	if (Fo_Plugin->ModalResult == mrOk) {
		args = FString(Fo_Plugin->GetArgs().c_str());
		if (dir) {
			*dir = FString(Fo_Plugin->GetOutputDir().c_str());
		}
		if (dataset_files) {
			*dataset_files = Fo_Plugin->GetDatasetFileNames();
		}
		if (output_file_name) {
			*output_file_name = FString(Fo_Plugin->GetOutputFileName().c_str());
		}
		return true;
	}else{
		return false;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::PluginClick(TObject *Sender)
{
	const NNCPlugin &plugin = plugins_.GetAt(((TMenuItem*)Sender)->Tag);
	ExecutePlugin(plugin);
}
//---------------------------------------------------------------------------
int TFo_Main::ExecutePlugin(const NNCPlugin &plugin)
{
	const FString file_location = GetCurrentFileLocation();
	/*
	FString s = plugin.file_name_ + FString("\n");
	for (int i = 0 ; i < plugin.arg_option_.GetSize() ; i++){
		s +=
			plugin.arg_option_.GetAt(i) +
			FString(" - ") +
			plugin.arg_name_.GetAt(i) +
			FString(" - ") +
			plugin.arg_description_.GetAt(i) +
			FString(" - ") +
			plugin.arg_default_.GetAt(i) +
			FString("\n");
	}
	ShowMessage(s.PChar());
	*/

	FString args, dir, output_file_name;
	FStringList dataset_files;
	int result = ShowPluginWindow(plugin, args, &dir, &output_file_name, &dataset_files);
	if (result){
		Log(FString("plugin : name=") +
			plugin.name_);


		//Update history and plugin menu
		int history_index = plugin_history_.IndexOf(plugin.name_);
		if (history_index >= 0) {
			plugin_history_.Delete(history_index);
		}
		plugin_history_.Insert(0, plugin.name_);
		while (plugin_history_.GetSize() > 5) {
			plugin_history_.Delete(5);
		}
		plugin_history_.SaveToFile(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\plugin_history.txt").c_str());
		RefreshPluginMenus(false);

		//Execute plugin
		ShowAnimationIndicator(UnicodeString(plugin.name_.PChar()) + UnicodeString("..."), true, true, true);

		nn_document_.Lock();
		Ti_AnimationTimer(this);
		SetEnvironment();
		ChDir(file_location.PChar());
		TThread::CreateAnonymousThread(
			new TDocumentIOFunc(&nn_document_, &settings_,
			UnicodeString("\"") +
			ExtractFilePath(ParamStr(0)) +
			UnicodeString("libs\\plugins\\") +
			UnicodeString(plugin.file_name_.PChar()) +
			UnicodeString("\" ") +
			UnicodeString(args.PChar()),
			 DocumentIOType::PythonCall, 0, online_debug_mode_)
			)->Start();
		WaitForDocumentUnlock();
		HideAnimationIndicator();
		ChDir(ExtractFileDir(ParamStr(0)));

		result &= !Pa_Processing->Visible;
		if (result) {
			//Success
			if (plugin.file_name_.Pos(FString("_Pre_Process\\_Create_Dataset")) == 1) {
				//Create Dataset Plugin
				for (int i = 0 ; i < dataset_files.GetSize() ; i++) {
					const FString file_name = dir + FString("\\") + dataset_files.GetAt(i);
					if (FileExists(file_name.PChar())) {
						OpenDataset(0, file_name.PChar(), true);
						RefreshDatasetFiles();
						if (TC_Default->ActiveTab == TI_Browser){
							Fr_DatasetFiles->SetItemIndex(0);
							DatasetsItemChanged(0);
						}
					}
				}
			}else if (plugin.file_name_.Pos(FString("_Pre_Process\\_Download_or_Generate_Dataset")) == 1) {
				//Enum and register Downloaded CSV files
				int csv_found = RegisterDatasetCSVs(dir);

				//Show downloaded datasets
				if (csv_found && activity_ == SDeepConsoleActivity::Browser){
					RefreshDatasetFiles();
					Fr_DatasetFiles->SetItemIndex(0);
					DatasetsItemChanged(0);
					BrowserItemChanged(1);//Dataset
				}
			}else if (plugin.file_name_.Pos(FString("_Post_Process")) == 1 ||
				(plugin.file_name_.Pos(FString("_Dataset")) == 1 && activity_ == SDeepConsoleActivity::Default)) {
				//Post process plugin or dataset plugin
				if (FileExists((file_location + output_file_name).PChar())) {
					//Saved in the result folder
					OpenEvaluationOthers(output_file_name);
					if (plugin.file_name_.Pos(FString("_Dataset")) == 1) {
						show_dataset_plugin_result_ = true;
					}
				}else if (FileExists(output_file_name.PChar())){
					//outside of the result folder
					OpenFile(output_file_name.PChar());
				}
			}else if (plugin.file_name_.Pos(FString("_NNablaExamples")) == 1 ||
				(plugin.file_name_.Pos(FString("_Dataset")) == 1 && activity_ == SDeepConsoleActivity::Browser)) {
				//NNablaExample plugin or dataset plugin at Browser
				if (OpenFileAsTable(file_location + output_file_name)){
					PrepareRightSubPane();
					image_cache_list_->ClearCache();
					csv_cache_list_->ClearCache();
					Re_RightBlankTop->Visible = true;
					Lo_CSVGraphType->Parent = Re_RightBlankTop;
					Invalidate();
                }
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------
void TFo_Main::InstallPackages()
{
	if (nn_document_.GetSDeepEngineType() != SDeepEngineType::Torch) {
		return;
	}

	AnsiString installed_list_file_name = ExtractFilePath(ParamStr(0)) + "settings\\install_pkg_list.txt";
	if (FileExists(installed_list_file_name)) {
		return;
	}

	ShowAnimationIndicator(GetTranslatedText("Installing packages..."), true, true, true);
	SetEnvironment();

	nn_document_.Lock();
	Ti_AnimationTimer(this);

	ChDir(ExtractFileDir(ParamStr(0)));
	TThread::CreateAnonymousThread(
		new TDocumentIOFunc(&nn_document_, &settings_,
		UnicodeString("\"") + ExtractFilePath(ParamStr(0)) + "libs\\scripts\\install_torch.py\"",
			DocumentIOType::PythonCall, 0, online_debug_mode_)
		)->Start();
	WaitForDocumentUnlock();

	HideAnimationIndicator();
}
//---------------------------------------------------------------------------
void TFo_Main::PrepareRightSubPane()
{
	Lo_Right->Visible = false;
	Re_RightDatasetPreviewTop->Visible = false;
	Re_RightBlankTop->Visible = false;
	Lo_RightSub->Visible = true;
	Sp_Right->Position->X = ClientWidth - Lo_RightSub->Width - Sp_Right->Width - Sp_Right->Margins->Right;
	Sp_Right->Align = TAlignLayout::None;
	Sp_Right->Align = TAlignLayout::Right;
	Lo_Table->Parent = Lo_RightSub;
	Lo_Table->Visible = true;
	Lo_RightSub->Repaint();
}
//---------------------------------------------------------------------------
void TFo_Main::OpenEvaluationOthers(const FString &file_name_)
{
	if (!Co_EvaluationOthers->Tag && activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Evaluation){
		const int result_index = nn_document_.GetCurrentResultIndex();
		RefreshOtherFilesInResultDir(result_index, file_name_);
	}
	const FString file_location = GetCurrentFileLocation();
	const FString file_name = file_location + file_name_;
	const UnicodeString ext = LowerCase(ExtractFileExt(file_name_.PChar()));
	if (ext == ".txt" && activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Dataset) {
		TI_Debug->Visible = true;
		Me_Debug->Lines->LoadFromFile(file_name_.PChar());
		SetActiveTab(TC_Default, TI_Debug);
	}else{
		OpenFileAsTable(file_name);
		if (activity_ == SDeepConsoleActivity::Default && TC_Default->ActiveTab == TI_Evaluation) {
			RB_EvaluationOthers->IsChecked = true;
		}
		RB_EvaluationOutputResultChange(this);
    }
}
//---------------------------------------------------------------------------
int TFo_Main::OpenFileAsTable(const FString &file_name)
{
	int result = true;
	if (!FileExists(file_name.PChar())){
		result = false;
	}else if (file_name.SubString(file_name.Length() - 3, 4).ToLower() == ".csv") {
		CSVDataset result_table;
		result_table.LoadFromFile(file_name.PChar(), false);
		nn_document_.SetResultCSV(2, result_table);
		if (file_name.Pos("profile.csv")){
			others_table_type_ = OthersTableType::Profile;
		}else {
			int file_name_found_in_cell = false;
			const CSVDataset &result_csv = nn_document_.GetResultCSV(2);
			if (result_csv.GetNumData() > 1) for (int ic = 0 ; ic < result_csv.GetNumColumn() && !file_name_found_in_cell ; ic++){
				file_name_found_in_cell |= IsFile(result_csv.GetValue(1, ic).PChar());
			}
			if (file_name_found_in_cell){
				result_table.LoadFromFile(file_name.PChar(), true, true);
				nn_document_.SetResultCSV(2, result_table);
				others_table_type_ = OthersTableType::Dataset;
			}else {
				others_table_type_ = OthersTableType::SimpleTable;
			}
		}
	}else if (IsImageFile(file_name.PChar())) {
		CSVDataset result_table;
		/*
		if (current_image_file_name != ""){
			result_table.SetColumnNames(FString("Input,Result"));
			result_table.SetValue(0, 0, current_image_file_name);
			result_table.SetValue(0, 1, file_name);
		}else {
			result_table.SetColumnNames(FString("Result"));
			result_table.SetValue(0, 0, file_name);
		}
		*/
		result_table.SetColumnNames(FString("Result"));
		result_table.SetValue(0, 0, file_name);
		nn_document_.SetResultCSV(2, result_table);
		others_table_type_ = OthersTableType::Default;
	}else {
		OpenFile(file_name.PChar());
		result = false;
	}
	return result;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Co_EvaluationOthersChange(TObject *Sender)
{
	const FString file_name = FString(Co_EvaluationOthers->ItemIndex >= 0 ? AnsiString(Co_EvaluationOthers->Items->Strings[Co_EvaluationOthers->ItemIndex]).c_str() : "");
	if (RB_EvaluationOthers->IsChecked && file_name != ""){
		Co_EvaluationOthers->Tag = 1;
		OpenEvaluationOthers(file_name);
		Co_EvaluationOthers->Tag = 0;
	}
}
//---------------------------------------------------------------------------
void TFo_Main::OpenInstanceList()
{
	const AnsiString instance_json_file_name = ExtractFilePath(ParamStr(0)) + "settings\\instance.json";
	FStringList sl;
	sl.LoadFromFile(instance_json_file_name.c_str());

	instance_list_ = sl.GetText();
}
//---------------------------------------------------------------------------
UnicodeString GetJSONStr(TJSONPair *json_pair)
{
	return StringReplace(json_pair->JsonValue->ToString(), "\"", "", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------
void TFo_Main::UpdateInstanceList(FString process)
{
	const FString language = FString("ja-JP");//ToDo : Add language into global setting

	const UnicodeString name_str_bak = Co_Instance->ItemIndex >= 0 ? Co_Instance->Selected->Text : UnicodeString("");
	Co_Instance->Tag = 1;
	Co_Instance->Items->BeginUpdate();
	Co_Instance->Items->Clear();
	instance_type_.Clear();
	instance_available_.Clear();
	instance_num_node_.Clear();
	Co_Instance->Items->Add("Local Processor");
	instance_type_.Add(0);
	instance_available_.Add(1);
	instance_num_node_.Add(FListT<int>());
	Co_Instance->ItemIndex = 0;

	int n = 1;
	int found = true;
	while (found) {
        found = false;
		for (int it = 0 ; it < settings_.GetLocalEnvironmentVariable().GetSize(); it++) {
			FString &s = settings_.GetLocalEnvironmentVariable().GetAt(it);
			int p = s.Pos("=");
			if ( p > 0 ) {
				FString name = s.SubString(1, p - 1);
				FString value = s.SubString(p + 1, s.Length());
				FString nm = FString("REMOTE_PROCESSOR_") + FString(n) + FString("_NAME");
				if (nm == name) {
					UnicodeString name_str = UnicodeString(value.PChar());
					Co_Instance->Items->Add(value.PChar());
					if (name_str == name_str_bak){
						Co_Instance->ItemIndex = Co_Instance->Items->Count - 1;
					}

					instance_type_.Add(0x7f000000 + n);//ssh
					instance_available_.Add(true);
					instance_num_node_.Add();
					found = true;
					n++;
					continue;
				}
			}
		}
    }

	TJSONObject* obj = (TJSONObject*)(TJSONObject::ParseJSONValue(UnicodeString(instance_list_.PChar())));
	if (obj){
		TJSONPair* type = obj->Get("instance_type");
		if (type){
			TJSONPair* train = ((TJSONObject*)type->JsonValue)->Get(process.PChar());
			if (train){
				TJSONArray* arr = (TJSONArray*)train->JsonValue;
				if (arr){
					for (int ii = 0 ; ii < arr->Size() ; ii++){
						TJSONObject *instance = (TJSONObject*)arr->Get(ii);
						if (instance){
							UnicodeString name_str = "Cloud : ";
							TJSONPair *desc = instance->Get("description");
							if (desc){
								TJSONPair *name = ((TJSONObject*)desc->JsonValue)->Get(language.PChar());
								if (name){
									name_str = GetJSONStr(name);
								}
							}
							TJSONPair *provider = instance->Get("provider");
							if (provider){
								UnicodeString provider_str = GetJSONStr(provider);
								if (provider_str != "aws"){
									name_str += UnicodeString("(") + UpperCase(provider_str) + UnicodeString(")");
								}
							}
							Co_Instance->Items->Add(name_str);
							if (name_str == name_str_bak){
								Co_Instance->ItemIndex = Co_Instance->Items->Count - 1;
							}

							int instance_type_value = 0;
							TJSONPair *instance_type = instance->Get("instance_type");
							if (instance_type){
								instance_type_value = StrToIntDef(GetJSONStr(instance_type), 0);
							}
							instance_type_.Add(instance_type_value);

							int available_value = 0;
							TJSONPair *available = instance->Get("available");
							if (available){
								available_value = GetJSONStr(available) == "true";
							}
							instance_available_.Add(available_value);

							FListT<int> nodes;
							TJSONPair *num_nodes = instance->Get("num_nodes");
							if (num_nodes){
								TJSONPair *node_enum = ((TJSONObject*)num_nodes->JsonValue)->Get("enum");
								TJSONPair *node_max = ((TJSONObject*)num_nodes->JsonValue)->Get("max");
								TJSONPair *node_min = ((TJSONObject*)num_nodes->JsonValue)->Get("min");
								if (node_enum) {
									TJSONArray* node_arr = (TJSONArray*)node_enum->JsonValue;
									for (int ina = 0 ; ina < node_arr->Size() ; ina++){
										nodes.Add(((TJSONNumber*)(node_arr->Get(ina)))->AsInt);
									}
								}else if (node_max && node_min){
									const int min = ((TJSONNumber*)node_min)->AsInt;
									const int max = ((TJSONNumber*)node_max)->AsInt;
									for (int in = min ; in <= max; in *= 2) {
										nodes.Add(in);
									}
								}
							}
							instance_num_node_.Add(nodes);
						}
					}
				}
			}
		}
	}
	Co_Instance->Items->EndUpdate();
	Co_Instance->Tag = 0;


	delete obj;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Main::Co_TrainingChange(TObject *Sender)
{
	switch (nn_document_.GetSDeepEngineType()){
	case SDeepEngineType::NNabla:
    case SDeepEngineType::Torch:
		if (Co_Training->Tag == 0 && Co_Training->ItemIndex >= 0){
			if (Co_Training->ItemIndex == 0){
				UpdateInstanceList(FString("train"));
			}else {
				UpdateInstanceList(FString(""));
			}
			Co_InstanceChange(Sender);
		}
		break;
	default:
        break;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_InstanceChange(TObject *Sender)
{
	if (Co_Instance->Tag){
		return;
	}

	Co_NumNode->Items->Clear();
	if (Co_Instance->ItemIndex <= 0 || Co_Instance->ItemIndex > instance_num_node_.GetSize()) {
		return;
	}
	FListT<int> &nodes = instance_num_node_.GetAt(Co_Instance->ItemIndex);
	for (int ina = 0 ; ina < nodes.GetSize() ; ina++) {
		Co_NumNode->Items->Add(IntToStr(nodes.GetAt(ina)));
	}
	if (Co_NumNode->Items->Count) {
		Co_NumNode->ItemIndex = 0;
	}

	if (instance_type_.GetAt(Co_Instance->ItemIndex) & 0x7f000000){
		//ssh
		return;
	}

	ShowAnimationIndicator(GetTranslatedText("Changing instance..."), false, false, false, true, true);

	nn_document_.Lock();
	Ti_AnimationTimer(this);
	SetEnvironment();
	UnicodeString command =
		UnicodeString("\"") +
		ExtractFilePath(ParamStr(0)) +
		UnicodeString("libs\\Python\\Lib\\site-packages\\nnabla_nnc\\cli.py") +
		UnicodeString("\" nncw_check_instance -i \"") +
		ExtractFilePath(ParamStr(0)) +
		UnicodeString("settings\\instance.json\"");

	TThread::CreateAnonymousThread(
		new TDocumentIOFunc(&nn_document_, &settings_,
			command,
			DocumentIOType::PythonCall, 0, online_debug_mode_)
		)->Start();
	WaitForDocumentUnlock();

	for (int il = 0 ; il < Me_Processing->Lines->Count ; il++){
		FString line = FString(AnsiString(Me_Processing->Lines->Strings[il]).c_str());
		if (line.Pos("  Need Login : True") == 1){
			ShowMessage(GetTranslatedText("You need to sign in to Neural Network Console again to continue."));
			Bu_SetupClick(Sender);
		}
	}

	HideAnimationIndicator();
}
//---------------------------------------------------------------------------
void TFo_Main::CheckLogLevel()
{
	FStringList sl;
	sl.LoadFromFile(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\log\\config.json").c_str());
	TJSONObject* obj = (TJSONObject*)(TJSONObject::ParseJSONValue(UnicodeString(sl.GetText().PChar())));
	if (obj){
		TJSONPair *log_upload_volume = obj->Get("log_upload_volume");
		if (log_upload_volume){
			UnicodeString log_upload_volume_str = GetJSONStr(log_upload_volume);
			settings_.SetLogLevel(StrToIntDef(log_upload_volume_str, 0));
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MILR_EditCommentClick(TObject *Sender)
{
	TFr_List *list = NULL;
	if (TC_Default->ActiveTab == TI_Browser && Fr_ProjectFiles->Visible) {
		//list = Fr_ProjectFiles;
		return;
	}else if ((TC_Default->ActiveTab == TI_Browser || TC_Default->ActiveTab == TI_Dataset) &&
			  Fr_DatasetFiles->Visible) {
		//list = Fr_DatasetFiles;
		return;
	}else if (TC_Left->ActiveTab == TI_TrainingLeft){
		list = Fr_ResultsList;
	}else {
		return;
	}

	if (list->GetItemIndex() < 0 || list->GetItemIndex() >= list->GetItems().GetSize()){
		return;
	}

	Fo_EditText = new TFo_EditText(this);
	Fo_EditText->SetLang(Lang->Lang);
	Fo_EditText->Set(GetTranslatedText("Comment"), GetTranslatedText("Comment"), list->GetItems().GetAt(list->GetItemIndex()).GetAttribute(FString("Comment")).PChar());
	Fo_EditText->ShowModal();
	if (Fo_EditText->ModalResult == mrOk){
		UnicodeString result_dir = GetResultDir();
		if (DirectoryExists(result_dir)) {
			UnicodeString result_file_name = result_dir + UnicodeString("\\result.ini");
			FFastIni ini = FFastIni(AnsiString(result_file_name).c_str());
			ini.Write<FString>("result", "comment", FString(AnsiString(Fo_EditText->GetResult()).c_str()));
		}

		MILR_RefreshClick(Sender);
	}
	Fo_EditText->Release();
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::Ed_NetworkSearchKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift)
{
	if (Key == 27){
		//Esc
		Ed_NetworkSearch->Text = "";
	}else if (Key == 13){
		SearchLayer(1);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_NetworkSearchChangeTracking(TObject *Sender)
{
	SearchLayer(0);
}
//---------------------------------------------------------------------------
void TFo_Main::SearchLayer(int select_next)
{
	const int current_layer_id_bak = nn_document_.GetCurrentLayerID();
	nn_document_.ClearSelection();

	//Enum layers which contains search text in the layer type or the layer name
	FString search_text = FString(AnsiString(Ed_NetworkSearch->Text).c_str()).ToLower();
	FListT<__int64> net_y_x_index_list;
	__int64 current_net_y_x_index = -1;
	int found_in_active_structure = false;
	for (int in = 0 ; in < nn_document_.GetNumStructure() ; in++) {
		const NNStructure &structure = nn_document_.GetStructure(in);
		for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
			const NNLayer &layer = structure.GetLayer(il);
			if (layer.GetType().ToLower().Pos(search_text) > 0 || layer.GetName().ToLower().Pos(search_text) > 0) {
				__int64 net_y_x_index =
					((__int64)in << 48) | ((__int64)layer.GetPosition(1) << 32) |
					((__int64)layer.GetPosition(1) << 16) | il;
				net_y_x_index_list.Add(net_y_x_index);
				if (in == nn_document_.GetActiveStructureIndex()){
					found_in_active_structure = true;
					if (layer.GetLayerID() == current_layer_id_bak) {
						current_net_y_x_index = net_y_x_index;
					}
				}
			}
		}
	}

	if (!net_y_x_index_list.GetSize()) {
		if (select_next) {
			ShowMessage(UnicodeString(search_text.PChar()) + GetTranslatedText(" is not found."));
		}
		return;
	}

	//Sort and choose next
	net_y_x_index_list.Sort();
	int first_network_index = -1, next_network_index = -1, select_next_network_index = -1, select_previous_network_index = -1, last_network_index = -1;
	int first_network_layer_index = -1, next_network_layer_index = -1, select_next_layer_index = -1, select_previous_layer_index = -1, last_network_layer_index = -1;
	int next_index = -1, fix_previous = false;
	for (int i = 0 ; i < net_y_x_index_list.GetSize() ; i++) {
		const __int64 net_y_x_index = net_y_x_index_list.GetAt(i);
		const int network_index = (int)((net_y_x_index & 0xffff000000000000) >> 48);
		const int layer_index = (int)(net_y_x_index & 0xffff);
		if (found_in_active_structure && network_index != nn_document_.GetActiveStructureIndex() && current_layer_id_bak < 0) {
			continue;
		}
		if (net_y_x_index == current_net_y_x_index) {
			next_index = i + 1;
			fix_previous = true;
		}else if (i == next_index) {
			select_next_network_index = network_index;
			select_next_layer_index = layer_index;
		}
		if (first_network_index < 0) {
			first_network_index = network_index;
			first_network_layer_index = layer_index;
		}
		if (next_network_index < 0 && network_index > nn_document_.GetActiveStructureIndex()) {
			next_network_index = network_index;
			next_network_layer_index = layer_index;
		}
		if (!fix_previous) {
			select_previous_network_index = network_index;
			select_previous_layer_index = layer_index;
		}
		if (i == net_y_x_index_list.GetSize() - 1) {
			last_network_index = network_index;
            last_network_layer_index = layer_index;
		}
	}

	int target_network_index = nn_document_.GetActiveStructureIndex();
	int target_layer_index = nn_document_.GetLayerIndexByID(current_layer_id_bak);

	if (select_next > 0) {
		//F3 or enter key
		if (select_next_network_index >= 0) {
			target_network_index = select_next_network_index;
			target_layer_index = select_next_layer_index;
		}else {
			//back to top
			target_network_index = first_network_index;
			target_layer_index = first_network_layer_index;
		}
	}else if (select_next < 0){
		//Shift+F3
		if (select_previous_network_index >= 0) {
			target_network_index = select_previous_network_index;
			target_layer_index = select_previous_layer_index;
		}else {
			//back to last
			target_network_index = last_network_index;
			target_layer_index = last_network_layer_index;
		}
	}else {
		//Incremental search
		if (!found_in_active_structure) {
			if (next_network_index >= 0) {
				target_network_index = next_network_index;
				target_layer_index = next_network_layer_index;
			}else {
				//back to top
				target_network_index = first_network_index;
				target_layer_index = first_network_layer_index;
			}
		}
	}

	//Move network
	if (target_network_index != nn_document_.GetActiveStructureIndex()) {
		nn_document_.KeepForUndo(FString("Change active network"));
		nn_document_.SetActiveStructure(target_network_index);
	}

	//Select layer
	for (int i = 0 ; i < net_y_x_index_list.GetSize() ; i++) {
		const __int64 net_y_x_index = net_y_x_index_list.GetAt(i);
		const int network_index = (int)((net_y_x_index & 0xffff000000000000) >> 48);
		const int layer_index = (int)(net_y_x_index & 0xffff);
		if (network_index == nn_document_.GetActiveStructureIndex()) {
			const NNLayer &layer = nn_document_.GetLayer(layer_index);
			nn_document_.SetLayerSelected(layer.GetLayerID(), true, false);
			if (target_layer_index < 0) {
				target_layer_index = layer_index;
			}
		}
	}

	//Set Current Layer
    const NNLayer &layer = nn_document_.GetLayer(target_layer_index);
	nn_document_.SetCurrentLayerID(layer.GetLayerID());
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::MIN_ReplaceClick(TObject *Sender)
{
    Application->ProcessMessages();
	UnicodeString current_type;
	TStringList *component_types = new TStringList();
	const SDeepEngineType sdeep_engine_type = nn_document_.GetSDeepEngineType();
	for (int ic = 0 ; ic < settings_.GetEngineInfo().GetAt((int)sdeep_engine_type).GetComponents().GetSize() ; ic++) {
		const NNLayerComponent &component = settings_.GetEngineInfo().GetAt((int)sdeep_engine_type).GetComponents().GetAt(ic);
		if (!component.GetHidden()){
			component_types->Add(component.GetType().PChar());
		}
	}
	component_types->Sort();

	if (nn_document_.GetCurrentLayerID() >= 0) {
		const NNLayer *layer = nn_document_.GetLayerByID(nn_document_.GetCurrentLayerID());
		if (layer) {
			current_type = layer->GetType().PChar();
		}
	}

	//Fo_Replace = new TFo_Replace(this);
	Fo_Replace->SetLang(Lang->Lang);
	int selected = false;
	for (int il = 0 ; il < nn_document_.GetNumLayer() && !selected ; il++) {
		const NNLayer &layer = nn_document_.GetLayer(il);
		selected |= layer.GetSelected();
	}
	Fo_Replace->Set(component_types, current_type, selected);
	Fo_Replace->ShowModal();

	if (Fo_Replace->replace_) {
		nn_document_.KeepForUndo(FString("Replace"));
		nn_document_.Replace(
			FString(AnsiString(Fo_Replace->from_).c_str()),
			FString(AnsiString(Fo_Replace->to_).c_str()),
			Fo_Replace->target_current_network_,
			Fo_Replace->target_selection_);
	}


	//Fo_Replace->Release();
	delete component_types;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MILR_DuplicateClick(TObject *Sender)
{
	//Copy
	bool success = false;
	const ListItem &list_item = Fr_ResultsList->GetItem(Fr_ResultsList->GetItemIndex());
	FString original_name = nn_document_.GetResultPath(FString("\\")) + list_item.GetAttributeValue(0);
	FString dest_name = original_name + FString("_copy");
	try {
		TDirectory::Copy(original_name.PChar(), dest_name.PChar());
		success = true;
	} catch (...) {
		ShowMessage(FString(
			GetTranslatedFString("Cannot duplicate") +
			FString(" \"") +
			list_item.GetAttributeValue(0) +
			FString("\".")
			).PChar());
	}
	if (success) {
		nn_document_.RefreshResults(FString("\\"), false);
		RefreshResultsList(false);
		OpenResult();
		OpenEvaluationResult(false, TC_Default->ActiveTab == TI_Evaluation);
	}

}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ch_AutoEvaluationChange(TObject *Sender)
{
	if (nn_document_.GetAutoEvaluation() != Ch_AutoEvaluation->IsChecked) {
		nn_document_.SetAutoEvaluation(Ch_AutoEvaluation->IsChecked);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Co_CSVGraphTypeChange(TObject *Sender)
{
	Invalidate();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MI_ClearParameterFilesClick(TObject *Sender)
{
	nn_document_.KeepForUndo(FString("Clear Parameter Files"));
	nn_document_.ClearParameterFiles();
	Invalidate();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MITM_CommandPromptClick(TObject *Sender)
{
	AnsiString dir = ExtractFileDir(GetCurrentTable().GetFileName().PChar());
	if (dir == ""){
		dir = ExtractFileDir(ParamStr(0));
	}
	ChDir(dir);
	RunApp("cmd", (FString("/K \"") + FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()) + FString("libs\\scripts\\env.bat\"")).PChar());
}
//---------------------------------------------------------------------------
void TFo_Main::ListMenuItemForTranslation(TComponent* parent)
{
	menu_items_.Clear();
	menu_item_original_texts_.Clear();
	ListMenuItemRecursiveForTranslation(parent);
}
//---------------------------------------------------------------------------
void TFo_Main::ListMenuItemRecursiveForTranslation(TComponent* parent)
{
	if (parent->ClassName() == "TMenuItem") {
		TMenuItem *menu_item = (TMenuItem*)parent;
		menu_items_.Add(menu_item);
		menu_item_original_texts_.Add(FString(AnsiString(menu_item->Text).c_str()));
	}
	if (parent->ComponentCount) {
		for (int i = 0 ; i < parent->ComponentCount ; i++) {
			ListMenuItemRecursiveForTranslation(parent->Components[i]);
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::Translate()
{
	UnicodeString lang_u = settings_.GetLanguage() == "auto" ? default_lang_ : UnicodeString(settings_.GetLanguage().PChar());
	Lang->AutoSelect = false;
	Lang->Lang = lang_u;
	for (int i = 0 ; i < menu_items_.GetSize() ; i++) {
		UnicodeString original_text = menu_item_original_texts_.GetAt(i).PChar();
		((TMenuItem*)menu_items_.GetAt(i))->Text = GetTranslatedText(original_text);
	}

	//Top Buttons
	AddHint(Bu_Back, GetTranslatedFString("Home") + FString(" (Ctrl+W)"));
	AddHint(Bu_Save, GetTranslatedFString("Save") + FString(" (Ctrl+S)"));
	AddHint(Bu_SaveAs, GetTranslatedFString("Save as") + FString(""));
	AddHint(Bu_Undo, GetTranslatedFString("Undo") + FString(" (Ctrl+Z)"));
	AddHint(Bu_Redo, GetTranslatedFString("Redo") + FString(" (Ctrl+Y or Ctrl+Shift+Z)"));
	AddHint(Bu_Cut, GetTranslatedFString("Cut") + FString(" (Ctrl+X)"));
	AddHint(Bu_Copy, GetTranslatedFString("Copy") + FString(" (Ctrl+C)"));
	AddHint(Bu_Paste, GetTranslatedFString("Paste") + FString(" (Ctrl+V)"));
	AddHint(Bu_NetworkTab, GetTranslatedFString("Switch to the edit tab (Ctrl+1)"));
	AddHint(Bu_TrainingTab, GetTranslatedFString("Switch to the training tab (Ctrl+2)"));
	AddHint(Bu_EvaluationTab, GetTranslatedFString("Switch to the evaluation tab (Ctrl+3)"));
	AddHint(Bu_DatasetTab, GetTranslatedFString("Switch to the dataset tab (Ctrl+4)"));
	AddHint(Bu_ConfigTab, GetTranslatedFString("Switch to the config tab (Ctrl+5)"));
	AddHint(Re_NetworkTab, GetTranslatedFString("Switch to the edit tab (Ctrl+1)"));
	AddHint(Re_TrainingTab, GetTranslatedFString("Switch to the training tab (Ctrl+2)"));
	AddHint(Re_EvaluationTab, GetTranslatedFString("Switch to the evaluation tab (Ctrl+3)"));
	AddHint(Re_DatasetTab, GetTranslatedFString("Switch to the dataset tab (Ctrl+4)"));
	AddHint(Re_ConfigTab, GetTranslatedFString("Switch to the config tab (Ctrl+5)"));
	AddHint(Bu_Setup, GetTranslatedFString("Setup (F10)"));
	AddHint(Bu_FullScreen, GetTranslatedFString("Full screen") + FString(" (F11)"));
	AddHint(Ed_DatasetFileName, GetTranslatedFString("Dataset URI"));
	AddHint(Bu_DatasetOpen, GetTranslatedFString("Open dataset") + FString(" (Ctrl+O)"));
	AddHint(Bu_NewProject, GetTranslatedFString("Create new empty project using nnabla"));
	AddHint(Bu_NewProjectTorch, GetTranslatedFString("Create new empty project using torch") + FString(" (Ctrl+N)"));
	AddHint(Bu_NewProjectTransformers, GetTranslatedFString("Create new project using Transformers"));
	AddHint(Bu_OpenProject, GetTranslatedFString("Open existing project file") + FString(" (Ctrl+O)"));
	AddHint(Bu_CreateDataset, GetTranslatedFString("Create a new dataset") + FString(" (Ctrl+N)"));
	AddHint(Bu_OpenDataset, GetTranslatedFString("Open dataset") + FString(" (Ctrl+O)"));
	AddHint(Bu_CreateDataset2, GetTranslatedFString("Create a new dataset") + FString(" (Ctrl+N)"));
	AddHint(Bu_OpenDataset2, GetTranslatedFString("Open dataset") + FString(" (Ctrl+O)"));
	AddHint(Bu_Unitize, GetTranslatedFString("Unitize"));
	AddHint(Bu_ZoomIn, GetTranslatedFString("Zoom In") + FString(" (Ctrl + +)"));
	AddHint(Bu_ZoomOut, GetTranslatedFString("Zoom Out") + FString(" (Ctrl + -)"));

	RefreshListSettings();
}
//---------------------------------------------------------------------------
UnicodeString TFo_Main::GetTranslatedText(UnicodeString text)
{
	UnicodeString lang_u = settings_.GetLanguage() == "auto" ? default_lang_ : UnicodeString(settings_.GetLanguage().PChar());
	TStrings *lang_str = Lang->LangStr[lang_u];
	if (lang_str) {
		UnicodeString text2 = text + "=";
		const int text2_length = text2.Length();
		for (int i2 = 0 ; i2 < lang_str->Count ; i2++) {
			if (lang_str->Strings[i2].SubString(1, text2_length) == text2) {
				return lang_str->Strings[i2].SubString(text2_length + 1, lang_str->Strings[i2].Length() - text2_length);
			}
		}
	}
	return text;
}
//---------------------------------------------------------------------------
FString TFo_Main::GetTranslatedFString(UnicodeString text)
{
	return FString(AnsiString(GetTranslatedText(text)).c_str());
}
//---------------------------------------------------------------------------
void TFo_Main::SetCSVGraphType()
{
	switch(settings_.GetDefaultCSVGraphType()){
	case CSVGraphType::Summary:
		Co_CSVGraphType->ItemIndex = 0;
		break;
	case CSVGraphType::TwoDim:
		Co_CSVGraphType->ItemIndex = 2;
		break;
	case CSVGraphType::TwoDim_Transposed:
		Co_CSVGraphType->ItemIndex = 3;
		break;
	default:
		Co_CSVGraphType->ItemIndex = 1;
		break;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Fr_ComponentBu_ListKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift)
{
	Fr_Component->Bu_ListKeyDown(Sender, Key, KeyChar, Shift);
	if (Key != 0) {
        Bu_NetworkKeyDown(Sender, Key, KeyChar, Shift);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PMC_RefreshClick(TObject *Sender)
{
	settings_.RefreshCustomUnits(FString(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\custom_units").c_str()));
	RefreshLayerComponentTree(nn_document_.GetSDeepEngineType());
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PMC_RegisterCustomUnitClick(TObject *Sender)
{
	if (OD_Project->Execute()) {
		if (CopyFile(OD_Project->FileName.c_str(), (ExtractFilePath(ParamStr(0)) + "settings\\custom_units\\" + ExtractFileName(OD_Project->FileName)).c_str(), false)){
			PMC_RefreshClick(Sender);
		}else {
			ShowMessage(GetTranslatedText("Failed to register custom unit."));
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PMC_DeleteCustomUnitClick(TObject *Sender)
{
	FListT<int> ci = GetComponentIndex();
	int component_index = ci.GetAt(0);
	int custom_unit_index = ci.GetAt(1);
	if (custom_unit_index >= 0 && custom_unit_index < settings_.GetCustomUnits().GetSize()) {
		if (DeleteFile(ExtractFilePath(ParamStr(0)) + "settings\\custom_units\\" + UnicodeString(settings_.GetCustomUnits().GetAt(custom_unit_index).PChar()) + UnicodeString(".sdcproj"))){
			PMC_RefreshClick(Sender);
		}else {
			ShowMessage(GetTranslatedText("Failed to delete custom unit."));
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Lo_PropertyLeftResized(TObject *Sender)
{
	PB_Property->Repaint();
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::Ch_OutOfCoreTrainingEnabledChange(TObject *Sender)
{
	if (nn_document_.GetOutOfCoreTrainingEnabled() != Ch_OutOfCoreTrainingEnabled->IsChecked) {
		nn_document_.SetOutOfCoreTrainingEnabled(Ch_OutOfCoreTrainingEnabled->IsChecked);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_OutOfCoreTrainingGPUMemorySizeChangeTracking(TObject *Sender)

{
	const double value = StrToFloatDef(Ed_OutOfCoreTrainingGPUMemorySize->Text, 0);
	if (nn_document_.GetOutOfCoreTrainingGPUMemorySize() != value) {
		nn_document_.SetOutOfCoreTrainingGPUMemorySize(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Ed_OutOfCoreTrainingWindowSizeChangeTracking(TObject *Sender)

{
	const double value = StrToFloatDef(Ed_OutOfCoreTrainingWindowSize->Text, 0);
	if (nn_document_.GetOutOfCoreTrainingWindowSize() != value) {
		nn_document_.SetOutOfCoreTrainingWindowSize(value);
		nn_document_.UpdateConfig();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MILR_ResumeTrainingClick(TObject *Sender)
{
	if (nn_document_.GetCurrentResultIndex() < 0) {
		return;
	}
	if (GetTraining(nn_document_.GetResult(nn_document_.GetCurrentResultIndex()).GetDir())){
		return;
	}

	UnicodeString result_dir = GetResultDir();
	if (DirectoryExists(result_dir)) {
		UnicodeString result_file_name = result_dir + UnicodeString("\\result.ini");
		FFastIni ini = FFastIni(AnsiString(result_file_name).c_str());
		ini.Write<int>("result", "schedule", 2);//resume
	}

	nn_document_.RefreshResults(FString("\\"));
	RefreshResultsList(false);

	Ch_StructureSearchEnabled->IsChecked = false;
	TrainScheduled();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIT_WeightCheckClick(TObject *Sender)
{
	TestCheckIfAllLayerHasInitialWeight();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_MonitorPerformancePaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_MonitorPerformance->Canvas;
	C->BeginScene();
	C->SetMatrix(PB_MonitorPerformance->AbsoluteMatrix);
	C->Clear(SDEEP_CONSOLE_COLOR_BACKGROUND);
	const int space = 32;
	const float hsize = PB_MonitorPerformance->Height - space * 2;

	if (hsize < space * 2 || PB_MonitorPerformance->Width < monitor_hspace * 2) {
		C->EndScene();
		return;
	}

	FListT<int> epoch;
	FListT<float> graph;
	float sum = 0.0;
	for (int i = 0 ; i < Me_Monitor->Lines->Count ; i++) {
		if (Me_Monitor->Lines->Strings[i].Pos("average load:")) {
			FString s = FString(AnsiString(Me_Monitor->Lines->Strings[i]).c_str());
			s.SplitBy("epoch ");
			epoch.Add(s.SplitBy(" of ").AsInt());
			s.SplitBy(RB_MonitorCPU->IsChecked ? "cpu:" : "gpu:");
			sum += graph.Add(s.SplitBy("%").AsFloat());
		}
	}


	// background scale
	C->Stroke->Kind = TBrushKind::Solid;
	C->Stroke->Thickness = 1;
	C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
	C->Stroke->Dash = TStrokeDash::Solid;
	C->DrawLine(
		PointF(monitor_hspace, space),
		PointF(PB_MonitorPerformance->Width - space, space),
		1.0f);

	if (PB_MonitorPerformance->Height > space * 4) {
		C->DrawLine(
			PointF(monitor_hspace, PB_MonitorPerformance->Height / 2),
			PointF(PB_MonitorPerformance->Width - space, PB_MonitorPerformance->Height / 2),
			1.0f);
	}

	C->DrawLine(
		PointF(monitor_hspace, PB_MonitorPerformance->Height - space),
		PointF(PB_MonitorPerformance->Width - space, PB_MonitorPerformance->Height - space),
		1.0f);

    int focus_index = -1;
	if (graph.GetSize()) {
		const float size = (PB_MonitorPerformance->Width - 32 - monitor_hspace) / graph.GetSize();

		if (size >= 0.0 && hsize >= 16.0) {
			// graph
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
			for (int i = 0 ; i < graph.GetSize() ; i++) {
				TRectF rect = RectF(
					monitor_hspace + i * size + 0.5,
					PB_MonitorPerformance->Height - space - graph.GetAt(i) * hsize / 100.0f,
					monitor_hspace + (i + 1) * size - 0.5,
					PB_MonitorPerformance->Height - space);
				if (rect.Contains(TPointF(monitor_performance_last_x_, monitor_performance_last_y_))) {
                    focus_index = i;
					C->Fill->Color = SDEEP_CONSOLE_COLOR_BRAND;
				}else {
					C->Fill->Color = SDEEP_CONSOLE_COLOR_LIGHTBRAND;
                }
				C->FillRect(rect, 0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			}


			// epoch text
			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->Font->Style = TFontStyles();
			C->Font->Size = 16;
			int lastx = 0;
			for (int i = 0 ; i < graph.GetSize() ; i++) {
				FString s = FString(epoch.GetAt(i));
				const int text_width = C->TextWidth(s.PChar());
				const int left = monitor_hspace + (i + 0.5) * size - text_width / 2;
				if (lastx + 16 < left && left + text_width < PB_MonitorPerformance->Width - space) {
					C->FillText(
						RectF(left, PB_MonitorPerformance->Height - space, left + text_width, PB_MonitorPerformance->Height),
						s.PChar(),
						false,
						1.0f,
						TFillTextFlags(),
						TTextAlign::Center,
						TTextAlign::Center
						);
					lastx = left + text_width;
				}
			}
		}
	}else {
		C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
		C->Font->Style = TFontStyles();
		C->Font->Size = 16;
		C->FillText(
			RectF(monitor_hspace + 16, space + 16, PB_MonitorPerformance->Width, PB_MonitorPerformance->Height),
			GetTranslatedText("Performance information is not available."),
			false,
			1.0f,
			TFillTextFlags(),
			TTextAlign::Leading,
			TTextAlign::Leading
			);
	}

	// foreground scale
	TPathData *path = new TPathData();
	path->MoveTo(PointF(monitor_hspace, space));
	path->LineTo(PointF(monitor_hspace, PB_MonitorPerformance->Height - space));
	path->LineTo(PointF(PB_MonitorPerformance->Width - 32, PB_MonitorPerformance->Height - space));
	C->DrawPath(path, 1.0f);
	delete path;

	C->Stroke->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
	C->DrawLine(
		PointF(monitor_hspace - graph_scale_size / 2, space),
		PointF(monitor_hspace + graph_scale_size / 2, space),
		1.0f);
	C->FillText(
		RectF(0, space - 16, monitor_hspace - graph_scale_size / 2, space + 16),
		"100%",
		false,
		1.0f,
		TFillTextFlags(),
		TTextAlign::Trailing,
		TTextAlign::Center
		);

	if (hsize > space * 2) {
		C->DrawLine(
			PointF(monitor_hspace - graph_scale_size / 2, PB_MonitorPerformance->Height / 2),
			PointF(monitor_hspace + graph_scale_size / 2, PB_MonitorPerformance->Height / 2),
			1.0f);
		C->FillText(
			RectF(0, PB_MonitorPerformance->Height / 2 - 16, monitor_hspace - graph_scale_size / 2, PB_MonitorPerformance->Height / 2 + 16),
			"50%",
			false,
			1.0f,
			TFillTextFlags(),
			TTextAlign::Trailing,
			TTextAlign::Center
			);
	}

	C->DrawLine(
		PointF(monitor_hspace - graph_scale_size / 2, PB_MonitorPerformance->Height - space),
		PointF(monitor_hspace + graph_scale_size / 2, PB_MonitorPerformance->Height - space),
		1.0f);
	C->FillText(
		RectF(0, PB_MonitorPerformance->Height -  space - 16, monitor_hspace - graph_scale_size / 2, PB_MonitorPerformance->Height - space + 16),
		"0%",
		false,
		1.0f,
		TFillTextFlags(),
		TTextAlign::Trailing,
		TTextAlign::Center
		);

	// average
	if (graph.GetSize()) {
		const float average = sum / graph.GetSize();
		const float y = PB_MonitorPerformance->Height - space - average * hsize / 100.0f;
		C->Stroke->Thickness = 3;
		C->Stroke->Dash = TStrokeDash::Dash;
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_BRAND;
		C->DrawLine(
			PointF(monitor_hspace, y),
			PointF(PB_MonitorPerformance->Width - space, y),
			1.0f);

		C->Fill->Color = SDEEP_CONSOLE_COLOR_BRAND;
		C->FillText(
			RectF(0, y - space, PB_MonitorPerformance->Width - space, y),
			UnicodeString("Average ") + FormatFloat("0.0", average) + "%",
			false,
			1.0f,
			TFillTextFlags(),
			TTextAlign::Trailing,
			TTextAlign::Center
			);
	}

	// focus
	if (focus_index >= 0 && Bu_MonitorPerformance->IsFocused) {
		TRectF rect = TRect(
			monitor_performance_last_x_ + 32,
			monitor_performance_last_y_ - 80,
			monitor_performance_last_x_ + 160,
			monitor_performance_last_y_ - 16);
			C->Stroke->Thickness = 1;
			C->Stroke->Dash = TStrokeDash::Solid;
			C->Stroke->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			C->Fill->Color = SDEEP_CONSOLE_COLOR_BACKGROUND;

			C->FillRect(rect, 0.0f, 0.0f, AllCorners, 0.75f, TCornerType::Round);
			C->DrawRect(rect, 0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);

			C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
			rect.left = rect.left + 16;
			C->FillText(
				rect,
				UnicodeString("Epoch : ") + IntToStr(epoch.GetAt(focus_index)) + UnicodeString("\n") + FormatFloat("0.0", graph.GetAt(focus_index)) + "%",
				false,
				1.0f,
				TFillTextFlags(),
				TTextAlign::Leading,
				TTextAlign::Center
				);
	}

	C->EndScene();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_MonitorPerformanceMouseMove(TObject *Sender, TShiftState Shift,
          float X, float Y)
{
	monitor_performance_last_x_ = X;
	monitor_performance_last_y_ = Y;
	PB_MonitorPerformance->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::RB_MonitorCPUClick(TObject *Sender)
{
    PB_MonitorPerformance->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Bu_OverviewExit(TObject *Sender)
{
    PB_NetworkOverview->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Bu_MonitorPerformanceExit(TObject *Sender)
{
	PB_MonitorPerformance->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::PB_MonitorPerformanceClick(TObject *Sender)
{
	Bu_MonitorPerformance->SetFocus();
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshAISolutionsList()
{
	NNCPlugins ai_solutions_plugins = plugins_.GetPlugins(FString("_AISolutions"));

	ListItems items;

	for (int i = 0 ; i < ai_solutions_plugins.GetSize() ; i++){
		const NNCPlugin &plugin = ai_solutions_plugins.GetAt(i);
		if (plugin.name_ != ""){
			ListItem &list_item = items.Add();
			list_item.SetOriginalIndex(i);
			list_item.AddAttribute(FString("Name"), plugin.name_);
			list_item.AddAttribute(FString("Description"), plugin.description_);
			list_item.AddAttribute(FString("Location"), plugin.file_name_);
			list_item.AddAttribute(FString("DateTime"), FString(AnsiString(DateTimeToStr(UnixToDateTime(plugin.date_modified_, false))).c_str()));
		}
	}

	Fr_AISolutions->SetItems(items);
	Invalidate();
}
//---------------------------------------------------------------------------
void TFo_Main::AISolutionsItemClick(int index)
{
	if (index >= 0 && index < Fr_AISolutions->GetItems().GetSize()) {
		const ListItem &item = Fr_AISolutions->GetItem(index);
		const FString plugin_name = item.GetAttribute(FString("Name"));
		//Search ai solution plugin
		for (int i = 0 ; i < plugins_.GetSize() ; i++){
			const NNCPlugin &plugin = plugins_.GetAt(i);
			if (plugin.name_ == plugin_name){
				if (!DirectoryExists(ExtractFilePath(ParamStr(0)) + "temp")){
					MkDir(ExtractFilePath(ParamStr(0)) + "\\temp");
				}
				UnicodeString output_dir = ExtractFilePath(ParamStr(0)) + "temp\\ai-solutions";
				if (!DirectoryExists(output_dir)){
					MkDir(output_dir);
				}
				UnicodeString plugin_name_str = ExtractFileName(plugin.file_name_.PChar());
				output_dir += UnicodeString("\\") + plugin_name_str.SubString(1, plugin_name_str.Length() - 3);

				UnicodeString command =
					UnicodeString("python");
				UnicodeString option =
					UnicodeString("\"") +
					ExtractFilePath(ParamStr(0)) +
					UnicodeString("libs\\plugins\\") +
					UnicodeString(plugin.file_name_.PChar()) +
					UnicodeString("\" -o ") +
					output_dir +
					UnicodeString("\"");

				SetEnvironment();
				RunApp(AnsiString(command).c_str(), AnsiString(option).c_str());

				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Main::RefreshNNablaExamplesList()
{
	NNCPlugins nnabla_examples_plugins = plugins_.GetPlugins(FString("_NNablaExamples"));

	ListItems items;

	for (int i = 0 ; i < nnabla_examples_plugins.GetSize() ; i++){
		const NNCPlugin &plugin = nnabla_examples_plugins.GetAt(i);
		if (plugin.name_ != ""){
			ListItem &list_item = items.Add();
			list_item.SetOriginalIndex(i);
			list_item.AddAttribute(FString("Name"), plugin.name_);
			list_item.AddAttribute(FString("Description"), plugin.description_);
			list_item.AddAttribute(FString("Location"), plugin.file_name_);
			list_item.AddAttribute(FString("DateTime"), FString(AnsiString(DateTimeToStr(UnixToDateTime(plugin.date_modified_, false))).c_str()));
		}
	}

	Fr_NNablaExamples->SetItems(items);
	Invalidate();
}
//---------------------------------------------------------------------------
void TFo_Main::NNablaExamplesItemClick(int index)
{
	if (index >= 0 && index < Fr_NNablaExamples->GetItems().GetSize()) {
		const ListItem &item = Fr_NNablaExamples->GetItem(index);
		const FString plugin_name = item.GetAttribute(FString("Name"));
		//Search nnabla example plugin
		for (int i = 0 ; i < plugins_.GetSize() ; i++){
			const NNCPlugin &plugin = plugins_.GetAt(i);
			if (plugin.name_ == plugin_name){
				if (!DirectoryExists(ExtractFilePath(ParamStr(0)) + "temp")){
					MkDir(ExtractFilePath(ParamStr(0)) + "\\temp");
				}
				if (!DirectoryExists(ExtractFilePath(ParamStr(0)) + "temp\\nnabla-examples")){
					MkDir(ExtractFilePath(ParamStr(0)) + "\\temp\\nnabla-examples");
				}
				while (ExecutePlugin(plugin)){

				}
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Fr_NNablaExamplesResize(TObject *Sender)
{
	RefreshListSettings();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIC_RecreateClick(TObject *Sender)
{
	Fo_EditText = new TFo_EditText(this);
	Fo_EditText->SetLang(Lang->Lang);
	Fo_EditText->Set(GetTranslatedText("Recreate Confusion Matrix"), GetTranslatedText("Binary threshold"), "0.5");
	Fo_EditText->ShowModal();
	if (Fo_EditText->ModalResult == mrOk){
		const double binary_threshold = StrToFloatDef(Fo_EditText->GetResult(), 0.5);
		UpdateConfusionMatrix(true, binary_threshold);
		RB_EvaluationOutputResultChange(Sender);
	}
	Fo_EditText->Release();

}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::MINPT_EmptyProjectClick(TObject *Sender)
{
	Log(FString("new project"));
	SetActivity(SDeepConsoleActivity::Default);
	Bu_NetworkTabClick(this);
	NewProject(SDeepEngineType::Transformers);
}
//---------------------------



void __fastcall TFo_Main::PB_DataPreviewPaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_DataPreview->Canvas;

	C->BeginScene();
	C->SetMatrix(PB_DataPreview->AbsoluteMatrix);
	C->Clear(SDEEP_CONSOLE_COLOR_BACKGROUND);

	if (nn_document_.GetLocked()) {
		C->EndScene();
		return;
	}
	UnicodeString data_text;

	if (TC_Default->ActiveTab == TI_Network) {
		if (last_layer_id_ >= 0) {
			const NNLayer *layer = nn_document_.GetLayerByID(last_layer_id_);
			if (layer) {
				const FString &layer_type = layer->GetType();
                data_text = layer->GetPropertyValue(FString("Comment")).PChar();
			}
		}
	}else {
		TableUIState &table_ui_state = GetCurrentTableUIState();
		const CSVDataset &dataset = GetCurrentTable();
		const int num_data = dataset.GetNumData();
		const int num_column = dataset.GetNumColumn();
		UnicodeString path = ExtractFilePath(dataset.GetFileName().PChar());

		if (num_data >= 1 && num_column > 0 && table_ui_state.col_index >= 0 && table_ui_state.col_index < num_column && table_ui_state.row_index >= 0 && table_ui_state.row_index < num_data) {
			data_text = UTF8String(dataset.GetValue(table_ui_state.row_index, table_ui_state.col_index).ReplaceStr("{SDCPATH}", AnsiString(ExtractFilePath(ParamStr(0))).c_str()).PChar());
		}
    }

	if (data_text != "") {
		C->Fill->Kind = TBrushKind::Solid;
		C->Fill->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
		C->Stroke->Color = SDEEP_CONSOLE_COLOR_LINE;
		C->Stroke->Kind = TBrushKind::Solid;
		C->Font->Size = 16;

		C->Stroke->Thickness = 1;

		const TRectF cell_rect = RectF(0, 0, PB_DataPreview->Width, PB_DataPreview->Height);

		FStringList request_images;
		FStringList request_csvs;
		FListT<CSVCacheFileType> request_csv_types;

		DrawDataCell(C, cell_rect, data_text, request_images, request_csvs, request_csv_types);
	}

	C->EndScene();
}
//---------------------------------------------------------------------------
bool IsAsciiOnly(TStringList *list) {
	for (int i = 0; i < list->Count; i++) {
		UnicodeString line = list->Strings[i];
		for (int j = 1; j <= line.Length(); j++) {
			if (line[j] > 0x7f) {
				return false;
			}
		}
	}
	return true;
}

void __fastcall TFo_Main::MICD_FromTextClick(TObject *Sender)
{
	Fo_EditText = new TFo_EditText(this);
	Fo_EditText->SetLang(Lang->Lang);
	Fo_EditText->Set(GetTranslatedText("New Dataset"), "", "", 2);
	Fo_EditText->ShowModal();
	if (Fo_EditText->ModalResult == mrOk){
		UnicodeString current_dir;
		if (nn_document_.GetFileName() != "") {
			current_dir = ExtractFileDir(nn_document_.GetFileName().PChar());
			SetCurrentDir(current_dir);
		}
		if (SD_CSV->Execute()) {
			TStringList *sl = new TStringList();
			sl->Text = Fo_EditText->GetResult();
			for (int i = 0 ; i < sl->Count ; i++) {
				if (current_dir != "") {
					sl->Strings[i] = StringReplace(sl->Strings[i], current_dir, ".", TReplaceFlags() << rfReplaceAll << rfIgnoreCase);
				}
				if (sl->Strings[i].Pos(",")) {
					sl->Strings[i] = UnicodeString("\"") + sl->Strings[i] + UnicodeString("\"");
				}
			}
			if (sl->Count > 0) {
				if (sl->Strings[0] != "x") {
					sl->Insert(0, "x");
				}
			}
			if (IsAsciiOnly(sl)) {
				sl->SaveToFile(SD_CSV->FileName);
			}else {
				sl->WriteBOM = true;
				sl->SaveToFile(SD_CSV->FileName, TEncoding::UTF8);
			}

			int dataset_index = 0;
			if (TC_Default->ActiveTab != TI_Browser){
				dataset_index = Fr_DatasetList->GetItemIndex();
				TC_DefaultChange(this);
			}
			OpenDataset(dataset_index, SD_CSV->FileName, true);
			RefreshDatasetFiles();
			if (TC_Default->ActiveTab == TI_Browser){
				Fr_DatasetFiles->SetItemIndex(0);
				DatasetsItemChanged(0);
			}
			delete sl;
		}
	}
	Fo_EditText->Release();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::MIT_OpenTableClick(TObject *Sender)
{
	const CSVDataset &dataset = GetCurrentTable();
	const FString &dataset_file_name = dataset.GetFileName();
	if (dataset_file_name != "") {
		OpenFile(AnsiString(dataset_file_name.PChar()).c_str());
	}
}
//---------------------------------------------------------------------------


void __fastcall TFo_Main::Bu_NewProjectTransformersClick(TObject *Sender)
{
	TPointF absolute_pos = Bu_NewProjectTransformers->LocalToAbsolute(TPointF(0, Bu_NewProjectTransformers->Height));
	TPointF screen_pos = ClientToScreen(absolute_pos);
	PM_NewProjectTransformers->Popup(screen_pos.X, screen_pos.Y);
}
//---------------------------------------------------------------------------

void __fastcall TFo_Main::Bu_NewProjectTorchClick(TObject *Sender)
{
	InstallPackages();

	Log(FString("new project for torch"));
	SetActivity(SDeepConsoleActivity::Default);
	Bu_NetworkTabClick(this);
	NewProject(SDeepEngineType::Torch);
}
//---------------------------------------------------------------------------





