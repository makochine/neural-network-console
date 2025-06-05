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

#include "nn_config.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
NNOptimizer &NNOptimizer::operator=(const NNOptimizer &source)
{
	range_from_ = source.range_from_;
	range_to_ = source.range_to_;
	range_unit_ = source.range_unit_;

	name_ = source.name_;
	network_name_ = source.network_name_;
	dataset_names_ = source.dataset_names_;

	update_interval_ = source.update_interval_;

	solver_components_ = source.solver_components_;
	solver_name_ = source.solver_name_;
	solver_argument_name_ = source.solver_argument_name_;
	solver_param_names_ = source.solver_param_names_;
	solver_param_arg_names_ = source.solver_param_arg_names_;
	solver_parameters_ = source.solver_parameters_;

	weight_decay_ = source.weight_decay_;
	learning_rate_multiplier_ = source.learning_rate_multiplier_;
	learning_rate_update_interval_ = source.learning_rate_update_interval_;
	learning_rate_update_interval_unit_ = source.learning_rate_update_interval_unit_;

	learning_rate_scheduler_ = source.learning_rate_scheduler_;
	learning_rate_power_ = source.learning_rate_power_;
	learning_rate_steps_ = source.learning_rate_steps_;

	learning_rate_warmup_scheduler_ = source.learning_rate_warmup_scheduler_;
	learning_rate_warmup_length_ = source.learning_rate_warmup_length_;
	learning_rate_warmup_length_unit_ = source.learning_rate_warmup_length_unit_;

	return *this;
}
//---------------------------------------------------------------------------
int NNOptimizer::SetSolver(FString solver_name, int load_default)
{
	if (solver_name_ == solver_name && !load_default) {
		return false;
	}

	const int solver_index = solver_components_->GetSolverComponentIndexByName(solver_name);
	if (solver_index < 0) {
		return false;
	}
	solver_name_ = solver_name;
	solver_argument_name_ = solver_components_->GetAt(solver_index).GetArgumentName();
	solver_param_names_ = solver_components_->GetAt(solver_index).GetParamNames();
	solver_param_arg_names_ = solver_components_->GetAt(solver_index).GetParamArgNames();
	solver_parameters_ = solver_components_->GetAt(solver_index).GetDefaultValues();
	return true;
}
//---------------------------------------------------------------------------
void NNOptimizer::LoadOptimizerFromIni(FFastIni *ini, FString section)
{
	SetRangeFrom(ini->ReadInteger(section.PChar(), "RangeFrom", -1));
	SetRangeTo(ini->ReadInteger(section.PChar(), "RangeTo", -1));
	SetRangeUnit(ini->ReadInteger(section.PChar(), "RangeUnit", 0));

	name_ = ini->ReadString(section.PChar(), "Optimizer_Name", name_);
	network_name_ = ini->ReadString(section.PChar(), "Optimizer_NetworkName", network_name_);
	dataset_names_.Clear();
	dataset_names_.SplitStr(ini->ReadString(section.PChar(), "Optimizer_DatasetName", dataset_names_.GetText(',')), FString(","), FString(""));

	SetUpdateInterval(ini->ReadInteger(section.PChar(), "UpdateInterval", GetUpdateInterval()));

	FString learning_rate_control_str = ini->ReadString(section.PChar(), "LearningRateControl", FString(""));
	if (learning_rate_control_str != "") {
		//Old format (version < 2.00a)
		if (learning_rate_control_str == "Default"){
			if (SetSolver(FString("Sgd"), true)){
				SetSolverParameter(0, ini->ReadDouble(section.PChar(), "BaseLearningRate", GetSolverParameter(0)));
			}
		}else if (learning_rate_control_str == "Momentum"){
			if (SetSolver(FString("Momentum"), true)){
				SetSolverParameter(0, ini->ReadDouble(section.PChar(), "BaseLearningRate", GetSolverParameter(0)));
				SetSolverParameter(1, ini->ReadDouble(section.PChar(), "LearningRateControlDecay", GetSolverParameter(1)));
			}
		}else if (learning_rate_control_str == "AdaGrad"){
			if (SetSolver(FString("Adagrad"), true)){
				SetSolverParameter(0, ini->ReadDouble(section.PChar(), "BaseLearningRate", GetSolverParameter(0)));
			}
		}else if (learning_rate_control_str == "AdaDelta"){
			if (SetSolver(FString("Adadelta"), true)){
				SetSolverParameter(0, ini->ReadDouble(section.PChar(), "BaseLearningRate", GetSolverParameter(0)));
				SetSolverParameter(1, ini->ReadDouble(section.PChar(), "LearningRateControlDecay", GetSolverParameter(1)));
			}
		}else if (learning_rate_control_str == "Adam"){
			if (SetSolver(FString("Adam"), true)){
				SetSolverParameter(0, ini->ReadDouble(section.PChar(), "LearningRate_AdamAlpha", GetSolverParameter(0)));
				SetSolverParameter(1, ini->ReadDouble(section.PChar(), "LearningRate_AdamBeta1", GetSolverParameter(1)));
				SetSolverParameter(2, ini->ReadDouble(section.PChar(), "LearningRate_AdamBeta2", GetSolverParameter(2)));
			}
		}
	}

	FString solver_name = ini->ReadString(section.PChar(), "SolverName", FString(""));
	if (solver_name != ""){
		SetSolver(solver_name);
		int solver_param_num = ini->ReadInteger(section.PChar(), "SolverParameterNum", 0);
		for (int i = 0; i < solver_param_names_.GetSize() && solver_param_num ; i++) {
			FString param_name = ini->ReadString(FString(section.PChar()), FString("SolverParameterName_") + FString(i), FString(""));
			if (solver_param_names_.GetAt(i) == param_name) {
				SetSolverParameter(i, ini->ReadDouble(FString(section.PChar()), FString("SolverParameter_") + FString(i), solver_parameters_.GetAt(i)));
			}
		}
	}

	SetWeightDecay(ini->ReadDouble(section.PChar(), "WeightDecay", GetWeightDecay()));

	SetLearningRateScheduler(ini->ReadString(section.PChar(), "LearningRateScheduler", GetLearningRateScheduler()));

	//Exponential
	SetLearningRateMultiplier(ini->ReadDouble(section.PChar(), "LearningRateMultiplier", GetLearningRateMultiplier()));
	SetLearningRateUpdateInterval(ini->ReadInteger(section.PChar(), "LearningRateUpdateInterval", GetLearningRateUpdateInterval()));
	SetLearningRateUpdateIntervalUnit(ini->ReadInteger(section.PChar(), "LearningRateUpdateIntervalUnit", 1));// use iteration as default value for compatibility

	//Polynomial, Step
	SetLearningRatePower(ini->ReadDouble(section.PChar(), "LearningRatePower", GetLearningRatePower()));

	//Step
	FString learning_rate_steps_str = ini->ReadString(section.PChar(), "LearningRateSteps", GetLearningRateStepsStr());
	FListT<int> learning_rate_steps;
	while (learning_rate_steps_str != ""){
		learning_rate_steps.Add(learning_rate_steps_str.SplitBy(",").AsInt());
	}
	SetLearningRateSteps(learning_rate_steps);

	//Warmup
	SetLearningRateWarmupScheduler(ini->ReadString(section.PChar(), "LearningRateWarmupScheduler", GetLearningRateWarmupScheduler()));
	SetLearningRateWarmupLength(ini->ReadInteger(section.PChar(), "LearningRateWarmupLength", GetLearningRateWarmupLength()));
	SetLearningRateWarmupLengthUnit(ini->ReadInteger(section.PChar(), "LearningRateWarmupLengthUnit", 0));
}
//---------------------------------------------------------------------------
FString NNOptimizer::GetLearningRateStepsStr() const
{
	FString learning_rate_steps_str;
	for (int i = 0 ; i < GetLearningRateSteps().GetSize() ; i++){
		if (learning_rate_steps_str != ""){
			learning_rate_steps_str += FString(",");
		}
		learning_rate_steps_str += FString(GetLearningRateSteps().GetAt(i));
	}
	return learning_rate_steps_str;
}
//---------------------------------------------------------------------------
void NNOptimizer::SaveOptimizerToIni(FFastIni *ini, FString section)
{
	ini->Write<int>(section.PChar(), "RangeFrom", GetRangeFrom());
	ini->Write<int>(section.PChar(), "RangeTo", GetRangeTo());
	ini->Write<int>(section.PChar(), "RangeUnit", GetRangeUnit());

	ini->Write<FString>(section.PChar(), "Optimizer_Name", name_);
	ini->Write<FString>(section.PChar(), "Optimizer_NetworkName", network_name_);
	ini->Write<FString>(section.PChar(), "Optimizer_DatasetName", dataset_names_.GetText(','));

	ini->Write<int>(section.PChar(), "UpdateInterval", GetUpdateInterval());

	ini->Write<FString>(section.PChar(), "SolverName", solver_name_);
	ini->Write<int>(section.PChar(), "SolverParameterNum", solver_param_names_.GetSize());
	for (int i = 0; i < solver_param_names_.GetSize() ; i++) {
		ini->Write<FString>(FString(section.PChar()), FString("SolverParameterName_") + FString(i), solver_param_names_.GetAt(i));
		ini->Write<double>(FString(section.PChar()), FString("SolverParameter_") + FString(i), solver_parameters_.GetAt(i));
	}

	ini->Write<double>(section.PChar(), "WeightDecay", GetWeightDecay());

	ini->Write<FString>(section.PChar(), "LearningRateScheduler", GetLearningRateScheduler());

	ini->Write<double>(section.PChar(), "LearningRateMultiplier", GetLearningRateMultiplier());
	ini->Write<int>(section.PChar(), "LearningRateUpdateInterval", GetLearningRateUpdateInterval());
	ini->Write<int>(section.PChar(), "LearningRateUpdateIntervalUnit", GetLearningRateUpdateIntervalUnit());

	ini->Write<double>(section.PChar(), "LearningRatePower", GetLearningRatePower());

	ini->Write<FString>(section.PChar(), "LearningRateSteps", GetLearningRateStepsStr());

	ini->Write<FString>(section.PChar(), "LearningRateWarmupScheduler", GetLearningRateWarmupScheduler());
	ini->Write<int>(section.PChar(), "LearningRateWarmupLength", GetLearningRateWarmupLength());
	ini->Write<int>(section.PChar(), "LearningRateWarmupLengthUnit", GetLearningRateWarmupLengthUnit());
}
//---------------------------------------------------------------------------
void NNOptimizer::GetOptimizerConfigText(FStringList &result) const
{
	result.Add(FString("[") + name_ + FString("] (Optimizer)"));
	if (range_from_ >= 0 || range_to_ >= 0){
		FString range_text;
		if (range_from_ >= 0){
			range_text += FString(" from ") + FString(range_from_);
		}
		if (range_to_ >= 0){
			range_text += FString(" to ") + FString(range_to_);
		}
		range_text += FString(range_unit_ ? " iteration" : " epoch");
		result.Add(FString("Range:") + range_text);
	}

	result.Add(FString("Network: ") + network_name_);
	result.Add(FString("Dataset: ") + dataset_names_.GetText(','));

	if (GetUpdateInterval() >= 1){
		result.Add(FString("Update Interval: ") + FString(GetUpdateInterval()));
	}
	result.Add(FString("Solver: ") + solver_name_);
	for (int i = 0; i < solver_param_names_.GetSize() ; i++) {
		result.Add(FString("  ") + solver_param_names_.GetAt(i) + FString(": ") + FString(solver_parameters_.GetAt(i)));
	}
	if (GetWeightDecay() >= 0.0){
		result.Add(FString("Weight Decay: ") + FString(GetWeightDecay()));
	}

	result.Add(FString("Learning Rate Scheduler: ") + GetLearningRateScheduler());
	if (GetLearningRateScheduler() == "Exponential" || GetLearningRateScheduler() == "Step"){
		result.Add(FString("  Multiplier: ") + FString(GetLearningRateMultiplier()));
		FString unit_str = FString(GetLearningRateUpdateIntervalUnit() ? " iteration" : " epoch");
		if (GetLearningRateScheduler() == "Exponential") {
			result.Add(FString("  Interval: ") + FString(GetLearningRateUpdateInterval()) + unit_str);
		}else {//Sep
			result.Add(FString("  Steps: ") + GetLearningRateStepsStr() + unit_str);
		}
	}else if (GetLearningRateScheduler() == "Polynomial") {
		result.Add(FString("  Power: ") + FString(GetLearningRatePower()));
	}

	if (GetLearningRateWarmupScheduler() != "None") {
		result.Add(FString("Learning Rate Warmup Scheduler: ") + GetLearningRateWarmupScheduler());
		if (GetLearningRateWarmupScheduler() == "Linear") {
			result.Add(FString("  Length: ") + FString(GetLearningRateWarmupLength()) + FString(GetLearningRateWarmupLengthUnit() ? " iteration" : " epoch"));
		}
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void NNOptimizers::LoadOptimizersFromIni(FFastIni *ini)
{
	const int num_optimizer = ini->ReadInteger("Config", "NumOptimizer", -1);
	if (num_optimizer >= 0) {
		Clear();
		for (int i = 0; i < num_optimizer ; i++) {
			NNOptimizer &optimizer = Add();
			optimizer.SetSolverComponents(solver_components_);
			optimizer.LoadOptimizerFromIni(ini, FString("Optimizer_") + FString(i));
		}
	}else {
		// version 2.20 and previous
		ClearOptimizers();
		GetAt(0).LoadOptimizerFromIni(ini);
		if (GetAt(0).GetDatasetNames().GetSize() == 1 && GetAt(0).GetDatasetNames().GetAt(0) == ""){
			GetAt(0).GetDatasetNames_().GetAt(0) = FString("Training");
		}
	}
}
//---------------------------------------------------------------------------
void NNOptimizers::SaveOptimizersToIni(FFastIni *ini)
{
	ini->Write<int>("Config", "NumOptimizer", GetSize());
	for (int i = 0; i < GetSize() ; i++) {
		GetAt(i).SaveOptimizerToIni(ini, FString("Optimizer_") + FString(i));
	}
}
//---------------------------------------------------------------------------
void NNOptimizers::GetOptimizersConfigText(FStringList &result) const
{
	for (int i = 0; i < GetSize() ; i++) {
		if (i > 0){
			result.Add(FString(""));
		}
		GetAt(i).GetOptimizerConfigText(result);
	}
}
//---------------------------------------------------------------------------
/*
const NNOptimizer *NNOptimizers::GetOptimizerByDataset(const FString &dataset_name) const
{
	for (int i = 0; i < GetSize() ; i++) {
		if (GetAt(i).GetDatasetNames().IndexOf(dataset_name) >= 0){
			return &GetAt(i);
		}
	}
	return NULL;
}
*/
//---------------------------------------------------------------------------
int NNOptimizers::GetOptimizerIndexByName(FString name) const
{
	for (int i = 0; i < GetSize() ; i++) {
		if (GetAt(i).GetOptimizerName() == name){
			return i;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
NNOptimizer &NNOptimizers::AddOptimizer()
{
	int index = 1;
	while (true){
		FString name = FString("Optimizer_") + FString(index++);
		int found = false;
		for (int i = 0 ; i < GetSize() && !found ; i++) {
			if (GetAt(i).GetOptimizerName() == name){
				found = true;
			}
		}
		if (!found) {
			NNOptimizer &optimizer = Add();
			optimizer.SetSolverComponents(solver_components_);
			optimizer.SetOptimizerName(name);
			return optimizer;
		}
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
NNMonitor &NNMonitor::operator=(const NNMonitor &source)
{
	name_ = source.name_;
	network_name_ = source.network_name_;
	dataset_names_ = source.dataset_names_;

	return *this;
}
//---------------------------------------------------------------------------
void NNMonitor::LoadMonitorFromIni(FFastIni *ini, FString section)
{
	name_ = ini->ReadString(section.PChar(), "Monitor_Name", name_);
	network_name_ = ini->ReadString(section.PChar(), "Monitor_NetworkName", network_name_);
	dataset_names_.Clear();
	dataset_names_.SplitStr(ini->ReadString(section.PChar(), "Monitor_DatasetName", dataset_names_.GetText(',')), FString(","), FString(""));
}
//---------------------------------------------------------------------------
void NNMonitor::SaveMonitorToIni(FFastIni *ini, FString section)
{
	ini->Write<FString>(section.PChar(), "Monitor_Name", name_);
	ini->Write<FString>(section.PChar(), "Monitor_NetworkName", network_name_);
	ini->Write<FString>(section.PChar(), "Monitor_DatasetName", dataset_names_.GetText(','));
}
//---------------------------------------------------------------------------
void NNMonitor::GetMonitorConfigText(FStringList &result) const
{
	result.Add(FString("[") + name_ + FString("] (Monitor)"));
	result.Add(FString("Network Name: ") + network_name_);
	result.Add(FString("Dataset Name: ") + dataset_names_.GetText(','));
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void NNMonitors::LoadMonitorsFromIni(FFastIni *ini)
{
	const int num_monitor = ini->ReadInteger("Config", "NumMonitor", -1);
	if (num_monitor >= 0) {
		Clear();
		for (int i = 0; i < num_monitor ; i++) {
			Add().LoadMonitorFromIni(ini, FString("Monitor_") + FString(i));
		}
	}else {
		// version 2.20 and previous
		ClearMonitors();
	}
}
//---------------------------------------------------------------------------
void NNMonitors::SaveMonitorsToIni(FFastIni *ini)
{
	ini->Write<int>("Config", "NumMonitor", GetSize());
	for (int i = 0; i < GetSize() ; i++) {
		GetAt(i).SaveMonitorToIni(ini, FString("Monitor_") + FString(i));
	}
}
//---------------------------------------------------------------------------
void NNMonitors::GetMonitorsConfigText(FStringList &result) const
{
	for (int i = 0; i < GetSize() ; i++) {
		if (i > 0){
			result.Add(FString(""));
		}
		GetAt(i).GetMonitorConfigText(result);
	}
}
//---------------------------------------------------------------------------
int NNMonitors::GetMonitorIndexByName(FString name) const
{
	for (int i = 0; i < GetSize() ; i++) {
		if (GetAt(i).GetMonitorName() == name){
			return i;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
NNMonitor &NNMonitors::AddMonitor_()
{
	int index = 1;
	while (true){
		FString name = FString("Monitor_") + FString(index++);
		int found = false;
		for (int i = 0 ; i < GetSize() && !found ; i++) {
			if (GetAt(i).GetMonitorName() == name){
				found = true;
			}
		}
		if (!found) {
			NNMonitor &monitor = Add();
			monitor.SetMonitorName(name);
			return monitor;
		}
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
NNExecutor &NNExecutor::operator=(const NNExecutor &source)
{
	name_ = source.name_;
	network_name_ = source.network_name_;

	repeat_evaluation_type_ = source.repeat_evaluation_type_;
	num_evaluations_ = source.num_evaluations_;
	need_back_propagation_ = source.need_back_propagation_;
	return *this;
}
//---------------------------------------------------------------------------
void NNExecutor::LoadExecutorFromIni(FFastIni *ini, FString section)
{
	name_ = ini->ReadString(section.PChar(), "Executor_Name", name_);
	network_name_ = ini->ReadString(section.PChar(), "Executor_NetworkName", network_name_);
	dataset_names_.Clear();
	dataset_names_.SplitStr(ini->ReadString(section.PChar(), "Executor_DatasetName", dataset_names_.GetText(',')), FString(","), FString(""));

	SetNumEvaluations(ini->ReadInteger(section.PChar(), "NumEvaluations", GetNumEvaluations()));
	SetRepeatEvaluationType(ini->ReadInteger(section.PChar(), "RepeatEvaluationType", GetRepeatEvaluationType()));
	SetNeedBackPropagation(ini->ReadInteger(section.PChar(), "NeedBackPropagation", GetNeedBackPropagation()));
}
//---------------------------------------------------------------------------
void NNExecutor::SaveExecutorToIni(FFastIni *ini, FString section)
{
	ini->Write<FString>(section.PChar(), "Executor_Name", name_);
	ini->Write<FString>(section.PChar(), "Executor_NetworkName", network_name_);
	ini->Write<FString>(section.PChar(), "Executor_DatasetName", dataset_names_.GetText(','));

	ini->Write<int>(section.PChar(), "NumEvaluations", GetNumEvaluations());
	ini->Write<int>(section.PChar(), "RepeatEvaluationType", GetRepeatEvaluationType());
	ini->Write<int>(section.PChar(), "NeedBackPropagation", GetNeedBackPropagation());
}
//---------------------------------------------------------------------------
void NNExecutor::GetExecutorConfigText(FStringList &result) const
{
	result.Add(FString("[") + name_ + FString("] (Executor)"));
	result.Add(FString("Network Name: ") + network_name_);
	result.Add(FString("Dataset Name: ") + dataset_names_.GetText(','));
	if (GetNumEvaluations() != 0){
		result.Add(FString("Number of Evaluations: ") + FString(GetNumEvaluations()));
		result.Add(FString("Repeat Evaluation Type: ") + FString(GetRepeatEvaluationType() ? "Last" : "Mean"));
	}
	if (GetNeedBackPropagation()){
		result.Add(FString("Backpropagation: True"));
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void NNExecutors::LoadExecutorsFromIni(FFastIni *ini)
{
	const int num_executor = ini->ReadInteger("Config", "NumExecutor", -1);
	if (num_executor >= 0) {
		Clear();
		for (int i = 0; i < num_executor ; i++) {
			Add().LoadExecutorFromIni(ini, FString("Executor_") + FString(i));
		}
	}else {
		// version 2.20 and previous
		ClearExecutors();
		GetAt(0).LoadExecutorFromIni(ini);
		if (GetAt(0).GetDatasetNames().GetSize() == 1 && GetAt(0).GetDatasetNames().GetAt(0) == ""){
			GetAt(0).GetDatasetNames_().GetAt(0) = FString("Validation");
		}
	}
}
//---------------------------------------------------------------------------
void NNExecutors::SaveExecutorsToIni(FFastIni *ini)
{
	ini->Write<int>("Config", "NumExecutor", GetSize());
	for (int i = 0; i < GetSize() ; i++) {
		GetAt(i).SaveExecutorToIni(ini, FString("Executor_") + FString(i));
	}
}
//---------------------------------------------------------------------------
void NNExecutors::GetExecutorsConfigText(FStringList &result) const
{
	for (int i = 0; i < GetSize() ; i++) {
		if (i > 0){
			result.Add(FString(""));
		}
		GetAt(i).GetExecutorConfigText(result);
	}
}
//---------------------------------------------------------------------------
int NNExecutors::GetExecutorIndexByName(FString name) const
{
	for (int i = 0; i < GetSize() ; i++) {
		if (GetAt(i).GetExecutorName() == name){
			return i;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
NNExecutor &NNExecutors::AddExecutor()
{
	int index = 1;
	while (true){
		FString name = FString("Executor_") + FString(index++);
		int found = false;
		for (int i = 0 ; i < GetSize() && !found ; i++) {
			if (GetAt(i).GetExecutorName() == name){
				found = true;
			}
		}
		if (!found) {
			NNExecutor &executor = Add();
			executor.SetExecutorName(name);
			return executor;
		}
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
NNConfig &NNConfig::operator=(const NNConfig &source)
{
	is_sample_ = source.is_sample_;

	max_epoch_ = source.max_epoch_;
	save_best_ = source.save_best_;
	batch_size_ = source.batch_size_;
	type_config_ = source.type_config_;
	monitor_interval_ = source.monitor_interval_;
	auto_evaluation_ = source.auto_evaluation_;
	command_type_ = source.command_type_;
	script_name_ = source.script_name_;
	script_type_ = source.script_type_;
	script_file_name_ = source.script_file_name_;
	script_args_ = source.script_args_;
	config_update_count_ = source.config_update_count_;

	NNOptimizers::operator=(source);
	NNMonitors::operator=(source);
	NNExecutors::operator=(source);

	structure_search_enabled_ = source.structure_search_enabled_;
	structure_search_method_ = source.structure_search_method_;
	structure_search_objective_ = source.structure_search_objective_;

	structure_search_constraint_attributes_ = source.structure_search_constraint_attributes_;
	structure_search_constraint_min_ = source.structure_search_constraint_min_;
	structure_search_constraint_max_ = source.structure_search_constraint_max_;

	early_stopping_ = source.early_stopping_;
	time_limit_ = source.time_limit_;

	//Obsolete
	dataset_uri_[0] = source.dataset_uri_[0];
	dataset_uri_[1] = source.dataset_uri_[1];
	shuffle_ = source.shuffle_;
	dataset_cache_enabled_ = source.dataset_cache_enabled_;
	image_normalization_ = source.image_normalization_;
	// set_lrate_of_convolution_as_zero_ = source.set_lrate_of_convolution_as_zero_;

	return *this;
}
//---------------------------------------------------------------------------
void NNConfig::LoadConfigFromIni(FFastIni *ini, int load_tmp_config)
{
	SetIsSample(ini->ReadInteger("General", "IsSample", GetIsSample()));

	SetMaxEpoch(ini->ReadInteger("Config", "MaxEpoch", GetMaxEpoch()));
	SetSaveBest(ini->ReadInteger("Config", "SaveBest", GetSaveBest()));
	SetBatchSize(ini->ReadInteger("Config", "BatchSize", GetBatchSize()));
	FString type_config_str = ini->ReadString("Config", "TypeConfig", FString(GetTypeConfig() == TypeConfig::Half ? "Half" : "Float"));
	SetTypeConfig(type_config_str == "Half" ? TypeConfig::Half : TypeConfig::Float);
	SetMonitorInterval(ini->ReadInteger("Config", "MonitorInterval", GetMonitorInterval()));
	SetAutoEvaluation(ini->ReadInteger("Config", "AutoEvaluation", GetAutoEvaluation()));

	LoadOptimizersFromIni(ini);
	LoadMonitorsFromIni(ini);
	LoadExecutorsFromIni(ini);

    //Structure Search
	SetStructureSearchEnabled(ini->ReadBool("StructureSearch", "Enabled", GetStructureSearchEnabled()));
	FString structure_search_method_str = ini->ReadString("StructureSearch", "Method", FString(""));
	if (structure_search_method_str == "Random") {
		SetStructureSearchMethod(StructureSearchMethod::Random);
	}else if (structure_search_method_str == "NetworkFeatureAndGaussianProcess") {
		SetStructureSearchMethod(StructureSearchMethod::NetworkFeatureAndGaussianProcess);
	}else if (structure_search_method_str == "NetworkFeatureAndGaussianProcessOptimisticProb") {
		SetStructureSearchMethod(StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticProb);
	}else if (structure_search_method_str == "NetworkFeatureAndGaussianProcessOptimisticArea") {
		SetStructureSearchMethod(StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticArea);
	}

	FString structure_search_objective_str = ini->ReadString("StructureSearch", "Objective", FString(""));
	if (structure_search_objective_str == "Error") {
		SetStructureSearchObjective(StructureSearchObjective::Error);
	}else if (structure_search_objective_str == "ErrorAndCalculation") {
		SetStructureSearchObjective(StructureSearchObjective::ErrorAndCalculation);
	}

	for (int ia = 0 ; ia < 2 ; ia++) {
		structure_search_constraint_min_.SetAt(ia, ini->ReadDouble("StructureSearch", structure_search_constraint_attributes_.GetAt(ia) + FString("_Min"), structure_search_constraint_min_.GetAt(ia)));
		structure_search_constraint_max_.SetAt(ia, ini->ReadDouble("StructureSearch", structure_search_constraint_attributes_.GetAt(ia) + FString("_Max"), structure_search_constraint_max_.GetAt(ia)));
	}

	SetEarlyStopping(ini->ReadBool("StructureSearch", "EarlyStopping", GetEarlyStopping()));
	SetTimeLimit(ini->ReadDouble("StructureSearch", "TimeLimit", GetTimeLimit()));

	//Out-of-core Training
	char buffer[65];
	SetOutOfCoreTrainingEnabled(ini->ReadBool("OutOfCoreTraining", "Enabled", GetOutOfCoreTrainingEnabled()));
	sprintf(buffer, "%I64d", GetOutOfCoreTrainingGPUMemorySize());
	SetOutOfCoreTrainingGPUMemorySize(strtoll(ini->ReadString("OutOfCoreTraining", "GPUMemorySize", FString(buffer)).PChar(), NULL, 10));
	sprintf(buffer, "%I64d", GetOutOfCoreTrainingWindowSize());
	SetOutOfCoreTrainingWindowSize(strtoll(ini->ReadString("OutOfCoreTraining", "WindowSize", FString(buffer)).PChar(), NULL, 10));

	//Temporary Config
	if (load_tmp_config) {
		FString command_type_str = ini->ReadString("Temporary", "CommandType", FString(""));
		if (command_type_str == "Normal") {
			SetCommandType(SDeepCommandType::Normal);
		}else if (command_type_str == "CreateTrainingResult") {
			SetCommandType(SDeepCommandType::CreateTrainingResult);
		}else if (command_type_str == "Profiling") {
			SetCommandType(SDeepCommandType::Profiling);
		}else if (command_type_str == "CompareWithCPU") {
			SetCommandType(SDeepCommandType::CompareWithCPU);
		}else if (command_type_str == "Script") {
			SetCommandType(SDeepCommandType::Script);
		}
		script_name_ = ini->ReadString("Temporary", "ScriptName", script_name_);
		script_type_ = ini->ReadString("Temporary", "ScriptType", script_type_);
		script_file_name_ = ini->ReadString("Temporary", "ScriptFileName", script_file_name_);
		script_args_ = ini->ReadString("Temporary", "ScriptArgs", script_args_);
    }

	//Obsolete
	dataset_uri_[0] = ini->ReadString("Config", "DatasetURI_0", FString(""));
	if (dataset_uri_[0] == "") {
		dataset_uri_[0] = ini->ReadString("Dataset", "Training", FString(""));
	}
	dataset_uri_[1] = ini->ReadString("Config", "DatasetURI_1", FString(""));
	if (dataset_uri_[1] == "") {
		dataset_uri_[1] = ini->ReadString("Dataset", "Validation", FString(""));
	}
	SetShuffle_Obsolete(ini->ReadInteger("Config", "Shuffle", GetShuffle_Obsolete()));
	SetDatasetCacheEnabled_Obsolete(ini->ReadBool("StructureSearch", "DatasetCacheEnabled", GetDatasetCacheEnabled_Obsolete()));
	SetImageNormalization_Obsolete(ini->ReadInteger("Config", "ImageNormalization", GetImageNormalization_Obsolete()));
}
//---------------------------------------------------------------------------
void NNConfig::SaveConfigToIni(FFastIni *ini, int save_tmp_config)
{
	//General->IsSample is read only. Do not save.

	ini->Write<int>("Config", "MaxEpoch", GetMaxEpoch());
	ini->Write<bool>("Config", "SaveBest", GetSaveBest());
	ini->Write<int>("Config", "BatchSize", GetBatchSize());
	ini->Write<FString>("Config", "TypeConfig", FString(GetTypeConfig() == TypeConfig::Half ? "Half" : "Float"));
	ini->Write<int>("Config", "MonitorInterval", GetMonitorInterval());
	ini->Write<int>("Config", "AutoEvaluation", GetAutoEvaluation());

	SaveOptimizersToIni(ini);
	SaveMonitorsToIni(ini);
	SaveExecutorsToIni(ini);

	//Structure Search
	ini->Write<bool>("StructureSearch", "Enabled", GetStructureSearchEnabled());
	FString structure_search_method_str;
	switch (GetStructureSearchMethod()) {
	case StructureSearchMethod::Random:
		structure_search_method_str = "Random";
		break;
	case StructureSearchMethod::NetworkFeatureAndGaussianProcess:
		structure_search_method_str = "NetworkFeatureAndGaussianProcess";
		break;
	case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticProb:
		structure_search_method_str = "NetworkFeatureAndGaussianProcessOptimisticProb";
		break;
	case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticArea:
		structure_search_method_str = "NetworkFeatureAndGaussianProcessOptimisticArea";
		break;
	default:
		;
	}
	ini->Write<FString>("StructureSearch", "Method", structure_search_method_str);

	FString structure_search_objective_str;
	switch (GetStructureSearchObjective()) {
	case StructureSearchObjective::Error:
		structure_search_objective_str = "Error";
		break;
	case StructureSearchObjective::ErrorAndCalculation:
		structure_search_objective_str = "ErrorAndCalculation";
		break;
	default:
		;
	}
	ini->Write<FString>("StructureSearch", "Objective", structure_search_objective_str);

	for (int ia = 0 ; ia < 2 ; ia++) {
		ini->Write<double>("StructureSearch", structure_search_constraint_attributes_.GetAt(ia) + FString("_Min"), structure_search_constraint_min_.GetAt(ia));
		ini->Write<double>("StructureSearch", structure_search_constraint_attributes_.GetAt(ia) + FString("_Max"), structure_search_constraint_max_.GetAt(ia));
	}

	ini->Write<bool>("StructureSearch", "EarlyStopping", GetEarlyStopping());
	ini->Write<double>("StructureSearch", "TimeLimit", GetTimeLimit());

	//Out-of-Core Training
	char buffer[65];
	ini->Write<bool>("OutOfCoreTraining", "Enabled", GetOutOfCoreTrainingEnabled());
	sprintf(buffer, "%I64d", GetOutOfCoreTrainingGPUMemorySize());
	ini->Write<FString>("OutOfCoreTraining", "GPUMemorySize", FString(buffer));
	sprintf(buffer, "%I64d", GetOutOfCoreTrainingWindowSize());
	ini->Write<FString>("OutOfCoreTraining", "WindowSize", FString(buffer));

	//Temporary Config
	if (save_tmp_config) {
		FString command_type_str;
		switch (GetCommandType()) {
		case SDeepCommandType::Normal:
			command_type_str = "Normal";
			break;
		case SDeepCommandType::CreateTrainingResult:
			command_type_str = "CreateTrainingResult";
			break;
		case SDeepCommandType::Profiling:
			command_type_str = "Profiling";
			break;
		case SDeepCommandType::CompareWithCPU:
			command_type_str = "CompareWithCPU";
			break;
		case SDeepCommandType::Script:
			command_type_str = "Script";
			break;
		default:
			;
		}
		ini->Write<FString>("Temporary", "CommandType", command_type_str);
		ini->Write<FString>("Temporary", "ScriptName", script_name_);
		ini->Write<FString>("Temporary", "ScriptType", script_type_);
		ini->Write<FString>("Temporary", "ScriptFileName", script_file_name_);
		ini->Write<FString>("Temporary", "ScriptArgs", script_args_);
    }
}
//---------------------------------------------------------------------------
int NNConfig::SetStructureSearchConstraint(FString attribute, int max, double value)
{
	const int attribute_index = structure_search_constraint_attributes_.IndexOf(attribute);
	if (attribute_index < 0) {
		return false;
	}

	if (max) {
		structure_search_constraint_max_.SetAt(attribute_index, value);
	}else {
		structure_search_constraint_min_.SetAt(attribute_index, value);
	}

	return true;
}
//---------------------------------------------------------------------------
double NNConfig::GetStructureSearchConstraint(FString attribute, int max) const
{
	const int attribute_index = structure_search_constraint_attributes_.IndexOf(attribute);
	if (attribute_index < 0) {
		return -1.0;
	}

	if (max) {
		return structure_search_constraint_max_.GetAt(attribute_index);
	}else {
		return structure_search_constraint_min_.GetAt(attribute_index);
	}
}
//---------------------------------------------------------------------------
FString NNConfig::GetConfigText() const
{
	FStringList result;
	result.Add(FString("Max Epoch: ") + FString(GetMaxEpoch()) + FString(GetSaveBest() ? " (Save best)" : ""));
	result.Add(FString("Batch Size: ") + FString(GetBatchSize()));
	result.Add(FString("Precision: ") + FString(GetTypeConfig() == TypeConfig::Half ? "Half" : "Float"));

	result.Add(FString(""));
	GetOptimizersConfigText(result);

	result.Add(FString(""));
	GetMonitorsConfigText(result);

	result.Add(FString(""));
	GetExecutorsConfigText(result);

	return result.GetText();
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
