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

#include "sdeep_console_global_settings.h"
#include "file_util.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
int SDeepConsoleGlobalSettings::SetupEngines(FString setting_path, FStringList &errors)
{
	if (!RegisterEngine(SDeepEngineType::Torch, setting_path + FString("Torch/"), errors)){
		errors.Add(FString("Failed to load settings for Torch engine."));
	}
	if (!RegisterEngine(SDeepEngineType::NNabla, setting_path + FString("NNabla/"), errors)){
		errors.Add(FString("Failed to load settings for NNabla engine."));
	}
	if (!RegisterEngine(SDeepEngineType::Transformers, setting_path + FString("Transformers/"), errors)){
		errors.Add(FString("Failed to load settings for Transformers."));
	}
	if (!RegisterDocumentConverterSetting(setting_path + FString("convert/"), errors)){
		errors.Add(FString("Failed to load settings for document converter."));
	}
    return errors.GetSize() == 0;
}
//---------------------------------------------------------------------------
int SDeepConsoleGlobalSettings::RegisterEngine(SDeepEngineType sdeep_engine_type, FString engine_setting_path, FStringList &errors)
{
	while (nn_engine_info_list_.GetSize() <= (int)sdeep_engine_type) {
		nn_engine_info_list_.Add();
	}

	NNEngineInfo &engine = nn_engine_info_list_.GetAt((int)sdeep_engine_type);

	//Layer Components
	{
		//Enum category
		FString layer_component_path = engine_setting_path + FString("layer_component/");
		int p = layer_component_path.Pos("/./");
		if (p) {
			layer_component_path =
				layer_component_path.SubString(1, p - 1) +
				layer_component_path.SubString(p + 2, layer_component_path.Length());
		}
		if (!engine.Components().LoadFromFolder(layer_component_path.PChar(), errors)){
			errors.Add(FString("An error occurred while loading layer components."));
		}
	}

	//Structure Search Policy
	engine.StructureSearchPolicies().LoadFromTextFile((engine_setting_path + "structure_search_policy.ini").PChar());

	//Solver Components
	if (!engine.Solvers().LoadFromTextFile((engine_setting_path + "solver_component.txt").PChar())){
		errors.Add(FString("An error occurred while loading solver components."));
	}

	//Scripts
	engine.Scripts().LoadFromTextFile((engine_setting_path + "scripts.ini").PChar());

	//Preset Projects
	{
		engine.PresetProjects().Clear();
		FString preset_project_path = engine_setting_path + FString("preset_project/");
		int p = preset_project_path.Pos("/./");
		if (p) {
			preset_project_path =
				preset_project_path.SubString(1, p - 1) +
				preset_project_path.SubString(p + 2, preset_project_path.Length());
		}
		FStringList project_files = EnumFiles(preset_project_path.PChar());
		for (int i = 0 ; i < project_files.GetSize() ; i++){
			FString s = project_files.GetAt(i);
			if (s.SubString(s.Length() - 7, 8).ToLower() == ".sdcproj"){
				FString file_name = preset_project_path + s;
				engine.PresetProjects().Add(file_name.ReplaceStr(FString("/"), FString("\\")));
			}
		}
	}

	return engine.GetComponents().GetSize() && engine.GetStructureSearchPolicies().GetSize();
}
//---------------------------------------------------------------------------
int SDeepConsoleGlobalSettings::RegisterDocumentConverterSetting(FString convert_setting_path, FStringList &errors)
{
	FFastIni ini = FFastIni((convert_setting_path + FString("convert.txt")).PChar());
	FStringList converter_files;
	ini.ReadSectionValues("Convert", &converter_files);
	int success_count = 0;
	for (int i = 0 ; i < converter_files.GetSize(); i++) {
		FString s = converter_files.GetAt(i);
		FString from = s.SplitBy("_to_");
		FString to = s;
		NNDocumentConverterSetting &setting =
			nn_document_converter_setting_list_.Add();
		success_count += setting.LoadFromTextFile((convert_setting_path + converter_files.GetAt(i) + FString(".txt")).PChar(), from, to);
	}

	return success_count == converter_files.GetSize();
}
//---------------------------------------------------------------------------
int SDeepConsoleGlobalSettings::SetupLinkCollection(FString setting_path, FStringList &errors)
{
	if (!link_collection_.LoadFromFile((setting_path + FString("link_collection.txt")).PChar())){
		errors.Add(FString("Failed to load link collection."));
	}
	return errors.GetSize() == 0;
}
//---------------------------------------------------------------------------
const FString SDeepConsoleGlobalSettings::ProcessorTypeStr()
{
	switch (local_sdeep_processor_type_)
	{
	case LocalSDeepProcessorType::CUDA:
		return FString("CUDA");
	case LocalSDeepProcessorType::cuDNN_CUDA:
		return FString("cuDNN_CUDA");
	default:
		return FString("CPU");
	}
}
//---------------------------------------------------------------------------
LocalSDeepProcessorType SDeepConsoleGlobalSettings::StrToProcessorType(FString s)
{
	if (s == "All"){
		return LocalSDeepProcessorType::CUDA;
	}else if (s == "cuDNN_CUDA" || s == "1"){
		return LocalSDeepProcessorType::cuDNN_CUDA;
	}else {
		return LocalSDeepProcessorType::CPU;
	}
}
//---------------------------------------------------------------------------
const FString SDeepConsoleGlobalSettings::CSVGraphTypeStr()
{
	switch (default_csv_graph_type_)
	{
	case CSVGraphType::Summary:
		return FString("Summary");
	case CSVGraphType::TwoDim:
		return FString("2D");
	case CSVGraphType::TwoDim_Transposed:
		return FString("2D_Transposed");
	default:
		return FString("All");
	}
}
//---------------------------------------------------------------------------
CSVGraphType SDeepConsoleGlobalSettings::StrToCSVGraphType(FString s)
{
	if (s == "Summary"){
		return CSVGraphType::Summary;
	}else if (s == "2D"){
		return CSVGraphType::TwoDim;
	}else if (s == "2D_Transposed"){
		return CSVGraphType::TwoDim_Transposed;
	}else {//All
		return CSVGraphType::All;
	}
}
//---------------------------------------------------------------------------
void SDeepConsoleGlobalSettings::SaveToIni(FFastIni* ini)
{
	ini->Write<int>("Account", "LogLevel", log_level_);

	//Engine
	ini->Write<FString>("Engine", "LocalSDeepProcessorType", ProcessorTypeStr());
	ini->Write<int>("Engine", "LocalSDeepGPUIndex", local_sdeep_gpu_index_);
	ini->Write<FString>("Engine", "LocalNNablaSDPCommand", local_nnabla_sdp_command_);
	FString env_str;
	for (int i = 0 ; i < local_environment_variable_.GetSize() ; i++) {
		if (i > 0) {
			env_str += "***";
		}
		env_str += local_environment_variable_.GetAt(i);
	}
	ini->Write<FString>("Engine", "LocalEnvironmentVariable", env_str);

	//View
	ini->Write<FString>("View", "Language", language_);
	ini->Write<int>("View", "GUIScale", gui_scale_);
	ini->Write<int>("View", "GUITransition", gui_transition_);
	ini->Write<int>("View", "PlayAudioFile", play_audio_file_);
	ini->Write<FString>("View", "DefaultCSVGraphType", CSVGraphTypeStr());

	//Save
	ini->Flush();}
//---------------------------------------------------------------------------
void SDeepConsoleGlobalSettings::LoadFromIni(FFastIni* ini)
{
	log_level_ = ini->ReadInteger("Account", "LogLevel", log_level_);

	//Engine
	local_sdeep_processor_type_ = StrToProcessorType(ini->ReadString("Engine", "LocalSDeepProcessorType", ProcessorTypeStr()));
	local_sdeep_gpu_index_ = ini->ReadInteger("Engine", "LocalSDeepGPUIndex", local_sdeep_gpu_index_);
	local_nnabla_sdp_command_ = ini->ReadString("Engine", "LocalNNablaSDPCommand", local_nnabla_sdp_command_);
	FString env_str = ini->ReadString("Engine", "LocalEnvironmentVariable", FString(""));
	if (env_str != "") {
		local_environment_variable_.Clear();
		while (env_str != ""){
			local_environment_variable_.Add(env_str.SplitBy("***"));
		}
	}

	//View
	language_ = ini->ReadString("View", "Language", language_);
	gui_scale_ = ini->ReadInteger("View", "GUIScale", gui_scale_);
	gui_transition_ = ini->ReadInteger("View", "GUITransition", gui_transition_);
	play_audio_file_ = ini->ReadInteger("View", "PlayAudioFile", play_audio_file_);
	default_csv_graph_type_ = StrToCSVGraphType(ini->ReadString("View", "DefaultCSVGraphType", CSVGraphTypeStr()));
}
//---------------------------------------------------------------------------
void SDeepConsoleGlobalSettings::RefreshCustomUnits(const FString &custom_unit_path)
{
	custom_units_ = EnumFiles(custom_unit_path.PChar());
	for (int it = 0 ; it < custom_units_.GetSize() ; it++){
		const FString &s = custom_units_.GetAt(it);
		if (s.SubString(s.Length() - 7, 8).ToLower() != ".sdcproj"){
			custom_units_.Delete(it);
			it--;
		}else{
			custom_units_.GetAt(it) = s.SubString(1, s.Length() - 8);
        }
	}
}
//---------------------------------------------------------------------------
};
#pragma package(smart_init)
