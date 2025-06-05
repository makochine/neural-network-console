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

#include <math.h>

#include "flist.h"
#include "nn_structure.h"

#include "nnstructure_to_vector.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
void NNStructureToVector(const NNStructure &source_structure, FListT<float> &result, FStringList &result_caption, int nnabla)
{
	NNStructure structure;
	structure = source_structure;

	//Arrange and Sort structure
	structure.ArrangeLayers(true);

	//Setup
	static const int depth_resolution = 8;
	FStringList main_layers;
	main_layers.Add(FString("Affine"));
	main_layers.Add(FString("Convolution"));
	if (!nnabla) {
		main_layers.Add(FString("LocallyConnectedLayer"));
	}
	main_layers.Add(FString("MaxPooling"));
	main_layers.Add(FString("AveragePooling"));
	main_layers.Add(FString("SumPooling"));
	if (!nnabla) {
		main_layers.Add(FString("Unpooling"));
		main_layers.Add(FString("Upsampling"));
	}
	main_layers.Add(FString("Tanh"));
	main_layers.Add(FString("Sigmoid"));
	if (!nnabla) {
		main_layers.Add(FString("Relu"));
		main_layers.Add(FString("PRelu"));
	}else {
		main_layers.Add(FString("ReLU"));
	}
	main_layers.Add(FString("ELU"));
	if (!nnabla) {
		main_layers.Add(FString("Softplus"));
	}
	main_layers.Add(FString("Abs"));
	if (!nnabla) {
		main_layers.Add(FString("Maxout"));
	}
	main_layers.Add(FString("BatchNormalization"));
	main_layers.Add(FString("Dropout"));
	if (!nnabla) {
		main_layers.Add(FString("DropMap"));
    }

	const int num_layer = main_layers.GetSize();

	//Check main_layers correctly defined
	for (int i = 0 ; i < num_layer ; i++) {
		int index = structure.GetNNLayerComponents()->GetComponentIndexByType(main_layers.GetAt(i));
		assert(index >= 0);
	}

	FStringList main_parameters;
	main_parameters.Add(FString("OutShape_of_Affine"));
	main_parameters.Add(FString("KernelShape:0_of_Convolution"));
	main_parameters.Add(FString("KernelShape:1_of_Convolution"));
	main_parameters.Add(FString("OutMaps_of_Convolution"));
	if (!nnabla) {
		main_parameters.Add(FString("PoolShape:0_of_MaxPooling"));
		main_parameters.Add(FString("PoolShape:1_of_MaxPooling"));
	}else {
		main_parameters.Add(FString("KernelShape:0_of_MaxPooling"));
		main_parameters.Add(FString("KernelShape:1_of_MaxPooling"));
    }
	main_parameters.Add(FString("Strides:0_of_MaxPooling"));
	main_parameters.Add(FString("Strides:1_of_MaxPooling"));
	if (!nnabla) {
		main_parameters.Add(FString("PoolShape:0_of_AveragePooling"));
		main_parameters.Add(FString("PoolShape:1_of_AveragePooling"));
	}else {
		main_parameters.Add(FString("KernelShape:0_of_AveragePooling"));
		main_parameters.Add(FString("KernelShape:1_of_AveragePooling"));
	}
	main_parameters.Add(FString("Strides:0_of_AveragePooling"));
	main_parameters.Add(FString("Strides:1_of_AveragePooling"));
	main_parameters.Add(FString("P_of_Dropout"));
	main_parameters.Add(FString("Output"));
	main_parameters.Add(FString("CostParameter"));
	main_parameters.Add(FString("CostMultiplyAdd"));

	//Result list
	result.Clear();
	result_caption.Clear();

	//Layer uni-gram
	float min_x = structure.GetNetworkViewSizeX();
	float max_y = 0.0, min_y = structure.GetNetworkViewSizeY();
	{
		int *uni_gram = new int[num_layer];
		memset(uni_gram, 0, sizeof(int) * num_layer);
		for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
			const NNLayer &layer = structure.GetLayer(il);
			const int layer_component_index = main_layers.IndexOf(layer.GetType());
			if (layer_component_index >= 0) {
				uni_gram[layer_component_index]++;
			}
			if (layer.GetPosition(0) < min_x) {
				min_x = layer.GetPosition(0);
			}
			if (layer.GetPosition(1) < min_y) {
				min_y = layer.GetPosition(1);
			}
			if (layer.GetPosition(1) > max_y) {
				max_y = layer.GetPosition(1);
			}
		}
		for (int i = 0 ; i < num_layer ; i++) {
			result.Add(uni_gram[i]);
			result_caption.Add(main_layers.GetAt(i));
		}
		delete[] uni_gram;
	}

	//Layer bi-gram
	{
		int *bi_gram = new int[num_layer * num_layer];
		memset(bi_gram, 0, sizeof(int) * num_layer * num_layer);
		for (int ili = 0 ; ili < structure.GetNumLink() ; ili++) {
			const NNLink &link = structure.GetLink(ili);
			const NNLayer *source_layer = structure.GetLayerByID(link.GetSourceLayerID());
			const NNLayer *dest_layer = structure.GetLayerByID(link.GetDestLayerID());
			if (source_layer && dest_layer) {
				const int source_layer_component_index = main_layers.IndexOf(source_layer->GetType());
				const int dest_layer_component_index = main_layers.IndexOf(dest_layer->GetType());
				if (source_layer_component_index >= 0 && dest_layer_component_index >= 0) {
					bi_gram[source_layer_component_index * num_layer + dest_layer_component_index]++;
				}
			}
		}
		for (int i = 0 ; i < num_layer * num_layer ; i++) {
			result.Add(bi_gram[i]);
			result_caption.Add(
				main_layers.GetAt(i / num_layer) +
				FString("_to_") +
                main_layers.GetAt(i % num_layer)
				);
		}
		delete[] bi_gram;
	}

	//Layer histogram
	{
		int *histogram = new int[num_layer * depth_resolution];
		memset(histogram, 0, sizeof(int) * num_layer * depth_resolution);
		for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
			const NNLayer &layer = structure.GetLayer(il);
			const int layer_component_index = main_layers.IndexOf(layer.GetType());
			if (layer_component_index >= 0) {
				int pos = (int)((layer.GetPosition(1) - min_y) / (max_y - min_y) * (depth_resolution - 1));
				assert(pos >= 0 && pos < depth_resolution);
				histogram[layer_component_index * depth_resolution + pos]++;
			}
		}
		for (int i = 0 ; i < num_layer * depth_resolution ; i++) {
			result.Add(histogram[i]);
			result_caption.Add(
				main_layers.GetAt(i / depth_resolution) +
				FString("_at_") +
				FString(i % depth_resolution)
				);
		}
		delete[] histogram;
	}

	//Layer Parallelization
	{
		int *parallelization = new int[depth_resolution];
		memset(parallelization, 0, sizeof(int) * depth_resolution);
		for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
			const NNLayer &layer = structure.GetLayer(il);
			const int pos = (int)((layer.GetPosition(1) - min_y) / (max_y - min_y) * (depth_resolution - 1));
			assert(pos >= 0 && pos < depth_resolution);
			const int stage = (int)((layer.GetPosition(0) - min_x) / (NNLayerDefaultWidth + NNLayerDefaultHSpace));
			assert(stage >= 0);
			if (parallelization[pos] < stage) {
				parallelization[pos] = stage;
			}
		}
		for (int i = 0 ; i < depth_resolution ; i++) {
			result.Add(parallelization[i]);
			result_caption.Add(
				FString("Parallelization_at_") +
				FString(i));
		}
		delete[] parallelization;
	}

	//Sum, Max and Avg of main parameters
	{
		float *sum_max_avg = new float[3 * main_parameters.GetSize() * depth_resolution];
		memset(sum_max_avg, 0, sizeof(float) * 3 * main_parameters.GetSize() * depth_resolution);
		for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
			const NNLayer &layer = structure.GetLayer(il);
			const int layer_component_index = main_layers.IndexOf(layer.GetType());
			if (layer_component_index >= 0) {
				int pos = (int)((layer.GetPosition(1) - min_y) / (max_y - min_y) * (depth_resolution - 1));
				assert(pos >= 0 && pos < depth_resolution);

				for (int ip = 0; ip < main_parameters.GetSize() ; ip++) {
					FString layer_type = main_parameters.GetAt(ip);
					FString property_index = layer_type.SplitBy("_of_");
					FString property_name = property_index.SplitBy(':');
					if (layer_type == "" || layer_type == layer.GetType()) {
						const NNLayerProperty *property = layer.GetPropertyByName(property_name);
						if (property) {
							int enable = false;
							float value = 0.0;
							if (property_index == "") {
								value = property->GetIntArrayProduct();
								enable = true;
							}else {
								const int property_index_int = property_index.AsInt();
								if (property_index_int < property->GetFloatArray().GetSize()) {
									value = property->GetFloatArray().GetAt(property_index_int);
									enable = true;
								}else if (property_index_int < property->GetIntArray().GetSize()) {
									value = property->GetIntArray().GetAt(property_index_int);
									enable = true;
								}
                            }
							if (enable) {
								//Sum
								sum_max_avg[0 * main_parameters.GetSize() * depth_resolution + ip * depth_resolution + pos] += value;
								//Max
								if (sum_max_avg[1 * main_parameters.GetSize() * depth_resolution + ip * depth_resolution + pos] < value){
									sum_max_avg[1 * main_parameters.GetSize() * depth_resolution + ip * depth_resolution + pos] = value;
								}
								//Avg(count)
								sum_max_avg[2 * main_parameters.GetSize() * depth_resolution + ip * depth_resolution + pos] += 1.0f;
							}
						}
					}
				}

			}
		}

		//Calc average
		for (int i = 0 ; i < main_parameters.GetSize() * depth_resolution ; i++) {
			sum_max_avg[2 * main_parameters.GetSize() * depth_resolution + i] =
				sum_max_avg[2 * main_parameters.GetSize() * depth_resolution + i] > 0.0f ?
					sum_max_avg[0 * main_parameters.GetSize() * depth_resolution + i] / sum_max_avg[2 * main_parameters.GetSize() * depth_resolution + i] :
					0.0;
		}

		//Linear to Log
		for (int i = 0 ; i < 3 * main_parameters.GetSize() * depth_resolution ; i++) {
			if (sum_max_avg[i] > 0.0f) {
				sum_max_avg[i] = log(sum_max_avg[i]);
			}
		}

		for (int i = 0 ; i < main_parameters.GetSize() * depth_resolution ; i++) {
			result.Add(sum_max_avg[i]);
			result_caption.Add(
				FString("Sum_") +
				main_parameters.GetAt(i / depth_resolution) +
				FString("_at_") +
				FString(i % depth_resolution));

		}
		for (int i = 0 ; i < main_parameters.GetSize() * depth_resolution ; i++) {
			result.Add(sum_max_avg[1 * main_parameters.GetSize() * depth_resolution + i]);
			result_caption.Add(
				FString("Max_") +
				main_parameters.GetAt(i / depth_resolution) +
				FString("_at_") +
				FString(i % depth_resolution));
		}
		for (int i = 0 ; i < main_parameters.GetSize() * depth_resolution ; i++) {
			result.Add(sum_max_avg[2 * main_parameters.GetSize() * depth_resolution + i]);
			result_caption.Add(
				FString("Avg_") +
				main_parameters.GetAt(i / depth_resolution) +
				FString("_at_") +
				FString(i % depth_resolution));
		}
		delete[] sum_max_avg;
	}
	assert(result.GetSize() == result_caption.GetSize());
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
