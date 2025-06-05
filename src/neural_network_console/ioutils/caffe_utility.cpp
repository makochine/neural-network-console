// Copyright 2016,2017,2018,2019,2020,2021 Sony Corporation.
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

#include "caffe_utility.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
FString GetPropertyValue(FStringList &property, FStringList &value, FString key)
{
    FString result;
	while (key != ""){
		FString key_1 = key.SplitBy(",");
		while (true){
			const int index = property.IndexOf(FString(key_1));
			if (index >= 0) {
				property.GetAt(index) = FString("*") + property.GetAt(index);
				FString ret = value.GetAt(index);
				property.Delete(index);
				value.Delete(index);

				if (ret.Length() >= 2 &&
					((ret.SubString(1,1) == "'" && ret.SubString(ret.Length(),1) == "'") ||
					 (ret.SubString(1,1) == "\"" && ret.SubString(ret.Length(),1) == "\""))){
					ret = ret.SubString(2, ret.Length() - 2);
				}
				if (result != ""){
					result += ",";
				}
				result += ret;
			}else {
				break;
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void CreateLayerFromPropertyAndValue_NNabla(
	FStringList &property,
	FStringList &value,
	NNStructure &structure,
	FStringList &error_str,
	FStringList &link_from_buffer,
	FListT<int> &link_to_id,
	FStringList &link_buffer_name,
	FListT<int> &link_buffer_id)
{
	for (int i = 0 ; i < property.GetSize() ; i++) {
		property.GetAt(i) = property.GetAt(i).ToLower().ReplaceStr("layers.layer.", "layer.").ReplaceStr("layers.", "layer.");
	}

	//Input data of deploy prototxt
	FString input_name = GetPropertyValue(property, value, FString("input")).ToLower();
	if (input_name != "") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Input"));
		NNLayer *layer = &structure.AddLayer(component_index);

		layer->SetPropertyValue(FString("Name"), input_name);

		FString input_dim_strs = GetPropertyValue(
			property, value,
			FString("input_dim,input_shape.dim"));
		FStringList dims;
		while (input_dim_strs != ""){
			FString input_dim_str = input_dim_strs.SplitBy(",");
			if (input_dim_str != "") {
				dims.Add(input_dim_str);
			}else {
				break;
			}
		}
		while (dims.GetSize() > 3){
			dims.Delete(0);
		}
		FString size_str;
		for (int i = 0 ; i < dims.GetSize() ; i++) {
			if (i > 0) {
				size_str += FString(",");
			}
			size_str += dims.GetAt(i);
		}
		layer->SetPropertyValue(FString("Size"), size_str);
		link_buffer_name.Add(input_name);
		link_buffer_id.Add(layer->GetLayerID());
	}

	//Ignore layer for test
	FString test_phase = GetPropertyValue(
		property, value,
		FString("layer.include.phase")).ToLower();
	if (test_phase == "test") {
		return;
	}

	//Create layer
	FString caffe_layer_type = GetPropertyValue(property, value, FString("layer.type")).ToLower();
	NNLayer *layer = NULL;
	if (caffe_layer_type == "input") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Input"));
		layer = &structure.AddLayer(component_index);
		FString shape_str = GetPropertyValue(
			property, value,
			FString("layer.input_param.shape.dim,layer.shape.dim"));
		if (shape_str != "") {
			shape_str.SplitBy(",");//remove batch_size
			if (shape_str == ""){
				shape_str = FString("1");
			}
			layer->SetPropertyValue(FString("Size"), shape_str);
		}
	}else if (caffe_layer_type == "data") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Input"));
		layer = &structure.AddLayer(component_index);
		FString size_str = GetPropertyValue(
			property, value,
			FString("layer.transform_param.crop_size,layer.cropsize"));
		if (size_str != "") {
			layer->SetPropertyValue(FString("Size"), FString("3,") + size_str + FString(",") +  size_str);
		}
	}else if (caffe_layer_type == "videodata" || caffe_layer_type == "annotateddata") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Input"));
		layer = &structure.AddLayer(component_index);
		FString width_str = GetPropertyValue(
			property, value,
			FString("layer.transform_param.resize_param.width,layer.resize_param.width"));
		FString height_str = GetPropertyValue(
			property, value,
			FString("layer.transform_param.resize_param.height,layer.resize_param.height"));
		if (width_str != "" && height_str != "") {
			layer->SetPropertyValue(FString("Size"), FString("3,") + height_str + FString(",") +  width_str);
		}
	}else if (caffe_layer_type == "softmaxwithloss" || caffe_layer_type == "softmax_loss" || caffe_layer_type == "softmax") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("SoftmaxCrossEntropy"));
		layer = &structure.AddLayer(component_index);
	}else if (caffe_layer_type == "sigmoidcrossentropyloss" || caffe_layer_type == "sigmoid_cross_entropy_loss") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("SigmoidCrossEntropy"));
		layer = &structure.AddLayer(component_index);
	}else if (caffe_layer_type == "euclidianloss" || caffe_layer_type == "euclidian_loss") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("SquaredError"));
		layer = &structure.AddLayer(component_index, true);
	}else if (caffe_layer_type == "innerproduct" || caffe_layer_type == "inner_product") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Affine"));
		layer = &structure.AddLayer(component_index);

		//OutShape
		FString out_str = GetPropertyValue(
			property, value,
			FString("layer.inner_product_param.num_output,layer.num_output"));
		if (out_str != "") {
			layer->SetPropertyValue(FString("OutShape"), FString(out_str));
		}

		//Bias
		FString bias_str = GetPropertyValue(
			property, value,
			FString("layer.inner_product_param.bias_term,layer.bias_term"));
		if (bias_str.ToLower() == "false") {
			layer->SetPropertyValue(FString("WithBias"), FString("False"));
		}else {
			layer->SetPropertyValue(FString("WithBias"), FString("True"));//Default=true
		}

		//Learning Rate Factor
		FString blobs_lr_str = GetPropertyValue(
			property, value,
			FString("layer.inner_product_param.blobs_lr,layer.blobs_lr,layer.param.lr_mult"));
		if (blobs_lr_str != "") {
			layer->SetPropertyValue(FString("W.LRateMultiplier"), FString(blobs_lr_str.AsFloat()));
		}
		FString blobs_lr_str_2 = GetPropertyValue(
			property, value,
			FString("layer.inner_product_param.blobs_lr,layer.blobs_lr,layer.param.lr_mult"));
		if (blobs_lr_str != "") {
			layer->SetPropertyValue(FString("b.LRateMultiplier"), FString(blobs_lr_str_2.AsFloat()));
		}
	}else if (caffe_layer_type == "convolution" || caffe_layer_type == "conv" || caffe_layer_type == "convolutiondepthwise") {
		//Group
		FString group_str = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.group,layer.group"));
		//OutMaps
		FString map_str = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.num_output,layer.convolution_param.c_o,layer.num_output,layer.c_o"));

		if ((group_str.AsInt() > 1 && group_str == map_str) || caffe_layer_type == "convolutiondepthwise"){
			int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("DepthwiseConvolution"));
			layer = &structure.AddLayer(component_index);
			group_str = "";
		}else {
			int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Convolution"));
			layer = &structure.AddLayer(component_index);
		}

        //Group and map
		if (map_str != "") {
			layer->SetPropertyValue(FString("OutMaps"), FString(map_str));
		}
		if (group_str != "") {
			layer->SetPropertyValue(FString("Group"), FString(group_str));
		}

		//Kernel Size
		FString kernel_str = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.kernel_size,layer.kernelsize"));
		int kernel_size[2] = {0, 0};
		if (kernel_str != "") {
			kernel_size[0] = kernel_size[1] = kernel_str.AsInt();
			layer->SetPropertyValue(FString("KernelShape"), FString(kernel_str) + FString(",") + FString(kernel_str));
		}else {
			FString kernel_w_str = GetPropertyValue(
				property, value,
				FString("layer.convolution_param.kernel_w"));
			FString kernel_h_str = GetPropertyValue(
				property, value,
				FString("layer.convolution_param.kernel_h"));
			if (kernel_w_str != "" && kernel_h_str != "") {
				kernel_size[0] = kernel_w_str.AsInt();
				kernel_size[1] = kernel_h_str.AsInt();
				layer->SetPropertyValue(FString("KernelShape"), FString(kernel_h_str) + FString(",") + FString(kernel_w_str));
			}
		}

		//Stride Size
		FString stride_str = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.stride,layer.stride"));
		if (stride_str != "") {
			layer->SetPropertyValue(FString("Strides"), FString(stride_str) + FString(",") + FString(stride_str));
		}else {
			FString stride_w_str = GetPropertyValue(
				property, value,
				FString("layer.convolution_param.stride_w"));
			FString stride_h_str = GetPropertyValue(
				property, value,
				FString("layer.convolution_param.stride_h"));
			if (stride_w_str != "" && stride_h_str != "") {
				layer->SetPropertyValue(FString("Strides"), FString(stride_h_str) + FString(",") + FString(stride_w_str));
			}
		}

		//Dilation
		FString dilation_str = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.dilation,layer.dilation"));
		if (dilation_str != "") {
			layer->SetPropertyValue(FString("Dilation"), FString(dilation_str) + FString(",") + FString(dilation_str));
		}

		//Bias
		FString bias_str = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.bias_term,layer.bias_term"));
		if (bias_str.ToLower() == "false") {
			layer->SetPropertyValue(FString("WithBias"), FString("False"));
		}else {
			layer->SetPropertyValue(FString("WithBias"), FString("True"));//Default=true
		}

		//Border Mode
		int pad_size[2] = {0, 0};
		FString padding_str = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.pad,layer.pad"));
		if (padding_str != "") {
			pad_size[0] = pad_size[1] = padding_str.AsInt();
		}else {
			FString padding_w_str = GetPropertyValue(property, value, FString("layer.convolution_param.pad_w,layer.pad_w"));
			FString padding_h_str = GetPropertyValue(property, value, FString("layer.convolution_param.pad_h,layer.pad_h"));
			if (padding_w_str != "" && padding_h_str != "") {
				pad_size[0] = padding_w_str.AsInt();
				pad_size[1] = padding_h_str.AsInt();
			}
		}
		if (pad_size[0] == 0 && pad_size[1] == 0) {
			layer->SetPropertyValue(FString("BorderMode"), FString("valid"));
		}else if (pad_size[0] * 2 + 1 == kernel_size[0] && pad_size[1] * 2 + 1 == kernel_size[1]) {
			layer->SetPropertyValue(FString("BorderMode"), FString("same"));
		}else if (pad_size[0] + 1 == kernel_size[0] && pad_size[1] + 1 == kernel_size[1]) {
			layer->SetPropertyValue(FString("BorderMode"), FString("full"));
		}else {
			layer->SetPropertyValue(FString("Padding"), FString(pad_size[1]) + FString(",") + FString(pad_size[0]));
		}

		//Learning Rate Factor
		FString blobs_lr_str = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.blobs_lr,layer.blobs_lr,layer.param.lr_mult"));
		if (blobs_lr_str != "") {
			layer->SetPropertyValue(FString("W.LRateMultiplier"), FString(blobs_lr_str.AsFloat()));
		}
		FString blobs_lr_str_2 = GetPropertyValue(
			property, value,
			FString("layer.convolution_param.blobs_lr,layer.blobs_lr,layer.param.lr_mult"));
		if (blobs_lr_str != "") {
			layer->SetPropertyValue(FString("b.LRateMultiplier"), FString(blobs_lr_str_2.AsFloat()));
		}
	}else if (caffe_layer_type == "pool" || caffe_layer_type == "pooling") {
		FString pool_type = GetPropertyValue(
			property, value,
			FString("layer.pooling_param.pool,layer.pool")).ToLower();
		if (pool_type == "max") {
			int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("MaxPooling"));
			layer = &structure.AddLayer(component_index);
		}else {//if (pool_type == "ave") {
			int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("AveragePooling"));
			layer = &structure.AddLayer(component_index);
		}

		//Kernel Size
		FString kernel_str = GetPropertyValue(
			property, value,
			FString("layer.pooling_param.kernel_size,layer.kernelsize"));
		if (kernel_str != "") {
			layer->SetPropertyValue(FString("KernelShape"), FString(kernel_str) + FString(",") + FString(kernel_str));
		}else {
			FString kernel_w_str = GetPropertyValue(
				property, value,
				FString("layer.pooling_param.kernel_w"));
			FString kernel_h_str = GetPropertyValue(
				property, value,
				FString("layer.pooling_param.kernel_h"));
			if (kernel_w_str != "" && kernel_h_str != "") {
				layer->SetPropertyValue(FString("KernelShape"), FString(kernel_h_str) + FString(",") + FString(kernel_w_str));
			}
		}

		//Stride Size
		layer->SetPropertyValue(FString("Strides"), FString("1,1"));
		FString stride_str = GetPropertyValue(
			property, value,
			FString("layer.pooling_param.stride,layer.stride"));
		if (stride_str != "") {
			layer->SetPropertyValue(FString("Strides"), FString(stride_str) + FString(",") + FString(stride_str));
		}else {
			FString stride_w_str = GetPropertyValue(
				property, value,
				FString("layer.pooling_param.stride_w"));
			FString stride_h_str = GetPropertyValue(
				property, value,
				FString("layer.pooling_param.stride_h"));
			if (stride_w_str != "" && stride_h_str != "") {
				layer->SetPropertyValue(FString("Strides"), FString(stride_h_str) + FString(",") + FString(stride_w_str));
			}
		}

		//Padding Size
		layer->SetPropertyValue(FString("Padding"), FString("0,0"));
		FString padding_str = GetPropertyValue(
			property, value,
			FString("layer.pooling_param.pad,layer.pad"));
		if (padding_str != "") {
			layer->SetPropertyValue(FString("Padding"), FString(padding_str) + FString(",") + FString(padding_str));
		}else {
			FString padding_w_str = GetPropertyValue(
				property, value,
				FString("layer.pooling_param.pad_w"));
			FString padding_h_str = GetPropertyValue(
				property, value,
				FString("layer.pooling_param.pad_h"));
			if (padding_w_str != "" && padding_h_str != "") {
				layer->SetPropertyValue(FString("Padding"), FString(padding_h_str) + FString(",") + FString(padding_w_str));
			}
		}

		//Ignore border
		layer->SetPropertyValue(FString("IgnoreBorder"), FString("False"));

		//Global Pooling
		FString global_pooling = GetPropertyValue(
			property, value,
			FString("layer.pooling_param.global_pooling,layer.global_pooling")).ToLower();
		if (global_pooling == "true"){
			layer->SetPropertyValue(FString("KernelShape"), FString("Input[1],Input[2]"));
			layer->SetPropertyValue(FString("Strides"), FString("Input[1],Input[2]"));
		}

	}else if (caffe_layer_type == "upsample") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Unpooling"));
		layer = &structure.AddLayer(component_index);
	}else if (caffe_layer_type == "relu") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("ReLU"));
		layer = &structure.AddLayer(component_index);
	}else if (caffe_layer_type == "tanh") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Tanh"));
		layer = &structure.AddLayer(component_index);
	}else if (caffe_layer_type == "sigmoid") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Sigmoid"));
		layer = &structure.AddLayer(component_index);
	}else if (caffe_layer_type == "absval") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Abs"));
		layer = &structure.AddLayer(component_index);
	/*
	}else if (caffe_layer_type == "lrn") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("LocalResponseNormalization"));
		layer = &structure.AddLayer(component_index);

		//Size
		FString size_str = GetPropertyValue(
			property, value,
			FString("layer.lrn_param.local_size,layer.local_size"));
		if (size_str != "") {
			layer->SetPropertyValue(FString("Size"), size_str);
		}

		//Alpha
		FString alpha_str = GetPropertyValue(
			property, value,
			FString("layer.lrn_param.alpha,layer.alpha"));
		if (alpha_str != "") {
			layer->SetPropertyValue(FString("Alpha"), alpha_str);
		}

		//Beta
		FString beta_str = GetPropertyValue(
			property, value,
			FString("layer.lrn_param.beta,layer.beta"));
		if (beta_str != "") {
			layer->SetPropertyValue(FString("Beta"), beta_str);
		}
	*/
	}else if (caffe_layer_type == "bn" || caffe_layer_type == "batchnorm") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("BatchNormalization"));
		layer = &structure.AddLayer(component_index);
	}else if (caffe_layer_type == "flatten") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Reshape"));
		layer = &structure.AddLayer(component_index);
		layer->SetPropertyValue(FString("OutShape"), FString("Input[0]*Input[1]*Input[2]"));
	}else if (caffe_layer_type == "permute") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Transpose"));
		layer = &structure.AddLayer(component_index);
		FString order_str = GetPropertyValue(
			property, value,
			FString("layer.permute_param.order,layer.order"));
        FString order;
		while (order_str != ""){
			FString s = order_str.SplitBy(",");
			const int order_i = s.AsInt() - 1;
			if (order_i >= 0){
				if (order != ""){
					order += ",";
				}
				order += FString(order_i);
			}
        }
		layer->SetPropertyValue(FString("Axes"), order);
	}else if (caffe_layer_type == "concat") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Concatenate"));
		layer = &structure.AddLayer(component_index);
	}else if (caffe_layer_type == "eltwise") {
		FString operation_str = GetPropertyValue(
			property, value,
			FString("layer.eltwise_param.operation,layer.operation"));
		if (operation_str == "" || operation_str.ToLower() == "sum"){
			int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Add2"));
			layer = &structure.AddLayer(component_index);
		}else if (operation_str.ToLower() == "prod"){
			int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Mul2"));
			layer = &structure.AddLayer(component_index);
		}else if (operation_str
		.ToLower() == "max"){
			int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Maximum2"));
			layer = &structure.AddLayer(component_index);
		}
	}else if (caffe_layer_type == "dropout") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Dropout"));
		layer = &structure.AddLayer(component_index);

		//P
		FString p_str = GetPropertyValue(
			property, value,
			FString("layer.dropout_param.dropout_ratio,layer.dropout_ratio"));
		if (p_str != "") {
			layer->SetPropertyValue(FString("P"), p_str);
		}
	}else if (caffe_layer_type == "accuracy") {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("TopNError"));
		layer = &structure.AddLayer(component_index);

		//k
		FString k_str = GetPropertyValue(
			property, value,
			FString("layer.accuracy_param.top_k,layer.top_k"));
		if (k_str != "") {
			layer->SetPropertyValue(FString("N"), k_str);
		}
	}

	//Unsupported layer
	if (!layer) {
		int component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Unsupported"));
		layer = &structure.AddLayer(component_index);
		layer->AddProperty(-1, FString("Type"), FString(""), true, false, false, false, NNLayerPropertyType::Error, caffe_layer_type, true);
		for (int ip = 0 ; ip < property.GetSize() ; ip++) {
			FString s = property.GetAt(ip);
			if (s.SubString(1, 1) == "*") {
				s = s.SubString(2, s.Length() - 1);
			}
			layer->AddProperty(-1, s, FString(""), true, false, false, false, NNLayerPropertyType::Error, value.GetAt(ip), true);
		}
	}

	//Set layer name
	//if (caffe_layer_type == "data") {
		FString layer_name = GetPropertyValue(
			property, value,
			FString("layer.name,name"));
		layer->SetPropertyValue(FString("Name"), layer_name.SplitBy(","));
	/*
	}else {
		layer->SetPropertyValue(FString("Name"), GetPropertyValue(property, value, FString("layer.name")));
	}
	*/

	//Link
	FString dest_names = GetPropertyValue(
		property, value,
		FString("layer.top,top"));
	while (dest_names != "") {
		FString dest_name = dest_names.SplitBy(",");

		link_buffer_name.Add(dest_name);
		link_buffer_id.Add(layer->GetLayerID());
	}

	FString parent_buffers = GetPropertyValue(
		property, value,
		FString("layer.bottom,bottom"));
	while (parent_buffers != "") {
		FString parent_buffer = parent_buffers.SplitBy(",");

		link_from_buffer.Add(parent_buffer);
		link_to_id.Add(layer->GetLayerID());
	}
}
//---------------------------------------------------------------------------
int CaffeProtoTxtToNNStructure_NNabla(const char *file_name, NNStructure &structure, FStringList &error_str)
{
	FStringList proto_txt;
	if (!proto_txt.LoadFromFile(file_name)) {
		error_str.Add(FString("Can't read from \"") + FString(file_name) + FString("\""));
		return false;
	}

	structure.Clear();

	FStringList nest_txt, property, value;
	FStringList link_from_buffer;
	FListT<int> link_to_id;
	FStringList link_buffer_name;
	FListT<int> link_buffer_id;
	//int line_shift = 0;
	FString last;
	for (int il = 0 ; il < proto_txt.GetSize() ; il++) {
		FString s = proto_txt.GetAt(il);
		s = s.ReplaceStr("\t", " ");
		while (s != "" && !s.Pos("#")){
			FString s1 = s.SplitBy(" ");
			if (s1 != "") {
				if (s1 == "}") {
					if (nest_txt.GetSize()) {
						nest_txt.Delete(nest_txt.GetSize() - 1);
					}else {
						//Error
						error_str.Add(FString("Unexpected '}' at line ") + FString(il + 1));
						break;
					}
					if (!nest_txt.GetSize()) {
						//Create layer from property and value
						CreateLayerFromPropertyAndValue_NNabla(property, value, structure, error_str, link_from_buffer, link_to_id, link_buffer_name, link_buffer_id);
						property.Clear();
						value.Clear();
					}
				}else if (s1 == "{") {
					nest_txt.Add(last.ReplaceStr(":", ""));
				}else if (!s1.Pos(":") && last.Pos(":")){
					FString s2 = last.ReplaceStr(":", "");
					for (int in = nest_txt.GetSize() - 1 ; in >= 0 ; in--) {
						s2 = nest_txt.GetAt(in) + FString(".") + s2;
					}
					property.Add(s2);
					value.Add(s1);
				}
				last = s1;
            }
		}
	}

	//Link
	for (int il = 0 ; il < link_from_buffer.GetSize() ; il++) {
		FString parent_layer_buffer = link_from_buffer.GetAt(il);

		int parent_layer_id = -1;
		for (int ib = 0 ; ib < link_buffer_name.GetSize() ; ib++) {
			if (link_buffer_name.GetAt(ib) == parent_layer_buffer){
				parent_layer_id = link_buffer_id.GetAt(ib);
				break;
			}
		}

		const NNLayer *layer = structure.GetLayerByID(link_to_id.GetAt(il));
		if (layer && parent_layer_id >= 0) {
			while (true){
				FListT<int> dest_ids = structure.EnumLinkIDBySourceLayerID(parent_layer_id, 0);
				if (!dest_ids.GetSize()) {
					break;
				}else {
					const NNLink *link = structure.GetLinkByID(dest_ids.GetAt(0));
					if (link) {
						const NNLayer *parent_layer = structure.GetLayerByID(link->GetDestLayerID());
						if (!parent_layer) {
							break;
						}else if (parent_layer->GetType() == "Unsupported" || parent_layer->GetType() == "ReLU" || parent_layer->GetType() == "Dropout" || parent_layer->GetType() == "BatchNormalization") {
							parent_layer_id = link->GetDestLayerID();
						}else {
							break;
						}
					}else {
						break;
					}
				}
			}

			const NNLayer *parent_layer = structure.GetLayerByID(parent_layer_id);
			if (layer->GetType() == "Convolution" &&
				//parent_layer->GetType() == "Unsupoorted" &&
				parent_layer->GetPropertyValue(FString("layer.type")).ToLower() == "padding") {
				FListT<int> link_ids = structure.EnumLinkIDByDestLayerID(parent_layer_id, 0);
				if (link_ids.GetSize() == 1) {
					const int pad = parent_layer->GetPropertyValue(FString("layer.pad")).AsInt();
					FString kernel_str = layer->GetPropertyValue(FString("KernelShape"));
					int kernel_size = 0;
					while (kernel_str != ""){
						kernel_size = kernel_str.SplitBy(",").AsInt();
					}

					if (pad * 2 + 1 == kernel_size) {
						structure.SetLayerPropertyValue(layer->GetLayerID(), layer->GetPropertyIndexByName("BorderMode"), FString("same"));
					}else if (pad + 1 == kernel_size) {
						structure.SetLayerPropertyValue(layer->GetLayerID(), layer->GetPropertyIndexByName("BorderMode"), FString("full"));
					}else {
						//Error (Unsupported padding size)
					}
					const NNLink *link = structure.GetLinkByID(link_ids.GetAt(0));
					const int parent_layer_id_bak = parent_layer_id;
					parent_layer_id = link->GetSourceLayerID();
					structure.ClearSelection();
					structure.SetLayerSelected(parent_layer_id_bak, true, false);
					structure.DeleteSelected();
				}
			}
			structure.AddLink(parent_layer_id, 0, layer->GetLayerID(), 0);
		}
	}

	//Clean up unnecessory layers
	for (int il = 0 ; il < structure.GetNumLayer() ; il++){
		const NNLayer &layer = structure.GetLayer(il);
		if (layer.GetType() == "Unsupported"){
			if (layer.GetPropertyValue(FString("Type")).ToLower() == "scale") {
                // if previous layer is batch_normalization, scale is already included.
				FListT<int> source_ids = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
				if (source_ids.GetSize() == 1){
					const NNLink *link = structure.GetLinkByID(source_ids.GetAt(0));
					assert(link);
					const NNLayer *source_layer = structure.GetLayerByID(link->GetSourceLayerID());
					if (source_layer){
						if (source_layer->GetType() == "BatchNormalization"){
							structure.RemoveLayer(layer.GetLayerID());
                            il--;
						}
					}
				}
			}
		}
	}

	structure.ClearSelection();
	structure.EscapeLayerName();
	structure.ArrangeLayers();
	structure.RefreshDisplayValue();
	structure.RefreshStatistics();

	return error_str.GetSize() == 0;
}
//---------------------------------------------------------------------------
int NNStructureToCaffeProtoTxt_NNabla(const NNStructure &structure_in, const char *file_name, FStringList &error_str)
{
	NNStructure structure;
	structure = structure_in;
	structure.ArrangeLayers(true);

	FStringList prototxt;
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		FStringList sl;
		int success = true;

		const NNLayer &layer = structure.GetLayer(il);
		sl.Add(FString("layer {"));

		//Name
		sl.Add(FString("  name: \"") + layer.GetPropertyValue(FString("Name")) + FString("\""));

		//Layer
		FListT<int> link_to = structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
		const NNLayer *next_layer = NULL;
		if (link_to.GetSize() == 1) {
			const NNLink *link = structure.GetLinkByID(link_to.GetAt(0));
			if (link) {
				next_layer = structure.GetLayerByID(link->GetDestLayerID());
			}
		}

		if (layer.GetType() == "Input") {
			sl.Add(FString("  type: \"Data\""));

		}else if (layer.GetType() == "Affine") {
			sl.Add(FString("  type: \"InnerProduct\""));
			sl.Add(FString("  inner_product_param {"));
			sl.Add(FString("    num_output: ") + FString((int)layer.GetPropertyByName("OutShape")->GetIntArrayProduct()));
			if (layer.GetPropertyValue(FString("WithBias")) != "True") {
				sl.Add(FString("    bias_term: false"));
			}
			sl.Add(FString("  }"));//inner_product_param

		}else if (layer.GetType() == "Convolution" || layer.GetType() == "DepthwiseConvolution") {
			sl.Add(FString("  type: \"Convolution\""));
			sl.Add(FString("  convolution_param {"));
			sl.Add(FString("    num_output: ") + FString((int)layer.GetPropertyByName("OutMaps")->GetIntArrayProduct()));
			const int group =
				layer.GetType() == "DepthwiseConvolution" ? (int)layer.GetPropertyByName("OutMaps")->GetIntArrayProduct() : (int)layer.GetPropertyByName("Group")->GetIntArrayProduct();
			if (group > 1){
				sl.Add(FString("    group: ") + FString(group));
			}

			int kernel_w = 1, kernel_h = 1;
			const NNLayerProperty *kernel_shape = layer.GetPropertyByName("KernelShape");
			if (kernel_shape) {
				if (kernel_shape->GetIntArray().GetSize()) {
					kernel_w = kernel_shape->GetIntArray().GetAt(0);
				}
				if (kernel_shape->GetIntArray().GetSize() > 1) {
					kernel_h = kernel_shape->GetIntArray().GetAt(1);
				}
			}
			if (kernel_w == kernel_h) {
				sl.Add(FString("    kernel_size: ") + FString(kernel_w));
			}else {
				sl.Add(FString("    kernel_w: ") + FString(kernel_w));
				sl.Add(FString("    kernel_h: ") + FString(kernel_h));
			}

			int stride_w = 1, stride_h = 1;
			const NNLayerProperty *stride = layer.GetPropertyByName("Strides");
			if (stride) {
				if (stride->GetIntArray().GetSize()) {
					stride_w = stride->GetIntArray().GetAt(0);
				}
				if (stride->GetIntArray().GetSize() > 1) {
					stride_h = stride->GetIntArray().GetAt(1);
				}
			}
			if (stride_w == stride_h) {
				if (stride_w != 1) {
					sl.Add(FString("    stride: ") + FString(stride_w));
				}
			}else {
				sl.Add(FString("    stride_w: ") + FString(stride_w));
				sl.Add(FString("    stride_h: ") + FString(stride_h));
			}

			const NNLayerProperty *dilation = layer.GetPropertyByName("Dilation");
			if (dilation) {
				int v = dilation->GetIntArray().GetAt(0);
				for (int id = 1; id < dilation->GetIntArray().GetSize() ; id++) {
					if (v != dilation->GetIntArray().GetAt(id)) {
						v = -1;
					}
				}
				if (v < 0) {
					error_str.Add(FString("Aeolotropic dilation is not supported."));
				}else {
					sl.Add(FString("    dilation: ") + FString(v));
				}
			}

			FString padding = layer.GetPropertyValue(FString("Padding"));
			if (padding == "*ConvolutionPaddingSize") {
				FString border_mode = layer.GetPropertyValue(FString("BorderMode"));
				if (border_mode == "same") {
					if (kernel_w == kernel_h) {
						if (kernel_w != 1) {
							sl.Add(FString("    pad: ") + FString((kernel_w - 1) / 2));
						}
					}else {
						sl.Add(FString("    pad_w: ") + FString((kernel_w - 1) / 2));
						sl.Add(FString("    pad_h: ") + FString((kernel_h - 1) / 2));
					}
				}else if (border_mode == "full") {
					if (kernel_w == kernel_h) {
						if (kernel_w != 1) {
							sl.Add(FString("    pad: ") + FString(kernel_w - 1));
						}
					}else {
						sl.Add(FString("    pad_w: ") + FString(kernel_w - 1));
						sl.Add(FString("    pad_h: ") + FString(kernel_h - 1));
					}
				}
			}else {
				int pad_w = 1, pad_h = 1;
				const NNLayerProperty *pad = layer.GetPropertyByName("Padding");
				if (pad) {
					if (pad->GetIntArray().GetSize()) {
						pad_w = pad->GetIntArray().GetAt(0);
					}
					if (pad->GetIntArray().GetSize() > 1) {
						pad_h = pad->GetIntArray().GetAt(1);
					}
				}
				if (pad_w == pad_h) {
					if (pad_w != 0) {
						sl.Add(FString("    pad: ") + FString(pad_w));
					}
				}else {
					sl.Add(FString("    pad_w: ") + FString(pad_w));
					sl.Add(FString("    pad_h: ") + FString(pad_h));
				}
			}

			if (layer.GetPropertyValue(FString("WithBias")) != "True") {
				sl.Add(FString("    bias_term: false"));
			}
			sl.Add(FString("  }"));//convolution_param

		}else if (layer.GetType() == "MaxPooling" || layer.GetType() == "SumPooling" || layer.GetType() == "AveragePooling") {
			sl.Add(FString("  type: \"Pooling\""));
			sl.Add(FString("  pooling_param {"));
			if (layer.GetType() == "MaxPooling") {
				sl.Add(FString("    pool: MAX"));
			}else if (layer.GetType() == "SumPooling" || layer.GetType() == "AveragePooling") {
				sl.Add(FString("    pool: AVE"));
			}

			//Pool shape
			int pool_w = 1, pool_h = 1;
			const NNLayerProperty *pool_shape = layer.GetPropertyByName("KernelShape");
			if (pool_shape) {
				if (pool_shape->GetIntArray().GetSize()) {
					pool_w = pool_shape->GetIntArray().GetAt(0);
				}
				if (pool_shape->GetIntArray().GetSize() > 1) {
					pool_h = pool_shape->GetIntArray().GetAt(1);
				}
			}
			if (pool_w == pool_h) {
				sl.Add(FString("    kernel_size: ") + FString(pool_w));
			}else {
				sl.Add(FString("    kernel_w: ") + FString(pool_w));
				sl.Add(FString("    kernel_h: ") + FString(pool_h));
			}

			//Stride
			int stride_w = 1, stride_h = 1;
			const NNLayerProperty *stride = layer.GetPropertyByName("Strides");
			if (stride) {
				if (stride->GetIntArray().GetSize()) {
					stride_w = stride->GetIntArray().GetAt(0);
				}
				if (stride->GetIntArray().GetSize() > 1) {
					stride_h = stride->GetIntArray().GetAt(1);
				}
			}
			if (stride_w == stride_h) {
				if (stride_w != 1) {
					sl.Add(FString("    stride: ") + FString(stride_w));
				}
			}else {
				sl.Add(FString("    stride_w: ") + FString(stride_w));
				sl.Add(FString("    stride_h: ") + FString(stride_h));
			}

			//Padding
			int padding_w = 1, padding_h = 1;
			const NNLayerProperty *padding = layer.GetPropertyByName("Padding");
			if (padding) {
				if (padding->GetIntArray().GetSize()) {
					padding_w = padding->GetIntArray().GetAt(0);
				}
				if (padding->GetIntArray().GetSize() > 1) {
					padding_h = padding->GetIntArray().GetAt(1);
				}
			}
			if (padding_w == padding_h) {
				if (padding_w != 0) {
					sl.Add(FString("    pad: ") + FString(padding_w));
				}
			}else {
				sl.Add(FString("    pad_w: ") + FString(padding_w));
				sl.Add(FString("    pad_h: ") + FString(padding_h));
			}

			sl.Add(FString("  }"));//pooling_param

		}else if (layer.GetType() == "Unpooling" || layer.GetType() == "Upsampling") {
			sl.Add(FString("  type: \"Upsample\""));
			sl.Add(FString("  upsample_param {"));

			//Output size
			int output_w = 32, output_h = 32;
			const NNLayerProperty *output = layer.GetPropertyByName("Output");
			if (output) {
				if (output->GetIntArray().GetSize()) {
					output_w = output->GetIntArray().GetAt(0);
				}
				if (output->GetIntArray().GetSize() > 1) {
					output_h = output->GetIntArray().GetAt(1);
				}
			}
			if (output_w == output_h) {
				sl.Add(FString("    upsample: ") + FString(output_w));
			}else {
				sl.Add(FString("    upsample_w: ") + FString(output_w));
				sl.Add(FString("    upsample_h: ") + FString(output_h));
			}

			sl.Add(FString("  }"));//upsample param

		}else if (layer.GetType() == "Tanh") {
			sl.Add(FString("  type: \"TanH\""));

		}else if (layer.GetType() == "Sigmoid") {
			int sigmoid_loss = false;
			if (next_layer) {
				if (next_layer->GetType() == "BinaryCrossentropy") {
					sl.Add(FString("  type: \"SigmoidCrossEntropyLoss\""));
					sigmoid_loss = true;
				}

			}
			if (!sigmoid_loss) {
				sl.Add(FString("  type: \"Sigmoid\""));
			}

		}else if (layer.GetType() == "ReLU") {
			sl.Add(FString("  type: \"ReLU\""));

		}else if (layer.GetType() == "Abs") {
			sl.Add(FString("  type: \"AbsVal\""));

		}else if (layer.GetType() == "Softmax") {
			int softmax_loss = false;
			if (next_layer) {
				if (next_layer->GetType() == "CategoricalCrossEntropy") {
					sl.Add(FString("  type: \"SoftmaxWithLoss\""));
					softmax_loss = true;
				}

			}
			if (!softmax_loss) {
				sl.Add(FString("  type: \"Softmax\""));
			}

		/*
		}else if (layer.GetType() == "LocalResponseNormalization") {
			sl.Add(FString("  type: \"LRN\""));
			sl.Add(FString("  lrn_param {"));
			sl.Add(FString("    local_size: ") + layer.GetPropertyValue(FString("Size")));
			sl.Add(FString("    alpha: ") + layer.GetPropertyValue(FString("Alpha")));
			sl.Add(FString("    beta: ") + layer.GetPropertyValue(FString("Beta")));
			sl.Add(FString("  }"));//lrn_param
        */
		}else if (layer.GetType() == "BatchNormalization") {
			sl.Add(FString("  type: \"BN\""));

		}else if (layer.GetType() == "Reshape") {
			const NNLayerProperty *out_shape = layer.GetPropertyByName("OutShape");
			if (out_shape){
				if (out_shape->GetIntArray().GetSize() == 1){
					sl.Add(FString("  type: \"Flatten\""));
					sl.Add(FString("  flatten_param {"));
					sl.Add(FString("    axis: 1"));
					sl.Add(FString("  }"));//flatten_param
				}else {
					//Todo:add reshape
					success = false;
				}
			}

		}else if (layer.GetType() == "Transpose") {
			const NNLayerProperty *out_shape = layer.GetPropertyByName("Axes");
			sl.Add(FString("  type: \"Permute\""));
			sl.Add(FString("  permute_param {"));
			sl.Add(FString("    order: 0"));
			if (out_shape){
				for (int i = 0 ; i < out_shape->GetIntArray().GetSize() ; i++){
					sl.Add(FString("    order: ") + FString((int)out_shape->GetIntArray().GetAt(i) + 1));
				}
			}
			sl.Add(FString("  }"));//permute_param

		}else if (layer.GetType() == "Concatenate") {
			sl.Add(FString("  type: \"Concat\""));
			sl.Add(FString("  concat_param {"));
			sl.Add(FString("    axis: 1"));
			sl.Add(FString("  }"));//concat_param

		}else if (layer.GetType() == "Add2") {
			sl.Add(FString("  type: \"eltwise\""));
			sl.Add(FString("  eltwise_param {"));
			sl.Add(FString("    operation: SUM"));
			sl.Add(FString("  }"));//eltwise_param

		}else if (layer.GetType() == "Mul2") {
			sl.Add(FString("  type: \"eltwise\""));
			sl.Add(FString("  eltwise_param {"));
			sl.Add(FString("    operation: PROD"));
			sl.Add(FString("  }"));//eltwise_param

		}else if (layer.GetType() == "Maximum2") {
			sl.Add(FString("  type: \"eltwise\""));
			sl.Add(FString("  eltwise_param {"));
			sl.Add(FString("    operation: MAX"));
			sl.Add(FString("  }"));//eltwise_param

		}else if (layer.GetType() == "Dropout") {
			sl.Add(FString("  type: \"Dropout\""));
			sl.Add(FString("  dropout_param {"));
			sl.Add(FString("    dropout_ratio:") + layer.GetPropertyValue(FString("P")));
			sl.Add(FString("  }"));//concat_param

		}else if (layer.GetType() == "SquaredError") {
			sl.Add(FString("  type: \"EuclideanLoss\""));

		}else if (layer.GetType() == "SigmoidCrossEntropy") {
			sl.Add(FString("  type: \"SigmoidCrossEntropyLoss\""));

		}else if (layer.GetType() == "SoftmaxCrossEntropy") {
			sl.Add(FString("  type: \"SoftmaxWithLoss\""));

		}else {
			error_str.Add(FString("Layer type \"") + layer.GetType() + FString("\" is not supported."));
			success = false;
		}


		//Link
		FListT<int> link_from = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
		for (int ili = 0 ; ili < link_from.GetSize() ; ili++) {
			const NNLink *link = structure.GetLinkByID(link_from.GetAt(ili));
			if (link) {
				const NNLayer *source_layer = structure.GetLayerByID(link->GetSourceLayerID());
				if (source_layer) {
					sl.Add(FString("  bottom: \"") + source_layer->GetPropertyValue(FString("Name")) + FString("\""));
				}
			}
		}
		sl.Add(FString("  top: \"") + layer.GetPropertyValue(FString("Name")) + FString("\""));

		sl.Add(FString("}"));

		if (success) {
            prototxt.Add(sl);
		}
	}

	if (!prototxt.SaveToFile(file_name)){
		error_str.Add(FString("Can't write to \"") + FString(file_name) + FString("\""));
	}

	return error_str.GetSize() == 0;
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
