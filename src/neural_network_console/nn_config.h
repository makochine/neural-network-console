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

#ifndef nn_configH
#define nn_configH
//---------------------------------------------------------------------------
#include "flist.h"
#include "fstring.h"
#include "ffastini.h"
#include "solver_component.h"

#ifdef _WIN32
#else
#include <stdint.h>
typedef int64_t __int64;
#endif

namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
enum class TypeConfig : int {
Float,
Half,
};
//---------------------------------------------------------------------------
enum class StructureSearchMethod : int {
Random,
NetworkFeatureAndGaussianProcess,
NetworkFeatureAndGaussianProcessOptimisticProb,
NetworkFeatureAndGaussianProcessOptimisticArea
};
//---------------------------------------------------------------------------
enum class StructureSearchObjective : int {
Error,
ErrorAndCalculation
};
//---------------------------------------------------------------------------
enum class RNNTrainingMode : int {
Scan,
Loop,
};
//---------------------------------------------------------------------------
enum class SDeepCommandType : int {
Normal,
CreateTrainingResult,
Profiling,
CompareWithCPU,
Script
};
//---------------------------------------------------------------------------
class NNOptimizer
{
public:
	NNOptimizer() :
		solver_components_(NULL),
		range_from_(-1),
		range_to_(-1),
		range_unit_(0),
		update_interval_(1),
		weight_decay_(0.0),
		learning_rate_multiplier_(1.0),
		learning_rate_update_interval_(1),
		learning_rate_update_interval_unit_(0),//0 = epoch, 1 = iteration
		learning_rate_power_(1.0),
		learning_rate_warmup_length_(0),
		learning_rate_warmup_length_unit_(0)//0 = epoch, 1 = iteration
		{};// initialized after SetSolverComponents
	virtual ~NNOptimizer() {};
	NNOptimizer &operator=(const NNOptimizer &source);
	void Clear(){
		name_ = FString("Optimizer");
		network_name_ = FString("Main");
		dataset_names_.Clear();
		dataset_names_.Add(FString("Training"));

		update_interval_ = 1;

		if (solver_components_) {
			const int default_solver_index = solver_components_->GetDefaultSolverIndex();
			SetSolver(solver_components_->GetAt(default_solver_index).GetName(), true);
		}

		weight_decay_ = 0.0;
		learning_rate_multiplier_ = 1.0;
		learning_rate_update_interval_ = 1;
		learning_rate_update_interval_unit_ = 0;

		learning_rate_scheduler_ = FString("Exponential");
		learning_rate_power_ = 1.0;
		learning_rate_steps_.Clear();

		learning_rate_warmup_scheduler_ = FString("None");
		learning_rate_warmup_length_ = 0;
		learning_rate_warmup_length_unit_ = 0;
	}
private:
	NNOptimizer(const NNOptimizer &source);
public:
	void LoadOptimizerFromIni(FFastIni *ini, FString section = FString("Config"));
	void SaveOptimizerToIni(FFastIni *ini, FString section = FString("Config"));
	void GetOptimizerConfigText(FStringList &result) const;
public:
	//Setup
	void SetSolverComponents(const SolverComponents *solver_components) {
		solver_components_ = solver_components;
		Clear();
	};
	const SolverComponents *GetSolverComponents() const {return solver_components_;}
public:
	void SetRangeFrom(int range_from) {range_from_ = range_from;}
	int GetRangeFrom() const {return range_from_;}
	void SetRangeTo(int range_to) {range_to_ = range_to;}
	int GetRangeTo() const {return range_to_;}
	void SetRangeUnit(int range_unit) {range_unit_ = range_unit;}
	int GetRangeUnit() const {return range_unit_;}

	void SetOptimizerName(FString name) {name_ = name;}
	const FString &GetOptimizerName() const {return name_;}
	void SetNetworkName(FString network_name) {network_name_ = network_name;}
	const FString &GetNetworkName() const {return network_name_;}
	void SetDatasetNames(FStringList dataset_names) {dataset_names_ = dataset_names;}
	const FStringList &GetDatasetNames() const {return dataset_names_;}
	FStringList &GetDatasetNames_() {return dataset_names_;}

	void SetUpdateInterval(int update_interval) {update_interval_ = update_interval;}
	int GetUpdateInterval() const {return update_interval_;}
	int SetSolver(FString solver_name, int load_default = false);
	const FString &GetSolverName() const {return solver_name_;}
	const FString &GetSolverArgumentName() const {return solver_argument_name_;}
	int GetSolverParameterNum() const {return solver_param_names_.GetSize();}
	FString &GetSolverParameterArgumentName(int index) const {return solver_param_arg_names_.GetAt(index);}
	FString &GetSolverParameterName(int index) const {return solver_param_names_.GetAt(index);}
	double GetSolverParameter(int index) const {return solver_parameters_.GetAt(index);}
	void SetSolverParameter(int index, double value) {solver_parameters_.SetAt(index, value);}

	void SetWeightDecay(double weight_decay) {weight_decay_ = weight_decay;}
	double GetWeightDecay() const {return weight_decay_;}

	void SetLearningRateScheduler(const FString &learning_rate_scheduler) {learning_rate_scheduler_ = learning_rate_scheduler;}
	const FString &GetLearningRateScheduler()const {return learning_rate_scheduler_;}

	void SetLearningRateMultiplier(double learning_rate_multiplier) {learning_rate_multiplier_ = learning_rate_multiplier;}
	double GetLearningRateMultiplier() const {return learning_rate_multiplier_;}
	void SetLearningRateUpdateInterval(int learning_rate_update_interval) {learning_rate_update_interval_ = learning_rate_update_interval;}
	int GetLearningRateUpdateInterval() const {return learning_rate_update_interval_;}
	void SetLearningRateUpdateIntervalUnit(int learning_rate_update_interval_unit) {learning_rate_update_interval_unit_ = learning_rate_update_interval_unit;}
	int GetLearningRateUpdateIntervalUnit() const {return learning_rate_update_interval_unit_;}

	void SetLearningRatePower(double learning_rate_power) {learning_rate_power_ = learning_rate_power;}
	double GetLearningRatePower() const {return learning_rate_power_;}

	const FListT<int> &GetLearningRateSteps() const {return learning_rate_steps_;}
	void SetLearningRateSteps(const FListT<int> &learning_rate_steps) {learning_rate_steps_ = learning_rate_steps;}
	FString GetLearningRateStepsStr() const;

	void SetLearningRateWarmupScheduler(const FString &learning_rate_warmup_scheduler) {learning_rate_warmup_scheduler_ = learning_rate_warmup_scheduler;}
	const FString &GetLearningRateWarmupScheduler()const {return learning_rate_warmup_scheduler_;}
	void SetLearningRateWarmupLength(int learning_rate_warmup_length) {learning_rate_warmup_length_ = learning_rate_warmup_length;}
	int GetLearningRateWarmupLength() const {return learning_rate_warmup_length_;}
	void SetLearningRateWarmupLengthUnit(int learning_rate_warmup_length_unit) {learning_rate_warmup_length_unit_ = learning_rate_warmup_length_unit;}
	int GetLearningRateWarmupLengthUnit() const {return learning_rate_warmup_length_unit_;}
private:
	int range_from_;
	int range_to_;
	int range_unit_;//0 = epoch, 1 = iteration

	FString name_;
	FString network_name_;
	FStringList dataset_names_;

	int update_interval_;

	const SolverComponents *solver_components_;
	FString solver_name_;
	FString solver_argument_name_;
	FStringList solver_param_names_;
	FStringList solver_param_arg_names_;
	FListT<double> solver_parameters_;

	double weight_decay_;
	double learning_rate_multiplier_;
	int learning_rate_update_interval_;
	int learning_rate_update_interval_unit_;//0 = epoch, 1 = iteration

	FString learning_rate_scheduler_;
	double learning_rate_power_;
	FListT<int> learning_rate_steps_;

	FString learning_rate_warmup_scheduler_;
	int learning_rate_warmup_length_;
	int learning_rate_warmup_length_unit_;//0 = epoch, 1 = iteration
};
//---------------------------------------------------------------------------
class NNOptimizers : protected FListP<NNOptimizer>
{
public:
	NNOptimizers() : solver_components_(NULL) {ClearOptimizers();};
	void ClearOptimizers(){
		Clear();
		NNOptimizer &new_optimizer = Add();
		new_optimizer.SetSolverComponents(solver_components_);
	}
public:
	//Setup
	void SetSolverComponents(const SolverComponents *solver_components) {
		solver_components_ = solver_components;
		ClearOptimizers();
	};
	const SolverComponents *GetSolverComponents() const {return solver_components_;}
private:
	const SolverComponents *solver_components_;
public:
	int GetNumOptimizer() const {return GetSize();};
	NNOptimizer &GetOptimizer(int index) {return GetAt(index);}
	const NNOptimizer &GetOptimizer(int index) const {return GetAt(index);}
	int GetOptimizerIndexByName(FString name) const;
	NNOptimizer &AddOptimizer();
	void DeleteOptimizer(int index) {Delete(index);}
	void SwapOptimizer(int index, int index2) {Swap(index, index2);}

	//const NNOptimizer *GetOptimizerByDataset(const FString &dataset_name) const;
public:
	void LoadOptimizersFromIni(FFastIni *ini);
	void SaveOptimizersToIni(FFastIni *ini);
	void GetOptimizersConfigText(FStringList &result) const;
};
//---------------------------------------------------------------------------
class NNMonitor
{
public:
	NNMonitor() {Clear();};
	virtual ~NNMonitor() {};
	NNMonitor &operator=(const NNMonitor &source);
	void Clear(){
		name_ = FString("train_error");
		network_name_ = FString("MainValidation");
		dataset_names_.Clear();
		dataset_names_.Add(FString("Training"));
	}
private:
	NNMonitor(const NNMonitor &source);
public:
	void LoadMonitorFromIni(FFastIni *ini, FString section = FString("Config"));
	void SaveMonitorToIni(FFastIni *ini, FString section = FString("Config"));
	void GetMonitorConfigText(FStringList &result) const;

public:
	void SetMonitorName(FString name) {name_ = name;}
	const FString &GetMonitorName() const {return name_;}
	void SetNetworkName(FString network_name) {network_name_ = network_name;}
	const FString &GetNetworkName() const {return network_name_;}
	void SetDatasetNames(FStringList dataset_names) {dataset_names_ = dataset_names;}
	const FStringList &GetDatasetNames() const {return dataset_names_;}

private:
	FString name_;
	FString network_name_;
	FStringList dataset_names_;
};
//---------------------------------------------------------------------------
class NNMonitors : protected FListP<NNMonitor>
{
public:
	NNMonitors() {ClearMonitors();}
public:
	void ClearMonitors(){
		FListP<NNMonitor>::Clear();
		Add();
		NNMonitor &valid_error = Add();
		valid_error.SetMonitorName(FString("valid_error"));
		FStringList dataset_names;
		dataset_names.Add(FString("Validation"));
		valid_error.SetDatasetNames(dataset_names);
	}
public:
	int GetNumMonitor() const {return GetSize();};
	NNMonitor &GetMonitor(int index) {return GetAt(index);}
	const NNMonitor &GetMonitor(int index) const {return GetAt(index);}
	int GetMonitorIndexByName(FString name) const;
	NNMonitor &AddMonitor_();
	void DeleteMonitor_(int index) {Delete(index);}
	void SwapMonitor(int index, int index2) {Swap(index, index2);}
public:
	void LoadMonitorsFromIni(FFastIni *ini);
	void SaveMonitorsToIni(FFastIni *ini);
	void GetMonitorsConfigText(FStringList &result) const;
};
//---------------------------------------------------------------------------
class NNExecutor
{
public:
	NNExecutor() {Clear();};
	virtual ~NNExecutor() {};
	NNExecutor &operator=(const NNExecutor &source);
	void Clear(){
		name_ = FString("Executor");
		network_name_ = FString("MainRuntime");
		dataset_names_.Clear();
		dataset_names_.Add(FString("Validation"));

		num_evaluations_ = 1;
		repeat_evaluation_type_ = 0;
		need_back_propagation_ = 0;
	}

private:
	NNExecutor(const NNExecutor &source);
public:
	void LoadExecutorFromIni(FFastIni *ini, FString section = FString("Config"));
	void SaveExecutorToIni(FFastIni *ini, FString section = FString("Config"));
	void GetExecutorConfigText(FStringList &result) const;
public:
	void SetExecutorName(FString name) {name_ = name;}
	const FString &GetExecutorName() const {return name_;}
	void SetNetworkName(FString network_name) {network_name_ = network_name;}
	const FString &GetNetworkName() const {return network_name_;}
	void SetDatasetNames(FStringList dataset_names) {dataset_names_ = dataset_names;}
	const FStringList &GetDatasetNames() const {return dataset_names_;}
	FStringList &GetDatasetNames_() {return dataset_names_;}

	void SetNumEvaluations(int num_evaluations) {num_evaluations_ = num_evaluations;}
	int GetNumEvaluations() const {return num_evaluations_;}
	void SetRepeatEvaluationType(int repeat_evaluation_type) {repeat_evaluation_type_ = repeat_evaluation_type;}
	int GetRepeatEvaluationType() const {return repeat_evaluation_type_;}
	void SetNeedBackPropagation(int need_back_propagation) {need_back_propagation_ = need_back_propagation;}
	int GetNeedBackPropagation() const {return need_back_propagation_;}

private:
	FString name_;
	FString network_name_;
	FStringList dataset_names_;

	int num_evaluations_;//Number of reputations at evaluation time
	int repeat_evaluation_type_;//0=mean, 1=last, 2=stdev
	int need_back_propagation_;
};
//---------------------------------------------------------------------------
class NNExecutors : protected FListP<NNExecutor>
{
public:
	NNExecutors() {ClearExecutors();}
public:
	void ClearExecutors(){
		FListP<NNExecutor>::Clear();
		Add();
	}
public:
	int GetNumExecutor() const {return GetSize();};
	NNExecutor &GetExecutor(int index) {return GetAt(index);}
	const NNExecutor &GetExecutor(int index) const {return GetAt(index);}
	int GetExecutorIndexByName(FString name) const;
	NNExecutor &AddExecutor();
	void DeleteExecutor(int index) {Delete(index);}
	void SwapExecutor(int index, int index2) {Swap(index, index2);}
public:
	void LoadExecutorsFromIni(FFastIni *ini);
	void SaveExecutorsToIni(FFastIni *ini);
	void GetExecutorsConfigText(FStringList &result) const;
};
//---------------------------------------------------------------------------
class NNConfig : public NNOptimizers, public NNMonitors, public NNExecutors
{
public:
	NNConfig() : config_update_count_(-1) {Clear();};
	virtual ~NNConfig() {};
	NNConfig &operator=(const NNConfig &source);
	void Clear(){
		//General
		is_sample_ = false;

		//
		max_epoch_ = 100;
		save_best_ = true;
		batch_size_ = 64;
		type_config_ = TypeConfig::Float;
		monitor_interval_ = 10;
        auto_evaluation_ = false;

		NNOptimizers::ClearOptimizers();
		NNMonitors::ClearMonitors();
		NNExecutors::ClearExecutors();

		structure_search_enabled_ = false;
		structure_search_method_ = StructureSearchMethod::Random;
		structure_search_objective_ = StructureSearchObjective::ErrorAndCalculation;
		structure_search_constraint_attributes_.Clear();
		structure_search_constraint_attributes_.Add(FString("Validation"));
		structure_search_constraint_attributes_.Add(FString("CostMultiplyAdd"));
		structure_search_constraint_max_.Clear();
		structure_search_constraint_max_.Add(-1.0);
		structure_search_constraint_max_.Add(-1.0);
		structure_search_constraint_min_.Clear();
		structure_search_constraint_min_.Add(-1.0);
		structure_search_constraint_min_.Add(-1.0);

		early_stopping_ = false;
		time_limit_ = 0.0;

		out_of_core_training_enabled_ = false;
		out_of_core_training_gpu_memory_size_ = 0;
		out_of_core_training_window_size_ = 0;

		//Temporary config
		command_type_ = SDeepCommandType::Normal;
		script_name_ = FString("");
		script_type_ = FString("");
		script_file_name_ = FString("");
		script_args_ = FString("");

		//Obsolete
		shuffle_ = true;
		dataset_cache_enabled_ = false;
		image_normalization_ = true;
		// set_lrate_of_convolution_as_zero_ = false;
		UpdateConfig();
	}
private:
	NNConfig(const NNConfig &source);
public:
	void LoadConfigFromIni(FFastIni *ini, int load_tmp_config = false);
	void SaveConfigToIni(FFastIni *ini, int save_tmp_config = false);

	void SetIsSample(int is_sample) {is_sample_ = is_sample;}
	int GetIsSample() const {return is_sample_;}

	void SetMaxEpoch(int max_epoch) {max_epoch_ = max_epoch;}
	int GetMaxEpoch() const {return max_epoch_;}
	void SetSaveBest(double save_best) {save_best_ = save_best;}
	int GetSaveBest() const {return save_best_;}
	void SetBatchSize(int batch_size) {batch_size_ = batch_size;}
	int GetBatchSize() const {return batch_size_;}
	void SetTypeConfig(TypeConfig type_config) {type_config_ = type_config;}
	TypeConfig GetTypeConfig() const {return type_config_;}
	void SetMonitorInterval(int monitor_interval) {monitor_interval_ = monitor_interval;}
	int GetMonitorInterval() const {return monitor_interval_;}
	void SetAutoEvaluation(int auto_evaluation) {auto_evaluation_ = auto_evaluation;}
	int GetAutoEvaluation() const {return auto_evaluation_;}

	void SetStructureSearchEnabled(int structure_search_enabled) {structure_search_enabled_ = structure_search_enabled;}
	int GetStructureSearchEnabled() const {return structure_search_enabled_;}
	void SetStructureSearchMethod(StructureSearchMethod structure_search_method) {structure_search_method_ = structure_search_method;}
	StructureSearchMethod GetStructureSearchMethod() const {return structure_search_method_;}
	void SetStructureSearchObjective(StructureSearchObjective structure_search_objective) {structure_search_objective_ = structure_search_objective;}
	StructureSearchObjective GetStructureSearchObjective() const {return structure_search_objective_;}
	int SetStructureSearchConstraint(FString attribute, int max, double value);
	double GetStructureSearchConstraint(FString attribute, int max) const;

	void SetEarlyStopping(int early_stopping) {early_stopping_ = early_stopping;}
	int GetEarlyStopping() const {return early_stopping_;}
	void SetTimeLimit(double time_limit) {time_limit_ = time_limit;}
	double GetTimeLimit() const {return time_limit_;}

	void SetOutOfCoreTrainingEnabled(int out_of_core_training_enabled) {out_of_core_training_enabled_ = out_of_core_training_enabled;}
	int GetOutOfCoreTrainingEnabled() const {return out_of_core_training_enabled_;}
	void SetOutOfCoreTrainingGPUMemorySize(__int64 out_of_core_training_gpu_memory_size) {out_of_core_training_gpu_memory_size_ = out_of_core_training_gpu_memory_size;}
	__int64 GetOutOfCoreTrainingGPUMemorySize() const {return out_of_core_training_gpu_memory_size_;}
	void SetOutOfCoreTrainingWindowSize(__int64 out_of_core_training_window_size) {out_of_core_training_window_size_ = out_of_core_training_window_size;}
	__int64 GetOutOfCoreTrainingWindowSize() const {return out_of_core_training_window_size_;}
	/*
	//Obsolete

	void SetSetLearningRateOfConvolutionAsZero(int set_lrate_of_convolution_as_zero) {set_lrate_of_convolution_as_zero_ = set_lrate_of_convolution_as_zero;}
	int GetSetLearningRateOfConvolutionAsZero() const {return set_lrate_of_convolution_as_zero_;}
	*/

	//Temporary Config
	void SetCommandType(SDeepCommandType command_type) {command_type_ = command_type;}
	SDeepCommandType GetCommandType() const {return command_type_;}
	void SetScriptName(FString script_name) {script_name_ = script_name;}
	FString GetScriptName() const {return script_name_;}
	void SetScriptType(FString script_type) {script_type_ = script_type;}
	FString GetScriptType() const {return script_type_;}
	void SetScriptFileName(FString script_file_name) {script_file_name_ = script_file_name;}
	FString GetScriptFileName() const {return script_file_name_;}
	void SetScriptArgs(FString script_args) {script_args_ = script_args;}
	FString GetScriptArgs() const {return script_args_;}

	FString GetConfigText() const;
private:
	//General
	int is_sample_;

	//
	int max_epoch_;
	int save_best_;
	int batch_size_;
	TypeConfig type_config_;
	int monitor_interval_;
	int auto_evaluation_;

	int structure_search_enabled_;
	StructureSearchMethod structure_search_method_;
	StructureSearchObjective structure_search_objective_;

	FStringList structure_search_constraint_attributes_;
	FListT<double> structure_search_constraint_min_;
	FListT<double> structure_search_constraint_max_;

	int early_stopping_;
	double time_limit_;
	//int set_lrate_of_convolution_as_zero_;

	int out_of_core_training_enabled_;
	__int64 out_of_core_training_gpu_memory_size_;
	__int64 out_of_core_training_window_size_;

	//Temporary config
	SDeepCommandType command_type_;
	FString script_name_;
	FString script_type_;
	FString script_file_name_;
	FString script_args_;

	int config_update_count_;

public:
	//Obsolete (keep for compatibility)

	//V2.20 and previous version
	//void SetDatasetURI(int validation, FString uri) {dataset_uri_[validation] = uri;}
	const FString &GetDatasetURI_Obsolete(int validation) const {return dataset_uri_[validation];}
	void SetShuffle_Obsolete(double shuffle) {shuffle_ = shuffle;}
	int GetShuffle_Obsolete() const {return shuffle_;}
	void SetDatasetCacheEnabled_Obsolete(int dataset_cache_enabled) {dataset_cache_enabled_ = dataset_cache_enabled;}
	int GetDatasetCacheEnabled_Obsolete() const {return dataset_cache_enabled_;}
	void SetImageNormalization_Obsolete(double image_normalization) {image_normalization_ = image_normalization;}
	int GetImageNormalization_Obsolete() const {return image_normalization_;}

	void UpdateConfig() {config_update_count_++;}
	int GetConfigUpdateCount() {return config_update_count_;}
private:
	FString dataset_uri_[2];
	int shuffle_;
	int dataset_cache_enabled_;
	int image_normalization_;
};
//---------------------------------------------------------------------------
};
#endif
