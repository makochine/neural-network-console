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

#ifndef sdeep_console_global_settingsH
#define sdeep_console_global_settingsH
//---------------------------------------------------------------------------
#include <math.h>

#include "flist.h"
#include "fstring.h"
#include "ffastini.h"

#include "nn_document.h"

#include "nn_layer_component.h"
#include "nn_structure_search_policy.h"
#include "solver_component.h"
#include "nn_document_converter_setting.h"

namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
enum class LocalSDeepProcessorType : int {
CPU,
CUDA,
cuDNN_CUDA,
};
//---------------------------------------------------------------------------
enum class CSVGraphType : int {
Summary,
All,
TwoDim,
TwoDim_Transposed,
};
//---------------------------------------------------------------------------
class SDeepConsoleGlobalSettings
{
public:
	SDeepConsoleGlobalSettings() {Clear();};
	virtual ~SDeepConsoleGlobalSettings() {Clear();};
	void Clear(){
		local_sdeep_processor_type_ = LocalSDeepProcessorType::cuDNN_CUDA;
		local_sdeep_gpu_index_ = -1;//Use Default GPU
		local_nnabla_sdp_command_ = "";
		local_environment_variable_.Clear();
		local_environment_variable_.Add();

		language_ = "auto";
		gui_scale_ = 0;
		gui_transition_ = 1;
		play_audio_file_ = 0;
		default_csv_graph_type_ = CSVGraphType::All;

		log_level_ = -1;

		//Custom Unit
		custom_units_.Clear();
	}
public:
	//Setup
	int SetupEngines(FString setting_path, FStringList &errors);
	const FListP<NNEngineInfo> &GetEngineInfo() const {return nn_engine_info_list_;};
	const FListP<NNDocumentConverterSetting> &GetDocumentConverterSettingList() const {return nn_document_converter_setting_list_;};

	int SetupLinkCollection(FString setting_path, FStringList &errors);
	const FStringList &GetLinkCollection() const {return link_collection_;}
private:
	FListP<NNEngineInfo> nn_engine_info_list_;//nn_engine_info_list_.GetAt(EngineIndex)
	FListP<NNDocumentConverterSetting> nn_document_converter_setting_list_;

	FStringList sample_datasets_;
	FStringList link_collection_;

	int RegisterEngine(SDeepEngineType sdeep_engine_type, FString engine_setting_path, FStringList &errors);
	int RegisterDocumentConverterSetting(FString convert_setting_path, FStringList &errors);
public:

	void SaveToIni(FFastIni* ini);
	void LoadFromIni(FFastIni* ini);
public:
	int GetLogLevel() const {return log_level_;}
	void SetLogLevel(int log_level) {log_level_ = log_level;}

	//Engine
	const FString ProcessorTypeStr();
	LocalSDeepProcessorType StrToProcessorType(FString s);
	void SetLocalSDeepProcessorType(LocalSDeepProcessorType local_sdeep_processor_type) {local_sdeep_processor_type_ = local_sdeep_processor_type;}
	LocalSDeepProcessorType GetLocalSDeepProcessorType() const {return local_sdeep_processor_type_;}
	void SetLocalSDeepGPUIndex(int local_sdeep_gpu_index) {local_sdeep_gpu_index_ = local_sdeep_gpu_index;}
	int GetLocalSDeepGPUIndex() const {return local_sdeep_gpu_index_;}
	void SetLocalNNablaSDPCommand(FString local_nnabla_sdp_command) {local_nnabla_sdp_command_ = local_nnabla_sdp_command;}
	const FString &GetLocalNNablaSDPCommand() const {return local_nnabla_sdp_command_;}
	void SetLocalEnvironmentVariable(const FStringList &local_environment_variable) {local_environment_variable_ = local_environment_variable;}
	const FStringList &GetLocalEnvironmentVariable() const {return local_environment_variable_;}

	//View
	const FString &GetLanguage() const {return language_;}
	void SetLanguage(FString language) {language_ = language;}
	void SetGUIScale(int gui_scale) {gui_scale_ = gui_scale;}
	int GetGUIScale() const {return gui_scale_;}
	static float GUIScaleToGUIScaleFactor(int gui_scale) {return (int)(pow(2.0f, gui_scale / 5.0f) * 8) * 0.125f;}
	void SetGUITransition(int gui_transition) {gui_transition_ = gui_transition;}
	int GetGUITransition() const {return gui_transition_;}
	void SetPlayAudioFile(int play_audio_file) {play_audio_file_ = play_audio_file;}
	int GetPlayAudioFile() const {return play_audio_file_;}
	const FString CSVGraphTypeStr();
	CSVGraphType StrToCSVGraphType(FString s);
	void SetDefaultCSVGraphType(CSVGraphType default_csv_graph_type) {default_csv_graph_type_ = default_csv_graph_type;}
	CSVGraphType GetDefaultCSVGraphType() {return default_csv_graph_type_;}

	//Custom Unit
	void RefreshCustomUnits(const FString &custom_unit_path);
	const FStringList &GetCustomUnits() {return custom_units_;}
private:
	//Engine
	LocalSDeepProcessorType local_sdeep_processor_type_;
	int local_sdeep_gpu_index_;
	FString local_nnabla_sdp_command_;
	FStringList local_environment_variable_;

	//View
	FString language_;
	int gui_scale_;//-5(50%)�`+5(200%)
	int gui_transition_;
	int play_audio_file_;
	CSVGraphType default_csv_graph_type_;//0=summary, 1=all, 2=2d, 3=2d(transparent)

	int log_level_;//-1 = unknown, 0 = None, 1(min) ~ 3(max)

	//Custom Unit
	FStringList custom_units_;
};
//---------------------------------------------------------------------------
};
#endif
