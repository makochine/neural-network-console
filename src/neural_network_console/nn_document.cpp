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

//sleep
#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/time.h>
#endif

#include <sys/stat.h>
#ifdef _WIN32
  #include <io.h>
#else
  #ifdef __APPLE__
    #include <sys/uio.h>
  #else
    #include <sys/io.h>
  #endif
#endif

#include "nn_document.h"
#include "file_util.h"
namespace sdeep_console{

static const int undo_max_size = 100;
//---------------------------------------------------------------------------
int NNTrainingResult::Open(FString path, FString dir)
{
	path_ = path;
	dir_ = dir;
	statistics_label_.Clear();
	statistics_value_.Clear();

	struct stat st;
	if (stat((path_ + dir_).PChar(), &st) == -1 ) {
		return false;
	}
	if ((st.st_mode & S_IFMT) != S_IFDIR ) {
		return false;
	}
	FString log_file_name = GetFullPath() + "result.ini";
	if (access(log_file_name.PChar(), 0) == 0) {
		int trial = 0;
		while (trial < 3) {
			FFastIni ini = FFastIni(log_file_name.PChar());
			double cost_multiply_add = ini.ReadDouble("result", "cost_multiply_add", NAN_VALUE);
			if (cost_multiply_add >= 0.0 && cost_multiply_add < NAN_VALUE) {
				schedule_ = ini.ReadInteger("result", "schedule", 0);
				complete_ = ini.ReadBool("result", "complete", false);
				training_ = ini.ReadDouble("result", "training", NAN_VALUE);
				validation_ = ini.ReadDouble("result", "validation", NAN_VALUE);
				last_epoch_ = ini.ReadInteger("result", "epoch", 0);
				max_epoch_ = ini.ReadInteger("result", "max_epoch", 1);
				best_epoch_ = ini.ReadInteger("result", "best_epoch", 0);
				best_validation_ = ini.ReadDouble("result", "best_validation", NAN_VALUE);
				pareto_optimal_ = training_ < NAN_VALUE_THRESHOLD && validation_ < NAN_VALUE_THRESHOLD;
				evaluated_ = access((GetFullPath() + "output_result.csv").PChar(), 0) == 0;
				comment_ = ini.ReadString("result", "comment", FString(""));

				FStringList statistics_str;
				ini.ReadSectionValues("statistics", &statistics_str);
				if (!statistics_str.GetSize()){
					statistics_label_.Add(FString("CostMultiplyAdd"));
					statistics_value_.Add(cost_multiply_add);
				}else {
					for (int is = 0 ; is < statistics_str.GetSize() ; is++){
						statistics_label_.Add(statistics_str.GetAt(is).SplitBy('='));
						statistics_value_.Add(FString(statistics_str.GetAt(is)).AsFloat());
					}
				}

				structure_expectation_of_log_myu_ = ini.ReadDouble("result", "structure_expectation_of_log_myu", 0.0);
				structure_expectation_of_log_sigma_ = ini.ReadDouble("result", "structure_expectation_of_log_sigma", 0.0);

				trial += 3;
			}else {
				#ifdef _WIN32
				Sleep(1);
				#else
				usleep(1000);
				#endif
			}

			trial++;
		}
	}

	return true;
}
//---------------------------------------------------------------------------
void NNTrainingResult::CheckParetoOptimal(const NNTrainingResult &result, const FString &pareto_optimal_statistics_label, int old)
{
	double cost_statistics = GetStatistics(pareto_optimal_statistics_label);
	double result_cost_statistics = result.GetStatistics(pareto_optimal_statistics_label);
	pareto_optimal_ &= cost_statistics >= 0.0;
	if (pareto_optimal_ && result_cost_statistics >= 0.0 &&
		GetBestValidation() >= 0.0 && result.GetBestValidation() >= 0.0) {
		pareto_optimal_ &=
			(result.GetBestValidation() > GetBestValidation() || result_cost_statistics > cost_statistics) ||
			(result.GetBestValidation() == GetBestValidation() && result_cost_statistics == cost_statistics && old);
	}
}
//---------------------------------------------------------------------------
double NNTrainingResult::GetStatistics(const FString &label) const
{
	int index = statistics_label_.IndexOf(label);
	if (index >= 0){
		return statistics_value_.GetAt(index);
	}else {
		return -1;
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
NNDocument::NNDocument() :
	sdeep_engine_type_(SDeepEngineType(SDeepEngineType::NNabla)),

	saved_update_count_(0), saved_config_update_count_(0), locked_(false), current_result_index_(-1),
	structure_expectation_of_log_myu_(0.0), structure_expectation_of_log_sigma_(0.0),
	pareto_optimal_statistics_label_(FString("CostMultiplyAdd"))
{
}
//---------------------------------------------------------------------------
NNDocument::~NNDocument()
{
	Clear(SDeepEngineType::NumEngineType);
}
//---------------------------------------------------------------------------
void NNDocument::Clear(SDeepEngineType sdeep_engine_type)
{
	if (sdeep_engine_type < SDeepEngineType::NumEngineType){
		sdeep_engine_type_ = sdeep_engine_type;
		assert (nn_engine_info_list_.GetSize() > (int)sdeep_engine_type);
		NNStructure::SetNNLayerComponents(&nn_engine_info_list_.GetAt((int)sdeep_engine_type)->GetComponents());
		NNStructure::SetNNStructureSearchPolicies(&nn_engine_info_list_.GetAt((int)sdeep_engine_type)->GetStructureSearchPolicies());
		NNConfig::SetSolverComponents(&nn_engine_info_list_.GetAt((int)sdeep_engine_type)->GetSolverComponents());
	}

	NNStructures::Clear();
	NNDatasets::Clear();
	NNConfig::Clear();

	description_ = "";

	saved_update_count_ = GetUpdateCount();
	saved_config_update_count_ = GetConfigUpdateCount();
	file_name_ = FString("");

	current_result_index_ = -1;
	results_.Clear();

	undo_list_.Clear();
	redo_list_.Clear();

	structure_expectation_of_log_myu_ = 0.0;
	structure_expectation_of_log_sigma_ = 0.0;
}
//---------------------------------------------------------------------------
int NNDocument::LoadFromFile(FString file_name, FString sdeep_console_path, int format_convert, int load_dataset_csv_and_result, int load_tmp_config)
{
	int result = true;

	FFastIni ini = FFastIni(file_name.PChar());

	//Engine Type
	FString engine_name = ini.ReadString("Engine", "SDeepEngineType", FString("NNabla"));
	if (format_convert) {
		FString fn = file_name_;
		Clear(sdeep_engine_type_);
		file_name_ = fn;
	}else {
		if (engine_name == "Torch") {
		  Clear(SDeepEngineType::Torch);
          file_name_ = file_name;
		}else if (engine_name == "NNabla" || engine_name == "DeLorean") {
			engine_name = "NNabla";
			Clear(SDeepEngineType::NNabla);
			file_name_ = file_name;
		}else if (engine_name == "Transformers") {
			Clear(SDeepEngineType::Transformers);
			file_name_ = file_name;
		}else {
			//Unknown engine type
			Clear();
			result = false;
		}
    }

	//Prepare converter
	FString current_engine_name;
	switch (sdeep_engine_type_) {
	case SDeepEngineType::NNabla:
		current_engine_name = "NNabla";
		break;
	case SDeepEngineType::Transformers:
		current_engine_name = "Transformers";
		break;
	case SDeepEngineType::Torch:
		current_engine_name = "Torch";
        break;
	default:
		;
	}
	const NNDocumentConverterSetting *setting = NULL;
	if (engine_name != current_engine_name) {
		for (int i = 0 ; i < nn_document_converter_setting_list_.GetSize() ;i++){
			if (nn_document_converter_setting_list_.GetAt(i)->GetEngineFrom() == engine_name &&
				nn_document_converter_setting_list_.GetAt(i)->GetEngineTo() == current_engine_name){
				setting = nn_document_converter_setting_list_.GetAt(i);
			}
		}
		if (setting == NULL) {
			//Converter not found
			result = false;
		}
	}

	//Network
	result &= IniToNetworks(ini, *dynamic_cast<NNStructures*>(this), false, setting);

	//Description
	SetDescription(ini.ReadString("Description", "Text", FString("")));

	//Config
	LoadConfigFromIni(&ini, load_tmp_config);
	int need_update_config_from_v220 = false;
	if (format_convert && GetNumOptimizer() == 1) {
		//Reset learning rate multiplier for incompatibility
		GetOptimizer(0).SetLearningRateMultiplier(1.0);
	}

	//Dataset
	if (!LoadDatasetFromIni(&ini)) {
		//project file of v2.20 and previous version
		need_update_config_from_v220 = true;
		SetBasicDataset(GetDatasetURI_Obsolete(0), GetDatasetURI_Obsolete(1), GetShuffle_Obsolete(), GetDatasetCacheEnabled_Obsolete(), GetImageNormalization_Obsolete());
	}
	if (load_dataset_csv_and_result) for (int i = 0 ; i < GetNumDataset() ; i++) {
		LoadDatasetFromFile(i, GetDataset(i).GetFileName().ReplaceStr("{SDCPATH}", sdeep_console_path.PChar()));
	}

	//Compatibility
	if (need_update_config_from_v220) {
		//Check network use
		FStringList optimizer_networks;
		FStringList monitor_networks;
		FStringList executor_networks;
		for (int in = 0 ; in < GetNumStructure() ; in++) {
			const NNStructure &structure = GetStructure(in);
			if (structure.GetName() == "Main") {
				optimizer_networks.Add(FString("Main"));
				monitor_networks.Add(FString("MainValidation"));
				executor_networks.Add(FString("MainRuntime"));
			}else {
				if (structure.GetUse_Obsolete(NNStructureUseType_Obsolete::Training)) {
					optimizer_networks.Add(structure.GetName());
				}
				if (structure.GetUse_Obsolete(NNStructureUseType_Obsolete::Validation)) {
					monitor_networks.Add(structure.GetName());
				}
				if (structure.GetUse_Obsolete(NNStructureUseType_Obsolete::Runtime)) {
					executor_networks.Add(structure.GetName());
				}
			}
		}

		//Set optimizer
		for (int io = 0; io < optimizer_networks.GetSize() ; io++) {
			if (io >= GetNumOptimizer()) {
				NNOptimizer &optimizer = AddOptimizer();
				const FString name_bak = optimizer.GetOptimizerName();
				optimizer = GetOptimizer(0);
				optimizer.SetOptimizerName(name_bak);
			}
			GetOptimizer(io).SetNetworkName(optimizer_networks.GetAt(io));
		}

		//Set monitor
		for (int im = 0; im < monitor_networks.GetSize() ; im++) {
			for (int im2 = 0; im2 < GetNumMonitor() ; im2++) {
				GetMonitor(im2).SetNetworkName(monitor_networks.GetAt(im));
			}
		}

		//Set executor
		for (int ie = 0; ie < executor_networks.GetSize() ; ie++) {
			if (ie >= GetNumExecutor()) {
				NNExecutor &executor = AddExecutor();
				const FString name_bak = executor.GetExecutorName();
				executor = GetExecutor(0);
				executor.SetExecutorName(name_bak);
			}
			GetExecutor(ie).SetNetworkName(executor_networks.GetAt(ie));
		}
	}

	RefreshDisplayValue();
	RefreshStatistics();
	if (load_dataset_csv_and_result) {
		RefreshResults(FString("\\"), true);
	}

	saved_update_count_ = GetUpdateCount();
	UpdateConfig();
	saved_config_update_count_ = GetConfigUpdateCount();

	return result;
}
//---------------------------------------------------------------------------
int NNDocument::SaveToFile(FString file_name, int save_all_properties_and_statistics, int silent_save, int save_tmp_config)
{
	int result = true;

	FFastIni ini = FFastIni(file_name.PChar());
	ini.Clear();

	//Engine Type
	switch (sdeep_engine_type_) {
	case SDeepEngineType::Torch:
		ini.Write<FString>("Engine", "SDeepEngineType", FString("Torch"));
		break;
	case SDeepEngineType::NNabla:
		ini.Write<FString>("Engine", "SDeepEngineType", FString("NNabla"));
		break;
	case SDeepEngineType::Transformers:
		ini.Write<FString>("Engine", "SDeepEngineType", FString("Transformers"));
		break;
	default:
		break;
	}

	//Network
	result &= NetworksToIni(*this, ini, save_all_properties_and_statistics);

	//Dataset
	SaveDatasetToIni(&ini);

	//Description
	ini.Write<FString>("Description", "Text", description_);

	//Config
	SaveConfigToIni(&ini, save_tmp_config);

	if (!silent_save){
		file_name_ = file_name;
		saved_update_count_ = GetUpdateCount();
		saved_config_update_count_ = GetConfigUpdateCount();
	}

	return result;
}
//---------------------------------------------------------------------------
FString NNDocument::GetSDeepEngineName() const
{
	switch (sdeep_engine_type_) {
	case SDeepEngineType::Torch:
        return FString("Torch");
	case SDeepEngineType::NNabla:
		return FString("NNabla");
	case SDeepEngineType::Transformers:
		return FString("Transformers");
	default:
		return FString("(None)");
	}
}
//---------------------------------------------------------------------------
FString NNDocument::NetworkToString(const NNStructure &network)
{
	FFastIni ini = FFastIni(NULL);
	NetworkToIni(FString(""), network, ini);

	FString result = ini.GetStringList().GetText();

	return result;
}
//---------------------------------------------------------------------------
void NNDocument::StringToNetwork(FString s, NNStructure &network)
{
	FStringList sl;
	sl.SplitStr(s, FString("\n"), FString(""));

	//Create network from string list
	FFastIni ini = FFastIni(NULL);
	ini.SetStringList(sl);
	network.SetNNLayerComponents(GetNNLayerComponents());
	network.SetNNStructureSearchPolicies(GetNNStructureSearchPolicies());
	IniToNetwork(FString(""), ini, network);
}
//---------------------------------------------------------------------------
void NNDocument::KeepForUndo(FString undo_name)
{
	redo_list_.Clear();
	NNStructures &backup = undo_list_.Insert(0, *this);
	backup.SetEditString(undo_name);
	prepared_for_undo_.Clear();

	while (undo_list_.GetSize() > undo_max_size){
		undo_list_.Delete(undo_list_.GetSize() - 1);
	}
}
//---------------------------------------------------------------------------
void NNDocument::PrepareUndoInAdvance()
{
	prepared_for_undo_ = *this;
}
//---------------------------------------------------------------------------
void NNDocument::KeepPreparedForUndo(FString undo_name)
{
	redo_list_.Clear();
	NNStructures &backup = undo_list_.Insert(0, prepared_for_undo_);
	backup.SetEditString(undo_name);
	prepared_for_undo_.Clear();

	while (undo_list_.GetSize() > undo_max_size){
		undo_list_.Delete(undo_list_.GetSize() - 1);
	}
}
//---------------------------------------------------------------------------
int NNDocument::Undo(int undo_index)
{
	if (undo_index >= undo_list_.GetSize()) {
		return false;
	}

	for (int i = 0 ; i <= undo_index ; i++) {
		NNStructure &redo = redo_list_.Insert(0, *this);
		redo.SetEditString(GetUndoName(0));
		*dynamic_cast<NNStructures*>(this) = undo_list_.GetAt(0);
		undo_list_.Delete(0);
	}
	return true;
}
//---------------------------------------------------------------------------
int NNDocument::Redo(int redo_index)
{
	if (redo_index >= redo_list_.GetSize()) {
		return false;
	}

	for (int i = 0 ; i <= redo_index ; i++) {
		NNStructures &undo = undo_list_.Insert(0, *this);
		undo.SetEditString(GetRedoName(0));
		*dynamic_cast<NNStructures*>(this) = redo_list_.GetAt(0);
		redo_list_.Delete(0);
	}
	return true;
}
//---------------------------------------------------------------------------
FString NNDocument::Cut()
{
	FString result = Copy();
	DeleteSelected();

	return result;
}
//---------------------------------------------------------------------------
FString NNDocument::Copy() const
{
	NNStructure network;
	network = GetStructure(GetActiveStructureIndex());

	network.InverseLayerSelection();
	network.DeleteSelected();

	FFastIni ini = FFastIni(NULL);
	NetworkToIni(FString(""), network, ini);

	FString result = ini.GetStringList().GetText();

	return result;
}
//---------------------------------------------------------------------------
void NNDocument::Paste(const FString &clipboard_str, int escape_position)
{
	FStringList sl;
	sl.SplitStr(clipboard_str, FString("\n"), FString(""));

	//Create network from string list
	FFastIni ini = FFastIni(NULL);
	ini.SetStringList(sl);
	NNStructure network;
	network.SetNNLayerComponents(GetNNLayerComponents());
	//IniToNetwork(ini, *(NNStructure*)this, true);
	IniToNetwork(FString(""), ini, network);

	NNStructure::Paste(network, true, FString(""), escape_position);
}
//---------------------------------------------------------------------------
int NNDocument::Mutate()
{
	FListT<int> check_structure_index = GetUsedStructureIndex();
	return ((NNStructures &)*this).Mutate(check_structure_index);
}
//---------------------------------------------------------------------------
void NNDocument::CheckError(int check_dataset_error)
{
	NNStructures::RefreshDisplayValue();
	NNStructures::RefreshStatistics();
	FListT<int> check_structure_index = GetUsedStructureIndex();
	check_structure_index.Remove(GetActiveStructureIndex());
	check_structure_index.Insert(0, GetActiveStructureIndex());//Check error of current structure first

	NNStructures structures_sorted;
	structures_sorted = *this;
	structures_sorted.ArrangeAllStructure();
	structures_sorted.CheckError(check_structure_index);
	errors_ = structures_sorted.GetErrors();

	//Configuration Error (Optimizer)
	for (int io = 0 ; io < GetNumOptimizer() ; io++) {
		const NNOptimizer &optimizer = GetOptimizer(io);
		const FString &network_name = optimizer.GetNetworkName();
		//Check if network exists
		if (GetStructureIndexByName(network_name) < 0){
			NNError &new_error = errors_.Add();
			new_error.SetString(
				FString("Optimizer \"") +
				FString(optimizer.GetOptimizerName()) +
				FString("\" : Network \"") +
				FString(network_name) +
				FString("\" is not defined.")
				);
		}

		FStringList dataset_names = optimizer.GetDatasetNames();
		for (int id = 0 ; id < dataset_names.GetSize() ; id++){
			const FString dataset_name = dataset_names.GetAt(id);
			if (GetDatasetIndexByName(dataset_name) < 0) {
				NNError &new_error = errors_.Add();
				new_error.SetString(
					FString("Optimizer \"") +
					FString(optimizer.GetOptimizerName()) +
					FString("\" : Dataset \"") +
					FString(dataset_name) +
					FString("\" is not found.")
					);
			}
		}
	}

	//Configuration Error (Monitor)
	for (int im = 0 ; im < GetNumMonitor() ; im++) {
		const NNMonitor &monitor = GetMonitor(im);
		const FString &network_name = monitor.GetNetworkName();
		//Check if network exists
		int network_index = network_name != "MainValidation" ?
			GetStructureIndexByName(network_name) :
			GetStructureIndexByName(FString("Main"));
		if (network_index < 0){
			NNError &new_error = errors_.Add();
			new_error.SetString(
				FString("Monitor \"") +
				FString(monitor.GetMonitorName()) +
				FString("\" : Network \"") +
				FString(network_name) +
				FString("\" is not defined.")
				);
		}

		FStringList dataset_names = monitor.GetDatasetNames();
		for (int id = 0 ; id < dataset_names.GetSize() ; id++){
			const FString dataset_name = dataset_names.GetAt(id);
			if (GetDatasetIndexByName(dataset_name) < 0) {
				NNError &new_error = errors_.Add();
				new_error.SetString(
					FString("Monitor \"") +
					FString(monitor.GetMonitorName()) +
					FString("\" : Dataset \"") +
					FString(dataset_name) +
					FString("\" is not found.")
					);
			}
		}
	}

	//Configuration Error (Executor)
	for (int ie = 0 ; ie < GetNumExecutor() ; ie++) {
		const NNExecutor &executor = GetExecutor(ie);
		const FString &network_name = executor.GetNetworkName();
		//Check if network exists
		int network_index = network_name != "MainRuntime" ?
			GetStructureIndexByName(network_name) :
			GetStructureIndexByName(FString("Main"));
		if (network_index < 0){
			NNError &new_error = errors_.Add();
			new_error.SetString(
				FString("Executor \"") +
				FString(executor.GetExecutorName()) +
				FString("\" : Network \"") +
				FString(network_name) +
				FString("\" is not defined.")
				);
		}

		FStringList dataset_names = executor.GetDatasetNames();
		for (int id = 0 ; id < dataset_names.GetSize() ; id++){
			const FString dataset_name = dataset_names.GetAt(id);
			if (GetDatasetIndexByName(dataset_name) < 0) {
				NNError &new_error = errors_.Add();
				new_error.SetString(
					FString("Executor \"") +
					FString(executor.GetExecutorName()) +
					FString("\" : Dataset \"") +
					FString(dataset_name) +
					FString("\" is not found.")
					);
			}
		}
	}


	if (check_dataset_error) {
		//Enum variable names
		FStringList variable_names;
		for (int id = 0 ; id < GetNumDataset() ; id++) {
			for (int ic = 0 ; ic < GetDataset(id).GetNumColumn() ; ic++) {
				FString header_1 = GetDataset(id).GetColumnName(ic);
				FString variable = header_1.SplitBy(':');
				FString variable_name = variable.SplitBy("__");
				if (variable_names.IndexOf(variable_name) < 0) {
					variable_names.Add(variable_name);
				}
			}
		}

		if (GetMaxEpoch() && GetCommandType() != SDeepCommandType::CreateTrainingResult) {
			//Combination Error
			for (int il = 0 ; il < GetNumLayer() ; il++) {
				const NNLayer &layer = GetLayer(il);
				const FString variable_name = layer.GetDisplayPropertyValue(FString("Dataset"));
				if (variable_name != "") {
					const FString generator_name = layer.GetDisplayPropertyValue(FString("Generator"));
					if (generator_name == "" || generator_name == "None") {
						if (variable_names.IndexOf(variable_name) < 0 && variable_name.SubString(1, 1) != "%") {
							NNError &new_error = errors_.Add();
							new_error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
							new_error.SetPropertyName(FString("Dataset"));
							new_error.SetString(
								FString((
									layer.GetDisplayPropertyValue(FString("Name")) +
									FString("->Dataset : Variable \"") +
									variable_name +
									FString("\" is not found in dataset.")
								)));
						}
					}
				}
			}

			//Dataset Error
			for (int id = 0 ; id < GetNumDataset() ; id++) {
				const NNDataset &dataset = GetDataset(id);
				if (dataset.GetFileName() == "" || dataset.GetNumData() == 0) {
					NNError &new_error = errors_.Add();
					new_error.SetString(
						FString("Dataset \"") +
						FString(dataset.GetName()) +
						FString("\" is empty."));
				}else {
					int used_in_optimizer_monitor = false;
					for (int io = 0 ; io < GetNumOptimizer() && !used_in_optimizer_monitor ; io++){
						const NNOptimizer &optimizer = GetOptimizer(io);
						used_in_optimizer_monitor |= optimizer.GetDatasetNames().IndexOf(dataset.GetName()) >= 0;
					}
					for (int im = 0 ; im < GetNumMonitor() && !used_in_optimizer_monitor ; im++){
						const NNMonitor &monitor = GetMonitor(im);
						used_in_optimizer_monitor |= monitor.GetDatasetNames().IndexOf(dataset.GetName()) >= 0;
					}
					if (used_in_optimizer_monitor && dataset.GetNumData() < GetBatchSize()) {
						NNError &new_error = errors_.Add();
						new_error.SetString(
							FString("Batch size is larger than dataset \"") +
							FString(dataset.GetName()) +
							FString("\".")
							);
					}
				}
			}
        }
	}
}
//---------------------------------------------------------------------------
void NNDocument::RefreshResults(FString path_separator, int select_last)
{
	FString result_path = GetResultPath(path_separator);

	FString last_result = GetResultName(current_result_index_);
	results_.Clear();
	FStringList result_files = EnumFiles(result_path.PChar());
    result_files.Sort();
	for (int i = result_files.GetSize() - 1 ; i >= 0 ; i--) {
		if (result_files.GetAt(i) != "." && result_files.GetAt(i) != "..") {
			NNTrainingResult &result = results_.Add();
			if (result.Open(GetResultPath(path_separator), result_files.GetAt(i))){
				if (result.GetDir() == last_result) {
					current_result_index_ = results_.GetSize() - 1;
				}
			}else {
				results_.Delete(results_.GetSize() - 1);
			}
		}
	}
	for (int i = 0 ; i < results_.GetSize() ; i++) {
		for (int i2 = 0 ; i2 < results_.GetSize() ; i2++) if (i != i2){
			results_.GetAt(i).CheckParetoOptimal(results_.GetAt(i2), pareto_optimal_statistics_label_, i < i2);
		}
	}
	if (select_last || current_result_index_ >= results_.GetSize()) {
		current_result_index_ = 0;
	}
}
//---------------------------------------------------------------------------
int NNDocument::NetworkToIni(FString name, const NNStructure &structure, FFastIni &ini, int save_display_value)
{
	int result = true;
	FString prefix = name != "" ? name + FString("_") : FString("");

	/*
	//Use
	ini.Write<int>(prefix + FString("Network_Global"), FString("Training"), structure.GetUse(NNStructureUseType::Training));
	ini.Write<int>(prefix + FString("Network_Global"), FString("Validation"), structure.GetUse(NNStructureUseType::Validation));
	ini.Write<int>(prefix + FString("Network_Global"), FString("Runtime"), structure.GetUse(NNStructureUseType::Runtime));
	*/

	//Network
	ini.Write<int>(prefix + FString("Network_Global"), FString("NumLayer"), structure.GetNumLayer());
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);
		FString section_name = prefix + FString("Layer_") + FString(il);
		ini.Write<int>(section_name.PChar(), "ID", layer.GetLayerID());
		ini.Write<FString>(section_name.PChar(), "Type", layer.GetType());
		for (int ipos = 0 ; ipos < 4 ; ipos++) {
			ini.Write<double>(section_name, FString("Position_") + FString(ipos), layer.GetPosition(ipos));
		}

		int property_index = 0;
		for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
			const NNLayerProperty &property = layer.GetProperty(ip);
			if (property.GetEditable() || save_display_value) {
				FString base_key_name = FString("Property_") + FString(property_index) + FString("_");
				ini.Write<FString>(section_name, base_key_name + FString("Name"), property.GetName());
				ini.Write<FString>(section_name, base_key_name + FString("Value"), save_display_value ? property.GetDisplayValue() : property.GetValue());
				if (save_display_value){
					ini.Write<FString>(section_name, base_key_name + FString("Type"), property.EncodePropertyType(property.GetType()));
					if (property.GetError()) {
						ini.Write<int>(section_name, base_key_name + FString("Warning"), 1);
					}
					if (property.GetDynamic()) {
						ini.Write<int>(section_name, base_key_name + FString("Editable"), property.GetEditable());
                    }
				}
				property_index++;
			}
		}
		ini.Write<int>(section_name.PChar(), "NumProperty", property_index);

		if (save_display_value){
			if (layer.GetRepeatIDs().GetSize()){
				FString repeat_ids, repeat_times;
				for (int ir = 0 ; ir < layer.GetRepeatIDs().GetSize() ; ir++){
					FString repeat_id = layer.GetRepeatIDs().GetAt(ir);
					if (repeat_ids != ""){
						repeat_ids += FString(",");
						repeat_times += FString(",");
					}
					repeat_ids += repeat_id;
					repeat_times += FString(structure.GetRepeatNum(repeat_id));
				}
				ini.Write<FString>(section_name.PChar(), "RepeatIDs", repeat_ids);
				ini.Write<FString>(section_name.PChar(), "RepeatTimes", repeat_times);
			}
		}
	}

	ini.Write<int>(prefix + FString("Network_Global"), FString("NumLink"), structure.GetNumLink());
	for (int ili = 0 ; ili < structure.GetNumLink() ; ili++) {
		const NNLink &link = structure.GetLink(ili);
		FString section_name = prefix + FString("Link_") + FString(ili);
		ini.Write<int>(section_name.PChar(), "ID", link.GetLinkID());
		ini.Write<int>(section_name.PChar(), "SourceLayerID", link.GetSourceLayerID());
		ini.Write<int>(section_name.PChar(), "SourcePinIndex", link.GetSourcePinIndex());
		ini.Write<int>(section_name.PChar(), "DestLayerID", link.GetDestLayerID());
		ini.Write<int>(section_name.PChar(), "DestPinIndex", link.GetDestPinIndex());
	}

	//Statistics
	if (save_display_value) {
		FString section_name = prefix + FString("Statistics");
		const FListP<NNStatistics> &statistics = structure.GetStatistics();
		for (int is = 0 ; is < statistics.GetSize() ; is++) {
			FString base_key_name = FString("Statistics_") + FString(is) + FString("_");
			ini.Write<FString>(section_name, base_key_name + FString("Name"), statistics.GetAt(is).GetLabel());
			ini.Write<double>(section_name, base_key_name + FString("Max"), statistics.GetAt(is).GetMax());
			ini.Write<double>(section_name, base_key_name + FString("Sum"), statistics.GetAt(is).GetSum());
		}
		ini.Write<int>(section_name.PChar(), "NumStatistics", statistics.GetSize());
	}

	return result;
}
//---------------------------------------------------------------------------
int NNDocument::NetworksToIni(const NNStructures &structures, FFastIni &ini, int save_display_value)
{
	int result = true;

	if (structures.GetNumStructure() > 1) {
		// multiple networks
		ini.Write<int>(FString("Global"), FString("NumNetwork"), structures.GetNumStructure());

		for (int i = 0 ; i < structures.GetNumStructure() ; i++) {
			NNStructure structure;
			structure = structures.GetStructure(i);
			ini.Write<FString>(FString("Global"), FString("Network_") + FString(i), structure.GetName());
			if (save_display_value) {
				structure.RefreshDisplayValue();
				structure.RefreshStatistics();
			}
			result &= NetworkToIni(structure.GetName(), structure, ini, save_display_value);
		}
	}else {
		// single network
		result &= NetworkToIni(FString(""), structures, ini, save_display_value);
	}

	return result;
}
//---------------------------------------------------------------------------
int NNDocument::IniToNetwork(
	FString name, FFastIni &ini, NNStructure &structure, int select_loaded,
	const NNDocumentConverterSetting *converter_setting)
{
	int result = true;
	FString prefix = name != "" ? name + FString("_") : FString("");

	FListT<int> layer_ids;

	//for project file conversion
	FListT<int> insert_request_layer_id;
	FStringList insert_request_layer_type;

	//Network
	const int num_layer = ini.ReadInteger(prefix + FString("Network_Global"), FString("NumLayer"), 0);
	for (int il = 0 ; il < num_layer ; il++) {
		FString section_name = prefix + FString("Layer_") + FString(il);
		const int layer_id = ini.ReadInteger(section_name.PChar(), "ID", -1);
		if (layer_id < 0) {
			result = false;
		}else {
			FStringList additional_process;//macro processed after layer creation
			FString layer_type = ini.ReadString(section_name.PChar(), "Type", FString(""));
			if (layer_type == "") {
				//for upward compatibility (old format doesn't have type property)
				layer_type = ini.ReadString(section_name.PChar(), "Name", FString(""));
			}
			if (converter_setting) {
				//Convert Layer type between engine
				for (int ifrom = 0 ; ifrom < converter_setting->GetLayerFrom().GetSize() ; ifrom++) {
					if (converter_setting->GetLayerFrom().GetAt(ifrom) == layer_type) {
						int convert = true;
						FString condition = converter_setting->GetLayerCondition().GetAt(ifrom);
						while (condition != "") {
							int condition_result = false;
							FString condition_1 = condition.SplitBy(',');
							FString condition_property = condition_1.SplitBy(" is ");

							const int num_property = ini.ReadInteger(section_name.PChar(), "NumProperty", 0);
							for (int ip = 0 ; ip < num_property ; ip++) {
								const FString base_key_name = FString("Property_") + FString(ip) + FString("_");
								FString property_name = ini.ReadString(section_name, base_key_name + FString("Name"), FString(""));
								if (property_name == condition_property) {
									FString value = ini.ReadString(section_name, base_key_name + FString("Value"), FString(""));
									condition_result = value == condition_1;
								}
							}
							convert &= condition_result;
						}
						if (convert) {
							layer_type = converter_setting->GetLayerTo().GetAt(ifrom);
							if (converter_setting->GetLayerAdditionalProcess().GetAt(ifrom) != "") {
								additional_process.Add(converter_setting->GetLayerAdditionalProcess().GetAt(ifrom));
							}
						}
					}
				}
			}
			const int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(layer_type);
			if (component_index < 0) {
				result = false;
			}else {
				NNLayer &new_layer = structure.AddLayer(component_index);
				new_layer.SetLayerID(layer_id);
				layer_ids.Add(layer_id);
				for (int ipos = 0 ; ipos < 4 ; ipos++) {
					new_layer.SetPosition(ipos, ini.ReadDouble(section_name, FString("Position_") + FString(ipos), new_layer.GetPosition(ipos)));
				}
				new_layer.SetCurrentPosition(new_layer.GetPosition());

                // Load properties
				const int num_property = ini.ReadInteger(section_name.PChar(), "NumProperty", 0);
				for (int ip = 0 ; ip < num_property ; ip++) {
					const FString base_key_name = FString("Property_") + FString(ip) + FString("_");
					FString property_name = ini.ReadString(section_name, base_key_name + FString("Name"), FString(""));
					FString value = ini.ReadString(section_name, base_key_name + FString("Value"), FString(""));
					if (converter_setting) {
						//Convert Property name (and value) between engine
						const int name_index = converter_setting->GetPropertyFrom().IndexOf(layer_type + FString(".") + property_name);
						if (name_index >= 0) {
							property_name = converter_setting->GetPropertyTo().GetAt(name_index);
							if (converter_setting->GetPropertyAdditionalProcess().GetAt(name_index) != "") {
								additional_process.Add(converter_setting->GetPropertyAdditionalProcess().GetAt(name_index));
							}
						}

						if (value.SubString(1, 1) == "*") {
                        	//Citation
							const int cite_index = converter_setting->GetPropertyFrom().IndexOf(layer_type + FString(".") + value.SubString(2, value.Length() - 1));
							if (cite_index >= 0) {
								value = FString("*") + converter_setting->GetPropertyTo().GetAt(cite_index);
							}
						}

						//Property value
						const int property_index = converter_setting->GetValueFrom().IndexOf(layer_type + FString(".") + property_name + FString(".") + value);
						if (property_index >= 0) {
							value = converter_setting->GetValueTo().GetAt(property_index);
							additional_process.Add(converter_setting->GetValueAdditionalProcess().GetAt(property_index));
						}

					}

					const int property_index = new_layer.GetPropertyIndexByName(property_name);
					if (property_index >= 0) {
						new_layer.SetPropertyValue(property_index, value);
					}else if (new_layer.GetType() == "Unsupported") {
						new_layer.AddProperty(-1, property_name, FString(""), true, false, false, false, NNLayerPropertyType::Error, value, true);
					}else if (new_layer.GetType() == "Unit") {
						new_layer.AddProperty(new_layer.GetPropertyIndexByName("Output"), property_name, FString(""), true, false, true, false, NNLayerPropertyType::Error, value, true, true);
					}else if (property_name == "SkipAtInspection"){
						const int property_index2 = new_layer.GetPropertyIndexByName(FString("SkipAtTest"));
						if (property_index2 >= 0) {
							new_layer.SetPropertyValue(property_index2, value);
						}
					}
				}

				//Process macro for project file conversion
				for (int iap = 0 ; iap < additional_process.GetSize() ; iap++) {
					FString &ss = additional_process.GetAt(iap);
					while (ss != ""){
						FString s = ss.SplitBy(',');
						if (s.Pos("set")) {
							s.SplitBy(' ');
							FString property_name = s.SplitBy(' ');
							new_layer.SetPropertyValue(property_name, s);
						}else if (s.Pos("insert")){
							s.SplitBy(' ');
							insert_request_layer_id.Add(new_layer.GetLayerID());
							insert_request_layer_type.Add(s);
						}else if (s.Pos("inv_sign")) {
							s.SplitBy(' ');
							const NNLayerProperty *property = new_layer.GetPropertyByName(s);
							if (property) {
								if (property->GetValue() != "") {
									new_layer.SetPropertyValue(s, FString(-property->GetValue().AsFloat()));
								}
							}
						}else if (s.Pos("inv_kernel")) {
							s.SplitBy(' ');
							const NNLayerProperty *property = new_layer.GetPropertyByName(s);
							if (property) if (property->GetValue() != "") {
								if (property->GetValue().Pos("*inv_kernel")) {
									new_layer.SetPropertyValue(s, property->GetValue().SubString(1, property->GetValue().Pos("*inv_kernel") - 1));
								}else {
									new_layer.SetPropertyValue(s, property->GetValue() + FString("*inv_kernel"));
								}
							}
						}else if (s.Pos("refresh_affine_weight_shape")) {
							s.SplitBy(' ');
							const NNLayerProperty *property = new_layer.GetPropertyByName(s);
							if (property) if (property->GetValue() != "") {
								if (property->GetValue().Pos("*refresh_affine_weight_shape")) {
									new_layer.SetPropertyValue(s, property->GetValue().SubString(1, property->GetValue().Pos("*refresh_affine_weight_shape") - 1));
								}else {
									new_layer.SetPropertyValue(s, property->GetValue() + FString("*refresh_affine_weight_shape"));
								}
							}
						}else if (s.Pos("clear_shape")) {
							s.SplitBy(' ');
							const NNLayerProperty *property = new_layer.GetPropertyByName(s);
							if (property) if (property->GetValue() != "") {
								if (!property->GetValue().Pos("*clear_shape")) {
									new_layer.SetPropertyValue(s, property->GetValue() + FString("*clear_shape"));
								}
							}
						}else if (s.Pos("sqrt")) {
							s.SplitBy(' ');
							const NNLayerProperty *property = new_layer.GetPropertyByName(s);
							if (property) if (property->GetValue() != "") {
								if (!property->GetValue().Pos("*sqrt")) {
									new_layer.SetPropertyValue(s, property->GetValue() + FString("*sqrt"));
								}
							}
						}else if (s.Pos("sqr")) {
							s.SplitBy(' ');
							const NNLayerProperty *property = new_layer.GetPropertyByName(s);
							if (property) if (property->GetValue() != "") {
								if (!property->GetValue().Pos("*sqr")) {
									new_layer.SetPropertyValue(s, property->GetValue() + FString("*sqr"));
								}
							}
						}else if (s.Pos("inv")) {
							s.SplitBy(' ');
							const NNLayerProperty *property = new_layer.GetPropertyByName(s);
							if (property) if (property->GetValue() != "") {
								float v = property->GetValue().AsFloat();
								if (v > 0) {
									new_layer.SetPropertyValue(s, FString(1.0 / v));
								}
							}
						}
					}
				}
			}
		}
	}

	const int num_link = ini.ReadInteger(prefix + FString("Network_Global"), FString("NumLink"), 0);
	for (int ili = 0 ; ili < num_link ; ili++) {
		FString section_name = prefix + FString("Link_") + FString(ili);
		const int link_id = ini.ReadInteger(section_name.PChar(), "ID", -1);
		if (link_id < 0) {
			result = false;
		}else {
			const int source_layer_id = ini.ReadInteger(section_name.PChar(), "SourceLayerID", -1);
			const int source_pin_index = ini.ReadInteger(section_name.PChar(), "SourcePinIndex", 0);
			const int dest_layer_id = ini.ReadInteger(section_name.PChar(), "DestLayerID", -1);
			const int dest_pin_index = ini.ReadInteger(section_name.PChar(), "DestPinIndex", 0);
			if (source_layer_id < 0 || dest_layer_id < 0) {
				result = false;
			}else {
				NNLink *new_link = structure.AddLink(source_layer_id, source_pin_index, dest_layer_id, dest_pin_index);
				if (!new_link) {
#ifdef _WIN32
					result = false;
#else
					// TODO:Work around for ML-2593 result = false;
#endif
				}else {
					new_link->SetLinkID(link_id);
				}
			}
		}
	}

	for (int i = 0 ; i < insert_request_layer_id.GetSize() ; i++) {
		const int insert_component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(insert_request_layer_type.GetAt(i));
		if (insert_component_index >= 0) {
			structure.InsertLayer(insert_request_layer_id.GetAt(i), insert_component_index);
		}
	}

	//structure.EscapeLayerName(); # Do not escape to load weight in sdnx file with old style layer name
	structure.ClearSelection();
	if (select_loaded) for (int il = 0 ; il < layer_ids.GetSize() ; il++) {
		structure.SetLayerSelected(layer_ids.GetAt(il), true, false);
	}

	//Obsolete
	structure.SetUse_Obsolete(NNStructureUseType_Obsolete::Training, ini.ReadInteger(prefix + FString("Network_Global"), FString("Training"), structure.GetUse_Obsolete(NNStructureUseType_Obsolete::Training)));
	structure.SetUse_Obsolete(NNStructureUseType_Obsolete::Validation, ini.ReadInteger(prefix + FString("Network_Global"), FString("Validation"), structure.GetUse_Obsolete(NNStructureUseType_Obsolete::Validation)));
	structure.SetUse_Obsolete(NNStructureUseType_Obsolete::Runtime, ini.ReadInteger(prefix + FString("Network_Global"), FString("Runtime"), structure.GetUse_Obsolete(NNStructureUseType_Obsolete::Runtime)));

	return result;
}
//---------------------------------------------------------------------------
int NNDocument::IniToNetworks(
	FFastIni &ini, NNStructures &structures, int select_loaded,
	const NNDocumentConverterSetting *converter_setting)
{
	int result = true;

	int num_network = ini.ReadInteger(FString("Global"), FString("NumNetwork"), 0);
	if (num_network) {
		// multiple networks
		for (int i = 0 ; i < num_network ; i++) {
			FString name = ini.ReadString(FString("Global"), FString("Network_") + FString(i), FString(""));
			while (structures.GetNumStructure() <= i) {
				structures.AddStructure();
			}
			result &= IniToNetwork(name, ini, structures.GetStructure_(i), select_loaded, converter_setting);
			structures.GetStructure_(i).SetName(name);
		}
	}else {
		// single network
		result &= IniToNetwork(FString(""), ini, *(NNStructure*)&structures, select_loaded, converter_setting);
	}

	CompleteNetworks(structures);
	return result;
}
//---------------------------------------------------------------------------
void NNDocument::CompleteNetworks(NNStructures &structures)
{
	for (int i = structures.GetNumStructure() - 1 ; i >= 0 ; i--) {
		structures.SetActiveStructure(i);
		structures.RefreshDisplayValue();
		structures.RefreshStatistics();
	}
}
//---------------------------------------------------------------------------
int NNDocument::IsStructureUsed(const FString &_network_name) const
{
	FString network_name = _network_name != "" ? _network_name : GetName();
	int used = false;

	//Optimizer
	for (int io = 0 ; io < GetNumOptimizer() && !used ; io++){
		const NNOptimizer &optimizer = GetOptimizer(io);
		used |= network_name == optimizer.GetNetworkName();
	}
	//Monitor
	for (int im = 0 ; im < GetNumMonitor() && !used ; im++){
		const NNMonitor &monitor = GetMonitor(im);
		used |= network_name == monitor.GetNetworkName();
	}
	//Executor
	for (int ie = 0 ; ie < GetNumExecutor() && !used ; ie++){
		const NNExecutor &executor = GetExecutor(ie);
		used |= network_name == executor.GetNetworkName();
	}

	return used;
}
//---------------------------------------------------------------------------
FListT<int> NNDocument::GetUsedStructureIndex() const
{
	FListT<int> result;
	for (int in = 0 ; in < GetNumStructure() ; in++) {
		if (IsStructureUsed(GetStructure(in).GetName())){
			result.Add(in);
		}
	}
	return result;
}
//---------------------------------------------------------------------------
void NNDocument::FillParameterName(const FStringList &parameter_names, const FStringList &parameter_file_names)
{
	for (int in = 0 ; in < GetNumStructure() ; in++) {
		SetActiveStructure(in);
		RefreshDisplayValue();

		for (int il = 0 ; il < GetNumLayer() ; il++) {
			const NNLayer &layer = GetLayer(il);
			FString layer_name = layer.GetPropertyValue(FString("Name"));
			while (layer_name.Pos("{")){
				const int p = layer_name.Pos("{");
				const int p2 = layer_name.Pos("}");
				layer_name = layer_name.SubString(1, p - 1) + layer_name.SubString(p2 + 1, layer_name.Length());
			}

			FStringList weight_file_names;
			FStringList property_names;
			FStringList result_files_tmp = parameter_file_names;
			FStringList weight_scopes_tmp = parameter_names;
			FStringList weight_scopes_org_tmp = parameter_names;
			for (int i = 0 ; i < weight_scopes_tmp.GetSize() ; i++){
                // remove [number] from weight_scopes
				while (true){
					int p = weight_scopes_tmp.GetAt(i).Pos("[");
					if (!p){
                        break;
					}
					FString left = weight_scopes_tmp.GetAt(i).SubString(1, p - 1);
					FString right = weight_scopes_tmp.GetAt(i).SubString(p + 1, weight_scopes_tmp.GetAt(i).Length() - p);
					int p2 = right.Pos("]");
					if (!p2){
						break;
					}
					weight_scopes_tmp.GetAt(i) = left + right.SubString(p2 + 1, right.Length() - p2);
				}
			}

			int found = true;
			while (found){
				found = false;
				if (layer.GetType() == "Parameter" || layer.GetType() == "WorkingMemory") {
					int p = weight_scopes_tmp.IndexOf(layer_name);
					if (p >= 0) {
						property_names.Add(FString("File"));
						weight_file_names.Add(result_files_tmp.GetAt(p));
						weight_scopes_tmp.Delete(p);
						result_files_tmp.Delete(p);
						found = true;
					}
				}else {
					const NNLayerComponent *component = layer.GetComponent();
					for (int ic = 0 ; ic < layer.GetNumSideConnector(0) ; ic++) {
						const NNLayerSideConnector &connector = layer.GetSideConnector(0, ic);
						if (connector.GetKind() == NNLayerComponentInputKind::Parameter) {
							FString property_name = connector.GetName() + FString(".File");
							const NNLayerProperty *property = layer.GetPropertyByName(property_name);
							FString layer_parameter_scope = layer.GetDisplayPropertyValue(FString("ParameterScope"));
							if (layer_parameter_scope != ""){
								layer_parameter_scope += "/";
							}
							if (property) {
								for (int io = 0 ; io < 2 ; io++) for (int ip = 0 ; ip < weight_scopes_tmp.GetSize() ; ip++){
									FString weight_scope = io == 0 ? weight_scopes_tmp.GetAt(ip) : weight_scopes_org_tmp.GetAt(ip);
									// ShowMessage((weight_scopes_tmp.GetAt(ip) + FString(" - ") + layer_parameter_scope + connector.GetParameterScope()).PChar());
									if (weight_scope.Pos(layer_parameter_scope +
											component->GetParameterScope() + FString("/") +
											connector.GetName() + FString(".")) == 1 ||
										weight_scope.Pos(layer_parameter_scope +
											component->GetParameterScope() + FString("/") +
											connector.GetName() + FString("[")) == 1) {
										property_names.Add(property_name);
										weight_file_names.Add(result_files_tmp.GetAt(ip));
										weight_scopes_tmp.Delete(ip);
										weight_scopes_org_tmp.Delete(ip);
										result_files_tmp.Delete(ip);
										found = true;
										ip--;
									}else if (weight_scope == layer_parameter_scope + connector.GetParameterScope()) {
										//ShowMessage((weight_scopes_tmp.GetAt(ip) + FString("===") + layer_parameter_scope + connector.GetParameterScope()).PChar());
										//necessary to make an exact match because things like affine/Wb and affine/W must not be matched.
										//Exact match
										property_names.Add(property_name);
										weight_file_names.Add(result_files_tmp.GetAt(ip));
										weight_scopes_tmp.Delete(ip);
										weight_scopes_org_tmp.Delete(ip);
										result_files_tmp.Delete(ip);
										found = true;
										ip--;
									}
								}
							}
						}
					}
				}
			}

			if (weight_file_names.GetSize()) {
				//Clear existing weight file
				for (int ip = 0; ip < layer.GetNumProperty() ; ip++) {
					const NNLayerProperty &property = layer.GetProperty(ip);
					if (property.GetType() == NNLayerPropertyType::File) {
						SetLayerPropertyValue(layer.GetLayerID(), property.GetName(), FString(""));
					}
				}

				for (int i = 0; i < weight_file_names.GetSize() ; i++) {
					FString value = GetLayerByID(layer.GetLayerID())->GetPropertyValue(property_names.GetAt(i));
					if (value != "") {
						value += ";";
					}
					value += weight_file_names.GetAt(i);
					SetLayerPropertyValue(layer.GetLayerID(), property_names.GetAt(i), value);
				}
			}
		}
	}
	SetActiveStructure(0);
	return;
}
//---------------------------------------------------------------------------
double NNDocument::GetRuntimeNetworkStatistics(const FString &label)
{
	double statistics = 0.0;
	for (int i = 0 ; i < GetNumExecutor() ; i++) {
		const NNExecutor &executor = GetExecutor(i);
		int structure_index = GetStructureIndexByName(executor.GetNetworkName());
		if (structure_index < 0 && executor.GetNetworkName() == "MainRuntime"){
			structure_index = GetStructureIndexByName(FString("Main"));
		}
		if (structure_index >= 0){
			NNStructure &structure = GetStructure_(structure_index);
			structure.RefreshDisplayValue();
            structure.RefreshStatistics();

			long long statistics_1 = structure.GetStatistics_Sum(label);
			statistics += statistics_1;
		}
	}
	return statistics;
}
//---------------------------------------------------------------------------
int NNDocument::Unitize(const FString &network_name)
{
	//Preparation for unitize
	int start_layer_id = -1;
	float start_layer_pos[4];
	memset(start_layer_pos, 0, sizeof(start_layer_pos));
	int stop_layer_id = -1;
	FString input_size;
	FListP<NNLink> input_links;
	FListP<NNLink> output_links;
	FListT<int> selected_layer_ids;

	for (int il = 0 ; il < GetNumLayer() ; il++) {
		const NNLayer &layer = GetLayer(il);
		if (layer.GetSelected()){
			selected_layer_ids.Add(layer.GetLayerID());
		}
	}

	for (int il = 0 ; il < GetNumLayer() && (start_layer_id < 0 || stop_layer_id < 0) ; il++) {
		const NNLayer &layer = GetLayer(il);
		if (layer.GetSelected()){
			//Check source layer
			FListT<int> source_link_ids = EnumLinkIDByDestLayerID(layer.GetLayerID(), -1);
			if (source_link_ids.GetSize() == 0){
				break;
			}else {
				for (int ili = 0 ; ili < source_link_ids.GetSize() ; ili++){
					const NNLink *link = GetLinkByID(source_link_ids.GetAt(ili));
					assert(link);
					if (selected_layer_ids.IndexOf(link->GetSourceLayerID()) < 0){
						//Source layer is not selected
						start_layer_id = layer.GetLayerID();
						input_links.Add(*link);

						for (int i = 0 ; i < 4 ; i++){
							start_layer_pos[i] = layer.GetPosition(i);
						}

						const NNLayer *source_layer = GetLayerByID(link->GetSourceLayerID());
						assert(source_layer);
						input_size = source_layer->GetDisplayPropertyValue(FString("Output"));
					}
				}
			}

			//Check dest layer
			FListT<int> dest_link_ids = EnumLinkIDBySourceLayerID(layer.GetLayerID(), -1);
			if (dest_link_ids.GetSize() == 0){
				stop_layer_id = layer.GetLayerID();
			}else {
				for (int ili = 0 ; ili < dest_link_ids.GetSize() ; ili++){
					const NNLink *link = GetLinkByID(dest_link_ids.GetAt(ili));
					assert(link);
					if (selected_layer_ids.IndexOf(link->GetDestLayerID()) < 0){
						//Dest layer is not selected
						stop_layer_id = layer.GetLayerID();
						output_links.Add(*link);
					}
				}
			}
		}
	}

	int input_component_index = 0;
	int unit_component_index = 0;
	for (int ic = 0 ; ic < nn_engine_info_list_.GetAt((int)sdeep_engine_type_)->GetComponents().GetSize() ; ic++) {
		const NNLayerComponent &component = nn_engine_info_list_.GetAt((int)sdeep_engine_type_)->GetComponents().GetAt(ic);
		if (component.GetType() == "Input"){
			input_component_index = ic;
		}else if (component.GetType() == "Unit"){
			unit_component_index = ic;
		}
	}

	const int original_network_index = GetActiveStructureIndex();


	//Unitize
	const FString unit_net = Cut();

	AddStructure();
	SetName(network_name);
	const FString unit_name = GetName();

	NNLayer &input_layer = AddLayer(input_component_index);
	GlobalEscapeLayerName(input_layer.GetLayerID());
	StartDrag();
	EndDrag();
	input_layer.SetPropertyValue(FString("Size"), input_size);

	Paste(unit_net);

	for (int il = 0 ; il < GetNumLayer() ; il++) {
		const NNLayer &layer = GetLayer(il);
		if (layer.GetSelected()){
			//Check source layer
			FListT<int> source_link_ids = EnumLinkIDByDestLayerID(layer.GetLayerID(), -1);
			if (source_link_ids.GetSize() == 0){
				AddLink(input_layer.GetLayerID(), 0, layer.GetLayerID(), 0);
			}
		}
	}

	ArrangeLayers();
	RefreshDisplayValue();
	RefreshStatistics();

	//Add the unit to the original network
	SetActiveStructure(original_network_index);
	NNLayer &unit_layer = AddLayer(unit_component_index);
	GlobalEscapeLayerName(unit_layer.GetLayerID());
	for (int i = 0 ; i < 4 ; i++){
		unit_layer.SetPosition(i, start_layer_pos[i]);
		unit_layer.SetCurrentPosition(i, start_layer_pos[i]);
	}
	StartDrag();
	EndDrag();
	unit_layer.SetPropertyValue(FString("Network"), unit_name);

	for (int ili = 0 ; ili < input_links.GetSize() ; ili++){
		const NNLink &link = input_links.GetAt(ili);
		AddLink(link.GetSourceLayerID(), link.GetSourcePinIndex(), unit_layer.GetLayerID(), 0);
	}
	for (int ili = 0 ; ili < output_links.GetSize() ; ili++){
		const NNLink &link = output_links.GetAt(ili);
		AddLink(unit_layer.GetLayerID(), 0, link.GetDestLayerID(), link.GetDestPinIndex());
	}

	RefreshDisplayValue();
	RefreshStatistics();
	return true;
}
//---------------------------------------------------------------------------

};
#pragma package(smart_init)
