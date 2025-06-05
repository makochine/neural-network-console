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

#include "structure_search.h"

#include "sdeep_console_global_settings.h"
#include "nn_document.h"
#include "nnstructure_to_vector.h"
#include "gaussian_process.h"

using namespace elfe;
using namespace sdeep_console;

//---------------------------------------------------------------------------
int structure_search_random(NNDocument& nn_document_, const FListP<NNDocument> &result_structures, const FListT<int> &pareto_optimal_index_all_sorted, const FListT<int> &pareto_optimal_index_seed)
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

		const double cost_multiply_add = nn_document_.GetStatistics(FString("CostMultiplyAdd"))->GetSum();
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

	return trial < 100;
}
//---------------------------------------------------------------------------
double erf_internal(double x)
{
	// constants
	double a1 =  0.254829592;
	double a2 = -0.284496736;
	double a3 =  1.421413741;
	double a4 = -1.453152027;
	double a5 =  1.061405429;
	double p1  =  0.3275911;

	// Save the sign of x
	int sign = 1;
	if (x < 0)
		sign = -1;
	x = fabs(x);

	// A&S formula 7.1.26
	double t = 1.0/(1.0 + p1*x);
	double y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

	return sign*y;
}
//---------------------------------------------------------------------------
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
		return (erf_internal(b) - erf_internal(a)) / 2;
	}else if (a <0.0 && b > 0.0) {
		return (erf_internal(b) + erf_internal(-a)) / 2;
	}else {
		return (erf_internal(-a) - erf_internal(-b)) / 2;
	}
}
//---------------------------------------------------------------------------
int structure_search_network_feature_and_gaussian_process(NNDocument& nn_document_, const FListP<NNDocument> &result_structures, const FListT<double> &result_errors, const FListT<double> &result_costs, const FListT<int> &pareto_optimal_index_all_sorted, const FListT<int> &pareto_optimal_index_seed)
{
	//Prepare Gaussian Process
	//Setup
	static const double exploration_sigma = 1.0;
	static const double max_error_ratio = 3.0;

	//Extract feature from result networks
	printf("Extracting features from result networks...\n");
	CSVDataset error_and_feature;
	int index = 0;
	for (int ir = 0 ; ir < result_structures.GetSize() ; ir++) {
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
			if (result_errors.GetAt(ir) < NAN_VALUE_THRESHOLD) {
				//Error
				error_and_feature.SetValue(index, 0, FString(log((double)result_errors.GetAt(ir))));
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
		//Fallback to random search if there are no training data for gaussian process
		printf("No training data for gaussian process. Using random search.\n");
		return structure_search_random(nn_document_, result_structures, pareto_optimal_index_all_sorted, pareto_optimal_index_seed);
	}

	//Create Candidates
	printf("Creating structure candidates...\n");
	FListP<NNStructures> candidates;
	const int num_candidate = 2000;
	for (int it = 0 ; it < num_candidate ; it++) {
		//Open seed network
		if (it < num_candidate / 2 && pareto_optimal_index_seed.GetSize()) {
			const int seed_result = rand() % pareto_optimal_index_seed.GetSize();
			nn_document_.NNStructures::operator =(result_structures.GetAt(pareto_optimal_index_seed.GetAt(seed_result)));
		}else {
			const int seed_result = rand() % result_structures.GetSize();
			nn_document_.NNStructures::operator =(result_structures.GetAt(seed_result));
		}
		nn_document_.Mutate();

		const double cost_multiply_add = nn_document_.GetStatistics(FString("CostMultiplyAdd"))->GetSum();
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
		for (int ir = 0; ir < result_structures.GetSize() && !same_found ; ir++) {
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
	}
	//error_and_feature.SaveToFile(AnsiString(ExtractFilePath(ParamStr(0)) + "test.csv").c_str());
	effective_col.Sort();
	assert(effective_col.GetSize() + 1 == error_and_feature.GetNumColumn());
	printf("\tnum result networks : %d\n", training_num);
	printf("\tnum effective error and feature dims : %d\n", error_and_feature.GetNumColumn());

	//Train Gaussian Process
	printf("Training gaussian process...\n");
	GaussianProcess_GaussianKernel gp;
	gp.SetGaussianKernelSigma(10.0);
	gp.SetBeta(4.0);
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
	printf("\tmin_error : %lf, max_error : %lf\n", min_error, max_error);

	gp.Train();
	//Tuning
	for (int i = 0 ; i < 100 ; i++) {
		gp.SearchHyperParameter();
	}

	//Search possible networks
	printf("Search possible networks...\n");
	double best_gain_area = -1.0e+304;
	NNStructures best_structure;
	/*
	structure_search_candidate_myu_.Clear();
	structure_search_candidate_sigma_.Clear();
	structure_search_candidate_cost_.Clear();
	structure_search_candidate_gain_.Clear();
	structure_search_candidate_index_ = -1;
	*/
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
				double area_gain = 0.0;
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
				FStringList sl;
				sl.Add(FString("myu\t") + FString(y_myu));
				sl.Add(FString("sigma\t") + FString(y_sigma));
				sl.Add(FString("myu2\t") + FString(y_myu2));
				sl.Add(FString("left\ttop\tright\tbottom\tscore\tarea\tsum"));

				switch (nn_document_.GetStructureSearchObjective()) {
				case StructureSearchObjective::Error:
					{
						const double best_validation = result_errors.GetAt(pareto_optimal_index_all_sorted.GetAt(pareto_optimal_index_all_sorted.GetSize() - 1));
						switch (nn_document_.GetStructureSearchMethod()) {
						case StructureSearchMethod::NetworkFeatureAndGaussianProcess:
						case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticProb:
							area_gain =
								p(y_myu2, y_sigma * y_sigma, y_myu2 - y_sigma * 10, log(best_validation));
							break;
						case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticArea:
							area_gain =
								y_myu2 < log(best_validation) ?
									log(best_validation) - y_myu2 :
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
								const double best_validation = result_errors.GetAt(pareto_optimal_index_all_sorted.GetAt(i));
								const double multiply_add = result_costs.GetAt(pareto_optimal_index_all_sorted.GetAt(i));
								if (best_validation > 0.0){
									if (multiply_add > cost_multiply_add &&
										(best_validation <= validation_max || validation_max <= 0.0)) {
										const double upper_error =
											(last_error >= -1.0e299 && (validation_max <= 0.0 || last_error < log(validation_max))) ?
												last_error:
												(validation_max >= 0.0 ?
													log(validation_max) :
													log(best_validation) + y_sigma * 8.0);
										const double prob =
											p(y_myu2, y_sigma * y_sigma, log(best_validation), upper_error);
										const double upper_cost =
											multiply_add > cost_multiply_add * 1.5 ?
											cost_multiply_add * 1.5 :
											multiply_add;
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
											FString(log(best_validation)) +
											FString("\t") +
											FString(prob) +
											FString("\t") +
											FString(prob * cost_diff) +
											FString("\t") +
											FString(area_gain)
											);
									}
									last_error = log(best_validation);
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
								const double best_validation = result_errors.GetAt(pareto_optimal_index_all_sorted.GetAt(i));
								const double multiply_add = result_costs.GetAt(pareto_optimal_index_all_sorted.GetAt(i));
								if (best_validation < 0.0){
									if (multiply_add > cost_multiply_add &&
										(best_validation <= validation_max || validation_max <= 0.0)) {
										const double upper_error =
											(last_error >= -1.0e299 && (validation_max <= 0.0 || last_error < log(validation_max))) ?
												last_error:
												(validation_max >= 0.0 ?
													log(validation_max) :
													log(best_validation * 1.1));
										const double lower_error =
											log(best_validation) < y_myu2 ? y_myu2 : log(best_validation);
										if (upper_error > lower_error) {
											const double length =
												upper_error - lower_error;
											const double upper_cost =
												multiply_add > cost_multiply_add * 1.5 ?
												cost_multiply_add * 1.5 :
												multiply_add;
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
									last_error = log(best_validation);
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

				/*
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
				*/
				if (area_gain > best_gain_area || (area_gain == best_gain_area && y_myu2 < best_myu2)) {
					best_gain_area = area_gain;
					best_myu2 = y_myu2;
					best_structure = candidates.GetAt(it);
					best_log = sl;
					//structure_search_candidate_index_ = structure_search_candidate_myu_.GetSize() - 1;
				}
			}
			// printf("\tNo. %d, myu = %lf, sigm = %lf, gain = %lf\n", it, y_myu, y_sigma, area_gain);
        }
	}

	/*
	if (online_debug_mode_) {
		for (int i = 0 ; i < best_log.GetSize() ; i++) {
			Me_Debug->Lines->Add(best_log.GetAt(i).PChar());
		}
	}
	*/

	if (best_structure.GetNumLayer() && best_gain_area > 0.0) {
		//success
		printf("\tExpected gain = %lf.\n", best_gain_area);
		nn_document_.NNStructures::operator =(best_structure);
		/*
		if (online_debug_mode_) {
			Me_Debug->Lines->Add(UnicodeString("Best Gain Area : ") + FloatToStr(best_gain_area));
		}
		if (structure_search_candidate_myu_.GetSize() && structure_search_candidate_index_ >= 0) {
			nn_document_.SetStructureSearchExpectiation(
				structure_search_candidate_myu_.GetAt(structure_search_candidate_index_),
				structure_search_candidate_sigma_.GetAt(structure_search_candidate_index_));
		}
		*/
		return true;
	}else {
		//random if failed
		printf("Gaussian process failed. Using random search.\n");
		return structure_search_random(nn_document_, result_structures, pareto_optimal_index_all_sorted, pareto_optimal_index_seed);
	}
}
//---------------------------------------------------------------------------
int structure_search(FStringList &args)
{
	//Check args
	if (args.GetSize() != 4) {
		printf("usage : sdeep_console_cli_util structure_search -i INCSV -o OUTPRJ\n");
		printf("\n");
		printf("optional argments:\n");
		printf("  -i INCSV\n");
		printf("  \tinput CSV file with sdcproj, best_validation_error and cost_multiply_add(*.csv)\n");
		printf("  -o OUTPRJ\n");
		printf("  \toutput project file(*.sdcproj)\n");
		return 0;
	}

	FString input_csv_file_name;
	FString output_project_file_name;
	for (int i = 2 ; i < 4 ; i++) {
		FString option = args.GetAt(i);
		if (option.SubString(1, 2) == "-i"){
			option.SplitBy(" ");
			input_csv_file_name = option;
		}else if (option.SubString(1, 2) == "-o"){
			option.SplitBy(" ");
			output_project_file_name = option;
		}
	}
	if (input_csv_file_name == "") {
		printf("error : argument -i is required.\n");
		return 1;
	}else {
		printf("input CSV file name : %s\n", input_csv_file_name.PChar());
	}
	if (output_project_file_name == "") {
		printf("error : argument -o is required.\n");
		return 1;
	}else {
		printf("output project file name : %s\n", output_project_file_name.PChar());
	}

	//Setup
	srand(time(NULL));//randomize
	SDeepConsoleGlobalSettings settings;
	{
		FStringList errors;
		settings.SetupEngines(FString("settings/"), errors);
		if (errors.GetSize()) {
			for (int ie = 0; ie < errors.GetSize() ; ie++) {
				printf("error : %s\n", errors.GetAt(ie).PChar());
			}
			return 1;
		}
	}

	//OpenCSVFile
	CSVDataset csv;
	if (!csv.LoadFromFile(input_csv_file_name.PChar(), false, false)){
		printf("error : An error occurred while loading input csv file.\n");
		return 1;
	}
	if (csv.GetNumColumn() != 3){
		printf("error : Illigal CSV file format. Number of column must be 3.\n");
		return 1;
	}

	//Open networks
	FListP<NNDocument> projects;
	FListT<double> errors;
	FListT<double> costs;
	printf("Loading networks...\n");
	for (int ip = 0 ; ip < csv.GetNumData() ; ip++){
		NNDocument &project = projects.Add();
		for (int i = 0; i < (int)SDeepEngineType::NumEngineType ; i++) {
			SDeepEngineType sdeep_engine_type = SDeepEngineType(i);
			const NNEngineInfo &engine_info = settings.GetEngineInfo().GetAt(i);
			project.RegisterNNEngineInfo(sdeep_engine_type, &engine_info);
		}
		for (int i = 0; i < settings.GetDocumentConverterSettingList().GetSize() ; i++) {
			project.RegisterNNDocumentConverterSetting(&settings.GetDocumentConverterSettingList().GetAt(i));
		}
		if (!project.LoadFromFile(csv.GetValue(ip, 0), FString(""), false, false) || !project.GetNumLayer()) {
			printf("error : An error occurred while loading input project file %s.\n", csv.GetValue(ip, 0).PChar());
			return 1;
		}
		if (csv.GetValue(ip, 1) != "" && csv.GetValue(ip, 2) != ""){
			errors.Add(csv.GetValue(ip, 1).AsFloat());
			costs.Add(csv.GetValue(ip, 2).AsFloat());
		}else {
			errors.Add(NAN_VALUE);
			costs.Add(NAN_VALUE);
		}
	}
	printf("\t%d networks are succcesfully loaded.\n", projects.GetSize());

	//Enum pareto optimal networks
	printf("Enumerating pareto optimal networks...\n");
	FListT<int> pareto_optimal_index_seed;// index of parato optimals within the constraints
	FListT<int> pareto_optimal_index_all_sorted;// index of all pareto optimals
	const NNDocument &last_project = projects.GetAt(projects.GetSize() - 1);
	for (int i = 0 ; i < projects.GetSize() ; i++){
		int pareto_optimal = errors.GetAt(i) < NAN_VALUE_THRESHOLD && costs.GetAt(i) < NAN_VALUE_THRESHOLD;
		for (int i2 = 0 ; i2 < projects.GetSize() && pareto_optimal ; i2++) if (i != i2){
			pareto_optimal &= !(errors.GetAt(i2) <= errors.GetAt(i) && costs.GetAt(i2) <= costs.GetAt(i));
		}
		if (pareto_optimal){
			pareto_optimal_index_all_sorted.Add(i);
			int add = true;

			const double validation_min = last_project.GetStructureSearchConstraint(FString("Validation"), 0);
			if (validation_min != 0.0) {
				add &= errors.GetAt(i) >= validation_min;
			}
			const double validation_max = last_project.GetStructureSearchConstraint(FString("Validation"), 1);
			if (validation_max != 0.0) {
				add &= errors.GetAt(i) <= validation_max;
			}

			const double cost_multiply_add_min = last_project.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 0);
			if (cost_multiply_add_min >= 0) {
				add &= costs.GetAt(i) >= cost_multiply_add_min;
			}
			const double cost_multiply_add_max = last_project.GetStructureSearchConstraint(FString("CostMultiplyAdd"), 1);
			if (cost_multiply_add_max >= 0) {
				add &= costs.GetAt(i) <= cost_multiply_add_max;
			}

			if (add) {
				pareto_optimal_index_seed.Add(i);
			}
		}
	}
	//Sort pareto_optimal_index_all_sorted by CostMultiplyAdd
	for (int i = 1 ; i < pareto_optimal_index_all_sorted.GetSize() ; i++) {
		for (int i2 = i; i2 > 0 ; i2--) {
			if (costs.GetAt(pareto_optimal_index_all_sorted.GetAt(i2)) <
				costs.GetAt(pareto_optimal_index_all_sorted.GetAt(i2 - 1))) {
				pareto_optimal_index_all_sorted.Swap(i2, i2 - 1);
			}else {
				break;
			}
		}
	}
	printf("\t%d pareto optimal networks.\n", pareto_optimal_index_all_sorted.GetSize());
	printf("\t%d pareto optimal networks within constraints.\n", pareto_optimal_index_seed.GetSize());

	//Prepare current project
	NNDocument project;
	for (int i = 0; i < (int)SDeepEngineType::NumEngineType ; i++) {
		SDeepEngineType sdeep_engine_type = SDeepEngineType(i);
		const NNEngineInfo &engine_info = settings.GetEngineInfo().GetAt(i);
		project.RegisterNNEngineInfo(sdeep_engine_type, &engine_info);
	}
	for (int i = 0; i < settings.GetDocumentConverterSettingList().GetSize() ; i++) {
		project.RegisterNNDocumentConverterSetting(&settings.GetDocumentConverterSettingList().GetAt(i));
	}
	if (!project.LoadFromFile(csv.GetValue(csv.GetNumData() - 1, 0), FString(""), false, false)) {
		printf("error : An error occurred while loading last project file.\n");
		return 1;
	}

	//Create new structure
	int result = true;
	switch (project.GetStructureSearchMethod()) {
	case StructureSearchMethod::Random:
		printf("Creating new structure with random search...\n");
		result = structure_search_random(project, projects, pareto_optimal_index_all_sorted, pareto_optimal_index_seed);
		break;
	case StructureSearchMethod::NetworkFeatureAndGaussianProcess:
	case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticProb:
	case StructureSearchMethod::NetworkFeatureAndGaussianProcessOptimisticArea:
		printf("Creating new structure with network feature and gaussian process...\n");
		result = structure_search_network_feature_and_gaussian_process(project, projects, errors, costs, pareto_optimal_index_all_sorted, pareto_optimal_index_seed);
		break;
	default:
		;
	}
	if (!result){
		printf("error : Failed to create new network structure.\n");
		return 1;
	}

	//Save
	if (!project.SaveToFile(output_project_file_name)) {
		printf("error : An error occurred while saving output project file.\n");
		return 1;
	}

	printf("The new network structure was successfully created.\n");

	return 0;
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
