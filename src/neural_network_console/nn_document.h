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

#ifndef nn_documentH
#define nn_documentH

#include "flist.h"
#include "fstring.h"
#include "nn_structure.h"
#include "nn_dataset.h"
#include "nn_config.h"
#include "csv_dataset.h"
#include "nn_document_converter_setting.h"
#include "nn_script.h"

namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
#define PRODUCT_NAME "Neural Network Console"

//---------------------------------------------------------------------------
#define NAN_VALUE 1.0e+38
#define NAN_VALUE_THRESHOLD 1.0e+37
//---------------------------------------------------------------------------
enum class SDeepEngineType : int {
Torch,
NNabla,
Transformers,

NumEngineType,
};
//---------------------------------------------------------------------------
class NNEngineInfo
{
public:
	NNEngineInfo() {};
private:
	NNEngineInfo(const NNEngineInfo &source) {assert(false);};
public:
	NNLayerComponents &Components() {return nn_layer_components_;};
	NNStructureSearchPolicies &StructureSearchPolicies() {return nn_structure_search_policies_;};
	NNScripts &Scripts() {return nn_scripts_;};
	SolverComponents &Solvers() {return solver_components_;};
	FStringList &PresetProjects() {return preset_projects_;};

	const NNLayerComponents &GetComponents() const {return nn_layer_components_;};
	const NNStructureSearchPolicies &GetStructureSearchPolicies() const {return nn_structure_search_policies_;};
	const NNScripts &GetScripts() const {return nn_scripts_;};
	const class SolverComponents &GetSolverComponents() const {return solver_components_;};
	const FStringList &GetPresetProjects() const {return preset_projects_;};
private:
	NNLayerComponents nn_layer_components_;//nn_layer_components_.GetAt(ComponentIndex)
	NNStructureSearchPolicies nn_structure_search_policies_;
	NNScripts nn_scripts_;
	class SolverComponents solver_components_;
	FStringList preset_projects_;
};
//---------------------------------------------------------------------------
class NNTrainingResult
{
public:
	NNTrainingResult() :
		schedule_(0),
		complete_(0),
		max_epoch_(0),
		last_epoch_(0),
		training_(NAN_VALUE),
		validation_(NAN_VALUE),
		best_epoch_(0),
		best_validation_(NAN_VALUE),
		evaluated_(0),

		structure_expectation_of_log_myu_(0.0),
		structure_expectation_of_log_sigma_(0.0),

		pareto_optimal_(true) {}
	~NNTrainingResult() {}
	int Open(FString path, FString dir_name);
private:
	NNTrainingResult(const NNTrainingResult &source);
	NNTrainingResult &operator=(const NNTrainingResult &source);
public:
	const FString &GetPath() const {return path_;}
	const FString &GetDir() const {return dir_;}
	int GetSchedule() const {return schedule_;}
	int GetComplete() const {return complete_;}
	double GetTraining() const {return training_;}
	double GetValidation() const {return validation_;}
	int GetLastEpoch() const {return last_epoch_;}
	int GetMaxEpoch() const {return max_epoch_;}
	int GetBestEpoch() const {return best_epoch_;}
	double GetBestValidation() const {return best_validation_ < NAN_VALUE_THRESHOLD ? best_validation_ : validation_;}
	double GetCostMultiplyAdd() const {return GetStatistics(FString("CostMultiplyAdd"));}
	double GetStatistics(const FString &label) const;
	int GetEvaluated() const {return evaluated_;}

	void GetStructureSearchExpectiation(double &log_myu, double &log_sigma) const{
		log_myu = structure_expectation_of_log_myu_;
		log_sigma = structure_expectation_of_log_sigma_;
	}

	FString GetFullPath() const {return path_ + dir_ + FString("/");}

	void CheckParetoOptimal(const NNTrainingResult &result, const FString &pareto_optimal_statistics_label, int old);
	int GetParetoOptimal() const {return pareto_optimal_;}

	const int GetNumStatistics() const {return statistics_label_.GetSize();}
	const int GetStatisticsIndex(const FString &label) const {return statistics_label_.IndexOf(label);}
	const FString &GetStatisticsLabel(int index) const {return statistics_label_.GetAt(index);}
	const double GetStatisticsValue(int index) const {return statistics_value_.GetAt(index);}

	const FString &GetComment() const {return comment_;};
private:
	FString path_;
	FString dir_;
	int schedule_;
	int complete_;
	int last_epoch_;
	int max_epoch_;
	double training_;
	double validation_;
	int best_epoch_;
	double best_validation_;
	int evaluated_;

	double structure_expectation_of_log_myu_;
	double structure_expectation_of_log_sigma_;

	FStringList statistics_label_;
	FListT<double> statistics_value_;

	FString comment_;
private:
	int pareto_optimal_;
};
//---------------------------------------------------------------------------
class NNDocument : public NNStructures, public NNDatasets, public NNConfig
{
public:
	NNDocument();
	virtual ~NNDocument();
private:
	NNDocument(const NNDocument &source);
	NNDocument &operator=(const NNDocument &source);
public:
	//Setup
	void RegisterNNEngineInfo(SDeepEngineType sdeep_engine_type, const NNEngineInfo *nn_engine_info) {
		while (nn_engine_info_list_.GetSize() <= (int)sdeep_engine_type){
			nn_engine_info_list_.Add(NULL);
		}
		nn_engine_info_list_.SetAt((int)sdeep_engine_type, nn_engine_info);
	}
	void RegisterNNDocumentConverterSetting(const NNDocumentConverterSetting *nn_document_converter_setting) {
		nn_document_converter_setting_list_.Add(nn_document_converter_setting);
	}

public:
	//Load / Save
	void Clear(SDeepEngineType sdeep_engine_type = SDeepEngineType(0));
	int LoadFromFile(FString file_name, FString sdeep_console_path = FString(""), int format_convert = false, int load_dataset_csv_and_result = true, int load_tmp_config = false);
	int SaveToFile(FString file_name, int save_all_properties_and_statistics = false, int silent_save = false, int save_tmp_config = false);

	int GetChanged() {return saved_update_count_ < GetUpdateCount() || saved_config_update_count_ < GetConfigUpdateCount();}
	void SetFileName(FString file_name) {file_name_ = file_name;}
	FString GetFileName() const {return file_name_;}
private:
	int saved_update_count_;
	int saved_config_update_count_;
	FString file_name_;

public:
	//Setter / Getter
	SDeepEngineType GetSDeepEngineType() const {return sdeep_engine_type_;}
	FString GetSDeepEngineName() const;
	const NNEngineInfo *GetNNEngineInfo() const {return nn_engine_info_list_.GetAt((int)sdeep_engine_type_);}

private:
	//Setup
	SDeepEngineType sdeep_engine_type_;
	FListT<const NNEngineInfo *> nn_engine_info_list_;//nn_engine_info_.GetAt(EngineIndex)->
	FListT<const NNDocumentConverterSetting *> nn_document_converter_setting_list_;//GetFrom() == "FromEngineName" && GetTo() == "ToEngineName"

public:
	//Description
	void SetDescription(const FString &description) {description_ = description;}
	const FString &GetDescription() const {return description_;}
private:
	FString description_;

public:
	//Operation
	int Mutate();
public:
	//Error
	void CheckError(int check_dataset_error = true);
	const FListP<NNError> &GetErrors() const {return errors_;}
	NNError &AddError() {return errors_.Add();}
private:
	FListP<NNError> errors_;

public:
	//Training Results
	void RefreshResults(FString path_separator, int select_last = false);
	void AddResult(FString result_name, FString path_separator) {current_result_index_ = results_.GetSize(); results_.Add().Open(GetResultPath(path_separator), result_name);}
	const FListP<NNTrainingResult> &GetResults() const {return results_;}
	const NNTrainingResult &GetResult(int result_index) const {return results_.GetAt(result_index);}
	FString GetCurrentResultName() const {return current_result_index_ >= 0 && current_result_index_ < results_.GetSize() ? results_.GetAt(current_result_index_).GetDir() : FString("");}
	FString GetResultName(int result_index) const {return result_index >= 0 && result_index < results_.GetSize() ? results_.GetAt(result_index).GetDir() : GetCurrentResultName();}
	void SetCurrentResultIndex(int current_result_index) {current_result_index_ = current_result_index;}
	int GetCurrentResultIndex() const {return current_result_index_;}
	FString GetResultPath(FString path_separator) const {return file_name_.SubString(1, file_name_.Length() - 7) + FString("files") + path_separator;}
	FString GetResultFullDir(FString path_separator) const {return GetResultPath(path_separator) + GetCurrentResultName();}
	void SetParetoOptimalStatisticsLabel(FString label) {pareto_optimal_statistics_label_ = label;}
private:
	int current_result_index_;
	FListP<NNTrainingResult> results_;
	FString pareto_optimal_statistics_label_;

public:
	//Result CSV (Evaluation Result and Confusion Matrix)
	int LoadResultCSVFromFile(int result_index, FString file_name) {return result_csv_[result_index].LoadFromFile(file_name.PChar(), true, true);}
	void SetResultCSV(int result_index, const CSVDataset &result_csv) {result_csv_[result_index] = result_csv;}
	void ClearResultCSV(int result_index) {result_csv_[result_index].Clear();}
	const CSVDataset &GetResultCSV(int result_index) const {return result_csv_[result_index];}
private:
	CSVDataset result_csv_[3];//[0]=Validation(Evaluation), [1]=Confusion Matrix, [2]=ErrorAnalysisResult

public:
	//Edit
	FString Cut();
	FString Copy() const;
	void Paste(const FString &clipboard_str, int escape_position = true);
	int Unitize(const FString &network_name);

public:
	//Undo, redo
	void KeepForUndo(FString undo_name);
	void PrepareUndoInAdvance();
	void KeepPreparedForUndo(FString undo_name);
	int GetNumUndo() {return undo_list_.GetSize();}
	int GetNumRedo() {return redo_list_.GetSize();}
	const FString GetUndoName(int undo_index) const {return undo_index < undo_list_.GetSize() ? undo_list_.GetAt(undo_index).GetEditString() : FString("");}
	const FString GetRedoName(int redo_index) const {return redo_index < redo_list_.GetSize() ? redo_list_.GetAt(redo_index).GetEditString() : FString("");}
	int Undo(int undo_index = 0);
	int Redo(int redo_index = 0);
private:
	NNStructures prepared_for_undo_;
	FListP<NNStructures> undo_list_;
	FListP<NNStructures> redo_list_;

public:
	//Temporary
	void Lock() {locked_++;}
	void Unlock() {locked_--;}
	int GetLocked() const {return locked_;}
private:
	int locked_;

public:
	//For structure search
	void SetStructureSearchExpectiation(double log_myu, double log_sigma){
		structure_expectation_of_log_myu_ = log_myu;
		structure_expectation_of_log_sigma_ = log_sigma;
	}
	void GetStructureSearchExpectiation(double &log_myu, double &log_sigma) const{
		log_myu = structure_expectation_of_log_myu_;
		log_sigma = structure_expectation_of_log_sigma_;
	}
private:
	double structure_expectation_of_log_myu_;
	double structure_expectation_of_log_sigma_;

public:
	//Helper
	static void CompleteNetworks(NNStructures &structures);//need to call before training
	FString NetworkToString(const NNStructure &structure);
	void StringToNetwork(FString s, NNStructure &structure);
	static int NetworksToIni(const NNStructures &structures, FFastIni &ini, int save_display_value = false);
	static int IniToNetworks(
		FFastIni &ini, NNStructures &structures, int select_loaded = false,
		const NNDocumentConverterSetting *converter_setting = NULL);
	int IsStructureUsed(const FString &network_name) const;
	FListT<int> GetUsedStructureIndex() const;
	void FillParameterName(const FStringList &parameter_names, const FStringList &parameter_file_names);//for SaaS
	double GetRuntimeNetworkStatistics(const FString &label);
private:
	static int NetworkToIni(FString name, const NNStructure &structure, FFastIni &ini, int save_display_value = false);
	static int IniToNetwork(
		FString name, FFastIni &ini, NNStructure &structure, int select_loaded = false,
		const NNDocumentConverterSetting *converter_setting = NULL);
};
//---------------------------------------------------------------------------
};
#endif
