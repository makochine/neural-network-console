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

#include <stdlib.h>
#include <math.h>
#include "math_util.h"

#include "nn_structure.h"

namespace sdeep_console{
double exp2(double x)
{
	return 0.0;
}
//---------------------------------------------------------------------------
NNLayerProperty &NNLayerProperty::operator=(const NNLayerProperty &source)
{
	name_ = source.name_;
	short_name_ = source.short_name_;
	editable_ = source.editable_;
	important_ = source.important_;
	required_ = source.required_;
	search_parameter_ = source.search_parameter_;
	type_ = source.type_;
	value_ = source.value_;
	default_value_ = source.default_value_;
	visible_ = source.visible_;
	auto_export_ = source.auto_export_;
	dynamic_ = source.dynamic_;

	display_value_ = source.display_value_;
	error_ = source.error_;
	update_requested_ = source.update_requested_;
	error_message_ = source.error_message_;
	int_array_ = source.int_array_;
	float_array_ = source.float_array_;

	return *this;
}
//---------------------------------------------------------------------------
int NNLayerProperty::IsSame(const NNLayerProperty &source) const
{
	return
		name_ == source.name_ &&
		value_ == source.value_ &&
		display_value_ == source.display_value_;
}
//---------------------------------------------------------------------------
void NNLayerProperty::SetDisplayValue(const FString &display_value)
{
	error_ = false;
	error_message_ = "";
	update_requested_ = false;
	int_array_.Clear();
	float_array_.Clear();
	switch (type_) {
	case NNLayerPropertyType::Boolean:
	case NNLayerPropertyType::BooleanOrMacro:
		display_value_ = display_value;
		if (display_value == "True") {
			int_array_.Add(1);
		}else if (display_value == "False") {
			int_array_.Add(0);
		}else {
			error_ = true;
			error_message_ = "Invalid property value";
		}
		break;
	case NNLayerPropertyType::Int:
	case NNLayerPropertyType::IntArray:
	case NNLayerPropertyType::PInt:
	case NNLayerPropertyType::PIntArray:
	case NNLayerPropertyType::PIntArrays:
	case NNLayerPropertyType::UInt:
	case NNLayerPropertyType::UIntArray:
		{
			FString s = display_value;
			//s = s.ReplaceStr("(", "");
			//s = s.ReplaceStr(")", "");
			int success = true;
			while (s != ""){
				FString s1 = s.SplitBy(',');
				if (s1.PChar()){
					if (s1.PChar()[0] == '(') {
						if (int_array_.GetSize()) {
							int_array_.Insert(0, -1);//Separator
						}
						s1 = s1.SubString(2, s1.Length() - 1);
					}
					if (s1.PChar()) if (s1.PChar()[0] != ')'){
						__int64 v = s1.AsFloat();
						if (((type_ == NNLayerPropertyType::UInt || type_ == NNLayerPropertyType::UIntArray) && v >= 0) ||
							v > 0 ||
							type_ == NNLayerPropertyType::Int || type_ == NNLayerPropertyType::IntArray) {
							int_array_.Insert(0, v);
						}else {
							success = false;
						}
					}
				}else {
                    success = false;
				}
			}

			if (success &&
				(int_array_.GetSize() > 0 || !required_) &&
				(type_ == NNLayerPropertyType::IntArray || type_ == NNLayerPropertyType::PIntArray ||
				 type_ == NNLayerPropertyType::PIntArrays || type_ == NNLayerPropertyType::UIntArray ||
				 int_array_.GetSize() == 1)) {
				display_value_ = display_value;
			}else {
				display_value_ = "";
				error_ = true;
				error_message_ = "Invalid property value";
			}
		}
		break;
	case NNLayerPropertyType::Float:
	case NNLayerPropertyType::FloatArray:
	case NNLayerPropertyType::FloatArrays:
		{
			FString s = display_value;
			s = s.ReplaceStr("(", "");
			s = s.ReplaceStr(")", "");
			while (s != ""){
				double v = s.SplitBy(',').AsFloat();
				float_array_.Insert(0, v);
			}

			if ((float_array_.GetSize() > 0 || !required_) &&
				(type_ == NNLayerPropertyType::FloatArray || type_ == NNLayerPropertyType::FloatArrays || float_array_.GetSize() == 1)) {
				display_value_ = display_value;
			}else {
				display_value_ = "";
				error_ = true;
				error_message_ = "Invalid property value";
			}
		}
		break;
	case NNLayerPropertyType::Option:
	case NNLayerPropertyType::Text:
	case NNLayerPropertyType::Model:
	case NNLayerPropertyType::Dataset:
	case NNLayerPropertyType::Error:
	default:
		display_value_ = display_value;
		if (required_ && display_value_ == "") {
			error_ = true;
			error_message_ = "Empry property";
		}else if (type_ == NNLayerPropertyType::Error) {
			error_ = true;
			error_message_ = "Unsupported";
		}
		break;
	}
}
//---------------------------------------------------------------------------
void NNLayerProperty::ResetDisplayValue()
{
	if (type_ >= NNLayerPropertyType::Option &&
		value_.SubString(1, 1) != "*") {

		SetDisplayValue(value_);
	}else {
		display_value_ = "";
		int_array_.Clear();
		if (required_) {
			error_ = true;
			error_message_ = "Empty property";
		}else {
			error_ = false;
			error_message_ = "";
		}
		update_requested_ = true;
	}
}
//---------------------------------------------------------------------------
__int64 NNLayerProperty::GetIntArrayProduct() const
{
	if (!int_array_.GetSize()) {
		return 0;
	}

	if (type_ == NNLayerPropertyType::Boolean) {
		return int_array_.GetAt(0);
	}

	__int64 result = 1, sum_result = 0;
	for (int id = 0 ; id < int_array_.GetSize() ; id++) {
		if (int_array_.GetAt(id) > 0) {
			result *= int_array_.GetAt(id);
		}else {
			sum_result = result;
			result = 1;
		}
	}
	return result + sum_result;
}
//---------------------------------------------------------------------------
NNLayerPropertyType NNLayerProperty::DecodePropertyType(const FString &type)
{
	if (type == "Boolean"){
		return NNLayerPropertyType::Boolean;
	}else if (type == "BooleanOrMacro"){
		return NNLayerPropertyType::BooleanOrMacro;
	}else if (type == "Int"){
		return NNLayerPropertyType::Int;
	}else if (type == "IntArray"){
		return NNLayerPropertyType::IntArray;
	}else if (type == "PInt"){
		return NNLayerPropertyType::PInt;
	}else if (type == "PIntArray"){
		return NNLayerPropertyType::PIntArray;
	}else if (type == "PIntArrays"){
		return NNLayerPropertyType::PIntArrays;
	}else if (type == "UInt"){
		return NNLayerPropertyType::UInt;
	}else if (type == "UIntArray"){
		return NNLayerPropertyType::UIntArray;
	}else if (type == "Float"){
		return NNLayerPropertyType::Float;
	}else if (type == "FloatArray"){
		return NNLayerPropertyType::FloatArray;
	}else if (type == "FloatArrays"){
		return NNLayerPropertyType::FloatArrays;
	}else if (type == "Option"){
		return NNLayerPropertyType::Option;
	}else if (type == "Dataset"){
		return NNLayerPropertyType::Dataset;
	}else if (type == "Text"){
		return NNLayerPropertyType::Text;
	}else if (type == "TextArray"){
		return NNLayerPropertyType::TextArray;
	}else if (type == "File"){
		return NNLayerPropertyType::File;
	}else if (type == "Model"){
		return NNLayerPropertyType::Model;
	}else if (type == "Any"){
		return NNLayerPropertyType::Any;
	}

	//else if (type == "Error"){
	return NNLayerPropertyType::Error;
}
//---------------------------------------------------------------------------
FString NNLayerProperty::EncodePropertyType(NNLayerPropertyType layer_property_type)
{
	switch (layer_property_type){
	case NNLayerPropertyType::Boolean:
		return FString("Boolean");
	case NNLayerPropertyType::BooleanOrMacro:
		return FString("BooleanOrMacro");
	case NNLayerPropertyType::Int:
		return FString("Int");
	case NNLayerPropertyType::IntArray:
		return FString("IntArray");
	case NNLayerPropertyType::PInt:			//Positive Int (not include 0)
		return FString("PInt");
	case NNLayerPropertyType::PIntArray:
		return FString("PIntArray");
	case NNLayerPropertyType::PIntArrays:
		return FString("PIntArrays");
	case NNLayerPropertyType::UInt:           //Unsigned Int (include 0)
		return FString("UInt");
	case NNLayerPropertyType::UIntArray:
		return FString("UIntArray");
	case NNLayerPropertyType::Float:
		return FString("Float");
	case NNLayerPropertyType::FloatArray:
		return FString("FloatArray");
	case NNLayerPropertyType::FloatArrays:
		return FString("FloatArrays");
	case NNLayerPropertyType::Option:
		return FString("Option");
	case NNLayerPropertyType::Dataset:
		return FString("Dataset");
	case NNLayerPropertyType::Text:
		return FString("Text");
	case NNLayerPropertyType::TextArray:
		return FString("TextArray");
	case NNLayerPropertyType::File:
		return FString("File");
	case NNLayerPropertyType::Model:
		return FString("Model");
	case NNLayerPropertyType::Any:
		return FString("Any");
	case NNLayerPropertyType::Error:
		return FString("Error");
	default:
		return FString("Unknown");
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
NNLayerSideConnector &NNLayerSideConnector::operator=(const NNLayerSideConnector &source)
{
	name_ = source.name_;
	short_name_ = source.short_name_;
	color_ = source.color_;
	enable_ = source.enable_;
	kind_ = source.kind_;
	shape_ = source.shape_;
	parameter_scope_ = source.parameter_scope_;
	dynamic_ = source.dynamic_;

	order_ = source.order_;
	current_shape_ = source.current_shape_;

	return *this;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
int NNWeight::Load(const char *file_name)
{
	Clear();

	FStringList sl;
	sl.LoadFromFile(file_name);
	FString shape_str = sl.GetSize() ? sl.GetAt(0).SubString(2, sl.GetAt(0).Length() - 2) : FString("");
	int shape_product = 1;
	while (shape_str != ""){
		FString shape_1 = shape_str.SplitBy(',');
		if (shape_1 != "") {
			const int size = shape_1.AsInt();
			shape_.Add(size);
			shape_product *= size;
		}
	}
	for (int i = 1 ; i < sl.GetSize() ; i++) {
		weight_.Add(sl.GetAt(i).AsFloat());
	}

	if (weight_.GetSize() != shape_product) {
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
int NNWeight::Save(const char *file_name) const
{
	FStringList sl;

	//Shape
	{
		FString shape_str = FString("(");
		for (int id = 0; id < shape_.GetSize() ; id++) {
			shape_str += FString((int)shape_.GetAt(id)) + FString("L");
			if (shape_.GetSize() == 1 || id < shape_.GetSize() - 1) {
				shape_str += ", ";
			}
		}
		sl.Add(shape_str + FString(")"));
	}

	for (int i = 0 ; i < weight_.GetSize(); i++) {
		sl.Add(FString(weight_.GetAt(i)));
	}


	return sl.SaveToFile(file_name);
}
//---------------------------------------------------------------------------
void NNWeight::RemoveSingleSizeDim()
{
	for (int i = shape_.GetSize() - 1 ; i >= 0 ; i--) {
		if (shape_.GetSize() > 1 && shape_.GetAt(i) == 1) {
			shape_.Delete(i);
		}
	}
}
//---------------------------------------------------------------------------
void NNWeight::TransposeRight()
{
	if (shape_.GetSize() < 2) {
		return;
	}
	const int step = shape_.GetAt(shape_.GetSize() - 1);
	shape_.Delete(shape_.GetSize() - 1);
	shape_.Insert(0, step);
	FListT<float> weight_bak = weight_;
	weight_.Clear();
	const int size = weight_bak.GetSize() / step;
	for (int i = 0 ; i < step ; i++) {
		for (int i2 = 0 ; i2 < size; i2++) {
			weight_.Add(weight_bak.GetAt(i + i2 * step));
		}
	}
}
//---------------------------------------------------------------------------
void NNWeight::CalcContributionRatio()
{
	contribution_ratio_.Clear();
	const int dim = shape_.GetAt(0);
	const int num_data = weight_.GetSize() / shape_.GetAt(0);
	if (num_data == 1 || dim > 4096){
		//Cannot calculate contribution ratio, or too large dim
		return;
	}

	float *cov_matrix = new float[dim * dim];
	MathUtil<float>::CovMatrix(weight_.GetRawBuffer(), dim, num_data, cov_matrix);

	float *eigen = new float[dim];
	MathUtil<float>::Eigen(cov_matrix, dim, 1000, 1e-5, eigen);

    float eigen_sum = 0.0f;
	for (int i = 0; i < dim ; i++) {
		eigen_sum += eigen[i];
	}
	FListT<float> tmp;
	if (eigen_sum > 0.0f) {
		for (int i = 0; i < dim ; i++) {
			tmp.Add(eigen[i] / eigen_sum);
		}
		tmp.Sort();
		for (int i = 0; i < dim ; i++) {
			contribution_ratio_.Add(tmp.GetAt(dim - 1 - i));
		}
	}

	delete[] eigen;
	delete[] cov_matrix;
}

//---------------------------------------------------------------------------
void NNWeight::InverseKernel(int kernel_size)
{
	if (kernel_size == 0){
		if (shape_.GetSize() >= 2){
			kernel_size = shape_.GetAt(shape_.GetSize() - 2) * shape_.GetAt(shape_.GetSize() - 1);
		}
	}
	if (kernel_size <= 0){
		//Error
		return;
	}
	const int data_size = weight_.GetSize() / kernel_size;

	FListT<float> data;
	data = weight_;
	weight_.Clear();
	for (int i = 0 ; i < data_size ; i++) {
		for (int id = 0 ; id < kernel_size ; id++) {
            weight_.Add(data.GetAt(i * kernel_size + kernel_size - 1 - id));
		}
	}
}
//---------------------------------------------------------------------------
void NNWeight::Sqrt()
{
	for (int i = 0 ; i < weight_.GetSize(); i++) {
		weight_.SetAt(i, sqrt(weight_.GetAt(i)));
	}
}
//---------------------------------------------------------------------------
void NNWeight::Sqr()
{
	for (int i = 0 ; i < weight_.GetSize(); i++) {
		weight_.SetAt(i, weight_.GetAt(i) * weight_.GetAt(i));
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
NNLayer::NNLayer() :
	layer_id_(0),
	selected_(false),
	dragged_(false),
	component_(NULL),

	inner_structure_args_(NULL),
	inner_structure_(NULL),

	is_param_property_index_(-1),
	input_property_index_(-1),
	output_property_index_(-1),
	in_place_property_index_(-1)
{
	memset(position_, 0, sizeof(position_));
	memset(current_position_, 0, sizeof(current_position_));
	memset(initial_offset_, 0, sizeof(initial_offset_));
}
//---------------------------------------------------------------------------
NNLayer &NNLayer::operator=(const NNLayer &source)
{
	layer_id_ = source.layer_id_;
	component_ = source.component_;
	properties_ = source.properties_;
	side_connectors_[0] = source.side_connectors_[0];
	side_connectors_[1] = source.side_connectors_[1];

	memcpy(position_, source.position_, sizeof(position_));

	selected_ = source.selected_;
	dragged_ = source.dragged_;
	memcpy(current_position_, source.current_position_, sizeof(current_position_));
	memcpy(initial_offset_, source.initial_offset_, sizeof(initial_offset_));
	input_pin_connected_ = source.input_pin_connected_;

	errors_ = source.errors_;

	repeat_ids_ = source.repeat_ids_;

	is_param_property_index_ = source.is_param_property_index_;
	input_property_index_ = source.input_property_index_;
	output_property_index_ = source.output_property_index_;
	in_place_property_index_ = source.in_place_property_index_;

	ClearCache();

	return *this;
}
//---------------------------------------------------------------------------
int NNLayer::IsSame(const NNLayer &source, const FStringList *exception_property) const
{
	if (properties_.GetSize() != source.properties_.GetSize()) {
        return false;
	}
	for (int ip = 0 ; ip < properties_.GetSize() ; ip++) {
        int skip = false;
		if (exception_property) {
			for (int ie = 0 ; ie < exception_property->GetSize() && !skip; ie++) {
				if (properties_.GetAt(ip).GetName().Pos(exception_property->GetAt(ie)) > 0) {
					skip = true;
				}
			}
		}
		if (!skip && !properties_.GetAt(ip).IsSame(source.properties_.GetAt(ip))){
			return false;
		}
	}
	return true;
}
//---------------------------------------------------------------------------
const NNLayerProperty *NNLayer::GetAvailableInputProperty() const
{
	if (GetInputPropertyIndex() < 0){
		return NULL;
	}
	const NNLayerProperty *property_input = &GetProperty(GetInputPropertyIndex());
	return property_input->GetIntArray().GetSize() ? property_input : NULL;
}
//---------------------------------------------------------------------------
int NNLayer::IsContainedInRect(const float *rect) const
{
	return
		position_[0] >= rect[0] &&
		position_[0] + position_[2] <= rect[2] &&
		position_[1] >= rect[1] &&
		position_[1] + position_[3] <= rect[3];
}
//---------------------------------------------------------------------------
int NNLayer::GetPinEnable(int output, int pin_index) const
{
	if (pin_index == 0) {
		return true;
	}

	if (GetNumSideConnector(output) <= pin_index - 1) {
		return false;
	}

	const NNLayerSideConnector &side_connector = GetSideConnector(output, pin_index - 1);

	if (side_connector.GetEnable() == "True") {
		return true;
	}else if (side_connector.GetEnable().SubString(1, 1) == "*") {
		return GetPropertyValue(side_connector.GetEnable().SubString(2, side_connector.GetEnable().Length() - 1)) == "True";
	}

	return false;
}
//---------------------------------------------------------------------------
int NNLayer::GetPinVisible(int output, int pin_index) const
{
	if (pin_index == 0) {
		return true;
	}

	if (GetNumSideConnector(output) <= pin_index - 1) {
		return false;
	}

	const NNLayerSideConnector &side_connector = GetSideConnector(output, pin_index - 1);
	if (side_connector.GetDynamic() && side_connector.GetKind() == NNLayerComponentInputKind::Parameter){
		// hide dynamic parameter side connector of Unit
		return false;
	}else if (side_connector.GetEnable() == "True") {
		return true;
	}else if (side_connector.GetEnable().SubString(1, 1) == "*") {
		return GetPropertyValue(side_connector.GetEnable().SubString(2, side_connector.GetEnable().Length() - 1)) == "True";
	}

	return false;
}
//---------------------------------------------------------------------------
void NNLayer::ResetInputPinConnected()
{
	input_pin_connected_.Clear();
	for (int i = 0; i < GetNumSideConnector(0) + 1 ; i++) {
		input_pin_connected_.Add(false);
	}
	for (int is = 0; is < GetNumSideConnector(0) ; is++) {
		side_connectors_[0].GetAt(is).SetCurrentShape(FString(""));
	}
}
//---------------------------------------------------------------------------
void NNLayer::SetInputPinConnected(int pin_index, int connected)
{
	assert(pin_index < input_pin_connected_.GetSize());
	input_pin_connected_.SetAt(pin_index, connected);
}
//---------------------------------------------------------------------------
int NNLayer::GetInputPinConnected(int pin_index) const
{
	return pin_index < input_pin_connected_.GetSize() ? input_pin_connected_.GetAt(pin_index) : false;
}
//---------------------------------------------------------------------------
NNLayerProperty *NNLayer::AddProperty(int pos, const FString &name, const FString &short_name, int editable, int important, int required, int search_parameter, NNLayerPropertyType type, const FString &value, int visible, int dynamic, int update_property_index)
{
	NNLayerProperty &property = pos < 0 ? properties_.Add() : properties_.Insert(pos);
	property.SetName(name);
	property.SetShortName(short_name);
	property.SetEditable(editable);
	property.SetImportant(important);
	property.SetRequired(required);
	property.SetSearchParameter(search_parameter);
	property.SetType(type);
	property.SetValue(value);
	property.SetDefaultValue(value);
	property.SetVisible(visible);
	property.SetAutoExport(name.PosChar('.') > 0);
	property.SetDynamic(dynamic);

	if (update_property_index){
		UpdatePropertyIndex();
	}
    return &property;
}
//---------------------------------------------------------------------------
void NNLayer::UpdatePropertyIndex()
{
	is_param_property_index_ = -1;
	input_property_index_ = -1;
	output_property_index_ = -1;
	in_place_property_index_ = -1;
	for (int ip = 0 ; ip < properties_.GetSize() ; ip++){
		NNLayerProperty &property = properties_.GetAt(ip);
		if (property.GetName() == "IsParam"){
			is_param_property_index_ = ip;
		}else if (property.GetName() == "Input"){
			input_property_index_ = ip;
		}else if (property.GetName() == "Output"){
			output_property_index_ = ip;
		}else if (property.GetName() == "InPlace"){
			in_place_property_index_ = ip;
		}
    }
}
//---------------------------------------------------------------------------
int NNLayer::SetPropertyValue(int property_index, const FString &value, int force)
{
	if (property_index < 0 || property_index >= GetNumProperty()) {
		return false;
	}

	NNLayerProperty &property = properties_.GetAt(property_index);
	if (!property.GetEditable() && !force) {
		return false;
	}

	property.SetValue(value);

	return true;
}
//---------------------------------------------------------------------------
int NNLayer::SetPropertyValue(const FString &property_name, const FString &value, int force)
{
	const int property_index = GetPropertyIndexByName(property_name);
	return SetPropertyValue(property_index, value, force);
}
//---------------------------------------------------------------------------
int NNLayer::SetPropertyValueFrom(const NNLayer &source)
{
	for (int ip = 0 ; ip < properties_.GetSize() ; ip++) {
		NNLayerProperty &property = properties_.GetAt(ip);
		if (property.GetEditable()) {
			const NNLayerProperty *source_property = source.GetPropertyByName(property.GetName());
			if (source_property) {
				property.SetValue(source_property->GetValue());
			}
		}
	}
	return true;
}
//---------------------------------------------------------------------------
FString NNLayer::GetPropertyValue(const FString &name) const
{
	const NNLayerProperty *property = GetPropertyByName(name);
	FString result;
	if (property) {
		result = property->GetValue();
	}

	return result;
}
//---------------------------------------------------------------------------
FString NNLayer::GetDisplayPropertyValue(const FString &name) const
{
	const NNLayerProperty *property = GetPropertyByName(name);
	FString result;
	if (property) {
		result = property->GetDisplayValue();
	}

	return result;
}
//---------------------------------------------------------------------------
void NNLayer::ResetPropertyDisplayValue(int *update_required_property_num)
{
	*update_required_property_num = 0;
	for (int ip = 0 ; ip < GetNumProperty() ; ip++) {
		NNLayerProperty &property = properties_.GetAt(ip);
		property.ResetDisplayValue();

		if (property.GetUpdateRequested()) {
			(*update_required_property_num)++;
		}
	}
	*update_required_property_num += GetNumSideConnector(0);
}
//---------------------------------------------------------------------------
void NNLayer::UpdatePropertyLocalDisplayValue(const NNStructure *current_network, const NNStructures *related_networks, int *update_required_property_num)
{
	*update_required_property_num = 0;
	for (int ip = 0 ; ip < GetNumProperty() ; ip++) {
		NNLayerProperty &property = properties_.GetAt(ip);

		if (property.GetUpdateRequested()) {
			const FString &macro = property.GetValue();
			FString value, error;
			if (EvalPropertyMacro(property.GetType(), current_network, related_networks, &property, macro, value, error)) {
				property.SetDisplayValue(value);
			}else if (error != "") {
				property.SetError(true, error);
			}
			(*update_required_property_num) += property.GetUpdateRequested();
		}
	}
	for (int is = 0 ; is < GetNumSideConnector(0) ; is++) {
        (*update_required_property_num) += GetSideConnector(0, is).GetCurrentShape() == "";
    }
}
//---------------------------------------------------------------------------
double StrToScalar(FString s)
{
	if (s.SubString(1, 1) == "("){
        double result = 0.0;
		while (s != ""){
			s.SplitBy('(');
			result += StrToScalar(s.SplitBy(')'));
		}
		return result;
	}

	if (s == "True"){
		return 1.0;
	}else if (s == "False"){
		return 0.0;
	}

	double result = 1.0;
	while (s != ""){
		FString s1 = s.SplitBy(',');
		result *= s1.AsFloat();
	}
	return result;
}
//---------------------------------------------------------------------------
double NNLayer::EvalMath(NNLayerPropertyType type, const NNStructure *current_network, const NNStructures *related_networks, const FString &_math, int &success, FString &error) const
{
	if (_math == "" || !success){
		return 0.0;
	}
	FString math = _math;

	static const int num_ops = 4;
	static const char ops[num_ops] = {
	'/',
	'*',
	'-',
	'+',
	};

	static const int ops_rank[num_ops] = {
	0,
	0,
	1,
	2,
	};

	//static const FString float_char = FString("0123456789-+e.");

	int len = math.Length();
	int brackets_whole = true;
	int brackets = 0;
	int is_float = true;
	for (int i = 0 ; i < len ; i++){
		const char c = math.PChar()[i];
		is_float &= (c >= '0' && c <= '9') || c=='.';
		if (c == '('){
			brackets_whole &= brackets > 0 || i == 0;
			brackets++;
		}else if (c == ')'){
			brackets--;
			brackets_whole &= brackets > 0 || i == len - 1;
		}
	}
	if (is_float){
		return math.AsFloat();
	}
	if (brackets_whole && math.PChar()[0] == '(' && math.PChar()[len - 1] == ')'){
		math = math.SubString(2, math.Length() - 2);
		len -= 2;
	}

	int op_pos = -1;
	int op_index = -1;
	int op_rank = -1;
	char buf[2];
	memset(buf, 0, sizeof(buf));
	for (int i = 0 ; i < len ; i++){
		const char c = buf[0] = math.PChar()[i];
		if (c == '(' || c == '['){
			brackets++;
		}else if (c == ')' || c == ']'){
			brackets--;
		}else if (brackets == 0){
			for (int io = 0 ; io < num_ops ; io++){
				if (c == ops[io]){
					if (ops_rank[io] >= op_rank){
						op_index = io;
						op_rank = ops_rank[io];
						op_pos = i;
					}
				}
			}
		}
	}

	if (len && op_index < 0){
		const int has_index = math.PChar()[len - 1] == ']';
		FString property_name = has_index ? math.SplitBy('[') : math;
		FString index_str = has_index ? math.SubString(1, math.Length() - 1) : FString("");

		const NNLayerProperty *property = GetPropertyByName(property_name);
		FString value;
		if (property) {
			value = property->GetDisplayValue();
		}else {
			FStringList visit_network;
			int found = false;
			value = current_network->GetArgument(related_networks, property_name, visit_network, found, error);
			success &= found && error=="";
		}
		if (success) {
			if (value != "") {

				//Create value array directly instead of GetIntArray() because IntArray is not available for Argument.Value
				FListT<double> array;
				while (value != ""){
					array.Add(value.SplitBy(',').AsFloat());
				}

				int index = index_str != "" ? (int)EvalMath(NNLayerPropertyType::UInt, current_network, related_networks, index_str, success, error) : 0;
				if (index >= 0 && index < array.GetSize()){
					return array.GetAt(index);
				}
			}
		}else {
			//Error
			error = FString("Unknown property name(") + property_name + FString(")");
		}
	}else {
		const double left_value = EvalMath(type, current_network, related_networks, math.SubString(1, op_pos), success, error);
		const double right_value = EvalMath(type, current_network, related_networks, math.SubString(op_pos + 2, math.Length()), success, error);
		if (error != ""){
			const double value = math.AsFloat();
			if (!success){
				//float value with exponential notation
				success = true;
				error = "";
				return value;
			}
		}else if (success){
			switch (op_index){
			case 0:// /
				if (right_value != 0.0){
					return type >= NNLayerPropertyType::Int && type <= NNLayerPropertyType::UIntArray ? (int)left_value / (int)right_value : left_value / right_value;
				}else {
					error = "Divided by zero";
				}
				break;
			case 1:// *
				return left_value * right_value;
				break;
			case 2:// -
				return left_value - right_value;
				break;
			case 3:// +
				return left_value + right_value;
				break;
			}
		}
	}

	success = false;
	return 0.0;//failed
}
//---------------------------------------------------------------------------
int NNLayer::EvalPropertyMacro(
	NNLayerPropertyType type,
	const NNStructure *current_network,
	const NNStructures *related_networks,
    const NNLayerProperty *property,
	const FString &macro,
	FString &value,
	FString &error)
{
	value = "";
	if (macro == ""){
		return true;
	}
	if (macro.PChar()[0] != '*'){
		if (type >= NNLayerPropertyType::Int && type <= NNLayerPropertyType::FloatArray){
			int success = true;
			if (macro.PosChar(',') <= 0){
				value = FString(EvalMath(type, current_network, related_networks, macro, success, error));
			}else {
				FString m = macro;
				while (m != ""){
					FString m1 = m.SplitBy(',');
					if (value != ""){
						value += ",";
					}
					int len = m1.Length();
					if (len) {
						int need_last = false;
						if (m1.PChar()[0] == '('){
							value += "(";
							m1 = m1.SubString(2, len - 1);
							len--;
						}
						if (len && m1.PChar()[len - 1] == ')'){
							m1 = m1.SubString(1, len - 1);
							need_last = true;
							len--;
						}
						const double v = EvalMath(type, current_network, related_networks, m1, success, error);
						value += FString(v);
						if (need_last){
							value += ")";
						}
					}
				}
			}
			if (success){
				return true;
			}else {
				value = "";
			}
		}else {
			value = macro;
			return true;
		}
		return false;
	}else {
		switch (macro.PChar()[1]){
		case 'A':
			if (macro == "*AutoInPlace" || macro == "*AutoInPlaceOnce"){
				//NOP
				return false;
			}else if (macro == "*AdaptiveSeparableConvVKernelShape" || macro == "*AdaptiveSeparableConvHKernelShape" ) {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				if (!property_input || !property_kernel){
					return false;
				}
				if (property_input->GetIntArray().GetSize() != 3 || property_kernel->GetIntArray().GetSize() != 2){
					return false;
				}
				if (macro == "*AdaptiveSeparableConvVKernelShape") {
					value = FString((int)property_kernel->GetIntArray().GetAt(0));
				}else {
					value = FString((int)property_kernel->GetIntArray().GetAt(1));
				}
				value +=
					FString(",") +
					FString((int)property_input->GetIntArray().GetAt(1)) +
					FString(",") +
					FString((int)property_input->GetIntArray().GetAt(0));
				return true;
			}else if (macro == "*AdaptiveSeparableConvolutionOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				if (property_kernel &&
					property_input->GetIntArray().GetSize() >= property_kernel->GetIntArray().GetSize()) {

					FString result;
					int success = true;
					for (int id = 0 ; id < property_kernel->GetIntArray().GetSize() ; id++) {
						int k = property_kernel->GetIntArray().GetAt(id);
						if (id >= property_input->GetIntArray().GetSize()){
							return false;
						}
						const int input = property_input->GetIntArray().GetAt(id);
						const int elements = input - k + 1;
						if (elements > 0) {
							result = FString(",") + FString(elements) + result;
						}else {
							success = false;
						}
					}
					if (success) {
						value = FString((int)property_input->GetIntArray().GetAt(property_input->GetIntArray().GetSize() - 1)) + result;
						return true;
					}
				}
				return false;
			}else if (macro == "*AdaptiveSeparableConvolutionMultiplyAddSize") {
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_output = GetOutputPropertyIndex() >= 0 ? &GetProperty(GetOutputPropertyIndex()) : NULL;
				if (property_kernel && property_output) {
					if (property_kernel->GetIntArray().GetSize() &&
						property_output->GetIntArray().GetSize()) {

						//Output_pixels * #num_input_image * kernel
						__int64 v = property_output->GetIntArrayProduct();
						for (int id = 0 ; id < property_kernel->GetIntArray().GetSize() ; id++) {
							v *= property_kernel->GetIntArray().GetAt(id);
						}
						value = FString((double)v);
						return true;
					}
				}
				return false;
			} else if (macro == "*AffineWeightShape") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_out_shape = GetPropertyByName("OutShape");
				if (property_out_shape) {
					if (property_out_shape->GetIntArray().GetSize()) {

						value =
							FString((int)property_input->GetIntArrayProduct());
						for (int i = property_out_shape->GetIntArray().GetSize() - 1 ; i >= 0 ; i--) {
							value +=
								FString(",") +
								FString((int)property_out_shape->GetIntArray().GetAt(i));
						}
						return true;
					}
				}
				return false;
			}else if (macro == "*AffineParameterSize" || macro == "*AffineParameterSizeDiv32") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_out_shape = GetPropertyByName("OutShape");
				const NNLayerProperty *property_withbias = GetPropertyByName("WithBias");
				if (property_out_shape && property_withbias) {
					if (property_out_shape->GetIntArray().GetSize() &&
						property_withbias->GetIntArray().GetSize()) {

						__int64 v = 0;
						const int binary = macro == "*AffineParameterSizeDiv32";
						if (!GetInputPinConnected(1)) {
							//Weight
							v += property_input->GetIntArrayProduct() * property_out_shape->GetIntArrayProduct();
							if (binary) {
								v = (v + 31) / 32;
							}
						}
						if (!GetInputPinConnected(2 + binary) && property_withbias->GetIntArray().GetAt(0)) {
							//Bias
							v += property_out_shape->GetIntArrayProduct();
						}
						value = FString((double)v);
						return true;
					}
				}
				return false;
			}else if (macro == "*AxisSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_axis = GetPropertyByName("Axis");
				if (property_axis) {
					if (property_axis->GetIntArray().GetSize() == 1) {
						const int axis = property_axis->GetIntArray().GetAt(0);
						if (property_input->GetIntArray().GetSize() > axis) {
							value = FString((int)property_input->GetIntArray().GetAt(property_input->GetIntArray().GetSize() - 1 - axis));
							return true;
						}
					}
				}
				return false;
			}
			break;
		case 'B':
			if (macro == "*BasicMathOutputSize") {
				const NNLayerProperty *property_inputs = GetPropertyByName("Inputs");
				if (property_inputs) {
					if (property_inputs->GetIntArray().GetSize()) {
						FString inputs_display_value = property_inputs->GetDisplayValue();
						FStringList input_disply_values;
						while (inputs_display_value != ""){
							FString s = inputs_display_value.SplitBy("),(");
							s = s.ReplaceStr("(","").ReplaceStr(")","");
							input_disply_values.Add(s);
						}

						if (input_disply_values.GetSize() < 2) {
							NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("Inputs"));
							property.SetError(true,
								FString("Number of inputs to ") +
								GetType() +
								FString(" MUST be more than two"));
						}else {
							int success = true;
							for (int iin = 1 ; iin < input_disply_values.GetSize() && success ; iin++) {
								success &= input_disply_values.GetAt(iin) == input_disply_values.GetAt(0);
							}
							if (success) {
								value = input_disply_values.GetAt(0);
								return true;
							}else {
								NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("Inputs"));
								property.SetError(true,
									FString("Different input size"));
							}
						}
					}
				}
				return false;
			}else if (macro == "*BasicMathOutputSizewBC") {
				const NNLayerProperty *property_inputs = GetPropertyByName("Inputs");
				if (property_inputs) {
					if (property_inputs->GetIntArray().GetSize()){
						int num_input = 0;
						FString inputs_display_macro = property_inputs->GetDisplayValue();
						FListP<FListT<int> > input_sizes;
						FListT<int> link_ids = current_network->EnumLinkIDByDestLayerID(GetLayerID(), 0);
						while (inputs_display_macro != ""){
							FString input_size = inputs_display_macro.SplitBy("),(");
							input_sizes.Add(FListT<int>());
							while (input_size != ""){
								if (input_size.SubString(1, 1) == "(" || input_size.SubString(1, 1) == ")"){
									input_size = input_size.SubString(2, input_size.Length() - 1);
								}else {
									input_sizes.GetAt(num_input).Add(input_size.SplitBy(",").AsInt());
								}
							}
							if (input_sizes.GetAt(num_input).GetSize() && num_input < link_ids.GetSize()){
								const NNLink *link = current_network->GetLinkByID(link_ids.GetAt(num_input));
								if (link) {
									const NNLayer *source_layer = current_network->GetLayerByID(link->GetSourceLayerID());
									if (source_layer){
										if (source_layer->GetIsParamPropertyIndex() >= 0){
											if (source_layer->GetProperty(source_layer->GetIsParamPropertyIndex()).GetIntArray().GetSize()){
												if (source_layer->GetProperty(source_layer->GetIsParamPropertyIndex()).GetIntArray().GetAt(0)){
													//Delete batch
													input_sizes.GetAt(num_input).Delete(0);
												}
											}
										}
									}
                                }

							}
							if (num_input){
								if (input_sizes.GetAt(num_input).GetSize() != input_sizes.GetAt(num_input - 1).GetSize()){
									//inconsistent input dim
									return false;
								}
							}
							num_input ++;
						}

						if (num_input >= 2) {
							const int dim = input_sizes.GetAt(0).GetSize();
							FString result;
							int success = true;
							for (int id = 0 ; id < dim ; id++) {
								if (result != "") {
									result += FString(",");
								}
								int elements = 1;
								for (int ii = 0 ; ii < num_input ; ii++) {
									if (elements != input_sizes.GetAt(ii).GetAt(id)){
										if (elements == 1){
											elements = input_sizes.GetAt(ii).GetAt(id);
										}else if (input_sizes.GetAt(ii).GetAt(id) != 1){
											//Cannot broadcast
											success = false;
										}
									}
								}
								result += FString(elements);
							}
							if (success) {
								value = result;
								return true;
							}
						}
					}
				}
				return false;
			}else if (macro == "*BatchDetOutputSize") {
				const NNLayerProperty *property_input = GetPropertyByName("Input");
				if (property_input) {
					const int dim = property_input->GetIntArray().GetSize();
					if (dim == 2) {
						if (property_input->GetIntArray().GetAt(0) == property_input->GetIntArray().GetAt(1)){
							int result = true;
							value = FString("1");
							return result;
						}
					}else{
						NNLayerProperty &property = properties_.GetAt(GetInputPropertyIndex());
						property.SetError(true,
							FString("Input must be square matrix."));
					}
				}
				return false;
			}else if (macro == "*BatchNormalizationShape" ||
					  macro == "*BatchNormalizationParameterSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_axes = GetPropertyByName("Axes");
				if (!property_axes) {
					property_axes = GetPropertyByName("ChannelAxis");
				}
				if (property_axes) {
					if (property_axes->GetIntArray().GetSize()) {

						value = "1";
						int v = 1;
						for (int id = property_input->GetIntArray().GetSize() - 1 ; id >= 0 ; id--) {
							value += ",";
							if (property_axes->GetIntArray().IndexOf(property_input->GetIntArray().GetSize() - 1 - id) >= 0) {
								v *= property_input->GetIntArray().GetAt(id);
								value += FString((int)property_input->GetIntArray().GetAt(id));
							}else {
								value += "1";
							}
						}
						if (macro == "*BatchNormalizationParameterSize") {
							value = FString(v);
						}
						return true;
					}
				}
				return false;
			}
			break;
		case 'C':
			if (macro == "*ConvolutionOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_pad = GetPropertyByName("Padding");
				const NNLayerProperty *property_dilation = GetPropertyByName("Dilation");
				const NNLayerProperty *property_stride = GetPropertyByName("Strides");
				const NNLayerProperty *property_border = GetPropertyByName("BorderMode");
				const NNLayerProperty *property_map = GetPropertyByName("OutMaps");
				const NNLayerProperty *property_channel_last = GetPropertyByName("ChannelLast");
				if (property_kernel && property_border && property_map) {
					if (property_kernel->GetIntArray().GetSize() &&
						property_stride->GetIntArray().GetSize() &&
						property_map->GetIntArray().GetSize() == 1 &&
						property_kernel->GetIntArray().GetSize() == property_stride->GetIntArray().GetSize() &&
						property_input->GetIntArray().GetSize() >= property_kernel->GetIntArray().GetSize()) {

						const int channel_last = property_channel_last && property_channel_last->GetIntArray().GetSize() && property_channel_last->GetIntArray().GetAt(0);

						FString result;
						int success = true;
						for (int id = 0 ; id < property_kernel->GetIntArray().GetSize() ; id++) {
							int k = property_kernel->GetIntArray().GetAt(id);
							if (property_dilation) {
								//NNabla
								if (property_dilation->GetIntArray().GetSize() == property_kernel->GetIntArray().GetSize()) {
									k = (k - 1) * property_dilation->GetIntArray().GetAt(id) + 1;
								}else {
									success = false;
								}
							}
							if (id + channel_last >= property_input->GetIntArray().GetSize()){
								return false;
							}
							const int input = property_input->GetIntArray().GetAt(id + channel_last);
							int pad = 0;
							if (property_pad) {
								if (property_pad->GetIntArray().GetSize() == property_kernel->GetIntArray().GetSize()) {
									pad = property_pad->GetIntArray().GetAt(id) * 2;
								}else {
									success = false;
								}
							}
							if (!property_dilation) {
								//sDeePy
								if (property_border->GetValue() == "same") {
									pad += k - 1;
								}else if (property_border->GetValue() == "full") {
									pad += (k - 1) * 2;
								}
							}
							const int stride = property_stride->GetIntArray().GetAt(id);
							const int elements = (input + pad - k) / stride + 1;
							if (elements > 0) {
								result = FString(",") + FString(elements) + result;
							}else {
								success = false;
							}
						}
						if (success) {
							if (!channel_last){
								//Channel First
								value = FString((double)property_map->GetIntArray().GetAt(0)) + result;
							}else {
								//Channel Last
								value = result.SubString(2, result.Length() - 1) + FString(",") + FString((double)property_map->GetIntArray().GetAt(0));
							}
							return true;
						}
					}
				}
				return false;
			}else if (macro == "*ConvolutionPaddingSize") {
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_dilation = GetPropertyByName("Dilation");
				const NNLayerProperty *property_border = GetPropertyByName("BorderMode");
				if (property_kernel && property_border && property_dilation) {
					if (property_kernel->GetIntArray().GetSize() &&
						property_kernel->GetIntArray().GetSize() == property_dilation->GetIntArray().GetSize()) {

						value = "";
						for (int id = 0 ; id < property_kernel->GetIntArray().GetSize() ; id++) {
							int k = property_kernel->GetIntArray().GetAt(id);
							k = (k - 1) * property_dilation->GetIntArray().GetAt(id) + 1;
							int pad = 0;
							if (property_border->GetValue() == "same") {
								pad = (k - 1) / 2;
							}else if (property_border->GetValue() == "full") {
								pad = k - 1;
							}
							if (value != "") {
								value = FString(",") + value;
							}
							value = FString(pad) + value;
						}
						return true;
					}
				}
				return false;
			}else if (macro == "*ConvolutionWeightShape" || macro == "*ConvolutionWeightShapeDeconvolution") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_map = GetPropertyByName("OutMaps");
				const NNLayerProperty *property_group = GetPropertyByName("Group");
				const NNLayerProperty *property_channel_last = GetPropertyByName("ChannelLast");
				if (property_kernel && property_map && property_group) {
					if (property_kernel->GetIntArray().GetSize() &&
						property_map->GetIntArray().GetSize() == 1 &&
						property_group->GetIntArray().GetSize() == 1 &&
						property_input->GetIntArray().GetSize() >= property_kernel->GetIntArray().GetSize()) {

						const int channel_last = property_channel_last && property_channel_last->GetIntArray().GetSize() && property_channel_last->GetIntArray().GetAt(0);
						const int group = (int)property_group->GetIntArray().GetAt(0);

						int success = true;
						FString in_maps;
						//InMaps
						if (property_input->GetIntArray().GetSize() > property_kernel->GetIntArray().GetSize()){
							int in_map = (int)property_input->GetIntArray().GetAt(channel_last ? 0 : property_kernel->GetIntArray().GetSize());
							//Channel First
							if (group > 0 && in_map % group == 0){
								if (macro == "*ConvolutionWeightShape") {
									in_maps = FString(in_map / group);
								} else {
									in_maps = FString(in_map);
								}
							}else {
								success = false;
							}
						}else {
							in_maps = "1";
						}
						//OutMaps
						FString out_maps;
						int out_map = (int)property_map->GetIntArray().GetAt(0);
						if (group > 0 && out_map % group == 0) {
							if (macro == "*ConvolutionWeightShapeDeconvolution") {
								out_maps = FString(out_map / group);
							} else {
								out_maps = FString(out_map);
							}
						} else {
							success = false;
						}
						//Kernel
						value = "";
						for (int id = property_kernel->GetIntArray().GetSize() - 1 ; id >= 0 ; id--) {
							value +=
								FString(",") +
								FString((int)property_kernel->GetIntArray().GetAt(id));
						}

						if (macro == "*ConvolutionWeightShapeDeconvolution") {
							if (!channel_last) {
								//*ConvolutionWeightShapeDeconvolution Channel First
								value = in_maps + FString(",") + out_maps + value;
							}else {
								//*ConvolutionWeightShapeDeconvolution Channel Last
								value = in_maps + value + FString(",") + out_maps;
							}
						}else {
							if (!channel_last) {
								//*ConvolutionWeightShape Channel First
								value = out_maps + FString(",") + in_maps + value;
							}else {
								//*ConvolutionWeightShape Channel Last
								value = out_maps  + value + FString(",") + in_maps;
							}
						}
						return success;
					}
				}
				return false;
			}else if (macro == "*ConvolutionParameterSize" || macro == "*ConvolutionParameterSizeDiv32") {
				//Used only for sDeePy engine
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_map = GetPropertyByName("OutMaps");
				const NNLayerProperty *property_withbias = GetPropertyByName("WithBias");
				if (property_kernel && property_map) {
					if (property_kernel->GetIntArray().GetSize() &&
						property_map->GetIntArray().GetSize() == 1 &&
						property_withbias->GetIntArray().GetSize() &&
						property_input->GetIntArray().GetSize() >= property_kernel->GetIntArray().GetSize()) {

						__int64 v = 0;
						const int binary = macro == "*ConvolutionParameterSizeDiv32";
						if (!GetInputPinConnected(1)) {
							//Weight
							v = property_map->GetIntArray().GetAt(0);
							for (int id = 0 ; id < property_input->GetIntArray().GetSize() ; id++) {
								int elements = id < property_kernel->GetIntArray().GetSize() ? property_kernel->GetIntArray().GetAt(id) : property_input->GetIntArray().GetAt(id);
								v *= elements;
							}
							if (binary) {
								v = (v + 31) / 32;
							}
						}
						if (!GetInputPinConnected(2 + binary) && property_withbias->GetIntArray().GetAt(0)) {
							//Bias
							v += property_map->GetIntArray().GetAt(0);
						}
						value = FString((double)v);
						return true;
					}
				}
				return false;
			}else if (macro == "*ConvolutionMultiplyAddSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_output = GetOutputPropertyIndex() >= 0 ? &GetProperty(GetOutputPropertyIndex()) : NULL;
				const NNLayerProperty *property_withbias = GetPropertyByName("WithBias");
				const NNLayerProperty *property_group = GetPropertyByName("Group");
				const NNLayerProperty *property_channel_last = GetPropertyByName("ChannelLast");
				if (property_kernel && property_output) {
					if (property_kernel->GetIntArray().GetSize() &&
						property_output->GetIntArray().GetSize() &&
						property_withbias->GetIntArray().GetSize() &&
						property_input->GetIntArray().GetSize() >= property_kernel->GetIntArray().GetSize()) {

						//Output_pixels * #num_input_image * kernel
						__int64 v = property_output->GetIntArrayProduct();
						int group = 1;
						if (property_group) if (property_group->GetIntArray().GetSize() == 1){
							group = property_group->GetIntArray().GetAt(0);
						}
						if (!property_channel_last || (property_channel_last->GetIntArray().GetSize() && !property_channel_last->GetIntArray().GetAt(0))){
							//Channel First
							for (int id = 0 ; id < property_input->GetIntArray().GetSize() ; id++) {
								int elements =
									id < property_kernel->GetIntArray().GetSize() ?
										property_kernel->GetIntArray().GetAt(id) :
											(id == property_kernel->GetIntArray().GetSize() ?
												property_input->GetIntArray().GetAt(id) / group :
												1);
								v *= elements;
							}
						}else {
							//Channel Last
							for (int id = 0 ; id < property_input->GetIntArray().GetSize() ; id++) {
								int elements =
									id == 0 ?
										property_input->GetIntArray().GetAt(0) / group :
										(id - 1 < property_kernel->GetIntArray().GetSize() ?
											property_kernel->GetIntArray().GetAt(id - 1) :
												1);
								v *= elements;
							}
						}

						value = FString((double)v);
						return true;
					}
				}
				return false;
			}else if (macro == "*ConcatenateActivationOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				FString result;
				for (int id = 0 ; id < property_input->GetIntArray().GetSize() ; id++) {
					if (result != "") {
						result = FString(",") + result;
					}
					result = FString((int)property_input->GetIntArray().GetAt(id) * (id < property_input->GetIntArray().GetSize() - 1 ? 1 : 2)) + result;
				}
				value = result;
				return true;
			}else if (macro == "*ConcatenateOutputSize") {
				const NNLayerProperty *property_inputs = GetPropertyByName("Inputs");
				const NNLayerProperty *property_axis = GetPropertyByName("Axis");
				if (property_inputs && property_axis) {
					if (property_inputs->GetIntArray().GetSize() &&
						property_axis->GetIntArray().GetSize() == 1){

						int num_input = 0;
						FString inputs_display_macro = property_inputs->GetDisplayValue();
						while (inputs_display_macro != ""){
							inputs_display_macro.SplitBy("),(");
							num_input ++;
						}

						if (num_input) {
							if ((property_inputs->GetIntArray().GetSize() + 1) / num_input > property_axis->GetIntArray().GetAt(0)){
								const int axis = property_inputs->GetIntArray().GetSize() / num_input - 1 - property_axis->GetIntArray().GetAt(0);

								const int dim = property_inputs->GetIntArray().GetSize() / num_input;
								int success = false;
								FString result;
								if (axis >= 0 && axis < dim && (property_inputs->GetIntArray().GetSize() + 1) % num_input == 0) {
									success = true;
									for (int id = 0 ; id < dim ; id++) {
										if (result != "") {
											result = FString(",") + result;
										}
										if (id == axis) {
											int elements = 0;
											for (int ii = 0 ; ii < num_input ; ii++) {
												elements += property_inputs->GetIntArray().GetAt(ii * (dim + 1) + id);
											}
											result = FString(elements) + result;
										}else {
											const int elements = property_inputs->GetIntArray().GetAt(id);
											for (int ii = 1 ; ii < num_input ; ii++) {
												if (elements != property_inputs->GetIntArray().GetAt(ii * (dim + 1) + id)){
													success = false;
												}
											}
											result = FString(elements) + result;
										}
									}
								}
								if (success) {
									value = result;
									return true;
								}
							}
						}
					}
				}
				return false;
			}else if (macro == "*CCEOutputSize") {
				const NNLayerProperty *property_input = GetPropertyByName("Input");
				const NNLayerProperty *property_axis = GetPropertyByName("Axis");
				if (property_input && property_axis) {
					if (property_input->GetIntArray().GetSize() &&
						property_axis->GetIntArray().GetSize() == 1){
						if (property_input->GetIntArray().GetSize() > property_axis->GetIntArray().GetAt(0)){
							const int axis = property_input->GetIntArray().GetSize() - 1 - property_axis->GetIntArray().GetAt(0);
							int success = false;
							FString result;
							if (axis >= 0) {
								success = true;
								for (int id = 0 ; id < property_input->GetIntArray().GetSize() ; id++) {
									const int elements = property_input->GetIntArray().GetAt(id);
									if (result != "") {
										result = FString(",") + result;
									}
									if (id == axis) {
										result = FString("1") + result;
									}else {
										result = FString(elements) + result;
									}
								}
							}
							if (success) {
								value = result;
								return true;
							}
						}
					}
				}
				return false;
			}else if (macro == "*CELTorOutputSize" || macro == "*CELTorShape") {
				const NNLayerProperty *property_input = GetPropertyByName("Input");
				const NNLayerProperty *property_class_num = GetPropertyByName("ClassNum");
				if (property_input && property_class_num) {
					FString s;
					if ((property_input->GetIntArray().GetSize() != 0) &&
						(property_class_num->GetIntArray().GetSize() != 0)) {

						int class_num = (int)(property_class_num->GetIntArray().GetAt(0));
						int prod = property_input->GetIntArrayProduct();
						if (prod % class_num != 0) {
						   return false;
						}
						if (prod != class_num) {
						   value = FString(prod / class_num);
						}
						else {
						   value = FString("1");
						}
						return true;
					}
				}
				return false;
			}else if (macro.SubString(2, 4) == "Calc") {
				FString m = macro;
				m.SplitBy(' ');
				__int64 v = 0;
				int success = true;
				while (m != ""){
					FString operation = m.SplitBy(':');
					FString _operator = operation.SubString(1, 1);
					FString argument = operation.SubString(2, operation.Length() - 1);
					FString macro_1v;
					EvalPropertyMacro(type, current_network, related_networks, property, argument, macro_1v, error);
					int success_1 = macro_1v != "" && error == "";
					if (success_1) {
						__int64 macro_1 = StrToScalar(macro_1v);
						if (_operator == "+") {
							v += macro_1;
						}else if (_operator == "*"){
							v *= macro_1;
						}else if (_operator == "/"){
							v /= macro_1;
						}else if (_operator == "-"){
							v -= macro_1;
						}else {
							success_1 = false;
						}
					}
					if (!success_1) {
						success = false;
						break;
					}
				}
				if (success) {
					value = FString(v * 1.0);
					return true;
				}
				return false;
			}
			break;
		case 'D':
			if (property && macro == "*Default") {
				value = property->GetDefaultValue();
				return true;
			}else if (macro == "*DeconvolutionOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_pad = GetPropertyByName("Padding");
				const NNLayerProperty *property_dilation = GetPropertyByName("Dilation");
				const NNLayerProperty *property_stride = GetPropertyByName("Strides");
				const NNLayerProperty *property_map = GetPropertyByName("OutMaps");
				const NNLayerProperty *property_channel_last = GetPropertyByName("ChannelLast");
				if (property_kernel && property_pad && property_stride && property_dilation && property_map) {
					const int channel_last = property_channel_last && property_channel_last->GetIntArray().GetSize() && property_channel_last->GetIntArray().GetAt(0);
					if (property_kernel->GetIntArray().GetSize() &&
						property_pad->GetIntArray().GetSize() &&
						property_stride->GetIntArray().GetSize() &&
						property_dilation->GetIntArray().GetSize() &&
						property_map->GetIntArray().GetSize() == 1 &&
						property_kernel->GetIntArray().GetSize() == property_pad->GetIntArray().GetSize() &&
						property_kernel->GetIntArray().GetSize() == property_dilation->GetIntArray().GetSize() &&
						property_kernel->GetIntArray().GetSize() == property_stride->GetIntArray().GetSize() &&
						property_input->GetIntArray().GetSize() >= property_kernel->GetIntArray().GetSize() + channel_last) {

						FString result;
						int success = true;
						for (int id = 0 ; id < property_kernel->GetIntArray().GetSize() ; id++) {
							const int input = property_input->GetIntArray().GetAt(id + channel_last);
							const int k = property_kernel->GetIntArray().GetAt(id);
							const int stride = property_stride->GetIntArray().GetAt(id);
							const int pad = property_pad->GetIntArray().GetAt(id);
							const int d = property_dilation->GetIntArray().GetAt(id);
							const int elements = stride * (input - 1) - 2 * pad + (d * (k - 1) + 1);
							if (elements > 0) {
								result = FString(",") + FString(elements) + result;
							}else {
								success = false;
							}
						}
						if (success) {
							if (!channel_last){
								//Channel First
								value = FString((double)property_map->GetIntArray().GetAt(0)) + result;
							}else {
								//Channel Last
								value = result.SubString(2, result.Length() - 1) + FString(",") + FString((double)property_map->GetIntArray().GetAt(0));
							}
							return true;
						}
					}
				}
				return false;
			}else if (macro == "*DeformableConvMaskShape" || macro == "*DeformableConvOffsetShape") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_deformable_group = GetPropertyByName("DeformableGroup");
				if (property_kernel && property_deformable_group) {
					if (property_kernel->GetIntArray().GetSize() &&
						property_deformable_group->GetIntArray().GetSize() == 1 &&
						property_input->GetIntArray().GetSize() == 3 && property_kernel->GetIntArray().GetSize() == 2) {

						int size = property_kernel->GetIntArray().GetAt(0) * property_kernel->GetIntArray().GetAt(1) * property_deformable_group->GetIntArray().GetAt(0);
						if (macro == "*DeformableConvOffsetShape") {
							size *= 2;
                        }

						int success = true;
						value =
							FString(size)+//Kernel
							FString(",") +
							FString((int)property_input->GetIntArray().GetAt(1)) +//H
							FString(",") +
							FString((int)property_input->GetIntArray().GetAt(0));//W

						return success;
					}
				}
				return false;
			}
			break;
		case 'E':
			if (macro == "*EmbeddingOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_dim = GetPropertyByName("Dim") ? GetPropertyByName("Dim") : GetPropertyByName("Shape");
				if (property_dim) {
					if (property_dim->GetIntArray().GetSize()) {

						value =
							property_input->GetDisplayValue() +
							FString(",") +
							property_dim->GetDisplayValue();
						return true;

					}
				}
				return false;
			}else if (macro == "*EmbeddingWeightSize") {
				const NNLayerProperty *property_num_class = GetPropertyByName("NumClass");
				const NNLayerProperty *property_dim = GetPropertyByName("Dim") ? GetPropertyByName("Dim") : GetPropertyByName("Shape");
				if (property_num_class && property_dim) {
					if (property_num_class->GetIntArray().GetSize() &&
						property_dim->GetIntArray().GetSize()) {

						value =
							property_num_class->GetDisplayValue() +
							FString(",") +
							property_dim->GetDisplayValue();
						return true;
					}
				}
				return false;
			}
			break;
		case 'F':
			if (macro == "*FFTMultiplyAddSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_ndim = GetPropertyByName("SignalNDim");
				if (property_ndim) {
					if (property_input->GetIntArray().GetSize() >= 2 && property_ndim->GetIntArray().GetSize() == 1) {
						const int dim = property_input->GetIntArray().GetSize();
						const int ndim = property_ndim->GetIntArray().GetAt(0);
						if (property_input->GetIntArray().GetAt(0) != 2){
							NNLayerProperty &property = properties_.GetAt(GetInputPropertyIndex());
							property.SetError(true,
								FString("Number of element in the last dimension must be 2 (real and imaginary)."));
						}else if (dim < ndim + 1){
							NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("SignalNDim"));
							property.SetError(true,
								FString("SignalNDim must be input dim - 1 or less."));
						}else {
							long long result = 0;
							long long elements = 1;
							for (int id2 = 1 ; id2 < dim ; id2++){
								elements *= property_input->GetIntArray().GetAt(id2);
							}
							for (int id = 0 ; id < ndim ; id++){
								long long result_1 =
									(int)ceil(log((double)property_input->GetIntArray().GetAt(id + 1)) / log((double)2)) *//log2(elements)
									4 * //complex multiplication
									2; //butterfly
								result += result_1;
							}
							value = FString((double)result * elements);
							return true;
						}
					}
				}
				return false;
			}
			break;
		case 'G':
			if (macro == "*GraphInput" || macro == "*GraphIO" || macro == "*GraphInputs" ||	macro == "*GraphOutput"){
				return false;
			}else if (macro == "*GramMatrixOutputSize"){
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const FListT<__int64> &int_array = property_input->GetIntArray();
				const int maps = int_array.GetAt(int_array.GetSize() - 1);
				value = FString(maps) + FString(",") + FString(maps);
				return true;
			}
			break;
		case 'I':
			if (macro == "*IsParam"){
				//NOP
				return false;
			}else if (macro.Pos("InputPinState")) {
				FString argument = macro;
				const int inv = argument.SubString(2, 1) == "!";
				argument.SplitBy('#');
				const int pin_index = argument.AsInt();
				int macro_1 = GetInputPinConnected(pin_index);
				if (inv) {
					macro_1 = 1 - macro_1;
				}
				value = FString(macro_1 ? "1" : "0");
				return true;
			}
			break;
		case 'L':
			if (macro == "*LayerNormalizationShape") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				value = "1";
				int v = 1;
				for (int id = property_input->GetIntArray().GetSize() - 1 ; id >= 0 ; id--) {
					value += ",";
					value += FString((int)property_input->GetIntArray().GetAt(id));
				}
				return true;
			}else if (macro == "*LinearWeightShape") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_out_shape = GetPropertyByName("OutFeatures");
				if (property_out_shape) {
					if ((property_out_shape->GetIntArray().GetSize() != 0) &&
						(property_input->GetIntArray().GetSize() != 0)) {

						int out_features = property_out_shape->GetIntArray().GetAt(0);
						int in_features = property_input->GetIntArray().GetAt(0);
						value = FString(out_features) + FString(",") + FString(in_features);
						return true;
					}
				}
				return false;
			}else if (macro == "*LinearOutShape") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_out_features = GetPropertyByName("OutFeatures");
  			    value = FString("");
				if (property_input->GetIntArray().GetSize()) for (int i = property_input->GetIntArray().GetSize() - 1; i >= 1; --i) {
				  value += FString((int)property_input->GetIntArray().GetAt(i)) + FString(",");
				}
				else {
                    return false;
                }
				value += FString(property_out_features->GetValue());
				return true;
			}else if (macro == "*LSTMWeightShape") {
				const NNLayerProperty *property_inner_shape = GetPropertyByName("InnerShape");
				const NNLayerProperty *property_size = GetPropertyByName("Size");
				if (property_inner_shape && property_size) {
					if (property_inner_shape->GetIntArray().GetSize() &&
						property_size->GetIntArray().GetSize()) {

						value =
							FString((int)(property_inner_shape->GetIntArrayProduct() + property_size->GetIntArrayProduct())) +
							FString(",") +
							property_size->GetDisplayValue();
						return true;
					}
				}
				return false;
			}else if (macro == "*LocallyConnectedOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_kernel = GetPropertyByName("KernelShape");
				const NNLayerProperty *property_map = GetPropertyByName("OutMaps");
				if (property_kernel && property_map) {
					if (property_kernel->GetIntArray().GetSize() &&
						property_map->GetIntArray().GetSize() == 1 &&
						property_input->GetIntArray().GetSize() >= property_kernel->GetIntArray().GetSize()) {

						FString result;
						int success = true;
						for (int id = 0 ; id < property_kernel->GetIntArray().GetSize() ; id++) {
							int elements = property_input->GetIntArray().GetAt(id) - property_kernel->GetIntArray().GetAt(id) + 1;
							if (elements > 0) {
								result = FString(",") + FString(elements) + result;
							}else {
								success = false;
							}
						}
						if (success) {
							value = FString((double)property_map->GetIntArray().GetAt(0)) + result;
							return true;
						}
					}
				}
				return false;
			}
			break;
		case 'M':
            if (macro == "*MatrixDiagOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				if (property_input->GetIntArray().GetSize() == 0) {
					return false;
				}
				value = property_input->GetDisplayValue() + FString(",") + FString((int)property_input->GetIntArray().GetAt(0));
				return true;
			}else if (macro == "*MatrixDiagPartOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				if (property_input->GetIntArray().GetSize() >= 2) {
					if (property_input->GetIntArray().GetAt(0) == property_input->GetIntArray().GetAt(1)){
						value = "";
						for (int id = 1; id < property_input->GetIntArray().GetSize(); id++) {
							if (value != "") {
								value = FString(",") + value;
							}
							value = FString((int)property_input->GetIntArray().GetAt(id)) + value;
						}
						return true;
					}
				}
				return false;
			}else if (macro == "*MatmulOutputSize") {
				const NNLayerProperty *property_input = GetPropertyByName("Inputs");
				if (property_input) {
					const int dim = property_input->GetIntArray().GetSize() / 2;
					if (dim >= 2 && property_input->GetIntArray().GetSize() == dim * 2 + 1) {
						if (property_input->GetIntArray().GetAt(1) == property_input->GetIntArray().GetAt(dim + 1)){
							int result = true;
							value =
								FString((int)property_input->GetIntArray().GetAt(dim + 1 + 1)) +
								FString(",") +
								FString((int)property_input->GetIntArray().GetAt(0));
							for (int id = 2; id < dim; id++) {
								if ((int)property_input->GetIntArray().GetAt(id) == (int)property_input->GetIntArray().GetAt(dim + id + 1)){
									value = FString((int)property_input->GetIntArray().GetAt(id)) + FString(",") + value;
								} else {
									result = false;
								}
							}
							return result;
						}
					}
				}
				return false;
			}else if (macro == "*MatmulMultiplyAddSize") {
				const NNLayerProperty *property_input = GetPropertyByName("Inputs");
				if (property_input) {
					const int dim = property_input->GetIntArray().GetSize() / 2;
					if (dim >= 2 && property_input->GetIntArray().GetSize() == dim * 2 + 1) {
						if (property_input->GetIntArray().GetAt(1) == property_input->GetIntArray().GetAt(dim + 1)){
							double size =
								property_input->GetIntArray().GetAt(0) *
								property_input->GetIntArray().GetAt(1) *
								property_input->GetIntArray().GetAt(dim + 1 + 1);

							int result = true;
							for (int id = 2 ; id < dim; id++) {
								if ((int)property_input->GetIntArray().GetAt(id) == (int)property_input->GetIntArray().GetAt(dim + id + 1)){
									size *= property_input->GetIntArray().GetAt(id);
								} else {
									result = false;
								}
							}
							value = FString(size);
							return result;
						}
					}
				}
				return false;
			}
			break;
		case 'O':
			if (macro == "*OneHotOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_shape = GetPropertyByName("Shape");
				if (property_input->GetIntArray().GetSize() && property_shape && property_shape->GetIntArray().GetSize()) {
					if (property_input->GetIntArray().GetAt(0) == property_shape->GetIntArray().GetSize()){
						value = property_shape->GetDisplayValue();
						for (int id = 1 ; id < property_input->GetIntArray().GetSize() ; id++){
							value = FString((int)property_input->GetIntArray().GetAt(id)) + FString(",") + value;
						}
                        return true;
					}else {
						NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("Shape"));
						property.SetError(true,
							FString("The number of dimensions must be the same as the number of elements of the last dimension of Input. ("));
						return false;
					}
				}
			}
			break;
		case 'P':
			if (macro == "*PoolingOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const int nnabla = GetPropertyByName("KernelShape") != NULL;
				const NNLayerProperty *property_pool = nnabla ? GetPropertyByName("KernelShape") : GetPropertyByName("PoolShape");
				const NNLayerProperty *property_stride = GetPropertyByName("Strides");
				const NNLayerProperty *property_ignore_border = GetPropertyByName("IgnoreBorder");
				const NNLayerProperty *property_padding = GetPropertyByName("Padding");
				const NNLayerProperty *property_channel_last = GetPropertyByName("ChannelLast");
				const int channel_last = (property_channel_last && property_channel_last->GetIntArray().GetSize()) ? property_channel_last->GetIntArray().GetAt(0) : false;
				if (property_pool && property_stride && property_ignore_border && property_padding) {
					if (property_pool->GetIntArray().GetSize() &&
						property_stride->GetIntArray().GetSize() &&
						property_ignore_border->GetIntArray().GetSize() &&
						property_padding->GetIntArray().GetSize() &&
						property_pool->GetIntArray().GetSize() == property_stride->GetIntArray().GetSize() &&
						property_pool->GetIntArray().GetSize() == property_padding->GetIntArray().GetSize() &&
						property_input->GetIntArray().GetSize() >= property_pool->GetIntArray().GetSize() + channel_last) {

						FString result;
						int success = true;
						for (int id = 0 ; id < property_input->GetIntArray().GetSize() ; id++) {
							int elements = property_input->GetIntArray().GetAt(id);
							if (id >= channel_last && id < property_pool->GetIntArray().GetSize() + channel_last){
								const int pool = property_pool->GetIntArray().GetAt(id - channel_last);
								const int pad = property_padding->GetIntArray().GetAt(id - channel_last);
								if (nnabla){
									if (property_ignore_border->GetIntArray().GetAt(0)) {
										// shape[i] = int((shape[i] - kernel_[i]) / stride_[i]) + 1
										elements = int((elements + 2 * pad - pool) / property_stride->GetIntArray().GetAt(id - channel_last)) + 1;
									}else {
										// shape[i] = ceil(shape[i] * 1.0 / stride_[i]
										elements = (int)ceil((elements + 2 * pad) * 1.0 / property_stride->GetIntArray().GetAt(id - channel_last));
									}
								}else {
									if (property_ignore_border->GetIntArray().GetAt(0)) {
										elements =
											elements >= pool ?
												(elements + pad * 2 - pool) / property_stride->GetIntArray().GetAt(id - channel_last) + 1 :
												0;
									}else {
										elements = (int)ceil((elements + pad * 2 - pool) * 1.0 / property_stride->GetIntArray().GetAt(id - channel_last)) + 1;
									}
								}
							}
							if (elements > 0) {
								if (result != "") {
									result = FString(",") + result;
								}
								result = FString(elements) + result;
							}else {
								success = false;
							}
						}
						if (success) {
							value = result;
							return true;
						}
					}
				}
				return false;
			}else if (macro == "*PReluParameterSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_axes = GetPropertyByName("Axes") ? GetPropertyByName("Axes") : GetPropertyByName("BaseAxis");
				if (property_axes) {
					if (property_axes->GetIntArray().GetSize()) {

						__int64 v = 1;
						for (int id = 0 ; id < property_axes->GetIntArray().GetSize() ; id++) {
							if (property_axes->GetIntArray().GetAt(id) >= 0) {
								if (property_axes->GetIntArray().GetAt(id) < property_input->GetIntArray().GetSize()) {
									v *= property_input->GetIntArray().GetAt(property_input->GetIntArray().GetSize() - 1 - property_axes->GetIntArray().GetAt(id));
								}else {
									NNLayerProperty &property = properties_.GetAt(
										GetPropertyIndexByName("Axes") >= 0 ? GetPropertyIndexByName("Axes") : GetPropertyIndexByName("BaseAxis"));
									property.SetError(true,
										FString("Axis index is larger than input size"));
								}
							}
						}
						value = FString((double)v);
						return true;
					}
				}
				return false;
			}else if (macro == "*PadOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_width = GetPropertyByName("PadWidth");
				if (property_width) {
					const int dim = property_input->GetIntArray().GetSize();
					if (property_width->GetIntArray().GetSize() % 2 == 0 &&
						property_width->GetIntArray().GetSize() / 2 <= dim){
						FString result;
						for (int id = 0 ; id < dim ; id++) {
							if (result != "") {
								result = FString(",") + result;
							}
							const int elements = property_input->GetIntArray().GetAt(id);
							const int left = property_width->GetIntArray().GetSize() > id * 2 + 1 ? property_width->GetIntArray().GetAt(id * 2 + 1) : 0;
							const int right = property_width->GetIntArray().GetSize() > id * 2 ? property_width->GetIntArray().GetAt(id * 2) : 0;
							result = FString(left + elements + right) + result;
						}
						value = result;
						return true;
					}
				}
				return false;
			}
			break;
		case 'R':
			if (macro == "*RecurrentLength"){
				//NOP
				return false;
			}else if (macro == "*ReshapeOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_out_shape = GetPropertyByName("OutShape");
				if (property_out_shape) {
					if (property_out_shape->GetIntArray().GetSize() &&
						property_input->GetIntArrayProduct() ==
						property_out_shape->GetIntArrayProduct()) {
						value = property_out_shape->GetDisplayValue();
						return true;
					}
				}
				return false;
			}else if (macro == "*RunningMeanShape") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_base_axis = GetPropertyByName("BaseAxis");
				if (property_base_axis) {
					if (property_base_axis->GetIntArray().GetSize() == 1) {

						value = "";
						for (int id = property_input->GetIntArray().GetSize() - 1 ; id >= 0 ; id--) {
							if (value != "") {
								value += ",";
							}
							if (property_base_axis->GetIntArray().GetAt(0) <= property_input->GetIntArray().GetSize() - 1 - id) {
								value += FString((int)property_input->GetIntArray().GetAt(id));
							}
						}
						return true;
					}
				}
				return false;
			}else if (macro == "*RecurrentOutputSize" || macro == "*RecurrentOutputSize2"){
				const NNLayerProperty *property_input = GetPropertyByName(macro == "*RecurrentOutputSize" ? "Input" : "Size");
				const NNLayerProperty *property_length = GetPropertyByName("Length");
				const NNLayerProperty *property_axis = GetPropertyByName("Axis");
				if (property_input && property_length) {
					const FListT<__int64> &int_array = property_input->GetIntArray();
					const FListT<__int64> &int_array_length = property_length->GetIntArray();
					if (int_array.GetSize() && int_array_length.GetSize() == 1) {
						int axis = 0;
						if (property_axis) {
							if (property_axis->GetIntArray().GetSize() == 1) {
								axis = property_axis->GetIntArray().GetAt(0);
							}
						}
						if (int_array.GetSize() >= axis) {
							FString result;
							for (int id = -(int_array.GetSize() == axis) ; id < property_input->GetIntArray().GetSize(); id++) {
								if (id >= 0) {
									if (result != "") {
										result = FString(",") + result;
									}
									result = FString((int)property_input->GetIntArray().GetAt(id)) + result;
								}
								if (id == property_input->GetIntArray().GetSize() - 1 - axis) {
									if (result != "") {
										result = FString(",") + result;
									}
									result = FString((int)int_array_length.GetAt(0)) + result;
								}
							}
							if (result == "") {
								result = "1";
							}
							value = result;
							return true;
						}
					}
				}
				return false;
			}
			break;
		case 'S':
			if (macro == "*SoftmaxLabelShape"){
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_axis = GetPropertyByName("Axis");
				if (property_axis) {
					const FListT<__int64> &int_array = property_input->GetIntArray();
					const int axis = property_axis->GetIntArray().GetSize() == 1 ? property_axis->GetIntArray().GetAt(0) : 0;
					value = "";
					for (int i = int_array.GetSize() - 1; i >= 0 ; i--) {
						if (value != "") {
							value += ",";
						}
						if (i == int_array.GetSize() - 1 - axis) {
							value += "1";
						}else {
							value += FString((int)int_array.GetAt(i));
						}
					}
					return true;
				}
				return false;
			}else if (macro == "*ShapeExceptAxis") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_axis = GetPropertyByName("Axis");
				int axis = 0;
				if (property_axis) {
					if (property_axis->GetIntArray().GetSize() == 1) {
						axis = property_axis->GetIntArray().GetAt(0);
					}
				}
				if (property_input->GetIntArray().GetSize() > axis) {
					FString result;
					for (int id = 0 ; id < property_input->GetIntArray().GetSize() ; id++) if (id != property_input->GetIntArray().GetSize() - 1 - axis){
						if (result != "") {
							result = FString(",") + result;
						}
						result = FString((int)property_input->GetIntArray().GetAt(id)) + result;
					}
					if (result == "") {
						result = "1";
					}
					value = result;
					return true;
				}
				return false;
			}else if (macro == "*SumOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_axes = GetPropertyByName("Axes");
				const NNLayerProperty *property_keep_dims = GetPropertyByName("KeepDims");
				if (property_axes && property_keep_dims) {
					if (property_axes->GetIntArray().GetSize() &&
						property_keep_dims->GetIntArray().GetSize() == 1){
						if (property_input->GetIntArray().GetSize()) {
							FString result;
							for (int ia = 0; ia < property_axes->GetIntArray().GetSize() ; ia++) {
								const int a = property_axes->GetIntArray().GetAt(ia);
								if (a < -1 || a >= property_input->GetIntArray().GetSize() || property_axes->GetIntArray().IndexOf(a) != ia) {
									NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("Axes"));
									property.SetError(true,
										FString("Invalid axes \"") + FString(a) + FString("\""));
									return false;
								}
								if (a == -1 && !property_keep_dims->GetIntArray().GetAt(0)) {
									NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("KeepDims"));
									property.SetError(true,
										FString("When using -1 as axes, True must be set to keep_dims"));
									return false;
								}
							}
							for (int id = 0; id < property_input->GetIntArray().GetSize(); id++) {
								FString s;
								int found = false;
								for (int ia = 0; ia < property_axes->GetIntArray().GetSize() ; ia++) {
									found |= id == property_input->GetIntArray().GetSize() - property_axes->GetIntArray().GetAt(ia) - 1;
								}
								if (found) {
									if (property_keep_dims->GetIntArray().GetAt(0)) {
										s = "1";
									}
								}else {
									s = FString((int)property_input->GetIntArray().GetAt(id));
								}
								if (s != "") {
									if (result != "") {
										result = FString(",") + result;
									}
									result = s + result;
								}
							}
							if (result == "") {
								result = "1";
							}
							value = result;
							return true;
						}
					}
				}
				return false;
			}else if (macro == "*StackOutputSize") {
				const NNLayerProperty *property_inputs = GetPropertyByName("Inputs");
				const NNLayerProperty *property_axis = GetPropertyByName("Axis");
				if (property_inputs && property_axis) {
					if (property_inputs->GetIntArray().GetSize() &&
						property_axis->GetIntArray().GetSize() == 1){

						int num_input = 0;
						FString inputs_display_macro = property_inputs->GetDisplayValue();
						while (inputs_display_macro != ""){
							inputs_display_macro.SplitBy("),(");
							num_input ++;
						}

						if (num_input) {
							if ((property_inputs->GetIntArray().GetSize() + 1) / num_input > property_axis->GetIntArray().GetAt(0)){
								const int axis = property_inputs->GetIntArray().GetSize() / num_input - 1 - property_axis->GetIntArray().GetAt(0);

								const int dim = property_inputs->GetIntArray().GetSize() / num_input;
								int success = true;
								FString result;
								if (axis < dim && (property_inputs->GetIntArray().GetSize() + 1) % num_input == 0) {
									for (int id = 0 ; id < dim ; id++) {
										if (result != "") {
											result = FString(",") + result;
										}
										{
											const int elements = property_inputs->GetIntArray().GetAt(id);
											for (int ii = 1 ; ii < num_input ; ii++) {
												if (elements != property_inputs->GetIntArray().GetAt(ii * (dim + 1) + id)){
													success = false;
												}
											}
											result = FString(elements) + result;
										}
										if (id == axis) {
											result = FString(num_input) + FString(",") + result;
										}
									}
								}
								if (success) {
									value = result;
									return true;
								}
							}
						}
					}
				}
				return false;
			}else if (macro == "*SliceOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_start = GetPropertyByName("Start");
				const NNLayerProperty *property_stop = GetPropertyByName("Stop");
				const NNLayerProperty *property_step = GetPropertyByName("Step");
				if (property_start && property_stop && property_step) {
					const int dim = property_input->GetIntArray().GetSize();
					if (property_start->GetIntArray().GetSize() == dim &&
						property_stop->GetIntArray().GetSize() == dim &&
						property_step->GetIntArray().GetSize() == dim){
						int success = true;
						FString result;
						for (int id = 0 ; id < dim && success ; id++) {
							if (result != "") {
								result = FString(",") + result;
							}
							const int elements = property_input->GetIntArray().GetAt(id);
							int start = property_start->GetIntArray().GetAt(id);
							if (start == 0x7fffffff) {
								start = 0;
							}
							int stop = property_stop->GetIntArray().GetAt(id);
							if (stop == 0x7fffffff) {
								stop = property_input->GetIntArray().GetAt(id);
							}
							int step = property_step->GetIntArray().GetAt(id);
							if (step == 0x7fffffff) {
								step = 1;
							}
							if (step <= 0 || start >= stop ||
								start < 0 || start >= elements ||
								stop < 1 || stop > elements) {
								success = false;
							}else {
								result = FString((stop - start - 1) / step + 1) + result;
							}
						}
						if (success) {
							value = result;
							return true;
						}
					}
				}
				return false;
			}else if (macro == "*SumParameterSize") {
				int found = true;
				double result = 0.0;
				for (int i = 1; i < input_pin_connected_.GetSize() && found; i++) {
					if (GetPinEnable(0, i)){
						//int connected = input_pin_connected_.GetAt(i);
						const NNLayerSideConnector &side_connector = GetSideConnector(0, i - 1);
						if (side_connector.GetKind() == NNLayerComponentInputKind::Parameter) {
							FString macro_1v;
							EvalPropertyMacro(type, current_network, related_networks, property, side_connector.GetShape(), macro_1v, error);
							if (!GetInputPinConnected(i)){
								found &= macro_1v != "";
								result += StrToScalar(macro_1v);
							}else if (current_network){
								//Check size of input parameter
								const NNLayer *layer = current_network->GetLayerByID(GetInputPinConnected(i) & 0x3fffff);
								if (layer){
									if (layer->GetOutputPropertyIndex() >= 0){
										const NNLayerProperty &property = layer->GetProperty(layer->GetOutputPropertyIndex());
										if (property.GetDisplayValue() != ""){
											if (macro_1v != property.GetDisplayValue()){
												NNLayerProperty &input_property = properties_.GetAt(GetInputPropertyIndex());
												input_property.SetError(true,
													FString("Parameter shape of \"") +
													side_connector.GetName() +
													FString("\" mismatch. expected: ") +
													macro_1v +
													FString(", actual: ") +
													property.GetDisplayValue());
											}
										}else {
											//Size of input parameter is not calculated yet
											found = false;
										}
									}
								}
							}
						}
					}
				}
				if (found) {
					value = FString(result);
					return true;
				}
				return false;
			}
			break;
		case 'T':
        	if (macro == "*TransposeOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_axes = GetPropertyByName("Axes");
				if (property_axes) {
					const int dim = property_input->GetIntArray().GetSize();
					if (property_axes->GetIntArray().GetSize() == dim){
						int success = true;
						FString result;
						FListT<int> used_axis;
						for (int id = 0 ; id < dim && success ; id++) {
							if (result != "") {
								result = FString(",") + result;
							}
							const int axis = property_axes->GetIntArray().GetAt(id);
							if (axis < 0 || axis >= dim || used_axis.IndexOf(axis) >= 0) {
								success = false;
							}else {
								used_axis.Add(axis);
								result = FString((int)property_input->GetIntArray().GetAt(dim - 1 - axis)) + result;
							}
						}
						if (success) {
							value = result;
							return true;
						}
					}else {
						NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("Axes"));
						property.SetError(true,
							FString("The length of Axes property must be the same as Input."));
					}
				}
				return false;
			}else if (macro == "*TopKDataOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_k = GetPropertyByName("K");
				const NNLayerProperty *property_reduce = GetPropertyByName("Reduce");
				const NNLayerProperty *property_base_axis = GetPropertyByName("BaseAxis");
				if (property_k && property_reduce && property_base_axis) {
					const int dim = property_input->GetIntArray().GetSize();
					if (property_k->GetIntArray().GetSize() == 1 &&
						property_reduce->GetIntArray().GetSize() == 1 &&
						property_base_axis->GetIntArray().GetSize() == 1){
						if (property_reduce->GetIntArray().GetAt(0)){
							//Reduce
							const int base_axis = property_base_axis->GetIntArray().GetAt(0);
							if (base_axis < dim){
								FString result;
								for (int id = 0 ; id < base_axis ; id++) {
									if (result != "") {
										result = result + FString(",");
									}
									result = result + FString((int)property_input->GetIntArray().GetAt(dim - 1 - id));
								}
								if (result != "") {
									result = result + FString(",");
								}
								value = result + FString((int)property_k->GetIntArray().GetAt(0));
								return true;
							}
						}else {
							//Not reduce
							value = property_input->GetDisplayValue();
							return true;
						}
					}
				}
				return false;
			}
			break;
		case 'U':
			if (macro == "*UnitOutput") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				if (related_networks) if (related_networks->GetNumStructure() >= 2){
					int complete = true;
					int cache_enabled = inner_structure_args_ != NULL;
					const NNLayerProperty *property_network = NULL;
					for (int is = 0 ; is < GetNumSideConnector(0) ; is++){
						if (GetSideConnector(0, is).GetCurrentShape() == "") {
							complete = false;
							break;
						}
						if (cache_enabled) {
							if (inner_structure_args_->GetNumSideConnector(0) == GetNumSideConnector(0)){
								cache_enabled &= GetSideConnector(0, is).GetCurrentShape() == inner_structure_args_->GetSideConnector(0, is).GetCurrentShape();
							}else {
								cache_enabled = false;
							}
						}
					}
					for (int ip = 0 ; ip < GetNumProperty() && complete ; ip++){
						const NNLayerProperty &property = GetProperty(ip);
						int check_cache = false;
						if (property.GetDynamic()){
							check_cache = property.GetValue() != "*UnitOutput";
							if (property.GetUpdateRequested() && !check_cache){
								complete = false;
								break;
							}
						}else if (property.GetName() == "Input"){
							check_cache = true;
						}else if (property.GetName() == "Network"){
							property_network = &property;
						}

						if (cache_enabled && check_cache) {
							if (inner_structure_args_->GetNumProperty() == GetNumProperty()){
								const NNLayerProperty &cached_property = inner_structure_args_->GetProperty(ip);
								cache_enabled &= property.GetDisplayValue() == cached_property.GetDisplayValue();
							}else {
								cache_enabled = false;
							}
						}
					}
					if (complete && property_network){
						// Find inner network
						FString network_name = property_network->GetDisplayValue();
						const int network_index = related_networks->GetStructureIndexByName(network_name);
						if (network_name == current_network->GetName() || current_network->GetUsedStructure().IndexOf(network_name) >= 0){
							NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("Network"));
							property.SetError(true,
								FString("Circular reference error of the network \"") +
								property.GetDisplayValue() + FString("\""));
							ClearCache();
							return false;
						}else if (network_index >= 0){
							if (!cache_enabled){
								ClearCache();

								inner_structure_args_ = new NNLayer();
								*inner_structure_args_ = *this;
								inner_structure_ = new NNStructure();
								*inner_structure_ = related_networks->GetStructure(network_index);
								inner_structure_->SetRelatedNetworks(related_networks);
								FStringList used_structure = current_network->GetUsedStructure();
								used_structure.Add(current_network->GetName());
								inner_structure_->SetUsedStructure(used_structure);

								// Set args
								FStringList input_size;
								input_size.Add(property_input->GetDisplayValue());
								for (int is = 0 ; is < GetNumSideConnector(0) ; is++){
									input_size.Add(GetSideConnector(0, is).GetCurrentShape());
                                }
								inner_structure_->SetExportedArgs(input_size, *this);

								//Refresh display value
								FListT<int> terminal_layer_ids = inner_structure_->EnumTerminalLayerID();
								if (terminal_layer_ids.GetSize() == 1){
									inner_structure_->RefreshDisplayValue();
									inner_structure_->RefreshStatistics();
								}
							}

							FListT<NNLayer*> test_layer = inner_structure_->GetLayerByType(FString("TestNetwork"), true);
							if (test_layer.GetSize()) {
								NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("Network"));
								property.SetError(true,
									FString("TestNetwork layer found inside network \"") +
									property.GetDisplayValue() + FString("\""));
								return false;
							}

							//Get terminal layers
							FListT<int> terminal_layer_ids = inner_structure_->EnumTerminalLayerID();
							if (terminal_layer_ids.GetSize() == 1 && inner_structure_->GetCompleteNetwork()){
								//Get statistics
								const FListP<NNStatistics> statistics = inner_structure_->GetStatistics();
								for (int is = 0 ; is < statistics.GetSize() ; is++){
									const NNStatistics &statistics_1 = statistics.GetAt(is);
									if (statistics_1.GetLabel() != "Output"){
										const int property_index = GetPropertyIndexByName(statistics_1.GetLabel());
										if (property_index >= 0){
											SetPropertyDisplayValue(property_index, FString(statistics_1.GetSum()));
										}
									}
								}

								//Get size of terminal layer
								const NNLayer *terminal_layer = inner_structure_->GetLayerByID(terminal_layer_ids.GetAt(0));
								if (terminal_layer){
									if (terminal_layer->GetOutputPropertyIndex() >= 0){
										value = terminal_layer->GetProperty(terminal_layer->GetOutputPropertyIndex()).GetDisplayValue();
									}
									return value != "";
								}
							}
						}
						NNLayerProperty &property = properties_.GetAt(GetPropertyIndexByName("Network"));
						property.SetError(true,
							FString("An error occurred inside network \"") +
							property.GetDisplayValue() + FString("\""));
						return false;
					}
				}
				return false;
			}else if (macro == "*UnpoolingOutputSize" || macro == "*UpsamplingOutputSize") {
				const NNLayerProperty *property_input = GetAvailableInputProperty();
				if (!property_input){
					return false;
				}
				const NNLayerProperty *property_upsample =
					GetPropertyByName("KernelShape") ?
						GetPropertyByName("KernelShape") : (
							GetPropertyByName("Reps") ?
								GetPropertyByName("Reps") :
								(GetPropertyByName(macro == "*UnpoolingOutputSize" ? "PoolShape" : "UpsampleShape")));
				if (property_upsample) {
					if (property_upsample->GetIntArray().GetSize()) {

						FString result;
						int success = true;
						for (int id = 0 ; id < property_input->GetIntArray().GetSize() ; id++) {
							int elements = property_input->GetIntArray().GetAt(id);
							if (id < property_upsample->GetIntArray().GetSize()){
								elements *= property_upsample->GetIntArray().GetAt(id);
							}
							if (elements > 0) {
								if (result != "") {
									result = FString(",") + result;
								}
								result = FString(elements) + result;
							}else {
								success = false;
							}
						}
						if (success) {
							value = result;
							return true;
						}
					}
				}
				return false;
			}
			break;
		default:
			break;
		}//switch
	}
	if (macro.PChar()[0] == '*') {
		const FString linked_property_name = macro.SubString(2, macro.Length() - 1);
		const NNLayerProperty *property2 = GetPropertyByName(linked_property_name);
		if (property2) {
			if (property2->GetDisplayValue() != "") {
				value = property2->GetDisplayValue();
				return true;
			}else {
            	return false;
			}
		}else {
			//Search argument
			int found = false;
			FStringList visit_network;
			value = current_network->GetArgument(related_networks, linked_property_name, visit_network, found, error);
			if (found && error == "") {
				return true;
			}
		}
		//Error
		error = FString("Unknown property name or macro for property macro (") + macro + FString(")");
		return false;
	}
	return false;
}

//---------------------------------------------------------------------------
int NNLayer::GetPropertyIndexByName(const FString &name) const
{
	return GetPropertyIndexByName(name.PChar());
}
//---------------------------------------------------------------------------
int NNLayer::GetPropertyIndexByName(const char *name) const
{
	for (int ip = 0 ; ip < properties_.GetSize() ; ip++) {
		if (properties_.GetAt(ip).GetName() == name) {
			return ip;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
const NNLayerProperty *NNLayer::GetPropertyByName(const FString &name) const
{
	return GetPropertyByName(name.PChar());
}
//---------------------------------------------------------------------------
const NNLayerProperty *NNLayer::GetPropertyByName(const char *name) const
{
	const int index = GetPropertyIndexByName(name);
	if (index >= 0) {
		return &properties_.GetAt(index);
	}
	return NULL;
}
//---------------------------------------------------------------------------
int NNLayer::GetPropertyIndexByValue(const FString &value) const
{
	for (int ip = 0 ; ip < properties_.GetSize() ; ip++) {
		if (properties_.GetAt(ip).GetValue() == value) {
			return ip;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
int NNLayer::SetPropertyDisplayValue(int property_index, const FString &value)
{
	properties_.GetAt(property_index).SetDisplayValue(value);
	return true;
}
//---------------------------------------------------------------------------
void NNLayer::ClearDynamicProperty()
{
	for (int ip = properties_.GetSize() - 1; ip >= 0 ; ip--){
		if (properties_.GetAt(ip).GetDynamic()){
			properties_.Delete(ip);
		}
	}
	UpdatePropertyIndex();
}
//---------------------------------------------------------------------------
void NNLayer::FixProperties()
{
	for (int ip = 0 ; ip < properties_.GetSize() ; ip++){
		properties_.GetAt(ip).SetValue(properties_.GetAt(ip).GetDisplayValue());
	}
}
//---------------------------------------------------------------------------
const NNLayerSideConnector *NNLayer::GetSideConnectorByName(int output, const FString &name) const
{
	const int side_connector_index = GetSideConnectorIndexByName(output, name);

	if (side_connector_index >= 0) {
		return &GetSideConnector(output, side_connector_index);
	}else {
		return NULL;
	}
}
//---------------------------------------------------------------------------
int NNLayer::GetSideConnectorIndexByName(int output, const FString &name) const
{
	for (int is = 0 ; is < side_connectors_[output].GetSize() ; is++) {
		const NNLayerSideConnector &side_connector = side_connectors_[output].GetAt(is);
		if (side_connector.GetName() == name) {
			return is;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
void NNLayer::AddSideConnector(int output, int order, const FString &name, const FString &short_name, int color, const FString &enable, NNLayerComponentInputKind kind, const FString &shape, const FString &parameter_scope, int dynamic)
{
	NNLayerSideConnector &connector = side_connectors_[output].Add();
	connector.SetName(name);
    connector.SetOrder(order);
	connector.SetShortName(short_name);
	connector.SetColor(color);
	connector.SetEnable(enable);
	connector.SetKind(kind);
	connector.SetShape(shape);
	connector.SetParameterScope(parameter_scope);
	connector.SetDynamic(dynamic);

	// insertion sort
	int pos = side_connectors_[output].GetSize() - 2;
	while (pos >= 0){
		const int order1 = side_connectors_[output].GetAt(pos).GetOrder();
		const NNLayerComponentInputKind kind1 = side_connectors_[output].GetAt(pos).GetKind();
		const int order2 = side_connectors_[output].GetAt(pos + 1).GetOrder();
		const NNLayerComponentInputKind kind2 = side_connectors_[output].GetAt(pos + 1).GetKind();
		if (order2 >= 0 && (order1 > order2 || order1 < 0)) {//(kind1 == NNLayerComponentInputKind::Parameter && kind2 == NNLayerComponentInputKind::Data) || (
			side_connectors_[output].Swap(pos, pos + 1);
			pos--;
		}else{
			break;
		}
	}
}
//---------------------------------------------------------------------------
void NNLayer::ClearDynamicSideConnector()
{
	for (int io = 0 ; io < 2 ; io++){
		for (int ic = side_connectors_[io].GetSize() - 1; ic >= 0 ; ic--){
			if (side_connectors_[io].GetAt(ic).GetDynamic()){
				side_connectors_[io].Delete(ic);
			}
		}
	}
}
//---------------------------------------------------------------------------
void NNLayer::CheckError()
{
	errors_.Clear();

	for (int ip = 0 ; ip < GetNumProperty() ; ip++) {
		const NNLayerProperty &property = GetProperty(ip);
		if (property.GetError()) {
			NNError &error = errors_.Add();
			error.SetLayerName(GetDisplayPropertyValue(FString("Name")));
			error.SetPropertyName(property.GetName());
			error.SetString(
				error.GetLayerName() + FString("->") + property.GetName() +
				FString(" : ") + property.GetErrorMessage());
		}
	}

	for (int is = 0 ; is < GetNumSideConnector(0) ; is++) {
		const NNLayerSideConnector &side_connector = GetSideConnector(0, is);
		if (side_connector.GetKind() == NNLayerComponentInputKind::Data && side_connector.GetDynamic()) {
			if (side_connector.GetOrder() != -1 && side_connector.GetOrder() != is + 1){
				NNError &error = errors_.Add();
				error.SetLayerName(GetDisplayPropertyValue(FString("Name")));
				error.SetString(
					error.GetLayerName() +
					FString(" : The Order property is not specified correctly for multiple inputs in the Unit"));
			}
		}
	}
}
//---------------------------------------------------------------------------
void NNLayer::ClearCache()
{
	if (inner_structure_args_){
		delete inner_structure_args_;
		inner_structure_args_ = NULL;
	}
	if (inner_structure_){
		delete inner_structure_;
		inner_structure_ = NULL;
	}
}
//---------------------------------------------------------------------------
void NNLayer::ClearParameterFiles()
{
	for (int ip = 0 ; ip < GetNumProperty() ; ip++) {
		const NNLayerProperty &property = GetProperty(ip);
		if (property.GetName().Pos(".File")) {
			if (property.GetValue() != "*Default") {
				SetPropertyValue(ip, FString(""));
			}
		}
    }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
NNStructure::NNStructure() :
	next_layer_id_(0),
	next_link_id_(0),
	related_networks_(NULL),

	nn_layer_components_(NULL),
	nn_structure_search_policies_(NULL),

	update_count_(0),
	view_update_count_(0),
	current_layer_id_(-1),
	current_link_id_(-1),
	current_property_index_(-1),
	current_statistics_index_(-1),

	max_x_(0.0f),
	max_y_(0.0f),
	complete_network_(false),

	dragging_(false),
	editted_through_drag_(false),

	link_editing_(false),
	link_editing_dest_(false),
	editing_link_id_(-1)
{
	memset(selection_rect_, 0, sizeof(selection_rect_));
	memset(editing_link_pos_, 0, sizeof(editing_link_pos_));
	memset(editing_link_layer_id_, 0, sizeof(editing_link_layer_id_));
	memset(editing_link_pin_index_, 0, sizeof(editing_link_pin_index_));
	can_unitize_ = false;

	//Obsolete
	memset(use_, 0, sizeof(use_));
	use_[0] = true;//for training only by default
}
//---------------------------------------------------------------------------
NNStructure::~NNStructure()
{
	Clear();
}
//---------------------------------------------------------------------------
NNStructure &NNStructure::operator=(const NNStructure &source)
{
	name_ = source.name_;

	next_layer_id_ = source.next_layer_id_;
	next_link_id_ = source.next_link_id_;
	related_networks_ = source.related_networks_;
	used_structure_ = source.used_structure_;

	layers_ = source.layers_;
	links_ = source.links_;

	nn_layer_components_ = source.nn_layer_components_;
	nn_structure_search_policies_ = source.nn_structure_search_policies_;

	update_count_ = source.update_count_;
	view_update_count_ = source.view_update_count_;
	current_layer_id_ = source.current_layer_id_;
	current_link_id_ = source.current_link_id_;
	current_property_index_ = source.current_property_index_;
	current_property_name_ = source.current_property_name_;
	current_statistics_index_ = source.current_statistics_index_;

	max_x_ = source.max_x_;
	max_y_ = source.max_y_;

	errors_ = source.errors_;
	complete_network_ = source.complete_network_;

	statistics_ = source.statistics_;
	repeat_info_ = source.repeat_info_;

	edit_string_ = source.edit_string_;

	dragging_ = source.dragging_;
	editted_through_drag_ = source.editted_through_drag_;
	memcpy(selection_rect_, source.selection_rect_, sizeof(selection_rect_));

	link_editing_ = source.link_editing_;
	link_editing_dest_ = source.link_editing_dest_;
	editing_link_id_ = source.editing_link_id_;
	memcpy(editing_link_pos_, source.editing_link_pos_, sizeof(editing_link_pos_));
	memcpy(editing_link_layer_id_, source.editing_link_layer_id_, sizeof(editing_link_layer_id_));
	memcpy(editing_link_pin_index_, source.editing_link_pin_index_, sizeof(editing_link_pin_index_));

	can_unitize_ = source.can_unitize_;

	//Obsolete
	memcpy(use_, source.use_, sizeof(use_));

	return *this;
}
//---------------------------------------------------------------------------
int NNStructure::IsSame(const NNStructure &source) const
{
	if (layers_.GetSize() != source.layers_.GetSize() ||
		links_.GetSize() != source.links_.GetSize()) {
		return false;
	}
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (!layers_.GetAt(il).IsSame(source.layers_.GetAt(il))){
			return false;
		}
	}
	return true;
}
//---------------------------------------------------------------------------
void NNStructure::Clear()
{
	//Document
    name_ = "Main";

	layers_.Clear();
	links_.Clear();

	next_layer_id_ = 0;
	next_link_id_ = 0;
	related_networks_ = NULL;
	used_structure_.Clear();

	//Temporary
	current_layer_id_ = -1;
	current_link_id_ = -1;
	current_property_index_ = -1;
	current_property_name_ = "";
	current_statistics_index_ = -1;
	hover_layer_ids_.Clear();

	max_x_ = 0.0f;
	max_y_ = 0.0f;

	errors_.Clear();
	complete_network_ = false;
	statistics_.Clear();
	repeat_info_.Clear();
	edit_string_ = "";

	dragging_ = false;
	memset(selection_rect_, 0, sizeof(selection_rect_));

	link_editing_ = false;
	link_editing_dest_ = false;
	editing_link_id_ = -1;
	memset(editing_link_pos_, 0, sizeof(editing_link_pos_));
	memset(editing_link_layer_id_, 0, sizeof(editing_link_layer_id_));
	memset(editing_link_pin_index_, 0, sizeof(editing_link_pin_index_));

	can_unitize_ = false;

	//Obsolete
	memset(use_, 0, sizeof(use_));
	use_[0] = true;//for training only by default

	Update();
}
//---------------------------------------------------------------------------
void NNStructure::ClearCache()
{
	for (int i = 0 ; i < layers_.GetSize() ; i++) {
		layers_.GetAt(i).ClearCache();
	}
}
//---------------------------------------------------------------------------
int NNStructure::GetNumSelectedLayer() const
{
	int count = 0;
	for (int i = 0 ; i < layers_.GetSize() ; i++) {
		count += layers_.GetAt(i).GetSelected();
	}

	return count;
}
//---------------------------------------------------------------------------
int NNStructure::GetLayerIndexByID(int layer_id) const
{
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (layers_.GetAt(il).GetLayerID() == layer_id){
			return il;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
const NNLayer *NNStructure::GetLayerByID(int layer_id) const
{
	const int layer_index = GetLayerIndexByID(layer_id);
	if (layer_index >= 0){
		return &layers_.GetAt(layer_index);
	}
	return NULL;
}
//---------------------------------------------------------------------------
FListT<NNLayer *> NNStructure::GetLayerByPropertyValue(const FString &property, const FString &value, int first_one) const
{
	FListT<NNLayer *> result;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (layers_.GetAt(il).GetPropertyValue(FString(property)) == value) {
			result.Add(&layers_.GetAt(il));
			if (first_one){
				break;
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------
FListT<NNLayer *> NNStructure::GetLayerByDisplayPropertyValue(const FString &property, const FString &value, int first_one) const
{
	FListT<NNLayer *> result;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (layers_.GetAt(il).GetDisplayPropertyValue(FString(property)) == value) {
			result.Add(&layers_.GetAt(il));
			if (first_one){
				break;
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------
FListT<NNLayer *> NNStructure::GetLayerByType(const FString &type, int first_one) const
{
	FListT<NNLayer *> result;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (layers_.GetAt(il).GetType() == type) {
			result.Add(&layers_.GetAt(il));
			if (first_one){
				break;
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------
int NNStructure::GetLayerIDByName(const FString &name) const
{
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		if (layer.GetPropertyValue(FString("Name")) == name){
			return layer.GetLayerID();
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
FListT<int> NNStructure::EnumInputLayerID() const
{
	FListT<int> source_layer_ids, dest_layer_ids;
	for (int il = 0 ; il < links_.GetSize() ; il++) {
		const NNLink &link = links_.GetAt(il);
		source_layer_ids.Add(link.GetSourceLayerID());
		dest_layer_ids.Add(link.GetDestLayerID());
	}
	source_layer_ids.SortAndDeleteDuplex();
	dest_layer_ids.SortAndDeleteDuplex();

	FListT<int> result;

	for (int id = 0 ; id < source_layer_ids.GetSize() ; id++){
		const int layer_id = source_layer_ids.GetAt(id);
		if (dest_layer_ids.IndexOf(layer_id) < 0){
			result.Add(layer_id);
		}
	}

	return result;
}
//---------------------------------------------------------------------------
FListT<int> NNStructure::EnumTerminalLayerID() const
{
	FListT<int> source_layer_ids, dest_layer_ids;
	for (int il = 0 ; il < links_.GetSize() ; il++) {
		const NNLink &link = links_.GetAt(il);
		source_layer_ids.Add(link.GetSourceLayerID());
		dest_layer_ids.Add(link.GetDestLayerID());
	}
	source_layer_ids.SortAndDeleteDuplex();
	dest_layer_ids.SortAndDeleteDuplex();

	FListT<int> result;

	for (int id = 0 ; id < dest_layer_ids.GetSize() ; id++){
		const int layer_id = dest_layer_ids.GetAt(id);
		if (source_layer_ids.IndexOf(layer_id) < 0){
			result.Add(layer_id);
		}
	}

	return result;
}
//---------------------------------------------------------------------------
const NNLink *NNStructure::GetLinkByID(int link_id) const
{
	const int index = GetLinkIndex(link_id);
	return index < 0 ? NULL : &links_.GetAt(index);
}
//---------------------------------------------------------------------------
void NNStructure::SetRankRecursive(int layer_id, FListT<int> &rank, int current_rank) const
{
	//Set rank
	const int index = GetLayerIndexByID(layer_id);
	assert(index >= 0);
	if (rank.GetAt(index)){
		//rank already set
		return;
	}
	rank.SetAt(index, current_rank);

	const NNLayer *layer = GetLayerByID(layer_id);
	assert(layer);
	if (layer->GetType() == "RecurrentInput"){
		//Set same rank to delay layers with same id
		const FString id = layer->GetDisplayPropertyValue(FString("ID"));
		for (int i = 0 ; i < layers_.GetSize() ; i++){
			const NNLayer &layer2 = layers_.GetAt(i);
			if (layer2.GetType() == "Delay"){
				if (layer2.GetDisplayPropertyValue(FString("ID")) == id){
                    SetRankRecursive(layer2.GetLayerID(), rank, current_rank + 2);
				}
			}
		}
	}

	//Enum destination
	FListT<int> links = EnumLinkIDBySourceLayerID(layer_id, -1);
	for (int il = 0 ; il < links.GetSize(); il ++){
		const NNLink *link = GetLinkByID(links.GetAt(il));
		assert(link);
		const NNLayer *dest_layer = GetLayerByID(link->GetDestLayerID());
		assert(dest_layer);

		//Enum source layer of dest_layer
		FListT<int> dest_source_links = EnumLinkIDByDestLayerID(dest_layer->GetLayerID(), -1);
		int rank_for_dest_layer = current_rank + 1 + (links.GetSize() > 1 || dest_source_links.GetSize() > 1);
		int complete = true;
		for (int il2 = 0 ; il2 < dest_source_links.GetSize() ; il2++){
			const NNLink *dest_source_link = GetLinkByID(dest_source_links.GetAt(il2));
			assert(dest_source_link);
			if (dest_source_link->GetSourceLayerID() != layer_id){
				const NNLayer *dest_source_layer = GetLayerByID(dest_source_link->GetSourceLayerID());
				assert(dest_source_layer);

				const int dest_source_index = GetLayerIndexByID(dest_source_layer->GetLayerID());
				assert(dest_source_index >= 0);

				if (rank.GetAt(dest_source_index)) {
					if (rank_for_dest_layer < rank.GetAt(dest_source_index) + 2){
						rank_for_dest_layer = rank.GetAt(dest_source_index) + 2;
					}
				}else {
					complete = false;
				}

			}
		}

		if (complete){
			SetRankRecursive(dest_layer->GetLayerID(), rank, rank_for_dest_layer);
		}
	}
}
//---------------------------------------------------------------------------
FListT<int> NNStructure::GetMaxRankLayerIndexSequence(const FListT<int> &rank) const
{
	FListT<int> result;

	int global_max_rank = 0;
	int global_max_rank_index = -1;
	for (int i = 0 ; i < layers_.GetSize() ; i++){
		if (rank.GetAt(i) > global_max_rank){
			global_max_rank = rank.GetAt(i);
			global_max_rank_index = i;
		}
	}
	if (global_max_rank_index < 0){
		return result;
	}

	int layer_index = global_max_rank_index;
    int last_rank = global_max_rank;
	while (true){
		result.Add(layer_index);
		const NNLayer &layer = layers_.GetAt(layer_index);

		FListT<int> links = EnumLinkIDByDestLayerID(layer.GetLayerID(), -1);

		int min_rank = 0;
		int min_rank_index = -1;
        int min_pin_index = -1;
		for (int il = 0 ; il < links.GetSize(); il ++){
			const NNLink *link = GetLinkByID(links.GetAt(il));
			assert(link);
			const NNLayer *source_layer = GetLayerByID(link->GetSourceLayerID());
			assert(source_layer);
			const int source_layer_index = GetLayerIndexByID(source_layer->GetLayerID());
			if (rank.GetAt(source_layer_index) && rank.GetAt(source_layer_index) < last_rank){
				int update = false;
				if (min_pin_index < 0 || link->GetSourcePinIndex() < min_pin_index){
					update = true;
				}else if (link->GetDestPinIndex() == min_pin_index && (!min_rank || rank.GetAt(source_layer_index) < min_rank)){
					update = true;
				}

				if (update){
					min_rank = rank.GetAt(source_layer_index);
					min_rank_index = source_layer_index;
					min_pin_index = link->GetDestPinIndex();
				}
			}
		}

		if (min_rank_index < 0){
			break;
		}

		layer_index = min_rank_index;
		last_rank = rank.GetAt(layer_index);
	}

	result.Reverse();

	return result;
}
//---------------------------------------------------------------------------
int NNStructure::GetLinkIndexByDestLayerID(int dest_layer_id, int dest_pin_index) const
{
	int index = -1;
	for (int il = 0 ; il < links_.GetSize() ; il++) {
		if (links_.GetAt(il).GetDestLayerID() == dest_layer_id &&
			links_.GetAt(il).GetDestPinIndex() == dest_pin_index){
			index = il;
			break;
		}
	}

	return index;
}
//---------------------------------------------------------------------------
const NNLink *NNStructure::GetLinkByDestLayerID(int dest_layer_id, int dest_pin_index) const
{
	const int index = GetLinkIndexByDestLayerID(dest_layer_id, dest_pin_index);
	return index < 0 ? NULL : &links_.GetAt(index);
}
//---------------------------------------------------------------------------
int NNStructure::GetLinkIndex(int link_id) const
{
	for (int il = 0 ; il < links_.GetSize() ; il++) {
		if (links_.GetAt(il).GetLinkID() == link_id){
			return il;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
FListT<int> NNStructure::EnumLinkIDBySourceLayerID(int source_layer_id, int source_pin_index) const
{
	FListT<int> result;
	for (int il = 0 ; il < links_.GetSize() ; il++) {
		if (links_.GetAt(il).GetSourceLayerID() == source_layer_id &&
			(source_pin_index < 0 || links_.GetAt(il).GetSourcePinIndex() == source_pin_index)){
			result.Add(links_.GetAt(il).GetLinkID());
		}
	}

	return result;
}
//---------------------------------------------------------------------------
FListT<int> NNStructure::EnumLinkIDByDestLayerID(int dest_layer_id, int dest_pin_index) const
{
	FListT<int> result;
	FListT<int> pin;
	for (int il = 0 ; il < links_.GetSize() ; il++) {
		if (links_.GetAt(il).GetDestLayerID() == dest_layer_id &&
			(dest_pin_index < 0 || links_.GetAt(il).GetDestPinIndex() == dest_pin_index)){
			result.Add(links_.GetAt(il).GetLinkID());
			pin.Add(links_.GetAt(il).GetDestPinIndex());
		}
	}

	// Sort result with pin index
	for (int i = 1 ; i < result.GetSize() ; i++){
		int index = i;
		while (index > 0){
			if (pin.GetAt(index) < pin.GetAt(index - 1)){
				const int tmp = result.GetAt(index);
				result.SetAt(index, result.GetAt(index - 1));
				result.SetAt(index - 1, tmp);
				const int tmp2 = pin.GetAt(index);
				pin.SetAt(index, pin.GetAt(index - 1));
				pin.SetAt(index - 1, tmp2);
				index--;
			}else {
				break;
			}
		}
	}

	return result;
}
//---------------------------------------------------------------------------
int NNStructure::SearchLinkID(int source_layer_id, int source_pin_index, int dest_layer_id, int dest_pin_index) const
{
	for (int il = 0 ; il < links_.GetSize() ; il++) {
		if (links_.GetAt(il).GetSourceLayerID() == source_layer_id &&
			links_.GetAt(il).GetSourcePinIndex() == source_pin_index &&
			links_.GetAt(il).GetDestLayerID() == dest_layer_id &&
			links_.GetAt(il).GetDestPinIndex() == dest_pin_index){

			return links_.GetAt(il).GetLinkID();
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
int NNStructure::GetPinEnable(int layer_id, int output, int pin_index) const
{
	if (pin_index == 0) {
		return true;
	}

	const NNLayer *layer = GetLayerByID(layer_id);
	if (!layer) {
		return false;
	}

	return layer->GetPinEnable(output, pin_index);
}
//---------------------------------------------------------------------------
int NNStructure::GetPinVisible(int layer_id, int output, int pin_index) const
{
	if (pin_index == 0) {
		return true;
	}

	const NNLayer *layer = GetLayerByID(layer_id);
	if (!layer) {
		return false;
	}

	return layer->GetPinVisible(output, pin_index);
}
//---------------------------------------------------------------------------
int NNStructure::CanLinkFrom(int source_layer_id, int source_pin_index) const
{
	const NNLayer *source_layer = GetLayerByID(source_layer_id);
	if (source_layer == NULL) {
		return false;
	}

	const NNLayerComponent *source_layer_component = source_layer->GetComponent();
	if (source_layer_component == NULL) {
		return false;
	}

	FListT<int> link_from_source = EnumLinkIDBySourceLayerID(source_layer_id, source_pin_index);

	/*if (source_pin_index > 0 && link_from_source.GetSize()) {
		return false;
	}else */
	if (source_layer_component->GetOutputType() == NNLayerComponentIOType::None) {
		return false;
	}else if (
		(source_layer_component->GetOutputType() == NNLayerComponentIOType::Single ||
		source_layer_component->GetOutputType() == NNLayerComponentIOType::SingleOrNone) &&
		link_from_source.GetSize()) {
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::CanLinkTo(int dest_layer_id, int dest_pin_index) const
{
	const NNLayer *dest_layer = GetLayerByID(dest_layer_id);
	if (dest_layer == NULL) {
		return false;
	}

	const NNLayerComponent *dest_layer_component = dest_layer->GetComponent();
	if (dest_layer_component == NULL) {
		return false;
	}

	FListT<int> link_to_dest = EnumLinkIDByDestLayerID(dest_layer_id, dest_pin_index);

	if (dest_pin_index > 0 && link_to_dest.GetSize()) {
		//Second and third pin takes single input
		return false;
	}else if (dest_layer_component->GetInputType() == NNLayerComponentIOType::None) {
		return false;
	}else if (
		(dest_layer_component->GetInputType() == NNLayerComponentIOType::Single ||
		dest_layer_component->GetInputType() == NNLayerComponentIOType::SingleOrNone) &&
		link_to_dest.GetSize()) {
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::CanLink(int source_layer_id, int source_pin_index, int dest_layer_id, int dest_pin_index) const
{
	if (source_layer_id == dest_layer_id) {
        return false;
	}

	const int existing_link_id = SearchLinkID(source_layer_id, source_pin_index, dest_layer_id, dest_pin_index);
	if (existing_link_id >= 0) {
		return false;
	}

	if (!CanLinkFrom(source_layer_id, source_pin_index)) {
		return false;
	}

	if (!CanLinkTo(dest_layer_id, dest_pin_index)) {
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::GetLayerPropertyVisible(const NNLayer &layer, int property_index) const
{
	const NNLayerProperty &property = layer.GetProperty(property_index);
	if (!property.GetName().PosChar('.')) {
		return property.GetVisible();
	}else {
		FString property_name = property.GetName();
		const int pos = property.GetName().PosCharR('.');
		FString connector_name = property.GetName().SubString(1, pos - 1);

		int connector_index = layer.GetSideConnectorIndexByName(0, connector_name);
		if (connector_index >= 0 && connector_index < layer.GetNumSideConnector(0)) {
			return layer.GetPinEnable(0, connector_index + 1) && !layer.GetInputPinConnected(connector_index + 1);
		}
		return true;
	}
}
//---------------------------------------------------------------------------
FString NNStructure::GetArgument(const NNStructures *related_networks, const FString &argument_name, FStringList &visit_network, int &found, FString &error) const
{
	if (visit_network.IndexOf(name_) >= 0) {
		error = FString("Circular reference of networks by Import layer, ");
		for (int i = 0 ; i < visit_network.GetSize() ; i++) {
			error += visit_network.GetAt(i) + FString("->");
		}
		error += name_;
		return FString("");
	}
	visit_network.Add(name_);

	const FListT<NNLayer *> layers = GetLayerByPropertyValue(FString("Name"), argument_name, true);
	if (layers.GetSize() == 1){
		if (layers.GetAt(0)->GetType() == "Argument"){
			const NNLayerProperty *property = layers.GetAt(0)->GetPropertyByName("Value");
			if (property) {
				found = true;
				return property->GetDisplayValue();
			}
		}
	}

	for (int il = 0 ; il < layers_.GetSize() && error == "" ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		if (layer.GetType() == "Import") {
			const NNLayerProperty *property_network = layer.GetPropertyByName("Network");
			if (property_network){
				FString network_name = property_network->GetDisplayValue();
				const int network_index = related_networks->GetStructureIndexByName(network_name);
				if (network_index >= 0) {
					FString value = related_networks->GetStructure(network_index).GetArgument(related_networks, argument_name, visit_network, found, error);
					if (found) {
						return value;
					}
				}
			}
		}
	}
	return FString("");
}
//---------------------------------------------------------------------------
NNLayer *NNStructure::GetLayerByID_(int layer_id)
{
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (layers_.GetAt(il).GetLayerID() == layer_id){
			return &layers_.GetAt(il);
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
NNLink *NNStructure::GetLinkByID_(int link_id)
{
	const int index = GetLinkIndex(link_id);
	return index < 0 ? NULL : &links_.GetAt(index);
}
//---------------------------------------------------------------------------
void NNStructure::ClearSelection()
{
	for (int i = 0 ; i < layers_.GetSize() ; i++) {
		layers_.GetAt(i).SetSelected(false);
	}
	SetCurrentLayerID(-1);
	SetCurrentLinkID(-1);
	ViewUpdate();
}
//---------------------------------------------------------------------------
void NNStructure::InverseLayerSelection()
{
	SetCurrentLayerID(-1);
	SetCurrentLinkID(-1);
	for (int i = 0 ; i < layers_.GetSize() ; i++) {
		layers_.GetAt(i).SetSelected(!layers_.GetAt(i).GetSelected());
	}
	ViewUpdate();
}
//---------------------------------------------------------------------------
int NNStructure::GetLayerSelected(int layer_id) const
{
	const NNLayer *layer = GetLayerByID(layer_id);
	if (layer) {
		return layer->GetSelected();
	}else {
        return false;
    }
}
//---------------------------------------------------------------------------
void NNStructure::SetLayerSelected(int layer_id, int selected, int group)
{

	NNLayer *layer = GetLayerByID_(layer_id);
	//ToDo : Group selection
	if (layer) {
		layer->SetSelected(selected);
		ViewUpdate();
	}
}
//---------------------------------------------------------------------------
void NNStructure::SelectAllLayer()
{
	SetCurrentLinkID(-1);
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		NNLayer &layer = layers_.GetAt(il);
		layer.SetSelected(true);
	}
	ViewUpdate();
}
//---------------------------------------------------------------------------
void NNStructure::SetSelectionRect(const float *rect)
{
	memcpy(selection_rect_, rect, sizeof(selection_rect_));

	float selection_rect2[4];
	selection_rect2[0] = selection_rect_[0] < selection_rect_[2] ? selection_rect_[0] : selection_rect_[2];
	selection_rect2[1] = selection_rect_[1] < selection_rect_[3] ? selection_rect_[1] : selection_rect_[3];
	selection_rect2[2] = selection_rect_[0] > selection_rect_[2] ? selection_rect_[0] : selection_rect_[2];
	selection_rect2[3] = selection_rect_[1] > selection_rect_[3] ? selection_rect_[1] : selection_rect_[3];
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		NNLayer &layer = layers_.GetAt(il);
		if (layer.IsContainedInRect(selection_rect2)) {
			layer.SetSelected(true);
			/*
			if (GetCurrentLayerID() < 0) {
				SetCurrentLayerID(layer.GetLayerID());
			}
			*/
			SetCurrentLinkID(-1);
		}
	}
	ViewUpdate();
}
//---------------------------------------------------------------------------
void NNStructure::ResetSelectionRect()
{
	memset(selection_rect_, 0, sizeof(selection_rect_));
	ViewUpdate();
}
//---------------------------------------------------------------------------
int NNStructure::GetCurrentLayerID() const
{
	return current_layer_id_;
}
//---------------------------------------------------------------------------
int NNStructure::MoveCursor(int direction, int add_selection)
{
	float current_x = max_x_ / 2;
	float current_y = max_y_ / 2;

	static const double distance_weight_2 = 0.1;
	double dixtance_weight_x_2 = 1.0;
	double dixtance_weight_y_2 = 1.0;

	switch (direction) {
	case 0:
		//Left
		current_x = max_x_;
		dixtance_weight_x_2 = distance_weight_2;
		break;
	case 1:
		//Up
		current_y = max_y_;
		dixtance_weight_y_2 = distance_weight_2;
		break;
	case 2:
		//Right
		current_x = 0;
		dixtance_weight_x_2 = distance_weight_2;
		break;
	case 3:
		//Down
		current_y = 0;
		dixtance_weight_y_2 = distance_weight_2;
		break;
	}

	if (current_layer_id_ < 0) {
		for (int il = 0 ; il < GetNumLayer() ; il++){
			if (GetLayer(il).GetSelected()){
				SetCurrentLayerID(GetLayer(il).GetLayerID());
				break;
			}
		}
    }
	if (current_layer_id_ >= 0) {
		const NNLayer *layer = GetLayerByID(current_layer_id_);
		if (layer) {
			current_x = layer->GetPosition(0) + layer->GetPosition(2) * 0.5f;
			current_y = layer->GetPosition(1) + layer->GetPosition(3) * 0.5f;
		}
	}
	float min_distance = 1.0e+10f;
	int min_distance_index = -1;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		if (layer.GetLayerID() != current_layer_id_) {
			float x = layer.GetPosition(0) + layer.GetPosition(2) * 0.5f;
			float y = layer.GetPosition(1) + layer.GetPosition(3) * 0.5f;

			int target = false;
			switch (direction) {
			case 0:
				//Left
				target = x < current_x;
				break;
			case 1:
				//Up
				target = y < current_y;
				break;
			case 2:
				//Right
				target = x > current_x;
				break;
			case 3:
				//Down
				target = y > current_y;
				break;
			}
			if (target) {
				const float distance =
					sqrt((x - current_x) * (x - current_x) * dixtance_weight_x_2 +
						 (y - current_y) * (y - current_y) * dixtance_weight_y_2);
				if (distance < min_distance) {
					min_distance = distance;
					min_distance_index = il;
				}
			}
		}
	}
	if (min_distance_index >= 0) {
		if (!add_selection) {
			ClearSelection();
		}
		const int layer_id = layers_.GetAt(min_distance_index).GetLayerID();
		SetLayerSelected(layer_id, true, false);
		SetCurrentLayerID(layer_id);
		return true;
	}else {
		return false;
    }
}
//---------------------------------------------------------------------------
int NNStructure::GetCurrentCursorPosition(float *cursor_position)
{
	if (link_editing_) {
		cursor_position[0] = cursor_position[2] = editing_link_pos_[0];
		cursor_position[1] = cursor_position[3] = editing_link_pos_[1];
		return true;
	}

	if (selection_rect_[0] != selection_rect_[2] || selection_rect_[1] != selection_rect_[3]) {
		cursor_position[0] = cursor_position[2] = selection_rect_[2];
		cursor_position[1] = cursor_position[3] = selection_rect_[3];
		return true;
	}

	const int current_layer_id = GetCurrentLayerID();
	if (current_layer_id >= 0) {
		const NNLayer *layer = GetLayerByID(current_layer_id);
		if (layer) {
			cursor_position[0] = layer->GetCurrentPosition(0);
			cursor_position[1] = layer->GetCurrentPosition(1);
			cursor_position[2] = layer->GetCurrentPosition(0) + layer->GetCurrentPosition(2);
			cursor_position[3] = layer->GetCurrentPosition(1) + layer->GetCurrentPosition(3);
			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------------
void NNStructure::SetCurrentLayerID(int current_layer_id)
{
	if (current_layer_id_ == current_layer_id) {
		return;
	}
	current_layer_id_ = current_layer_id;

	current_property_index_ = -1;

	if (current_layer_id_ >= 0) {
		const NNLayer *layer = GetLayerByID(current_layer_id_);
		if (layer) {
			for (int ip = 0 ; ip < layer->GetNumProperty() ; ip++) {
				const NNLayerProperty &property = layer->GetProperty(ip);
				if (property.GetName() == current_property_name_) {
                    current_property_index_ = ip;
				}
			}
		}
	}
	ViewUpdate();
}
//---------------------------------------------------------------------------
int NNStructure::GetCurrentLinkID() const
{
	return current_link_id_;
}
//---------------------------------------------------------------------------
void NNStructure::SetCurrentLinkID(int current_link_id)
{
	if (current_link_id_ == current_link_id) {
		return;
	}
	current_link_id_ = current_link_id;
	ViewUpdate();
}
//---------------------------------------------------------------------------
int NNStructure::SetCurrentPropertyIndex(int current_property_index)
{
	if (current_layer_id_ < 0) {
		return false;
	}

	const NNLayer *layer = GetLayerByID(current_layer_id_);
	if (!layer) {
		return false;
	}

	if (current_property_index < -1 || current_property_index >= layer->GetNumProperty()){
		return false;
	}

	current_property_index_ = current_property_index;
	if (current_property_index_ >= 0) {
		current_property_name_ = layer->GetProperty(current_property_index_).GetName();
	}

	int new_statistics_index = GetStatisticsIndexByLabel(current_property_name_);
	if (new_statistics_index >= 0) {
		current_statistics_index_ = new_statistics_index;
	}
	ViewUpdate();

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::SetCurrentStatisticsIndex(int current_statistics_index)
{
	if (current_statistics_index < 0 || current_statistics_index >= statistics_.GetSize()) {
		return false;
	}
	current_statistics_index_ = current_statistics_index;

	current_property_index_ = -1;
	const FString name = statistics_.GetAt(current_statistics_index).GetLabel();
	const NNLayer *layer = GetLayerByID(current_layer_id_);
	if (layer) {
		int index = layer->GetPropertyIndexByName(name);
		current_property_index_ = index;
		current_property_name_ = name;
	}
	ViewUpdate();

	return true;
}
//---------------------------------------------------------------------------
void NNStructure::ClearInitialOffset()
{
	for (int il = 0 ; il < layers_.GetSize(); il++) {
		NNLayer &layer = layers_.GetAt(il);
		layer.SetInitialOffset(0, 0.0f);
		layer.SetInitialOffset(1, 0.0f);
	}
}
//---------------------------------------------------------------------------
void NNStructure::StartDrag()
{
	ClearInitialOffset();
	for (int il = 0 ; il < layers_.GetSize(); il++) {
		NNLayer &layer = layers_.GetAt(il);
		layer.SetDragged(layer.GetSelected());
	}
	dragging_ = true;
}
//---------------------------------------------------------------------------
int NNStructure::EndDrag()
{
	int result = editted_through_drag_;
	//Set current position
	for (int il = 0 ; il < layers_.GetSize(); il++) {
		NNLayer &layer = layers_.GetAt(il);
		if (layer.GetDragged()) {
			layer.SetCurrentPosition(layer.GetPosition());
			layer.SetDragged(false);

			//Check link update
			for (int il2 = 0 ; il2 < layers_.GetSize(); il2++) if (il2 != il){
				NNLayer &layer2 = layers_.GetAt(il2);
				if (!layer2.GetDragged()) {
					if (fabs(layer.GetPosition(0) - layer2.GetPosition(0)) < 0.5f) {
						if (fabs(layer.GetPosition(1) - (layer2.GetPosition(1) + layer2.GetPosition(3))) < 0.5f) {
							//layer2 -> layer

							//Delete old link
							FListT<int> link_from_source = EnumLinkIDBySourceLayerID(layer2.GetLayerID(), 0);
							FListT<int> link_to_dest = EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
							int link_id = SearchLinkID(layer2.GetLayerID(), 0, layer.GetLayerID(), 0);
							for (int ili = 0 ; ili < link_from_source.GetSize() ; ili++) {
								if (link_from_source.GetAt(ili) != link_id) {
									links_.Delete(GetLinkIndex(link_from_source.GetAt(ili)));
								}
							}
							for (int ili = 0 ; ili < link_to_dest.GetSize() ; ili++) {
								if (link_to_dest.GetAt(ili) != link_id) {
									links_.Delete(GetLinkIndex(link_to_dest.GetAt(ili)));
								}
							}
							AddLink(layer2.GetLayerID(), 0, layer.GetLayerID(), 0);
							result = true;
						}else if (fabs(layer2.GetPosition(1) - (layer.GetPosition(1) + layer.GetPosition(3))) < 0.5f){
							//layer -> layer2

							//Delete old link
							FListT<int> link_from_source = EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
							FListT<int> link_to_dest = EnumLinkIDByDestLayerID(layer2.GetLayerID(), 0);
							int link_id = SearchLinkID(layer.GetLayerID(), 0, layer2.GetLayerID(), 0);
							for (int ili = 0 ; ili < link_from_source.GetSize() ; ili++) {
								if (link_from_source.GetAt(ili) != link_id) {
									links_.Delete(GetLinkIndex(link_from_source.GetAt(ili)));
								}
							}
							for (int ili = 0 ; ili < link_to_dest.GetSize() ; ili++) {
								if (link_to_dest.GetAt(ili) != link_id) {
									links_.Delete(GetLinkIndex(link_to_dest.GetAt(ili)));
								}
							}
							AddLink(layer.GetLayerID(), 0, layer2.GetLayerID(), 0);
							result = true;
						}
					}
				}
			}

		}
	}
	Update();

	dragging_ = false;
	return result;
}
//---------------------------------------------------------------------------
void NNStructure::Drag(float dx, float dy)
{
	for (int il = 0 ; il < layers_.GetSize(); il++) {
		NNLayer &layer = layers_.GetAt(il);
		if (layer.GetDragged()) {
			float x = layer.GetCurrentPosition(0);
			float y = layer.GetCurrentPosition(1);
			x += dx;
			y += dy;
			layer.SetPosition(0, (float)((int)(x / NNLayerDefaultGrid + 0.5) * NNLayerDefaultGrid));
			layer.SetPosition(1, (float)((int)(y / NNLayerDefaultGrid + 0.5) * NNLayerDefaultGrid));
			layer.SetCurrentPosition(0, x);
			layer.SetCurrentPosition(1, y);
			editted_through_drag_ = true;
		}
	}
	for (int il = 0 ; il < layers_.GetSize(); il++) {
		NNLayer &layer = layers_.GetAt(il);
		if (layer.GetDragged()) {
			Escape(il);
		}
	}
	ViewUpdate();
}
//---------------------------------------------------------------------------
int NNStructure::StartLinkEdit_New(int source_layer_id, int source_pin_index, float x, float y)
{
	if (!CanLinkFrom(source_layer_id, source_pin_index)){
		return false;
	}

	link_editing_ = true;
	link_editing_dest_ = true;
	editing_link_id_ = -1;
	editing_link_pos_[0] = x;
	editing_link_pos_[1] = y;
	editing_link_layer_id_[0] = source_layer_id;
	editing_link_layer_id_[1] = -1;
	editing_link_pin_index_[0] = 0;
	editing_link_pin_index_[1] = 0;

	ClearSelection();
	SetCurrentLayerID(editing_link_layer_id_[!link_editing_dest_]);
	SetLayerSelected(editing_link_layer_id_[!link_editing_dest_], true, false);

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::StartLinkEdit_Existing(int link_id, int dest, float x, float y)
{
	const NNLink *link = GetLinkByID(link_id);
	if (!link) {
		return false;
	}

	link_editing_ = true;
	link_editing_dest_ = dest;
	editing_link_id_ = link_id;
	editing_link_pos_[0] = x;
	editing_link_pos_[1] = y;
	editing_link_layer_id_[0] = link->GetSourceLayerID();
	editing_link_pin_index_[0] = link->GetSourcePinIndex();
	editing_link_layer_id_[1] = link->GetDestLayerID();
	editing_link_pin_index_[1] = link->GetDestPinIndex();

	ClearSelection();
	SetCurrentLayerID(editing_link_layer_id_[!link_editing_dest_]);
	SetLayerSelected(editing_link_layer_id_[0], true, false);
	SetLayerSelected(editing_link_layer_id_[1], true, false);

	return true;
}
//---------------------------------------------------------------------------
void NNStructure::DragLink(float x, float y, int snap_layer_id, int snap_input_pin_index, int snap_output_pin_index)
{
	editing_link_pos_[0] = x;
	editing_link_pos_[1] = y;
	editing_link_layer_id_[link_editing_dest_] = -1;
	editing_link_pin_index_[link_editing_dest_] = 0;

	int original_source_layer_id_ = -1;
	int original_source_pin_index_ = -1;
	int original_dest_layer_id_ = -1;
	int original_dest_pin_index_ = -1;
	const NNLink *editing_link = GetLinkByID(editing_link_id_);
	if (editing_link) {
		original_source_layer_id_ = editing_link->GetSourceLayerID();
		original_source_pin_index_ = editing_link->GetSourcePinIndex();
		original_dest_layer_id_ = editing_link->GetDestLayerID();
		original_dest_pin_index_ = editing_link->GetDestPinIndex();
	}

	if (snap_layer_id >= 0 && editing_link_layer_id_[!link_editing_dest_] != snap_layer_id) {
		if (link_editing_dest_) {
			//editing dest
			if ((snap_layer_id == original_dest_layer_id_ && snap_input_pin_index == original_dest_pin_index_) ||
				CanLinkTo(snap_layer_id, snap_input_pin_index)) {
				editing_link_layer_id_[1] = snap_layer_id;
				editing_link_pin_index_[1] = snap_input_pin_index;
			}
		}else {
			//editing source
			if ((snap_layer_id == original_source_layer_id_ && snap_output_pin_index == original_source_pin_index_) ||
				CanLinkFrom(snap_layer_id, snap_output_pin_index)) {
				editing_link_layer_id_[0] = snap_layer_id;
				editing_link_pin_index_[0] = snap_output_pin_index;
			}
		}
	}
	int can_snap_to_existing = false;
	if (editing_link) {
		if (editing_link->GetSourceLayerID() == editing_link_layer_id_[0] && editing_link->GetDestLayerID() == editing_link_layer_id_[1]){
			can_snap_to_existing = true;
		}
	}
	if (!can_snap_to_existing) {
		const int existing_link_id = SearchLinkID(editing_link_layer_id_[0], editing_link_pin_index_[0], editing_link_layer_id_[1], editing_link_pin_index_[1]);
		if (existing_link_id >= 0) {
        	editing_link_layer_id_[link_editing_dest_] = -1;
		}
	}

	ClearSelection();
	if (editing_link_id_ >= 0) {
		SetCurrentLinkID(editing_link_id_);
    }
	SetCurrentLayerID(editing_link_layer_id_[!link_editing_dest_]);
	if (editing_link_layer_id_[0] >= 0){
		SetLayerSelected(editing_link_layer_id_[0], true, false);
	}
	if (editing_link_layer_id_[1] >= 0){
		SetLayerSelected(editing_link_layer_id_[1], true, false);
    }

	ViewUpdate();
}
//---------------------------------------------------------------------------
int NNStructure::EndLinkEdit()
{
	int result = false;
	ClearSelection();
	if (editing_link_id_ >= 0) {
		//Existing Link
		if (editing_link_layer_id_[link_editing_dest_] >= 0){
			NNLink *link = GetLinkByID_(editing_link_id_);
			if (link) {
				const int existing = SearchLinkID(editing_link_layer_id_[0], editing_link_pin_index_[0], editing_link_layer_id_[1], editing_link_pin_index_[1]);
				if (existing < 0) {
					if (link_editing_dest_) {
						GetLinkByID_(editing_link_id_)->SetDestLayerID(editing_link_layer_id_[1]);
						GetLinkByID_(editing_link_id_)->SetDestPinIndex(editing_link_pin_index_[1]);
						result = true;
					}else {
						GetLinkByID_(editing_link_id_)->SetSourceLayerID(editing_link_layer_id_[0]);
						GetLinkByID_(editing_link_id_)->SetSourcePinIndex(editing_link_pin_index_[0]);
						result = true;
					}
				}
			}
		}
		SetCurrentLinkID(editing_link_id_);
	}else {
		//New Link
		if (editing_link_layer_id_[link_editing_dest_] >= 0){
			NNLink *link = AddLink(
				editing_link_layer_id_[0],
				editing_link_pin_index_[0],
				editing_link_layer_id_[1],
				editing_link_pin_index_[1]);
			if (link) {
				SetCurrentLinkID(link->GetLinkID());
			}
			result = true;
		}
	}
	link_editing_ = false;
	editing_link_id_ = -1;

	memset(editing_link_pos_, 0, sizeof(editing_link_pos_));

	Update();

	return result;
}
//---------------------------------------------------------------------------
void NNStructure::ArrangeLayers(int sort)
{
	FListP<FListT<int> > input_link;
	FListP<FListT<int> > output_link;
	FListT<int> arranged;
	FListT<int> rank;
	for (int i = 0 ; i < layers_.GetSize() ; i++){
		arranged.Add(0);
		rank.Add(0);
	}

	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		NNLayer &layer = layers_.GetAt(il);
		input_link.Add(EnumLinkIDByDestLayerID(layer.GetLayerID(), -1));
		output_link.Add(EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0));

        //Set layer position to avoid escape
		layer.SetPosition(0, -NNLayerDefaultWidth - NNLayerDefaultHSpace);
		layer.SetPosition(1, NNLayerDefaultHeight);
		layer.SetCurrentPosition(layer.GetPosition());
	}

	double max_y = 0.0;
	double next_x = NNLayerDefaultSpace;
	FListT<int> arranged_layer_ids;

	//Allocate argument layers
	{
		int x = next_x;
		int y = NNLayerDefaultSpace;
		while (true){
			int allocated = false;
			for (int il = 0 ; il < layers_.GetSize() ; il++) if (!arranged.GetAt(il)){
				NNLayer &layer = layers_.GetAt(il);
				if (layer.GetType() == "Argument") {
					layer.SetPosition(0, x);
					layer.SetPosition(1, y);
					next_x = x + NNLayerDefaultWidth + NNLayerDefaultHSpace;
					y += NNLayerDefaultHeight;
					layer.SetCurrentPosition(layer.GetPosition());
					arranged.SetAt(il, true);
					allocated = true;
					arranged_layer_ids.Add(layer.GetLayerID());
					break;
				}
			}
			if (!allocated) {
				break;
			}else if (max_y < y) {
				max_y = y;
			}
		}
	}

	//Set rank
	FListT<int> input_layers = EnumInputLayerID();
	for (int i = 0 ; i < input_layers.GetSize() ; i++){
		SetRankRecursive(input_layers.GetAt(i), rank, 1);
	}
	FListT<int> rank_bak = rank;

	int max_rank = 0;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (rank_bak.GetAt(il) > max_rank){
			max_rank = rank_bak.GetAt(il);
		}
	}
	max_rank++;
	FListT<double> rank_max_x;
	for (int ir = 0 ; ir < max_rank ; ir++){
		rank_max_x.Add(next_x);
	}

	while (true){
		FListT<int> layer_index_sequence = GetMaxRankLayerIndexSequence(rank);

		if (!layer_index_sequence.GetSize()){
			break;
		}

		double max_x = next_x;
		int min_local_rank = rank_bak.GetAt(layer_index_sequence.GetAt(0));
		int max_local_rank = rank_bak.GetAt(layer_index_sequence.GetAt(layer_index_sequence.GetSize() - 1));

		//Check min_local_rank
		{
			//check revious layer
			const NNLayer &initial_layer = layers_.GetAt(layer_index_sequence.GetAt(0));
			FListT<int> links = EnumLinkIDByDestLayerID(initial_layer.GetLayerID(), -1);
			for (int i = 0 ; i < links.GetSize() ; i++) {
				const NNLink *link = GetLinkByID(links.GetAt(i));
				assert(link);
				const int previous_layer_index = GetLayerIndexByID(link->GetSourceLayerID());
				assert(previous_layer_index >= 0);
				if (rank_bak.GetAt(previous_layer_index) < min_local_rank){
					min_local_rank = rank_bak.GetAt(previous_layer_index);
				}
				if (rank_bak.GetAt(previous_layer_index) > max_local_rank){
					max_local_rank = rank_bak.GetAt(previous_layer_index);
				}
				if (rank_max_x.GetAt(rank_bak.GetAt(previous_layer_index)) > max_x){
					max_x = rank_max_x.GetAt(rank_bak.GetAt(previous_layer_index));
				}
			}

		}

		//Check max_local_rank
		{
			//check revious layer
			const NNLayer &terminal_layer = layers_.GetAt(layer_index_sequence.GetAt(layer_index_sequence.GetSize() - 1));
			FListT<int> links = EnumLinkIDBySourceLayerID(terminal_layer.GetLayerID(), -1);
			for (int i = 0 ; i < links.GetSize() ; i++) {
				const NNLink *link = GetLinkByID(links.GetAt(i));
				assert(link);
				const int next_layer_index = GetLayerIndexByID(link->GetDestLayerID());
				assert(next_layer_index >= 0);
				if (rank_bak.GetAt(next_layer_index) < min_local_rank){
					min_local_rank = rank_bak.GetAt(next_layer_index);
				}
				if (rank_bak.GetAt(next_layer_index) > max_local_rank){
					max_local_rank = rank_bak.GetAt(next_layer_index);
				}
				if (rank_max_x.GetAt(rank_bak.GetAt(next_layer_index)) > max_x){
					max_x = rank_max_x.GetAt(rank_bak.GetAt(next_layer_index));
				}
			}

		}

		//set rank_max_x
		for (int ir = min_local_rank ; ir <= max_local_rank ; ir++){
			if (max_x < rank_max_x.GetAt(ir)){
				max_x = rank_max_x.GetAt(ir);
			}
		}

		//set position
		for (int i = 0 ; i < layer_index_sequence.GetSize() ; i++){
			const int layer_index = layer_index_sequence.GetAt(i);
			const int layer_rank = rank_bak.GetAt(layer_index);
			rank.SetAt(layer_index, 0);

			NNLayer &layer = layers_.GetAt(layer_index);
			layer.SetPosition(0, max_x);
			layer.SetPosition(1, NNLayerDefaultSpace + (layer_rank - 1) * NNLayerDefaultHeight);
			layer.SetCurrentPosition(layer.GetPosition());
			if (layer.GetPosition(1) + layer.GetPosition(3) + NNLayerDefaultSpace > max_y){
				max_y = layer.GetPosition(1) + layer.GetPosition(3) + NNLayerDefaultSpace;
			}
			layer.SetCurrentPosition(layer.GetPosition());
			arranged.SetAt(layer_index, true);
			arranged_layer_ids.Add(layer.GetLayerID());
		}

		//update rank_max_x
		for (int ir = min_local_rank ; ir <= max_local_rank ; ir++){
			rank_max_x.SetAt(ir, max_x + NNLayerDefaultWidth + NNLayerDefaultHSpace);
		}
	}

	//Allocate remaining layers
	{
		int x = NNLayerDefaultSpace;
		int y = max_y + NNLayerDefaultSpace;
		for (int il = 0 ; il < layers_.GetSize() ; il++) if (!arranged.GetAt(il)){
			NNLayer &layer = layers_.GetAt(il);
			layer.SetPosition(0, x);
			layer.SetPosition(1, y);
			layer.SetCurrentPosition(layer.GetPosition());
			arranged.SetAt(il, true);
			arranged_layer_ids.Add(layer.GetLayerID());

			y += NNLayerDefaultHeight + NNLayerDefaultVSpace;
		}
	}

	if (sort) for (int il = 1 ; il < layers_.GetSize() ; il++) {
		for (int il2 = il ; il2 > 0 ; il2--){
			if (rank_bak.GetAt(il2) < rank_bak.GetAt(il2 - 1)){
				int tmp = rank_bak.GetAt(il2 - 1);
				rank_bak.SetAt(il2 - 1, rank_bak.GetAt(il2));
				rank_bak.SetAt(il2, tmp);
				layers_.Swap(il2 - 1, il2);
			}
		}
	}

	Update();
}
//---------------------------------------------------------------------------
int NNStructure::Replace(FString from, FString to, int target_selection)
{
	const int component_index = nn_layer_components_->GetComponentIndexByType(to);
	FListT<int> selected_layer_ids;
	FListT<int> layer_id_to_delete, layer_id_to_select;
	const int num_layer_bak = layers_.GetSize();
	for (int il = 0 ; il < num_layer_bak ; il++) {
		NNLayer &layer = layers_.GetAt(il);
		if (layer.GetSelected()) {
			selected_layer_ids.Add(layer.GetLayerID());
		}
	}
	for (int il = 0 ; il < num_layer_bak ; il++) {
		NNLayer &org_layer = layers_.GetAt(il);
		if (org_layer.GetType() == from && (!target_selection || selected_layer_ids.IndexOf(org_layer.GetLayerID()) >= 0)) {
			FString name_index = org_layer.GetName();
			FString name = name_index.SplitBy("_");
			const int replace_name = name == org_layer.GetType();
			layer_id_to_select.Add(org_layer.GetLayerID());
			NNLayer &new_layer = AddLayer(component_index);
			const int layer_id_bak = new_layer.GetLayerID();
			new_layer.SetLayerID(org_layer.GetLayerID());
			org_layer.SetLayerID(layer_id_bak);
			layer_id_to_delete.Add(layer_id_bak);
			new_layer.SetPosition(org_layer.GetPosition());
			new_layer.SetCurrentPosition(org_layer.GetPosition());
			new_layer.SetInitialOffset(0, 0.0f);
			new_layer.SetInitialOffset(1, 0.0f);
			FString layer_name_bak = new_layer.GetPropertyValue(FString("Name"));
			new_layer.SetPropertyValueFrom(org_layer);
			if (replace_name) {
				new_layer.SetPropertyValue(FString("Name"), layer_name_bak);
			}
		}
	}

	ClearSelection();
	for (int i = 0 ; i < layer_id_to_delete.GetSize() ; i++) {
		SetLayerSelected(layer_id_to_delete.GetAt(i), true, false);
	}
	DeleteSelected();
	for (int i = 0 ; i < layer_id_to_select.GetSize() ; i++) {
		SetLayerSelected(layer_id_to_select.GetAt(i), true, false);
	}

    return layer_id_to_select.GetSize();
}
//---------------------------------------------------------------------------
NNLayer& NNStructure::AddLayer(int component_index, int auto_link, float x, float y, int escape)
{
	const NNLayerComponent &component = nn_layer_components_->GetAt(component_index);

	const int new_layer_index = layers_.GetSize();
	NNLayer &new_layer = layers_.Add();
	new_layer.SetLayerID(next_layer_id_++);
	new_layer.SetComponent(&component);

	for (int ip = 0 ; ip < component.GetNumProperty() ; ip++) {
		const NNLayerComponentProperty &property = component.GetProperty(ip);
		new_layer.AddProperty(
			-1,
			property.GetName(),
			property.GetShortName(),
			property.GetEditable(),
			property.GetImportant(),
			property.GetRequired(),
			property.GetSearchParameter(),
			(NNLayerPropertyType)property.GetType(),
			property.GetDefaultValue(),
			property.GetVisible(),
			false,
			false
			);
	}
	new_layer.UpdatePropertyIndex();
	for (int io = 0 ; io < 2 ; io++) {
		for (int ic = 0 ; ic < component.GetNumSideConnector(io) ; ic++){
			const NNLayerComponentSideConnector &connector = component.GetSideConnector(io, ic);
			new_layer.AddSideConnector(
				io,
                ic,
				connector.GetName(),
				connector.GetShortName(),
				connector.GetColor(),
				connector.GetEnable(),
				connector.GetKind(),
				connector.GetShape(),
				connector.GetParameterScope()
				);
		}
	}

	EscapeLayerName(layers_.GetSize() - 1);

	int auto_linked = false;
	if (auto_link && current_layer_id_ >= 0 && x < 0.0f && y < 0.0f) {
		if (component.GetInputType() != NNLayerComponentIOType::None) {
			const NNLayer *parent = GetLayerByID(current_layer_id_);
			if (parent) {
				const NNLayerComponent *parent_component = parent->GetComponent();
				if (parent_component) {
					if (parent_component->GetOutputType() == NNLayerComponentIOType::Multiple ||
						parent_component->GetOutputType() == NNLayerComponentIOType::Any) {
						//Auto Link
						AddLink(current_layer_id_, 0, new_layer.GetLayerID(), 0);

						const NNLayer* source_layer = GetLayerByID(current_layer_id_);
						new_layer.SetPosition(source_layer->GetPosition());
						new_layer.SetPosition(1, source_layer->GetPosition(1) + source_layer->GetPosition(3));
						auto_linked = true;
					}
				}
			}
		}
	}
	if (!auto_linked){
		float new_x = NNLayerDefaultSpace;
		float new_y = max_y_ + NNLayerDefaultVSpace;
		if (x >= 0.0f && y >= 0.0f) {
			x -= NNLayerDefaultWidth / 2;
			y -= NNLayerDefaultHeight / 2;
			if (escape) {
				if (x < 0.0f) {
					x = 0.0f;
				}
				if (y < 0.0f) {
					y = 0.0f;
				}
				new_x = (int)(x / NNLayerDefaultGrid + 0.5) * NNLayerDefaultGrid;
				new_y = (int)(y / NNLayerDefaultGrid + 0.5) * NNLayerDefaultGrid;
			}else {
				new_x = x;
				new_y = y;
			}
		}else if (component.GetDefaultPositionVertical() == NNLayerComponentDefaultPosition::Top) {
			new_y = NNLayerDefaultSpace;
		}
		float position[4] = {
			new_x, new_y,
			NNLayerDefaultWidth, NNLayerDefaultHeight};
		new_layer.SetPosition(position);
	}

	ClearSelection();
	SetCurrentLayerID(new_layer.GetLayerID());
	new_layer.SetSelected(true);
	new_layer.SetDragged(false);

	if (escape) {
		Escape(new_layer_index);
    }

	new_layer.SetCurrentPosition(new_layer.GetPosition());
	new_layer.SetInitialOffset(0, 0.0f);
	new_layer.SetInitialOffset(1, 0.0f);

	Update();

	return new_layer;
}
//---------------------------------------------------------------------------
NNLink *NNStructure::AddLink(int source_layer_id, int source_pin_index, int dest_layer_id, int dest_pin_index, int force)
{
	if (!force && !CanLink(source_layer_id, source_pin_index, dest_layer_id, dest_pin_index)){
		return NULL;
	}
	NNLink &new_link = links_.Add();
	new_link.SetLinkID(next_link_id_++);
	new_link.SetSourceLayerID(source_layer_id);
	new_link.SetSourcePinIndex(source_pin_index);
	new_link.SetDestLayerID(dest_layer_id);
	new_link.SetDestPinIndex(dest_pin_index);

	Update();

	return &new_link;
}
//---------------------------------------------------------------------------
int NNStructure::DeleteSelected()
{
	int deleted_num = 0;

	//Layer
	for (int il = layers_.GetSize() - 1 ; il >= 0 ; il--) {
		NNLayer &layer = layers_.GetAt(il);
		if (layer.GetSelected()) {
			const int layer_id = layer.GetLayerID();
			for (int ili = links_.GetSize() - 1 ; ili >= 0; ili--) {
				NNLink &link = links_.GetAt(ili);
				if (link.GetSourceLayerID() == layer_id || link.GetDestLayerID() == layer_id) {
					links_.Delete(ili);
				}
			}

			if (current_layer_id_ == layer.GetLayerID()) {
				current_layer_id_ = -1;
			}
			layers_.Delete(il);
			deleted_num++;
		}
	}

	//Link
	if (GetCurrentLinkID() >= 0) {
		int index = GetLinkIndex(GetCurrentLinkID());
		if (index >= 0) {
			links_.Delete(index);
			deleted_num++;
		}
	}

	Update();

	return deleted_num;
}
//---------------------------------------------------------------------------
int NNStructure::SetLayerPropertyValue(int layer_id, int property_index, const FString &value, int escape, int force)
{
	const int layer_index = GetLayerIndexByID(layer_id);
	if (layer_index < 0) {
		return false;
	}
	NNLayer &layer = layers_.GetAt(layer_index);

	layer.SetPropertyValue(property_index, value, force);
	if (escape && layer.GetProperty(property_index).GetName() == "Name") {
		EscapeLayerName(layer_index);
	}
	Update();

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::SetLayerPropertyValue(int layer_id, const FString &property_name, const FString &value, int escape, int force)
{
	const int layer_index = GetLayerIndexByID(layer_id);
	if (layer_index < 0) {
		return false;
	}
	NNLayer &layer = layers_.GetAt(layer_index);

	const int property_index = layer.GetPropertyIndexByName(property_name);
	if (property_index < 0) {
        return false;
	}
	layer.SetPropertyValue(property_index, value, force);
	if (escape && layer.GetProperty(property_index).GetName() == "Name") {
		EscapeLayerName(layer_index);
	}
	Update();

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::Paste(const NNStructure &structure, bool escape, FString allowed_char, bool escape_position)
{
	double min_x = 0.0;
	int min_layer_id = 0;
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);
		if (il == 0 || min_x > layer.GetPosition(0)) {
			min_x = layer.GetPosition(0);
		}
		if (il == 0 || min_layer_id > layer.GetLayerID()) {
			min_layer_id = layer.GetLayerID();
		}
	}
	int min_link_id = 0;
	for (int ili = 0 ; ili < structure.GetNumLink() ; ili++) {
		const NNLink &link = structure.GetLink(ili);
		if (ili == 0 || min_link_id > link.GetLinkID()) {
			min_link_id = link.GetLinkID();
		}
	}

	const double x_escape_amount = escape_position ? (layers_.GetSize() ? max_x_ + NNLayerDefaultHSpace - min_x : 0.0) : 0.0;
	const int layer_id_offset = next_layer_id_ - min_layer_id;
	const int link_id_offset = next_link_id_ - min_link_id;

	ClearSelection();

	//Network
	float max_x = 0.0f;
	float max_y = 0.0f;
	int current_layer_id = -1;
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		NNLayer layer;
		layer = structure.GetLayer(il);
		layer.SetPosition(0, layer.GetPosition(0) + x_escape_amount);
		layer.SetCurrentPosition(layer.GetPosition());
		layer.SetLayerID(layer.GetLayerID() + layer_id_offset);
		layer.SetSelected(true);
		layers_.Add(layer);
		if (escape) {
			EscapeLayerName(layers_.GetSize() - 1, allowed_char);
		}

		if (layer.GetPosition(0) > max_x) {
			max_x = layer.GetPosition(0);
			max_y = layer.GetPosition(1);
			current_layer_id = layer.GetLayerID();
		}else if (layer.GetPosition(0) == max_x && layer.GetPosition(1) > max_y) {
			max_y = layer.GetPosition(1);
			current_layer_id = layer.GetLayerID();
		}
	}

	for (int ili = 0 ; ili < structure.GetNumLink() ; ili++) {
		NNLink link;
		link = structure.GetLink(ili);
		link.SetLinkID(link.GetLinkID() + link_id_offset);
		link.SetSourceLayerID(link.GetSourceLayerID() + layer_id_offset);
		link.SetDestLayerID(link.GetDestLayerID() + layer_id_offset);
		links_.Add(link);
	}

	if (current_layer_id >= 0) {
		SetCurrentLayerID(current_layer_id);
	}

	Update();

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::RemoveLayer(int layer_id)
{
	const int layer_index = GetLayerIndexByID(layer_id);
	if (layer_index < 0) {
		return false;
	}

	FListT<int> source_link_ids = EnumLinkIDByDestLayerID(layer_id, 0);
	FListT<int> source_layer_ids;
	for (int ili = 0 ; ili < source_link_ids.GetSize(); ili++) {
		source_layer_ids.Add(GetLinkByID(source_link_ids.GetAt(ili))->GetSourceLayerID());
	}

	FListT<int> dest_link_ids = EnumLinkIDBySourceLayerID(layer_id, 0);
	FListT<int> dest_layer_ids;
	FListT<int> dest_layer_pins;
	for (int ili = 0 ; ili < dest_link_ids.GetSize(); ili++) {
		const NNLink *link = GetLinkByID(dest_link_ids.GetAt(ili));
		assert(link);
		dest_layer_ids.Add(link->GetDestLayerID());
		dest_layer_pins.Add(link->GetDestPinIndex());
	}

	ClearSelection();
	SetLayerSelected(layer_id, true, false);
	DeleteSelected();

	for (int ilf = 0 ; ilf < source_layer_ids.GetSize() ; ilf++) {
		const int source_layer_id = source_layer_ids.GetAt(ilf);
		for (int ilt = 0 ; ilt < dest_layer_ids.GetSize() ; ilt++) {
			AddLink(source_layer_id, 0, dest_layer_ids.GetAt(ilt), dest_layer_pins.GetAt(ilt));
		}
		if (ilf == 0) {
			SetLayerSelected(source_layer_id, true, false);
			SetCurrentLayerID(source_layer_id);
		}
	}

	Update();
	return true;
}
//---------------------------------------------------------------------------
NNLayer *NNStructure::InsertLayer(int layer_id, int component_index)
{
	const int layer_index = GetLayerIndexByID(layer_id);
	if (layer_index < 0) {
		return NULL;
	}

	FListT<int> source_link_ids = EnumLinkIDByDestLayerID(layer_id, 0);

	//Move to bottom
	NNLayer last_layer;
	last_layer = layers_.GetAt(layer_index);
	for (int il = 0 ; il < GetNumLayer() ; il++) {
		NNLayer &layer = layers_.GetAt(il);
		if (layer.GetPosition(1) >= last_layer.GetPosition(1)) {
			layer.SetPosition(1, (int)layer.GetPosition(1) + (int)NNLayerDefaultHeight);
			layer.SetCurrentPosition(1, layer.GetPosition(1));
		}
	}

	//Add layer
	NNLayer &new_layer = AddLayer(component_index, false, 0.0, 0.0, false);
	new_layer.SetPosition(last_layer.GetPosition());
	new_layer.SetCurrentPosition(last_layer.GetPosition());

	//Change link edge
	for (int ili = 0 ; ili < source_link_ids.GetSize() ; ili++) {
		NNLink *link = GetLinkByID_(source_link_ids.GetAt(ili));
		AddLink(link->GetSourceLayerID(), 0, new_layer.GetLayerID(), 0);
		ClearSelection();
		SetCurrentLinkID(source_link_ids.GetAt(ili));
		DeleteSelected();
	}
	AddLink(new_layer.GetLayerID(), 0, last_layer.GetLayerID(), 0);

	ClearSelection();
	SetLayerSelected(last_layer.GetLayerID(), true, false);
	SetCurrentLayerID(last_layer.GetLayerID());

	Update();
	return &new_layer;
}
//---------------------------------------------------------------------------
int NNStructure::MutateSub(FListT<int> &added_layer_id)
{
    int result_1 = false;
	switch (rand() % 10) {
	case 0:
		result_1 = MutateByReplace();
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		result_1 = MutateParameter();
		break;
	case 6:
	case 7:
	case 8:
	case 9:
		result_1 = MutateStructure(added_layer_id);
		break;
	}

	return result_1;
}
//---------------------------------------------------------------------------
int NNStructure::Mutate(FListT<int> &added_layer_id)
{
	if (!complete_network_) {
		return false;
	}

	NNStructure backup;
	backup = *this;

	int result = false;
	int trial = 0;
	while (trial < 100){
		FListT<int> added_layer_id_1;
		int result_1 = MutateSub(added_layer_id_1);
		if (result_1) {
			RefreshDisplayValue();
			RefreshStatistics();
			if (complete_network_ && backup.IsSameInOutSetting(*this)) {
				added_layer_id.Add(added_layer_id_1);
				result = true;
				break;
			}
		}

		*this = backup;
		trial++;
	}

	return result;
}
//---------------------------------------------------------------------------
int NNStructure::MutateByReplace()
{
	if (!complete_network_) {
		return false;
	}

	FListT<int> mutate_layer_index;
	FListT<int> mutate_target_component_index;

	//Enum replace possibility
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		const int component_index = nn_layer_components_->GetComponentIndexByType(layer.GetType());
		if (component_index >= 0) {
			FListT<int> components = nn_layer_components_->EnumCompatibleComponentIndexes(component_index);
			for (int ic = 0 ; ic < components.GetSize() ; ic++) {
				mutate_layer_index.Add(il);
				mutate_target_component_index.Add(components.GetAt(ic));
			}
		}
	}

	if (mutate_layer_index.GetSize() == 0) {
		return false;
	}

	const int rand_index = rand() % mutate_layer_index.GetSize();
	const int layer_index = mutate_layer_index.GetAt(rand_index);
	const NNLayer &org_layer = layers_.GetAt(layer_index);
	const int component_index = mutate_target_component_index.GetAt(rand_index);

	SetCurrentLayerID(-1);
	NNLayer &new_layer = AddLayer(component_index);
	new_layer.SetPosition(org_layer.GetPosition());
	new_layer.SetCurrentPosition(org_layer.GetPosition());
	new_layer.SetInitialOffset(0, 0.0f);
	new_layer.SetInitialOffset(1, 0.0f);
	new_layer.SetLayerID(org_layer.GetLayerID());
	FString layer_name_bak = new_layer.GetPropertyValue(FString("Name"));
	new_layer.SetPropertyValueFrom(org_layer);
	new_layer.SetPropertyValue(FString("Name"), layer_name_bak);
	layers_.Delete(layer_index);

	ClearSelection();
	SetLayerSelected(new_layer.GetLayerID(), true, false);
	SetCurrentLayerID(new_layer.GetLayerID());

	Update();

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::MutateParameter()
{
	if (!complete_network_) {
		return false;
	}

	FListT<int> mutate_layer_index;
	FListT<int> mutate_property_index;

	//Enum mutation possibility
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
			const NNLayerProperty &property = layer.GetProperty(ip);
			if (property.GetEditable() &&
				property.GetSearchParameter() &&
				property.GetType() != NNLayerPropertyType::Text &&
				property.GetType() != NNLayerPropertyType::TextArray &&
				property.GetType() != NNLayerPropertyType::Option &&
				property.GetType() != NNLayerPropertyType::Dataset &&
				property.GetType() != NNLayerPropertyType::File &&
				property.GetType() != NNLayerPropertyType::Model &&
				property.GetType() != NNLayerPropertyType::Error &&
				property.GetValue().SubString(1, 1) != "*") {//ToDo
				mutate_layer_index.Add(il);
				mutate_property_index.Add(ip);
			}
		}
	}

	if (mutate_layer_index.GetSize() == 0) {
		return false;
	}

	const int rand_index = rand() % mutate_layer_index.GetSize();
	const int layer_index = mutate_layer_index.GetAt(rand_index);
	NNLayer &layer = layers_.GetAt(layer_index);
	const int property_index = mutate_property_index.GetAt(rand_index);
	const NNLayerProperty &property = layer.GetProperty(property_index);

	switch (property.GetType()) {
	case NNLayerPropertyType::Boolean:
		//Inverse boolean
		layer.SetPropertyValue(property_index, property.GetValue() == "True" ? FString("False") : FString("True"));
		break;
	case NNLayerPropertyType::Int:
	case NNLayerPropertyType::PInt:
	case NNLayerPropertyType::UInt:
		if (property.GetIntArray().GetSize() != 0) {
			__int64 value = property.GetIntArray().GetAt(0);
			const __int64 last_value = value;
			const int min_value = property.GetType() != NNLayerPropertyType::UInt;
			if (value <= 1 + min_value) {
				value = rand() % 4 + min_value;
			}
			while (value == last_value || value < min_value){
				value = (__int64) (last_value * pow(2.0, (rand() % 201 - 100) * 0.01) + 0.5);
			}
			layer.SetPropertyValue(property_index, FString((double)value));
		}
		break;
	case NNLayerPropertyType::PIntArray:
	case NNLayerPropertyType::UIntArray:
		{
			//Kernel shape for BorderMode=same Convolution should be odd
			int odd = false;
			if (property.GetName() == "KernelShape"){
				if (layer.GetType() == "Convolution"){
					const NNLayerProperty *property = layer.GetPropertyByName("BorderMode");
					if (property) {
						if (property->GetValue() == "same") {
							odd = true;
						}
					}
				}
			}
			const int min_value = property.GetType() != NNLayerPropertyType::UIntArray;
			while (true){
				FString result;
				for (int i = 0 ; i < property.GetIntArray().GetSize() ; i++) {
					if (i > 0) {
						 result = FString(",") + result;
					}
					if (odd) {
						__int64 value = (__int64) (property.GetIntArray().GetAt(i) / 2 + 1);
						value = value + rand() % 3 - 1;
						if (value < min_value) {
							value = min_value;
						}
						result = FString((double)(value * 2 - 1)) + result;
					}else {
						__int64 value = (__int64) (property.GetIntArray().GetAt(i) * pow(2.0, (rand() % 201 - 100) * 0.01) + 0.5);
						value = value + rand() % 3 - 1;
						if (value < min_value) {
							value = min_value;
						}
						result = FString((double)value) + result;
					}
				}
				if (result != property.GetValue()){
					layer.SetPropertyValue(property_index, result);
					break;
				}
			}
        }
		break;
	case NNLayerPropertyType::Float:
		if (property.GetFloatArray().GetSize() != 0) {
			const double value = property.GetFloatArray().GetAt(0) * pow(2.0, (rand() % 101 - 50) * 0.01);
			layer.SetPropertyValue(property_index, FString((double)value));
		}
		break;
	case NNLayerPropertyType::FloatArray:
		{
			FString result;
			for (int i = 0 ; i < property.GetIntArray().GetSize() ; i++) {
				if (i > 0) {
					 result = FString(",") + result;
				}
				const double value = property.GetFloatArray().GetAt(i) * pow(2.0, (rand() % 101 - 50) * 0.01);
				result = FString((double)value) + result;
			}
			layer.SetPropertyValue(property_index, result);
		}
		break;
	default:
		//
		break;
	}

	ClearSelection();
	SetLayerSelected(layer.GetLayerID(), true, false);
	SetCurrentLayerID(layer.GetLayerID());
	SetCurrentPropertyIndex(property_index);

	Update();

	return true;
}
//---------------------------------------------------------------------------
int NNStructure::MutateStructure(FListT<int> &added_layer_id)
{
	if (!complete_network_) {
		return false;
	}

	FListT<int> mutate_layer_index;
	FListT<int> mutate_policy_index;

	//Enum mutation possibility
	for (int ip = 0 ; ip < nn_structure_search_policies_->GetSize() ; ip++) {
		const NNStructureSearchPolicy &policy = nn_structure_search_policies_->GetAt(ip);
		const FStringList &conditions = policy.GetConditions();

		if (conditions.GetSize()) {
			for (int il = 0 ; il < layers_.GetSize() ; il++) {
				const NNLayer &layer = layers_.GetAt(il);
				int possible = false;
				for (int ic = 0 ; ic < conditions.GetSize() ; ic++) {
					FString condition = conditions.GetAt(ic);
					int error = false;

					int cond_logic_and = false;
					if (condition.SubString(1, 1) == "&") {
						cond_logic_and = true;
						condition.SplitBy('&');
					}

					int cond_negative = false;
					if (condition.SubString(1, 1) == "!") {
						cond_negative = true;
						condition.SplitBy('!');
					}
					if (condition.Pos("!")) {
						//Unexpected ! found
						error = true;
					}

					//ToDo
					int condition_pos = 0;
					if (condition.SubString(1, 3) == "-1:") {
						condition_pos = -1;
						condition.SplitBy(':');
					}
					if (condition.SubString(1, 2) == "1:") {
						condition_pos = 1;
						condition.SplitBy(':');
					}
					if (condition == "" || condition.Pos(":")) {
						//Empty or Unsopported position of condition
						error = true;
					}

					if (!error){
						int cond_result = false;
						if (condition_pos == 0) {
							if (condition.SubString(1,1) == "*"){
								//Todo
								error = true;
							}else {
								cond_result = layer.GetType() == condition;
							}
						}else if (condition_pos == -1) {
							FListT<int> link_ids = EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
							for (int ili = 0 ; ili < link_ids.GetSize() ; ili++) {
								const NNLink *link = GetLinkByID(link_ids.GetAt(ili));
								assert(link);
								if (condition.SubString(1,1) == "*"){
									if (condition == "*BranchStart"){
										FListT<int> branch_link_ids = EnumLinkIDBySourceLayerID(link->GetSourceLayerID(), 0);
										cond_result |= branch_link_ids.GetSize() >= 2;
									}else {
										//Todo
										error = true;
									}
								}else {
									const NNLayer *layer2 = GetLayerByID(link->GetSourceLayerID());
									assert(layer2);
									cond_result |= layer2->GetType() == condition;
								}
							}
						}else if (condition_pos == 1) {
							FListT<int> link_ids = EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
							for (int ili = 0 ; ili < link_ids.GetSize() ; ili++) {
								const NNLink *link = GetLinkByID(link_ids.GetAt(ili));
								assert(link);
								if (condition.SubString(1,1) == "*"){
									//Todo
									error = true;
								}else {
									const NNLayer *layer2 = GetLayerByID(link->GetDestLayerID());
									assert(layer2);
									cond_result |= layer2->GetType() == condition;
								}
							}
						}

						if (cond_negative) {
							cond_result = !cond_result;
						}
						if (cond_logic_and) {
							possible &= cond_result;
						}else {
							possible |= cond_result;
						}
                    }
					if (error) {
						possible = false;
						break;
					}
				}
				if (possible) {
					mutate_layer_index.Add(il);
					mutate_policy_index.Add(ip);
				}
			}
		}
	}

	if (mutate_layer_index.GetSize() == 0) {
		return false;
	}

	const int rand_index = rand() % mutate_layer_index.GetSize();
	const int layer_index = mutate_layer_index.GetAt(rand_index);
	const NNLayer &layer = GetLayer(layer_index);
	const int policy_index = mutate_policy_index.GetAt(rand_index);
	const NNStructureSearchPolicy &policy = nn_structure_search_policies_->GetAt(policy_index);
	const FStringList &actions = policy.GetActions();

	const int last_duplication = CountDuplication();
	ClearSelection();
	SetCurrentLayerID(layer.GetLayerID());

	for (int ia = 0; ia < actions.GetSize() ; ia++) {
		if (GetCurrentLayerID() < 0){
			return false;
		}
		FString action = actions.GetAt(ia);
		FStringList action_args;
		const int action_arg_pos = action.Pos("(");
		if (action_arg_pos) {
			FString action_args_str = action.SubString(action_arg_pos + 1, action.Length() - action_arg_pos - 1);
			action = action.SubString(1, action_arg_pos - 1);
			action_args.SplitStr(action_args_str, ",");
		}
		if (action == "RemoveLayer") {
			RemoveLayer(GetCurrentLayerID());
		}else if (action == "InsertLayer" && action_args.GetSize()) {
			FString arg = action_args.GetAt(rand() % action_args.GetSize());
			if (arg.SubString(1, 1) == "*"){
				if (arg == "*Convolution" || arg == "*DepthwiseConvolution"){
					const int component_index = nn_layer_components_->GetComponentIndexByType(arg.SubString(2, arg.Length() - 1));
					if (component_index >= 0) {
						NNLayer *new_layer = InsertLayer(GetCurrentLayerID(), component_index);
						new_layer->SetPropertyValue(FString("BorderMode"), FString("same"));
						if (arg == "*Convolution"){
							new_layer->SetPropertyValue(FString("OutMaps"), FString("Input[0]"));
						}
						added_layer_id.Add(new_layer->GetLayerID());
					}else {
						return false;
					}
				}else {
					return false;
				}
			}else {
				const int component_index = nn_layer_components_->GetComponentIndexByType(arg);
				if (component_index >= 0) {
					added_layer_id.Add(InsertLayer(GetCurrentLayerID(), component_index)->GetLayerID());
				}else {
					return false;
				}
			}
		}else if (action == "RemoveLayers") {
			int count = 1;
			while (GetCurrentLayerID() >= 0){
				const NNLayer *layer = GetLayerByID(GetCurrentLayerID());
				if (layer->GetComponent()){
					if (layer->GetComponent()->GetCategory() == "Basic"){
						if (count-- <= 0){
							break;
						}
					}
				}

				//remove and select next
				FListT<int> link_id = EnumLinkIDBySourceLayerID(layer->GetLayerID(), 0);
				int next_layer_id = -1;
				if (link_id.GetSize()){
					const NNLink *link = GetLinkByID(link_id.GetAt(0));
					next_layer_id = link->GetDestLayerID();
				}
				RemoveLayer(layer->GetLayerID());
				SetCurrentLayerID(next_layer_id);
			}
		}else if (action == "Branch") {
			FListT<const NNLayer*> source_layers;
			int count = 1;
			const NNLayer *branch_end_layer = NULL;
			while (GetCurrentLayerID() >= 0){
				const NNLayer *layer = GetLayerByID(GetCurrentLayerID());
				if (layer->GetComponent()){
					if (layer->GetComponent()->GetCategory() == "Basic"){
						if (count-- <= 0){
							branch_end_layer = layer;
							break;
						}
					}
				}
				source_layers.Add(layer);

				//next layer
				FListT<int> link_ids = EnumLinkIDBySourceLayerID(GetCurrentLayerID(), 0);
				if (!link_ids.GetSize()){
					return false;
				}
				const NNLink *link = GetLinkByID(link_ids.GetAt(0));
				assert(link);
                SetCurrentLayerID(link->GetDestLayerID());
			}
			if (!source_layers.GetSize() || !branch_end_layer){
				return false;
			}
			FListT<NNLayer*> new_layers;
			for (int il = 0 ; il < source_layers.GetSize() ; il++){
				const int component_index = nn_layer_components_->GetComponentIndexByType(source_layers.GetAt(il)->GetType());
				NNLayer &new_layer = AddLayer(component_index);
				new_layer.SetPropertyValueFrom(*source_layers.GetAt(il));
				new_layers.Add(&new_layer);
				added_layer_id.Add(new_layer.GetLayerID());
				if (il > 0){
					//Add link from previous layer
					if (!AddLink(new_layers.GetAt(il - 1)->GetLayerID(), 0, new_layer.GetLayerID(), 0)){
						return false;
					}
				}
			}

			//Add link from branch
			FListT<int> link_ids = EnumLinkIDByDestLayerID(source_layers.GetAt(0)->GetLayerID(), 0);
			if (!link_ids.GetSize()){
				return false;
			}
			const NNLink *link = GetLinkByID(link_ids.GetAt(0));
			assert(link);
			if (!AddLink(link->GetSourceLayerID(), 0, new_layers.GetAt(0)->GetLayerID(), 0)){
				return false;
			}

			//Insert join layer into branch_end_layer position
			SetCurrentLayerID(branch_end_layer->GetLayerID());
			static const int num_join_type = 3;
			static const char * join_types[num_join_type] = {
			"Add2",
			"Mul2",
            "Concatenate"
			};
			const int component_index = nn_layer_components_->GetComponentIndexByType(FString(join_types[rand() % num_join_type]));
			if (component_index < 0){
				return false;
			}
			NNLayer *new_layer = InsertLayer(GetCurrentLayerID(), component_index);
			added_layer_id.Add(new_layer->GetLayerID());
			if (!AddLink(new_layers.GetAt(new_layers.GetSize() - 1)->GetLayerID(), 0, new_layer->GetLayerID(), 0)){
				return false;
			}
		}else if (action == "RemoveUntilJoin") {
			int count = 1;
			while (GetCurrentLayerID() >= 0 && count){
				const NNLayer *layer = GetLayerByID(GetCurrentLayerID());
				if (layer->GetType() == "Concatenate"){
					count--;
				}else if (layer->GetComponent()){
					if (layer->GetComponent()->GetCategory() == "Arithmetic (2 Inputs)"){
						count--;
					}
				}

				//remove and select next
				FListT<int> link_id = EnumLinkIDBySourceLayerID(layer->GetLayerID(), 0);
				int next_layer_id = -1;
				if (link_id.GetSize()){
					const NNLink *link = GetLinkByID(link_id.GetAt(0));
					next_layer_id = link->GetDestLayerID();
				}
				RemoveLayer(layer->GetLayerID());
				SetCurrentLayerID(next_layer_id);
			}
		}else {
			return false;
		}
	}

	if (CountDuplication() > last_duplication) {
		//Duplication increased
        return false;
	}

	ArrangeLayers();

	return true;
}
//---------------------------------------------------------------------------
void NNStructure::ClearParameterFiles()
{
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		layers_.GetAt(il).ClearParameterFiles();
	}
    RefreshDisplayValue();
}
//---------------------------------------------------------------------------
void NNStructure::Update()
{
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		const int id = layer.GetLayerID();
		if (id >= next_layer_id_) {
			next_layer_id_ = id + 1;
		}
	}

	for (int ili = 0 ; ili < links_.GetSize() ; ili++) {
		const NNLink &link = links_.GetAt(ili);
		const int id = link.GetLinkID();
		if (id >= next_link_id_) {
			next_link_id_ = id + 1;
		}
	}
	update_count_++;
	ViewUpdate();
}
//---------------------------------------------------------------------------
void NNStructure::ViewUpdate()
{
	max_x_ = 0.0f;
	max_y_ = 0.0f;
	FListT<int> selected_layer_ids;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		const float x = layer.GetPosition(0) + layer.GetPosition(2);
		if (max_x_ < x) {
			max_x_ = x;
		}
		const float y = layer.GetPosition(1) + layer.GetPosition(3);
		if (max_y_ < y) {
			max_y_ = y;
		}

		if (layer.GetSelected()){
			selected_layer_ids.Add(layer.GetLayerID());
		}
	}

	//Check unitize enabled
	if (selected_layer_ids.GetSize()) {
		FListT<int> start_layer_ids;
		FListT<int> stop_layer_ids;
		FStringList repeat_start_ids;
		FStringList repeat_end_ids;
		int enabled = true;
		for (int il = 0 ; il < layers_.GetSize() ; il++) {
			const NNLayer &layer = layers_.GetAt(il);
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
							if (start_layer_ids.IndexOf(layer.GetLayerID()) < 0){
								start_layer_ids.Add(layer.GetLayerID());
							}
							enabled &= link->GetDestPinIndex() == 0;
						}
					}
				}

				//Check dest layer
				FListT<int> dest_link_ids = EnumLinkIDBySourceLayerID(layer.GetLayerID(), -1);
				if (dest_link_ids.GetSize() == 0){
					stop_layer_ids.Add(layer.GetLayerID());
				}else {
					for (int ili = 0 ; ili < dest_link_ids.GetSize() ; ili++){
						const NNLink *link = GetLinkByID(dest_link_ids.GetAt(ili));
						assert(link);
						if (selected_layer_ids.IndexOf(link->GetDestLayerID()) < 0){
							//Dest layer is not selected
							if (stop_layer_ids.IndexOf(layer.GetLayerID()) < 0){
								stop_layer_ids.Add(layer.GetLayerID());
							}
							enabled &= link->GetSourcePinIndex() == 0;
						}
					}
				}

				//Check repeat id
				if (layer.GetType() == "RepeatStart" || layer.GetType() == "RecurrentInput"){
					repeat_start_ids.Add(layer.GetDisplayPropertyValue(FString("ID")));
				}else if (layer.GetType() == "RepeatEnd" || layer.GetType() == "RecurrentOutput"){
					repeat_end_ids.Add(layer.GetDisplayPropertyValue(FString("ID")));
				}
			}
		}
		repeat_start_ids.SortAndDeleteDuplex();
		repeat_end_ids.SortAndDeleteDuplex();
		can_unitize_ = enabled && start_layer_ids.GetSize() == 1 && stop_layer_ids.GetSize() == 1 && repeat_start_ids == repeat_end_ids;
	}else {
		can_unitize_ = false;
	}

	view_update_count_++;
}
//---------------------------------------------------------------------------
void NNStructure::CheckNextID()
{
	next_layer_id_ = 0;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		const int id = layer.GetLayerID();
		if (id >= next_layer_id_) {
			next_layer_id_ = id + 1;
		}
	}
	next_link_id_ = 0;
	for (int ili = 0 ; ili < links_.GetSize() ; ili++) {
		const NNLink &link = links_.GetAt(ili);
		const int id = link.GetLinkID();
		if (id >= next_link_id_) {
			next_link_id_ = id + 1;
		}
	}
}
//---------------------------------------------------------------------------
int NNStructure::GetGridSize() const
{
	return NNLayerDefaultGrid;
}
//---------------------------------------------------------------------------
void NNStructure::RefreshDynamicProperties()
{
	//Refresh Property of Network layers
	if (related_networks_) for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (layers_.GetAt(il).GetType() == "Unit"){
			NNLayer &layer = layers_.GetAt(il);
			//Find network
			const NNLayerProperty *property_network = layer.GetPropertyByName("Network");
			if (property_network){
				FString network_name = property_network->GetDisplayValue();
				const int network_index = related_networks_->GetStructureIndexByName(network_name);
				if (network_name == GetName() || GetUsedStructure().IndexOf(network_name) >= 0){
					//Clear dynamic properties and side_connectors
					layer.ClearDynamicProperty();
					layer.ClearDynamicSideConnector();
				}else if(network_index >= 0){
					NNStructure inner_network;
					inner_network = related_networks_->GetStructure(network_index);
					inner_network.SetRelatedNetworks(related_networks_);
					FStringList used_structure = GetUsedStructure();
					used_structure.Add(GetName());
					inner_network.SetUsedStructure(used_structure);
					inner_network.RefreshDynamicProperties();

					FStringList properties, values;
					//Backup dynamic properties
					for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
						const NNLayerProperty &property = layer.GetProperty(ip);
						if (property.GetDynamic()){
							properties.Add(property.GetName());
							values.Add(property.GetValue());
						}
					}

					//Clear dynamic properties and side_connectors
					layer.ClearDynamicProperty();
					layer.ClearDynamicSideConnector();

					int insert_pos = layer.GetOutputPropertyIndex();

					//Add exported property as dynamic property
					for (int il2 = 0 ; il2 < inner_network.GetNumLayer() ; il2++) {
						const NNLayer &inner_layer = inner_network.GetLayer(il2);
						if (inner_layer.GetType() == "Argument"){
							const NNLayerProperty *property_name = inner_layer.GetPropertyByName("Name");
							const NNLayerProperty *property_value = inner_layer.GetPropertyByName("Value");
							const NNLayerProperty *property_search = inner_layer.GetPropertyByName("Search");
							const NNLayerProperty *property_type = inner_layer.GetPropertyByName("Type");

							if (property_name && property_value && property_search && property_type){
								//void AddProperty(int pos, FString name, FString short_name, int editable, int important, int required,
								//                 int search_parameter, NNLayerPropertyType type, FString value, int dynamic = false);

								const int backup_index = properties.IndexOf(property_name->GetValue());
								FString value = backup_index >= 0 ? values.GetAt(backup_index) : FString("*Default");

								NNLayerProperty *property = layer.AddProperty(
									insert_pos++,//insert pos
									property_name->GetValue(),//name
									property_name->GetValue(),//short_name
									true,//editable
									false,//important
									true,//required,
									property_search->GetValue() == "True",//search
									property_type->DecodePropertyType(property_type->GetValue()),//type
									value,
									true,//visible
									true,//dynamic
									false);//update index

								property->SetDefaultValue(property_value->GetValue() != "*Default" ? property_value->GetValue() : property_value->GetDefaultValue());
							}
						}
					}
					for (int il2 = 0 ; il2 < inner_network.GetNumLayer() ; il2++) {
						NNLayer &inner_layer = inner_network.GetLayer_(il2);
						//Add remaining inputs as dynamic side connector
						if (inner_layer.GetType() == "Input"){
							const NNLayerProperty *order_property = inner_layer.GetPropertyByName(FString("Order"));
							const NNLayerProperty *size_property = inner_layer.GetPropertyByName(FString("Size"));
							const NNLayerProperty *dataset_property = inner_layer.GetPropertyByName(FString("Dataset"));
							const NNLayerProperty *generator_property = inner_layer.GetPropertyByName(FString("Generator"));
							const NNLayerProperty *generator_multiplier_property = inner_layer.GetPropertyByName(FString("GeneratorMultiplier"));
							if (order_property && order_property->GetIntArray().GetSize() == 1 &&
								size_property && size_property->GetIntArray().GetSize() &&
								dataset_property && generator_property && generator_multiplier_property){
								const int order = order_property->GetIntArray().GetAt(0);
								if (order > 0 && generator_property->GetValue() == "None") {
									FString connector_name = inner_layer.GetName();
									FString connector_short_name = connector_name.SubString(1,1);
									FString inner_parameter_scope = inner_layer.GetDisplayPropertyValue(FString("ParameterScope"));
									{
										const FString name = connector_name + FString(".Dataset");
										const int backup_index = properties.IndexOf(name);
										FString value = backup_index >= 0 ? values.GetAt(backup_index) : FString("*Default");

										NNLayerProperty *property = layer.AddProperty(
											insert_pos++,//insert pos
											name,//name
											FString(""),//short_name
											dataset_property->GetEditable(),//editable
											false,//important
											dataset_property->GetRequired(),//required,
											dataset_property->GetSearchParameter(),//search
											dataset_property->GetType(),//type
											value,
											true,//visible
											true,//dynamic
											false);//update index

										property->SetDefaultValue(dataset_property->GetValue() != "*Default" ? dataset_property->GetValue() : dataset_property->GetDefaultValue());
									}


									{
										const FString name = connector_name + FString(".Generator");
										const int backup_index = properties.IndexOf(name);
										FString value = backup_index >= 0 ? values.GetAt(backup_index) : FString("*Default");

										NNLayerProperty *property = layer.AddProperty(
											insert_pos++,//insert pos
											name,//name
											FString(""),//short_name
											generator_property->GetEditable(),//editable
											false,//important
											generator_property->GetRequired(),//required,
											generator_property->GetSearchParameter(),//search
											generator_property->GetType(),//type
											value,
											true,//visible
											true,//dynamic
											false);//update index

										property->SetDefaultValue(generator_property->GetValue() != "*Default" ? generator_property->GetValue() : generator_property->GetDefaultValue());
									}

									{
										const FString name = connector_name + FString(".GeneratorMultiplier");
										const int backup_index = properties.IndexOf(name);
										FString value = backup_index >= 0 ? values.GetAt(backup_index) : FString("*Default");

										NNLayerProperty *property = layer.AddProperty(
											insert_pos++,//insert pos
											name,//name
											FString(""),//short_name
											generator_multiplier_property->GetEditable(),//editable
											false,//important
											generator_multiplier_property->GetRequired(),//required,
											generator_multiplier_property->GetSearchParameter(),//search
											generator_multiplier_property->GetType(),//type
											value,
											true,//visible
											true,//dynamic
											false);//update index

										property->SetDefaultValue(generator_multiplier_property->GetValue() != "*Default" ? generator_multiplier_property->GetValue() : generator_multiplier_property->GetDefaultValue());
									}

									{
										const FString name = connector_name;
										const int backup_index = properties.IndexOf(name);
										FString value = backup_index >= 0 ? values.GetAt(backup_index) : FString("*Default");

										layer.AddSideConnector(
											0,
											order,
											connector_name,//name
											connector_short_name,//short name
											0x00a0b0,//color
											FString("True"),//enable
											NNLayerComponentInputKind::Data,//kind
											size_property->GetValue(),//shape
											value,//parameter_scope
											true);//dynamic
									}
								}
							}
						}

						if (inner_layer.GetNumSideConnector(0)){
							const NNLayerComponent *inner_layer_component = inner_layer.GetComponent();
							FString inner_layer_name;
							assert(inner_layer_component);
							//Add parameterer file property as dynamic property
							for (int ip2 = 0 ; ip2 < inner_layer.GetNumProperty() ; ip2++) {
								const NNLayerProperty &inner_layer_property = inner_layer.GetProperty(ip2);
								if (inner_layer_property.GetAutoExport() &&
									inner_network.GetLayerPropertyVisible(inner_layer, ip2) &&
									inner_layer_property.GetType() != NNLayerPropertyType::Option){
									if (inner_layer_name == ""){
										inner_layer_name = inner_layer.GetPropertyValue(FString("Name"));
									}
									FString property_name = inner_layer_name + FString(".") + inner_layer_property.GetName();
									const int backup_index = properties.IndexOf(property_name);
									FString value = backup_index >= 0 ? values.GetAt(backup_index) : FString("*Default");

									NNLayerProperty *property = layer.AddProperty(
										insert_pos++,//insert pos
										property_name,//name
										FString(""),//short_name
										inner_layer_property.GetEditable(),//editable
										false,//important
										inner_layer_property.GetRequired(),//required,
										inner_layer_property.GetSearchParameter(),//search
										inner_layer_property.GetType(),//type
										value,
										true,//visible
										true,//dynamic
										false);//update index

									property->SetDefaultValue(inner_layer_property.GetValue() != "*Default" ? inner_layer_property.GetValue() : inner_layer_property.GetDefaultValue());
								}
							}

							//Add not connected input side connector as dynamic side connector
							for (int ic = 0 ; ic < inner_layer.GetNumSideConnector(0) ; ic++) {
								if (!inner_layer.GetInputPinConnected(ic + 1)){
									const NNLayerSideConnector &inner_layer_connector = inner_layer.GetSideConnector(0, ic);
									if (inner_layer.GetPinEnable(0, ic + 1)){
										FString connector_name = inner_layer_name + FString(".") + inner_layer_connector.GetName();
										FString connector_short_name = inner_layer_name.SubString(1,1) + inner_layer_connector.GetName().SubString(1,1);
										FString inner_parameter_scope = inner_layer.GetDisplayPropertyValue(FString("ParameterScope"));
										FString parameter_scope =
											(inner_parameter_scope != "" ? inner_parameter_scope + FString("/") : FString("")) +
											(inner_layer_connector.GetParameterScope() != "" ? inner_layer_connector.GetParameterScope() : inner_layer_component->GetParameterScope() + FString("/") + inner_layer_connector.GetName());

										//void AddSideConnector(int output, FString name, FString short_name, int color, FString enable, NNLayerComponentInputKind kind, FString shape, FString parameter_scope, int dynamic = false);
										FString result, error;
										inner_layer.EvalPropertyMacro(NNLayerPropertyType::PIntArray, this, related_networks_, NULL, inner_layer_connector.GetShape(), result, error);
										layer.AddSideConnector(0, -1,
											connector_name,//name
											connector_short_name,//short name
											inner_layer_connector.GetColor(),//color
											FString("True"),//enable
											inner_layer_connector.GetKind(),//kind
											result,//shape
											parameter_scope,//parameter_scope
											true);//dynamic
									}

								}
							}
						}
					}
					layer.UpdatePropertyIndex();
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void NNStructure::RefreshDisplayValue(int test)
{
	//Refresh Property of Network layers
	RefreshDynamicProperties();

	//Remove dead links
	for (int ili = links_.GetSize() - 1 ; ili >= 0  ; ili--) {
		const NNLink &link = links_.GetAt(ili);
		if (link.GetDestPinIndex() > 0) {
			if (!GetPinVisible(link.GetDestLayerID(), 0, link.GetDestPinIndex())) {
				links_.Delete(ili);
			}
		}
	}

	//Reset and update display Value
	int *update_request_num = new int[layers_.GetSize()];
	memset(update_request_num, 0, sizeof(int) * layers_.GetSize());
	int sum_update_request_num = 0;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		NNLayer &layer = layers_.GetAt(il);
		int *urn = update_request_num + il;
		layer.ResetInputPinConnected();
		layer.ResetPropertyDisplayValue(urn);
		layer.UpdatePropertyLocalDisplayValue(this, related_networks_, urn);
		sum_update_request_num += *urn;
	}

	//Check RepeatID, RecurrentGroupID and its Length
	repeat_info_.Clear();
	FStringList repeat_ids;
	FListT<NNRepeatType> repeat_types;
	FListT<int> repeat_length;
	FListP<FStringList> layer_repeat_ids;
	FListT<int> layer_processed;
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		layer_repeat_ids.Add(FStringList());
		layer_processed.Add(false);
	}
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (layers_.GetAt(il).GetType() == "Input") {
			FStringList current;
			CheckRepeatID(
				il,
				current,
				repeat_ids,
				repeat_types,
				repeat_length,
				layer_repeat_ids,
				layer_processed);
		}
	}
	for (int ir = 0 ; ir < repeat_ids.GetSize() ; ir++) {
		NNRepeatInfo &repeat_info = repeat_info_.Add();
		repeat_info.Set(repeat_ids.GetAt(ir), repeat_types.GetAt(ir), repeat_length.GetAt(ir));
	}
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		layers_.GetAt(il).SetRepeatIDs(layer_repeat_ids.GetAt(il));
	}

	//Set Input Pin Connected
	for (int ili = 0 ; ili < links_.GetSize() ; ili++) {
		const NNLink &link = links_.GetAt(ili);
		int layer_index = GetLayerIndexByID(link.GetDestLayerID());
		if (layer_index >= 0) {
			layers_.GetAt(layer_index).SetInputPinConnected(link.GetDestPinIndex(), 0x40000000 | link.GetSourceLayerID());
		}
	}

	//Set shape of the input side connector which has no connection
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		NNLayer &layer = layers_.GetAt(il);
		for (int is = 0 ; is < layer.GetNumSideConnector(0) ; is++) {
			if (!layer.GetInputPinConnected(is + 1)) {
				const NNLayerSideConnector &side_connector = layer.GetSideConnector(0, is);
				layer.SetSideConnectorCurrentShape(0, is, side_connector.GetShape());
                update_request_num[il] --;
			}
		}
	}

	//Cache connection information
	FListT<int> cache_graph_input_property_index;
	FListT<int> cache_graph_inputs_property_index;
	FListP<FListP<FListT<int> > > cache_source_layer_indexes;

	FListT<int> cache_graph_output_property_index;
	FListP<FListT<int> > cache_dest_layer_indexes;
	FListP<FStringList > cache_dest_layer_macros;

	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		NNLayer &layer = layers_.GetAt(il);

		FListP<FListT<int> > &layer_indexes_all = cache_source_layer_indexes.Add(FListP<FListT<int> >());
		FListT<int> &layer_indexes = layer_indexes_all.Add(FListT<int>());
		int &graph_input_index = cache_graph_input_property_index.Add(-1);
		int &graph_inputs_index = cache_graph_inputs_property_index.Add(-1);
		int &graph_output_index = cache_graph_output_property_index.Add(-1);
		FListT<int> &dest_layer_indexes = cache_dest_layer_indexes.Add(FListT<int>());
		FStringList &dest_layer_macros = cache_dest_layer_macros.Add();
		if (update_request_num[il]) {
			//Graph input
			FListT<int> link_indexes, dest_link_indexes;
			graph_input_index = layer.GetPropertyIndexByValue(FString("*GraphInput"));
			if (graph_input_index < 0) {
				graph_input_index = layer.GetPropertyIndexByValue(FString("*GraphIO"));
			}
			if (graph_input_index >= 0) {
				const int link_index = GetLinkIndexByDestLayerID(layer.GetLayerID(), 0);
				if (link_index >= 0) {
					link_indexes.Add(link_index);
				}
			}else {
				graph_inputs_index = layer.GetPropertyIndexByValue(FString("*GraphInputs"));
				if (graph_inputs_index >= 0) {
					link_indexes = EnumLinkIDByDestLayerID(layer.GetLayerID(), -1);
					for (int ili = 0 ; ili < link_indexes.GetSize() ; ili++) {
						const int link_index = GetLinkIndex(link_indexes.GetAt(ili));
						if (link_index >= 0) {
							link_indexes.SetAt(ili, link_index);
						}else {
							link_indexes.Delete(ili);
							ili--;
						}
					}
				}
			}
			if (link_indexes.GetSize()) {
				for (int ili = 0 ; ili < link_indexes.GetSize() ; ili++) {
					NNLink &link = links_.GetAt(link_indexes.GetAt(ili));
					const int layer_index = GetLayerIndexByID(link.GetSourceLayerID());
					layer_indexes.Add(layer_index);
				}
			}

			//Graph side connector input
			for (int is = 0 ; is < layer.GetNumSideConnector(0) ; is++) {
				FListT<int> &layer_side_indexes = layer_indexes_all.Add(FListT<int>());
				if (layer.GetInputPinConnected(is + 1)) {
					const int side_link_index = GetLinkIndexByDestLayerID(layer.GetLayerID(), is + 1);
					if (side_link_index >= 0) {
						NNLink &link = links_.GetAt(side_link_index);
						const int layer_index = GetLayerIndexByID(link.GetSourceLayerID());
						layer_side_indexes.Add(layer_index);
					}
                }
			}

			//Graph output
			graph_output_index = layer.GetPropertyIndexByValue(FString("*GraphOutput"));
			if (graph_output_index < 0) {
				graph_output_index = layer.GetPropertyIndexByValue(FString("*GraphIO"));
			}
			if (graph_output_index >= 0) {
				dest_link_indexes = EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);//Link ID
				for (int ili = 0 ; ili < dest_link_indexes.GetSize() ; ili++) {
					const int link_index = GetLinkIndex(dest_link_indexes.GetAt(ili));
					if (link_index >= 0) {
						dest_link_indexes.SetAt(ili, link_index);//Link Index
					}else {
						dest_link_indexes.Delete(ili);
						ili--;
					}
				}
			}
			if (dest_link_indexes.GetSize()) {
				for (int ili = 0 ; ili < dest_link_indexes.GetSize() ; ili++) {
					NNLink &link = links_.GetAt(dest_link_indexes.GetAt(ili));
					const int layer_index = GetLayerIndexByID(link.GetDestLayerID());
					dest_layer_indexes.Add(layer_index);
					const NNLayer &dest_layer = layers_.GetAt(layer_index);
					const int dest_pin_index = link.GetDestPinIndex();
					FString &macro = dest_layer_macros.Add(FString(""));
					if (dest_pin_index > 0) {
						const NNLayerSideConnector &connector = dest_layer.GetSideConnector(0, dest_pin_index - 1);
						macro = connector.GetShape();
					}
				}
			}
		}
	}

	//Update loop
	int last_sum_update_request_num = sum_update_request_num;
	while (true){
		sum_update_request_num = 0;
		for (int il = 0 ; il < layers_.GetSize() ; il++) if (update_request_num[il]){
			NNLayer &layer = layers_.GetAt(il);
			//Update IsParam
			{
				const int property_index = layer.GetIsParamPropertyIndex();
				const FListT<int> &source_layer_indexes = cache_source_layer_indexes.GetAt(il).GetAt(0);
				if (property_index >= 0 && source_layer_indexes.GetSize() >= 1) {
					const NNLayerProperty &property = layer.GetProperty(property_index);
					if (property.GetDisplayValue() == "") {
						const NNLayer &source_layer = layers_.GetAt(source_layer_indexes.GetAt(0));
						int source_property_index = source_layer.GetIsParamPropertyIndex();
						if (source_property_index >= 0) {
							const FString &value = source_layer.GetProperty(source_property_index).GetDisplayValue();
							if (value != "") {
								layer.SetPropertyDisplayValue(property_index, value);
							}
						}
					}
				}
			}

			//Update GraphInput
			{
				const int property_index = cache_graph_input_property_index.GetAt(il);
				const FListT<int> &source_layer_indexes = cache_source_layer_indexes.GetAt(il).GetAt(0);
				if (property_index >= 0 && source_layer_indexes.GetSize() == 1) {
					const NNLayerProperty &property = layer.GetProperty(property_index);
					if (property.GetDisplayValue() == "") {
						const NNLayer &source_layer = layers_.GetAt(source_layer_indexes.GetAt(0));
						int source_property_index = source_layer.GetOutputPropertyIndex();
						if (source_property_index >= 0) {
							const FString &value = source_layer.GetProperty(source_property_index).GetDisplayValue();
							if (value != "") {
								layer.SetPropertyDisplayValue(property_index, value);
							}
						}
					}
				}
			}

			//Update GraphInputs
			{
				const int property_index = cache_graph_inputs_property_index.GetAt(il);
				const FListT<int> &source_layer_indexes = cache_source_layer_indexes.GetAt(il).GetAt(0);
				if (property_index >= 0 && source_layer_indexes.GetSize()) {
					const NNLayerProperty &property = layer.GetProperty(property_index);
					if (property.GetDisplayValue() == "") {
						int num_input = 0;
						//int num_dim = 0;
						FString display_value;
						for (int il2 = 0 ; il2 < source_layer_indexes.GetSize() ; il2++) {
							int success = false;
							const NNLayer &source_layer = layers_.GetAt(source_layer_indexes.GetAt(il2));
							int source_property_index = source_layer.GetOutputPropertyIndex();
							if (source_property_index >= 0) {
								/*
								if (num_dim == 0) {
									num_dim = source_layer.GetProperty(source_property_index).GetIntArray().GetSize();
								}
								if (num_dim == source_layer.GetProperty(source_property_index).GetIntArray().GetSize()) {
								*/
								const FString value = source_layer.GetProperty(source_property_index).GetDisplayValue();
								if (value != "") {
									if (display_value != "") {
										display_value += ",";
									}
									display_value += FString("(") + value + FString(")");
									success = true;
								}
								//}
							}
							num_input += success;
						}
						if (num_input == source_layer_indexes.GetSize()) {
							layer.SetPropertyDisplayValue(property_index, display_value);
						}
					}
				}
			}

			//Update GraphOutput
			{
				const int property_index = cache_graph_output_property_index.GetAt(il);
				const FListT<int> &dest_layer_indexes = cache_dest_layer_indexes.GetAt(il);
				const FStringList &dest_layer_macros = cache_dest_layer_macros.GetAt(il);
				if (property_index >= 0) for (int il2 = 0 ; il2 < dest_layer_indexes.GetSize() ; il2++) {
					const NNLayerProperty &property = layer.GetProperty(property_index);
					if (property.GetDisplayValue() == "") {
						NNLayer &dest_layer = layers_.GetAt(dest_layer_indexes.GetAt(il2));
						const int dest_layer_input_property_index = dest_layer.GetInputPropertyIndex();
						const FString &dest_layer_macro = dest_layer_macros.GetAt(il2);
						FString value, error;
						if (dest_layer_macro != ""){
							/*if (dest_layer_macro.SubString(1, 1) != "*") {
								layer.SetPropertyDisplayValue(property_index, dest_layer_macro);
							}else */if (dest_layer.EvalPropertyMacro(NNLayerPropertyType::PIntArray, this, related_networks_, NULL, dest_layer_macro, value, error)) {
								layer.SetPropertyDisplayValue(property_index, value);
								if (!layer.GetProperty(property_index).GetUpdateRequested()) {
									update_request_num[il]--;
								}
							}
						}else if (dest_layer_input_property_index >= 0){
							const FString &value = dest_layer.GetProperty(dest_layer_input_property_index).GetDisplayValue();
							if (value != "") {
								layer.SetPropertyDisplayValue(property_index, value);
							}
						}
					}
				}
			}

			//Update RepeatTimes, RecurrentLength
			if (layer.GetType() == "RepeatEnd" || layer.GetType() == "RecurrentOutput"){
				const int recurrent_type = layer.GetType() == "RecurrentOutput";
				const NNLayerProperty *property_repeat_id = layer.GetPropertyByName("ID");
				const int property_index = layer.GetPropertyIndexByName(recurrent_type ? "Length" : "Times");
				if (property_index >= 0 && property_repeat_id) {
					const NNLayerProperty &property = layer.GetProperty(property_index);
					if (property.GetDisplayValue() == "") {
						for (int il2 = 0 ; il2 < layers_.GetSize() ; il2++) if (update_request_num[il2] == 0){//Already fixed layer only
							const NNLayer &layer2 = GetLayer(il2);
							if (layer2.GetType() == "RepeatStart" || layer2.GetType() == "RecurrentInput") {
								const NNLayerProperty *property_repeat_id2 = layer2.GetPropertyByName("Name");
								if (property_repeat_id2) {
									if (property_repeat_id->GetDisplayValue() == property_repeat_id2->GetDisplayValue()) {
										if (layer.GetType() == "RepeatEnd"){
											const NNLayerProperty *property_times = layer2.GetPropertyByName("Times");
											if (property_times) {
												if (property_times->GetIntArray().GetSize() == 1) {
													layer.SetPropertyDisplayValue(property_index, FString((int)property_times->GetIntArray().GetAt(0)));
												}
											}
										}else {//layer.GetType() == "RecurrentOutput"
											const int input_property_index = layer2.GetInputPropertyIndex();
											if (input_property_index >= 0) {
												const NNLayerProperty &input_property = layer2.GetProperty(input_property_index);
												const NNLayerProperty *property_axis = layer2.GetPropertyByName("Axis");
												int axis = 0;
												if (property_axis) {
													if (property_axis->GetIntArray().GetSize() == 1) {
														axis = property_axis->GetIntArray().GetAt(0);
													}
												}
												if (axis < input_property.GetIntArray().GetSize()) {
													layer.SetPropertyDisplayValue(property_index, FString((int)input_property.GetIntArray().GetAt(input_property.GetIntArray().GetSize() - 1 - axis)));
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}

			//Update Auto Inplace
			{
				const int property_index = layer.GetInPlacePropertyIndex();
				const FListT<int> &source_layer_indexes = cache_source_layer_indexes.GetAt(il).GetAt(0);
				if (property_index >= 0 && source_layer_indexes.GetSize()) {
					const NNLayerProperty &property = layer.GetProperty(property_index);// in-place property
					if (property.GetDisplayValue() == "") {
						const FString &value_or_macro = property.GetValue();
						FString display_value;
						if (value_or_macro == "True" || value_or_macro == "False") {
							display_value = value_or_macro;
						}else  if (value_or_macro == "*AutoInPlace" || value_or_macro == "*AutoInPlaceOnce"){
							int in_place = true;
							int success = true;
							CheckInplaceRecursive(il, in_place, success, cache_source_layer_indexes);

							if (success) {
								display_value = in_place ? "True" : "False";
							}
						}
						if (display_value != "") {
							layer.SetPropertyDisplayValue(property_index, display_value);
						}
					}
				}
			}

			//Update shape of side connector
			for (int is = 0 ; is < layer.GetNumSideConnector(0); is++){
				FListT<int> &source_layer_indexes = cache_source_layer_indexes.GetAt(il).GetAt(is + 1);
				if (source_layer_indexes.GetSize() == 1) {
					const NNLayer &source_layer = layers_.GetAt(source_layer_indexes.GetAt(0));
					int source_property_index = source_layer.GetOutputPropertyIndex();
					if (source_property_index >= 0) {
						const FString &value = source_layer.GetProperty(source_property_index).GetDisplayValue();
						if (value != "") {
							layer.SetSideConnectorCurrentShape(0, is, value);
							source_layer_indexes.Delete(0);
						}
					}
				}
			}

			//Update property
			layers_.GetAt(il).UpdatePropertyLocalDisplayValue(this, related_networks_, update_request_num + il);

			//Sum update request
			sum_update_request_num += update_request_num[il];
		}

		if (sum_update_request_num == 0 || sum_update_request_num == last_sum_update_request_num) {
			break;
		}
		last_sum_update_request_num = sum_update_request_num;
	}

	//delete [] link_source_layer_index;
	//delete [] link_dest_layer_index;
	delete[] update_request_num;

	CheckError();
}
//---------------------------------------------------------------------------
void NNStructure::CheckInplaceRecursive(int layer_index, int &in_place, int &success, FListP<FListP<FListT<int> > > &cache_source_layer_indexes)
{
	// const NNLayer &layer = layers_.GetAt(layer_index);
	const FListT<int> &source_layer_indexes = cache_source_layer_indexes.GetAt(layer_index).GetAt(0);
	for (int is = 0; is < source_layer_indexes.GetSize() && success && in_place; is++) {
		const int source_layer_index = source_layer_indexes.GetAt(is);
		const NNLayer &previous_layer = GetLayer(source_layer_index);

		// if input is not parameter (cannot accumulate diff of parameter with in-place processing)
		const FListT<int> &source_layer_input_layers = cache_source_layer_indexes.GetAt(source_layer_index).GetAt(0);
		if (source_layer_input_layers.GetSize() <= 0) {
			in_place = false;
			continue;
		}

		// if the input variable has single output (cannot accumulate diff of parameter with in-place processing)
		const FListT<int> source_layer_output_links = EnumLinkIDBySourceLayerID(previous_layer.GetLayerID(), 0);
		if (source_layer_output_links.GetSize() > 1) {
			in_place = false;
			continue;
		}

		// if previous function is not in-place or can inplace next
		const int previous_property_index = previous_layer.GetInPlacePropertyIndex();
		if (previous_property_index >= 0) {
			const NNLayerProperty &previous_property = previous_layer.GetProperty(previous_property_index);// in-place property
			if (previous_property.GetDisplayValue() != "False" && previous_property.GetValue() != "*AutoInPlace") {
				in_place = false;
			}else if (previous_property.GetDisplayValue() == "") {
				success = false;
			}
			continue;
		}

		// if previous function is identity layer and will be removed.
		if (previous_layer.GetType() == "Identity" || previous_layer.GetType() == "RepeatEnd" || previous_layer.GetType() == "RecurrentOutput") {
			CheckInplaceRecursive(source_layer_index, in_place, success, cache_source_layer_indexes);
		}else if (previous_layer.GetType() == "RepeatStart" || previous_layer.GetType() == "RecurrentInput" || previous_layer.GetType() == "Delay") {
            in_place = false;
		}
	}
}
//---------------------------------------------------------------------------
void NNStructure::CheckError()
{
	errors_.Clear();

	int num_loss_function = 0;

	FStringList variables;
	FListT<int> variable_layer_index;
	FStringList variable_size;

	FStringList group_ids;
	FListT<int> group_layer_index;

	FListT<int> weight_cost_group_layer_index;
	FStringList weight_cost_group_id;
	FListT<int> loss_cost_group_layer_index;
	FStringList loss_cost_group_id;

	FStringList recurrent_group_ids;
	FListT<int> recurrent_group_length;

	FStringList setting_layers;

	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		NNLayer &layer = layers_.GetAt(il);
		layer.CheckError();

		//Count output num
		if (layer.GetPropertyValue(FString("IsLossFunction")) != ""){
			num_loss_function++;
		}

		//Check Unknown layer
		const NNLayerComponent *layer_component = layer.GetComponent();
		if (!layer_component) {
			NNError &error = layer.AddError();
			error.SetNetworkName(name_);
			error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
			error.SetString(error.GetLayerName() + FString(" : Unknown layer."));
		}else {
			//Check link
			if (layer_component->GetInputType() != NNLayerComponentIOType::None &&
				layer_component->GetInputType() != NNLayerComponentIOType::SingleOrNone) {
				if (EnumLinkIDByDestLayerID(layer.GetLayerID(), 0).GetSize() == 0) {
					NNError &error = layer.AddError();
					error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
					error.SetNetworkName(name_);
					error.SetString(layer.GetDisplayPropertyValue(FString("Name")) + FString(" : Input pin is not connected."));
				}
			}
			if (!(layer_component->GetOutputType() == NNLayerComponentIOType::None ||
				  layer_component->GetOutputType() == NNLayerComponentIOType::Any)) {
				if (EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0).GetSize() == 0) {
					NNError &error = layer.AddError();
					error.SetNetworkName(name_);
					error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
					error.SetString(layer.GetDisplayPropertyValue(FString("Name")) + FString(" : Output pin is not connected."));
				}
			}

			//Variables
			const int dataset_property_index = layer.GetPropertyIndexByName("Dataset");
			if (dataset_property_index >= 0) {
				const int generator_property_index = layer.GetPropertyIndexByName("Generator");
				FString variable_name = layer.GetProperty(dataset_property_index).GetDisplayValue();
				if (generator_property_index >= 0) {
					if (layer.GetProperty(generator_property_index).GetDisplayValue() != "None"){
                        //skip size check if generator is used
                        variable_name = "";
					}
				}
				if (variable_name != "") {
					FString size;
					const NNLayerProperty *size_property = layer.GetPropertyByName("Size");
					if (size_property) {
						size = size_property->GetDisplayValue();
					}else if (layer.GetInputPropertyIndex() > 0) {
						size = layer.GetProperty(layer.GetInputPropertyIndex()).GetDisplayValue();
					}

					int variable_index = variables.IndexOf(variable_name);
					if (variable_index < 0) {
						variables.Add(variable_name);
						variable_layer_index.Add(il);
						variable_size.Add(size);
					}else if (variable_size.GetAt(variable_index) != size){
						const NNLayer &target_layer = layers_.GetAt(variable_layer_index.GetAt(variable_index));
						NNError &new_error = errors_.Add();
						new_error.SetNetworkName(name_);
						new_error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
						new_error.SetPropertyName(FString("Dataset"));
						new_error.SetString(
							layer.GetDisplayPropertyValue(FString("Name")) +
							FString("->Dataset") +
							FString(" : Size of variable \"") +
							variable_name +
							FString("\" is not same between \"") +
							target_layer.GetDisplayPropertyValue(FString("Name")) +
							FString("\" and \"") +
							layer.GetDisplayPropertyValue(FString("Name")) +
							FString("\" (\"") +
							variable_size.GetAt(variable_index) +
							FString("\", \"") +
							size +
							FString("\").")
							);
					}
				}
			}

			//Repeat
			if (layer.GetType() == "RepeatStart") {
				//ToDo
			}

			//RNN Groups
			const int recurrent_group_id_property_index = layer.GetPropertyIndexByName("RecurrentGroupID");
			if (recurrent_group_id_property_index >= 0) {
				FString recurrent_group_id = layer.GetProperty(recurrent_group_id_property_index).GetDisplayValue();
				int group_index = recurrent_group_ids.IndexOf(recurrent_group_id);
				if (group_index < 0) {
					group_index = recurrent_group_ids.GetSize();
					recurrent_group_ids.Add(recurrent_group_id);
					recurrent_group_length.Add(-1);
				}
				if (layer.GetType() == "RecurrentInput" || layer.GetType() == "RecurrentOutput") {
					//Check difference of length among inputs and outputs
					int length = -1;
					FString property;
					if (layer.GetType() == "RecurrentInput") {
						const int input_property_index = layer.GetInputPropertyIndex();
						property = "Input";
						if (input_property_index >= 0) {
							if (layer.GetProperty(input_property_index).GetIntArray().GetSize() > 0){
								int axis = 0;
								const NNLayerProperty *property_axis = layer.GetPropertyByName("Axis");
								if (property_axis) {
									if (property_axis->GetIntArray().GetSize() == 1) {
										axis = property_axis->GetIntArray().GetAt(0);
									}
								}
								if (axis >= layer.GetProperty(input_property_index).GetIntArray().GetSize()) {
									axis = 0;
								}
								length = layer.GetProperty(input_property_index).GetIntArray().GetAt(layer.GetProperty(input_property_index).GetIntArray().GetSize() - 1 - axis);
							}
						}
					}else {
						//Recurrent Output
						const int length_property_index = layer.GetPropertyIndexByName("Length");
						property = "Length";
						if (length_property_index >= 0) {
							if (layer.GetProperty(length_property_index).GetIntArray().GetSize() == 1){
								length = layer.GetProperty(length_property_index).GetIntArray().GetAt(0);
							}
						}
					}
					if (recurrent_group_length.GetAt(group_index) < 0) {
						recurrent_group_length.SetAt(group_index, length);
					}else if (recurrent_group_length.GetAt(group_index) != length) {
						NNError &new_error = errors_.Add();
						new_error.SetNetworkName(name_);
						new_error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
						new_error.SetString(
							layer.GetDisplayPropertyValue(FString("Name")) +
							(property != "" ? FString("->") + property: FString("")) +
							FString(" : Sequence Length is different among RecurrentInputs/Outputs (recurrent group id = \"") +
							recurrent_group_id +
							FString("\").")
							);
						if (property != "") {
							new_error.SetPropertyName(property);
						}
					}
				}else if (layer.GetType() == "Delay") {
					//Check difference of input and output size of delay
					const int input_property_index = layer.GetInputPropertyIndex();
					const int size_property_index = layer.GetPropertyIndexByName("Size");
					if (input_property_index >= 0 && size_property_index >= 0) {
						if (layer.GetProperty(input_property_index).GetDisplayValue() != layer.GetProperty(size_property_index).GetDisplayValue()){
							NNError &new_error = errors_.Add();
							new_error.SetNetworkName(name_);
							new_error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
							new_error.SetPropertyName(FString("Size"));
							new_error.SetString(
								layer.GetDisplayPropertyValue(FString("Name")) +
								FString("->Size : Data size is different between Input and Size (") +
								layer.GetProperty(input_property_index).GetDisplayValue() +
								FString(", ") +
								layer.GetProperty(size_property_index).GetDisplayValue() +
								FString(").")
								);
						}
					}
				}
			}

			//Check duplicate of setting layer
			if (layer_component->GetCategory() == "Setting") {
				if (setting_layers.IndexOf(layer.GetType()) >= 0) {
					//duplicate
					NNError &new_error = errors_.Add();
					new_error.SetNetworkName(name_);
					new_error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
					new_error.SetString(
						layer.GetDisplayPropertyValue(FString("Name")) +
						FString(" : More than one ") +
						layer.GetType() +
						FString(" layer exists.")
						);
				}else {
					setting_layers.Add(layer.GetType());
                }
			}
		}
		for (int ie = 0; ie < layer.GetErrors().GetSize(); ie++) {
			NNError error;
			error = layer.GetErrors().GetAt(ie);
			error.SetNetworkName(name_);
			errors_.Add(error);
		}
	}

	//Check loop layers
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		NNLayer &layer = layers_.GetAt(il);

		// Update loop times
		if (layer.GetType() == "RepeatStart" || layer.GetType() == "RecurrentInput"){
			const int repeat_info_index = GetRepeatInfoIndex(layer.GetDisplayPropertyValue(FString("Name")));
			if (repeat_info_index >= 0) {
				NNRepeatInfo &repeat_info = repeat_info_.GetAt(repeat_info_index);
				if (repeat_info.GetType() == NNRepeatType::Recurrent) {
					const NNLayerProperty *property_input = layer.GetAvailableInputProperty();
					const NNLayerProperty *property_axis = layer.GetPropertyByName("Axis");
					int axis = 0;
					if (property_axis) {
						if (property_axis->GetIntArray().GetSize() == 1) {
							axis = property_axis->GetIntArray().GetAt(0);
						}
					}
					int length = 1;
					if (property_input && axis < property_input->GetIntArray().GetSize()) {
						length = property_input->GetIntArray().GetAt(property_input->GetIntArray().GetSize() - 1 - axis);
					}
					repeat_info.Set(repeat_info.GetID(), repeat_info.GetType(), length);
				}else {
					const NNLayerProperty *property_times = layer.GetPropertyByName("Times");
					int length = 1;
					if (property_times) {
						if (property_times->GetIntArray().GetSize() == 1) {
							length = property_times->GetIntArray().GetAt(0);
						}
					}
					repeat_info.Set(repeat_info.GetID(), repeat_info.GetType(), length);
				}
            }
		}

		// Check if output size of repeat start and input size of repeat end are same
		if (layer.GetType() == "RepeatEnd") {
			FString loop_id = layer.GetDisplayPropertyValue(FString("ID"));
			const int loop_start_layer_id = GetLayerIDByName(loop_id);
			if (loop_start_layer_id >= 0) {
				const int loop_start_layer_index = GetLayerIndexByID(loop_start_layer_id);
				assert(loop_start_layer_index >= 0);
				NNLayer &loop_start_layer = layers_.GetAt(loop_start_layer_index);
				FString repeat_end_input_size = layer.GetInputPropertyIndex() >= 0 ? layer.GetProperty(layer.GetInputPropertyIndex()).GetDisplayValue() : FString("");
				FString repeat_start_output_size = loop_start_layer.GetOutputPropertyIndex() >= 0 ? loop_start_layer.GetProperty(loop_start_layer.GetOutputPropertyIndex()).GetDisplayValue() : FString("");
				if (repeat_end_input_size != repeat_start_output_size) {
					const int property_index = layer.GetInputPropertyIndex();
					assert(property_index >= 0);
					NNLayerProperty &property = layer.GetProperty_(property_index);
					FString message =
						FString("I/O size must be the same for \"") +
						loop_start_layer.GetDisplayPropertyValue(FString("Name")) +
						FString("\"(") +
						repeat_start_output_size +
						FString(") and \"") +
						layer.GetDisplayPropertyValue(FString("Name")) +
						FString("\"(") +
						repeat_end_input_size +
						FString(").");
					property.SetError(true, message);
					layer.CheckError();

					NNError &new_error = errors_.Add();
					new_error.SetString(
						layer.GetDisplayPropertyValue(FString("Name")) +
						FString("->Input : ") +
						message);
					new_error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
					new_error.SetPropertyName(FString("Input"));
					new_error.SetNetworkName(name_);
				}
			}
		}

		//Check circular reference
		if (layer.GetType() == "Import"){
			int found = false;
			FString error;
            FStringList visit_networks;
			GetArgument(related_networks_, FString(""), visit_networks, found, error);
			if (error != "") {
				NNError &new_error = errors_.Add();
				new_error.SetNetworkName(name_);
				new_error.SetString(error);
			}
        }
	}

	if (layers_.GetSize() == 0) {
		NNError &new_error = errors_.Add();
		new_error.SetNetworkName(name_);
		new_error.SetString(FString("Network is empty."));
	}else if (num_loss_function == 0 && name_ == "Main") {
		NNError &new_error = errors_.Add();
		new_error.SetNetworkName(name_);
		new_error.SetString(FString("Main network must contain at least one loss function."));
	}

	//Add network name to error messages
	for (int ie = 0; ie < errors_.GetSize(); ie++) {
		errors_.GetAt(ie).SetString(errors_.GetAt(ie).GetNetworkName() + FString("->") + errors_.GetAt(ie).GetString());
	}
	complete_network_ = errors_.GetSize() == 0;
}
//---------------------------------------------------------------------------
void NNStructure::CheckRepeatID(
	int layer_index,
	FStringList current_repeat_ids,

	FStringList &repeat_ids,
	FListT<NNRepeatType> &repeat_types,
	FListT<int> &repeat_length,

	FListP<FStringList> &layer_repeat_ids,
	FListT<int> &layer_processed)
{
	if (layer_processed.GetAt(layer_index)) {
		//Already processed
		return;
	}

	NNLayer &layer = layers_.GetAt(layer_index);
	const int recurrent_type = layer.GetType() == "RecurrentInput" || layer.GetType() == "RecurrentOutput" || layer.GetType() == "Delay";
	if (recurrent_type ||
		layer.GetType() == "RepeatStart" || layer.GetType() == "RepeatEnd") {
		const int property_index = layer.GetPropertyIndexByName("Name");
		/*
		const int property_index =
			recurrent_type ?
			layer.GetPropertyIndexByName("RecurrentGroupID") :
			layer.GetPropertyIndexByName("RepeatID");
		*/
		assert (property_index >= 0);
		FString repeat_id = layer.GetProperty(property_index).GetValue();

		if (layer.GetType() == "RepeatStart" || layer.GetType() == "RecurrentInput"){
			// Loop start
			const int repeat_id_index = repeat_ids.IndexOf(repeat_id);
			if (repeat_id_index < 0) {
				repeat_ids.Add(repeat_id);
				repeat_length.Add(1);// Temporary
				if (recurrent_type) {
					/*
					const NNLayerProperty *property_input = layer.GetPropertyByName("Input");
					const NNLayerProperty *property_axis = layer.GetPropertyByName("Axis");
					int axis = 0;
					if (property_axis) {
						if (property_axis->GetIntArray().GetSize() == 1) {
							axis = property_axis->GetIntArray().GetAt(0);
						}
					}
					int length = 1;
					if (axis < property_input->GetIntArray().GetSize()) {
						length = property_input->GetIntArray().GetAt(property_input->GetIntArray().GetSize() - 1 - axis);
					}
					repeat_length.Add(length);
					*/
					repeat_types.Add(NNRepeatType::Recurrent);
				}else {
					/*
					const NNLayerProperty *property_times = layer.GetPropertyByName("Times");
					if (property_times) {
						if (property_times->GetIntArray().GetSize() == 1) {
							repeat_length.Add(property_times->GetIntArray().GetAt(0));
						}else {
							repeat_length.Add(1);
						}
					}else {
						repeat_length.Add(1);
					}
					*/
					repeat_types.Add(NNRepeatType::Repeat);
				}
			}
			current_repeat_ids.Add(repeat_id);
		}else {
			// Loop end or delay
			if (current_repeat_ids.GetSize()){
				//current_repeat_ids.Remove(repeat_id);

				FString last_repeat_id = current_repeat_ids.GetAt(current_repeat_ids.GetSize() - 1);
				const int index = repeat_ids.IndexOf(last_repeat_id);
				assert(index >= 0);
				const NNRepeatType type = repeat_types.GetAt(index);
				if (recurrent_type == (type == NNRepeatType::Recurrent)) {
					const int property_index = layer.GetPropertyIndexByName("ID");
					assert(property_index >= 0);
					layer.SetPropertyDisplayValue(property_index, last_repeat_id);

					if (layer.GetType() != "Delay") {
						//Loop end
						current_repeat_ids.Delete(current_repeat_ids.GetSize() - 1);
					}
				}else {
					const int property_index = layer.GetPropertyIndexByName("Name");
					assert(property_index >= 0);
					NNLayerProperty &property = layer.GetProperty_(property_index);
					const int index = repeat_ids.IndexOf(current_repeat_ids.GetAt(0));
					assert(index >= 0);
					const NNRepeatType type = repeat_types.GetAt(index);
					FString message;
					if (type == NNRepeatType::Repeat){
						message =
							FString("RepeatStart must be closed with RepeatEnd.");
					}else {//if (type == NNRepeatType::Recurrent)
						message =
							FString("RecurrentInput must be closed with RecurrentOutput.");
					}
					property.SetError(true, message);
				}
			}else {
				// error : repeat end appears before repeat start
				const int property_index = layer.GetPropertyIndexByName("Name");
				assert(property_index >= 0);
				NNLayerProperty &property = layer.GetProperty_(property_index);
				FString message;
				if (layer.GetType() == "RepeatEnd"){
					message = "RepeatEnd appeared before RepeatStart.";
				} else if (layer.GetType() == "RecurrentOutput"){
					message = "RepeatOutput appeared before RecurrentInput.";
				} else {//if (layer.GetType() == "Delay"){
					message = "Delay appeared before RecurrentInput.";
				}
				property.SetError(true, message);
			}
		}
	}

	{
		//Set group ids to this layer
		for (int ii = 0 ; ii < current_repeat_ids.GetSize() ; ii++) {
			layer_repeat_ids.GetAt(layer_index).Add(current_repeat_ids.GetAt(ii));
		}
	}

	//Set processed
	layer_processed.SetAt(layer_index, true);

	//Process recursive
	FListT<int> link_ids = EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
	if (link_ids.GetSize()) {
		for (int ili = 0 ; ili < link_ids.GetSize() ; ili++) {
			const NNLink *link = GetLinkByID(link_ids.GetAt(ili));
			if (link) {
				if (link->GetDestPinIndex() == 0){
					const int layer_id = link->GetDestLayerID();
					const int layer_index = GetLayerIndexByID(layer_id);
					CheckRepeatID(
						layer_index,
						current_repeat_ids,
						repeat_ids,
						repeat_types,
						repeat_length,
						layer_repeat_ids,
						layer_processed);
                }
			}
		}
	}else if (current_repeat_ids.GetSize()){
		//Reached the end of the network, but repeat_id still remains
		const int loop_start_layer_id = GetLayerIDByName(current_repeat_ids.GetAt(0));
		assert(loop_start_layer_id >= 0);
		const int loop_start_layer_index = GetLayerIndexByID(loop_start_layer_id);
		assert(loop_start_layer_index >= 0);
		NNLayer &loop_start_layer = layers_.GetAt(loop_start_layer_index);
		const int property_index = loop_start_layer.GetPropertyIndexByName("Name");
		assert(property_index >= 0);
		NNLayerProperty &property = loop_start_layer.GetProperty_(property_index);
		const int index = repeat_ids.IndexOf(current_repeat_ids.GetAt(0));
		assert(index >= 0);
		const NNRepeatType type = repeat_types.GetAt(index);
		FString message;
		if (type == NNRepeatType::Repeat) {
			message =
				FString("RepeatStart must be closed with RepeatEnd.");
		}else {//if (type == NNRepeatType::Recurrent)
			message =
				FString("RecurrentInput must be closed with RecurrentOutput.");
		}
		property.SetError(true, message);
	}
}
//---------------------------------------------------------------------------
void NNStructure::RefreshStatistics()
{
	static const int statistics_num = 8;
	static const FString statistics_item[statistics_num] = {
	FString("Output"),
	FString("CostParameter"),
	FString("CostAdd"),
	FString("CostMultiply"),
	FString("CostMultiplyAdd"),
	FString("CostDivision"),
	FString("CostExp"),
	FString("CostIf"),
	};
	statistics_.Clear();

	//Repeat parameters
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		if (layers_.GetAt(il).GetType() == "Parameter") {
			NNLayer &layer = layers_.GetAt(il);

			//Enum repeat ids from all destination layer
			FListT<int> link_ids = EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
			FListP<FStringList> repeat_ids_list;
			for (int ili = 0 ; ili < link_ids.GetSize() ; ili++) {
				const NNLink *link = GetLinkByID(link_ids.GetAt(ili));
				if (link) {
					const NNLayer *layer2 = GetLayerByID(link->GetDestLayerID());
					if (layer2) {
						FStringList &repeat_ids = repeat_ids_list.Add();
						for (int ir = 0 ; ir < layer2->GetRepeatIDs().GetSize(); ir++) {
							if (GetRepeatType(layer2->GetRepeatIDs().GetAt(ir)) == NNRepeatType::Repeat) {
								repeat_ids.Add(layer2->GetRepeatIDs().GetAt(ir));
							}
						}
					}
				}
			}

			if (repeat_ids_list.GetSize()) {
				//Check if every repeat ids are same
				int success = true;
				for (int irs = 1 ; irs < repeat_ids_list.GetSize() ; irs++) {
					success &= repeat_ids_list.GetAt(irs).GetText() == repeat_ids_list.GetAt(0).GetText();
				}

				if (success) {
					layer.SetRepeatIDs(repeat_ids_list.GetAt(0));
				}
            }
		}
	}

	//Calc statistics
	for (int is = 0 ; is < statistics_num ; is++) {
		const FString &label = statistics_item[is];
		NNStatistics &stat_1 = statistics_.Add();

		float sum = 0.0f, max = 0.0f;
		for (int il = 0 ; il < layers_.GetSize() ; il++) {
			const NNLayerProperty *property = layers_.GetAt(il).GetPropertyByName(label);
			if (property) {
				if (property->GetIntArray().GetSize()) {
					double v = is > 0 ?
						property->GetIntArray().GetAt(0) : //Scalar statistics
						property->GetIntArrayProduct();//Output

					const FStringList &layer_repeat = GetLayer(il).GetRepeatIDs();
					for (int ir = 0 ; ir < layer_repeat.GetSize() ; ir++) {
						//ToDo : Do not multiply if repeat is for parameter
						v *= GetRepeatNum(layer_repeat.GetAt(ir));
					}

					sum += v;
					if (v > max) {
						max = v;
					}
                }
			}
		}
		stat_1.Set(label, sum, max);
	}
};
//---------------------------------------------------------------------------
const NNStatistics *NNStructure::GetStatistics(const FString &label) const
{
	const int index = GetStatisticsIndexByLabel(label);
	if (index >= 0) {
		return &statistics_.GetAt(index);
	}
	return NULL;
}
//---------------------------------------------------------------------------
int NNStructure::GetStatisticsIndexByLabel(const FString &label) const
{
	for (int is = 0 ; is < statistics_.GetSize() ; is++) {
		if (statistics_.GetAt(is).GetLabel() == label) {
			return is;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
double NNStructure::GetStatistics_Sum(const FString &label) const
{
	const int index = GetStatisticsIndexByLabel(label);
	if (index >= 0) {
        return statistics_.GetAt(index).GetSum();
	}
	return 0.0;
}
//---------------------------------------------------------------------------
int NNStructure::GetRepeatInfoIndex(const FString &repeat_id) const
{
	for (int ir = 0 ; ir < repeat_info_.GetSize() ; ir++) {
		if (repeat_info_.GetAt(ir).GetID() == repeat_id) {
			return ir;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
int NNStructure::GetRepeatNum(const FString &repeat_id) const
{
	const int index = GetRepeatInfoIndex(repeat_id);
	if (index >= 0) {
		return repeat_info_.GetAt(index).GetNumRepeat();
	}
	return 1;
}
//---------------------------------------------------------------------------
FStringList NNStructure::GetRepeatExtensions(const NNLayer &layer, int weight) const
{
	FStringList result;
	FString parameter_scope = layer.GetDisplayPropertyValue(FString("ParameterScope"));
	if (parameter_scope == ""){
		//Parameter layer
		result.Add(layer.GetDisplayPropertyValue(FString("Name")));
	}else {
		result.Add(parameter_scope);
	}
	const FStringList &repeat_ids = layer.GetRepeatIDs();

	for (int ir = 0 ; ir < repeat_ids.GetSize() ; ir++) {
		const FString &repeat_id = repeat_ids.GetAt(ir);
		const NNRepeatType repeat_type = GetRepeatType(repeat_id);
		const int repeat_num = GetRepeatNum(repeat_id);
		if ((!weight || repeat_type != NNRepeatType::Recurrent) && repeat_num) {
			FStringList seed = result;
			result.Clear();
			for (int is = 0 ; is < seed.GetSize() ; is++) {
				for (int i = 0 ; i < repeat_num ; i++) {
					result.Add(
						seed.GetAt(is).ReplaceStr(
							FString("{") + repeat_id + FString("}"),
							FString("[") + FString(i) +	FString("]")));
				}
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------
FStringList NNStructure::SetExportedArgs(const FStringList &input_size, const NNLayer &layer)
{
	FStringList result;
	for (int is = 0 ; is < layer.GetNumSideConnector(0) + 1; is++) {
        result.Add(FString(""));
	}
	result.Add(FString(""));

	//Get input layer
	if (input_size.GetAt(0) != ""){
		FListT<int> input_layer_ids = EnumInputLayerID();
		for (int il = 0 ; il < input_layer_ids.GetSize() ; il++) {
			const NNLayer *input_layer = GetLayerByID(input_layer_ids.GetAt(il));
			if (input_layer) {
				if (input_layer->GetPropertyValue(FString("Generator")) == "None") {
					int order = input_layer->GetPropertyValue(FString("Order")).AsInt();
					if (order < 0) {
                        order = 0;
					}
					if (order < input_size.GetSize() && order < result.GetSize() && result.GetAt(order) == "") {
						//Set size of the input layer
						SetLayerPropertyValue(input_layer_ids.GetAt(il), FString("Size"), input_size.GetAt(order));
						result.GetAt(order) = GetLayerByID(input_layer_ids.GetAt(il))->GetPropertyValue(FString("Name"));
					}else {
						//error
                        return FStringList();
                    }
				}
            }
		}
	}

	//Set args according to layer properties
	for (int il = 0 ; il < GetNumLayer() ; il++) {
		const NNLayer &inner_layer = GetLayer(il);
		if (inner_layer.GetType() == "Argument"){
			const NNLayerProperty *property_name = inner_layer.GetPropertyByName("Name");

			if (property_name){
				const FString &value = layer.GetDisplayPropertyValue(property_name->GetDisplayValue());
				SetLayerPropertyValue(inner_layer.GetLayerID(), FString("Value"), value);
			}
		}
	}

	//Set weight properties
	for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
		const NNLayerProperty &property = layer.GetProperty(ip);
		const int pos = property.GetName().PosChar('.');
		if (pos){//if (property.GetDynamic())
			//Property has scope
			FString property_name = property.GetName().SubString(pos + 1, property.GetName().Length() - pos);
			FString inner_layer_name = property.GetName().SubString(1, pos - 1);
			int inner_layer_id = GetLayerIDByName(inner_layer_name);
			if (inner_layer_id >= 0){
				const NNLayer *inner_layer = GetLayerByID(inner_layer_id);
				if (inner_layer){
					SetLayerPropertyValue(inner_layer->GetLayerID(), property_name, property.GetDisplayValue(), false);
				}
			}
		}
	}
	return result;//input layer name
}
//---------------------------------------------------------------------------
NNRepeatType NNStructure::GetRepeatType(const FString &repeat_id) const
{
	for (int ir = 0 ; ir < repeat_info_.GetSize() ; ir++) {
		if (repeat_info_.GetAt(ir).GetID() == repeat_id) {
			return repeat_info_.GetAt(ir).GetType();
		}
	}
	return NNRepeatType::None;
}
//---------------------------------------------------------------------------
void NNStructure::Escape(int layer_index)
{
	NNLayer &layer = layers_.GetAt(layer_index);
	if (layer.GetPosition(0) < 0.0f) {
		layer.SetPosition(0, 0.0f);
	}
	if (layer.GetPosition(1) < 0.0f) {
		layer.SetPosition(1, 0.0f);
	}
	for (int il = 0 ; il < layers_.GetSize() ; il++) if (il != layer_index){
		NNLayer &layer2 = layers_.GetAt(il);
		if (fabs(layer2.GetPosition(1) - layer.GetPosition(1)) < NNLayerDefaultHeight) {
			if (fabs(layer2.GetPosition(0) - layer.GetPosition(0)) < NNLayerDefaultWidth + NNLayerDefaultHSpace) {
				const int ix = (int)((layer2.GetPosition(0) + layer2.GetPosition(2) + NNLayerDefaultHSpace) / NNLayerDefaultGrid + 0.5);
				layer.SetPosition(0, ix * NNLayerDefaultGrid);
			}
		}
	}
}
//---------------------------------------------------------------------------
void NNStructure::EscapeLayerName(FString allowed_char)
{
	for (int il = 0 ; il < layers_.GetSize(); il++) {
        EscapeLayerName(il, allowed_char);
	}
}
//---------------------------------------------------------------------------
FString NNStructure::EscapeNameStr(const FString &name_str, FString allowed_char)
{
	char *tmp = new char[name_str.Length() + 1];
	for (int i = 0 ; i < name_str.Length() ; i++) {
		const char c = name_str.PChar()[i];
		if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '\'' || c == '/' || allowed_char.Pos(name_str.SubString(i + 1, 1))) {
			tmp[i] = c;
		}else {
			tmp[i] = ' ';
		}
	}
	tmp[name_str.Length()] = '\0';
	FString s = FString(tmp);
	delete[] tmp;
	return s.ReplaceStr(" ", "");
}
//---------------------------------------------------------------------------
void NNStructure::EscapeLayerName(int layer_index, FString allowed_char)
{
	NNLayer &layer = layers_.GetAt(layer_index);
	FString layer_name = layer.GetPropertyValue(FString("Name"));
	if (layer_name == "") {
		layer_name = layer.GetType();
		layer.SetPropertyValue(FString("Name"), layer_name);
	}
	layer_name = layer_name.ReplaceStr("#", "_");

	layer_name = EscapeNameStr(layer_name, allowed_char);

	if (layer.GetPropertyByName("Name")->GetValue() != layer_name) {
		layer.SetPropertyValue(FString("Name"), layer_name);
	}
	layer_name = layer_name.SplitBy('_');
	int name_index = 1;
	while (true){
		int same_name_layer = false;
		for (int il = 0 ; il < layers_.GetSize() ; il++) if (il != layer_index){
			if (layers_.GetAt(il).GetPropertyByName("Name")->GetValue() ==
				layer.GetPropertyByName("Name")->GetValue()) {
				name_index++;
				layer.SetPropertyValue(FString("Name"), layer_name + FString("_") + FString(name_index));
				same_name_layer = true;
				break;
			}
		}
		if (!same_name_layer) {
			break;
		}
	}
}
//---------------------------------------------------------------------------
int NNStructure::CountDuplication() const
{
	int count = 0;
	for (int il = 0 ; il < GetNumLayer() ; il++) {
		const NNLayer &layer = GetLayer(il);
		const int component_index = nn_layer_components_->GetComponentIndexByType(layer.GetType());
		if (component_index >= 0) {
			const NNLayerComponent &component = nn_layer_components_->GetAt(component_index);

			FListT<int> link_ids = EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
			for (int ili = 0 ; ili < link_ids.GetSize() ; ili++) {
				const NNLink *link = GetLinkByID(link_ids.GetAt(ili));
				const NNLayer *layer2 = GetLayerByID(link->GetDestLayerID());
				if (layer.GetType() == layer2->GetType()) {
					count++;
				}else if (component.GetCategory() != "Others"){
					const int component_index2 = nn_layer_components_->GetComponentIndexByType(layer2->GetType());
					if (component_index2 >= 0) {
						const NNLayerComponent &component2 = nn_layer_components_->GetAt(component_index2);
						if (component.GetCategory() == component2.GetCategory()) {
							count++;
						}
					}
				}
			}
		}
	}
	return count;
}
//---------------------------------------------------------------------------
int NNStructure::ExpandAndFix(const FStringList &used_networks)
{
	//Refresh args for inner layers
	RefreshDisplayValue();
	NNStructure before_expand_structure;
	before_expand_structure = *this;

	int result = true;
	int expanded = false;
	for (int il = 0 ; il < GetNumLayer() ; il++) {
		const NNLayer &layer = GetLayer(il);
		const int is_unit = layer.GetType() == "Unit";
		const int is_import = layer.GetType() == "Import";
		if (is_unit || is_import) {
			// Find inner network
			const NNLayerProperty *property_input = layer.GetAvailableInputProperty();
			const NNLayerProperty *property_network = layer.GetPropertyByName("Network");
			const NNLayerProperty *property_parameter_scope = layer.GetPropertyByName("ParameterScope");
			if (property_network && (is_import || (property_input && property_parameter_scope))){
				FString network_name = property_network->GetDisplayValue();
				const int network_index = related_networks_->GetStructureIndexByName(network_name);
				if (used_networks.IndexOf(network_name) < 0 && network_index >= 0){
					NNStructure inner_network;
					inner_network = related_networks_->GetStructure(network_index);
					inner_network.SetRelatedNetworks(related_networks_);

					FStringList input_layer_names;
					if (is_unit) {
						// Set args
						FStringList input_size;
						input_size.Add(property_input->GetDisplayValue());
						for (int is = 0 ; is < layer.GetNumSideConnector(0) ; is++) {
							input_size.Add(layer.GetSideConnector(0, is).GetCurrentShape());
						}
						input_layer_names = inner_network.SetExportedArgs(input_size, layer);
					}

					FStringList used_networks2 = used_networks;
                    used_networks2.Add(network_name);
					result &= inner_network.ExpandAndFix(used_networks2);

					//Set layer names in the unit
					/*
					Do not use "ParameterScope" as prefix. it should be "Name" to support multiple unit sharing same parameters,
					If "ParameterScope" is used for prefix, layer name will be duplicated and escaped at Paste function.
					And the function in the unit will not be connected properly to the destination layer in the calling network.

					FString parameter_scope_prefix = layer.GetDisplayPropertyValue(FString("ParameterScope"));
					if (parameter_scope_prefix != ""){
						parameter_scope_prefix += FString("/");
					}
					*/
					FString parameter_scope_prefix = layer.GetDisplayPropertyValue(FString("Name")) + FString("/");
					FString outputs_layer_name_;
					if (is_unit) {
						for (int iul = 0 ; iul < inner_network.GetNumLayer() ; iul++) {
							const NNLayer &u_layer = inner_network.GetLayer(iul);
							//Set name of layer in the unit
							FString layer_name = parameter_scope_prefix + u_layer.GetPropertyValue(FString("Name"));
							inner_network.SetLayerPropertyValue(u_layer.GetLayerID(), FString("Name"), layer_name, false);

							//Set parameter scope in the unit
							const NNLayerProperty *u_property_parameter_scope = u_layer.GetPropertyByName("ParameterScope");
							if (u_property_parameter_scope){
								FString parameter_scope = u_property_parameter_scope->GetValue();
								FString path_str;
								if (property_parameter_scope->GetDisplayValue() != ""){
									path_str = property_parameter_scope->GetDisplayValue() + FString("/");
								}

								//replace inner_repeat_id
								{
									FString inner_scope = parameter_scope;
									parameter_scope = "";
									while (inner_scope != ""){
										parameter_scope += inner_scope.SplitBy('{');
										if (inner_scope != ""){
											parameter_scope +=
												FString("{") + parameter_scope_prefix +
												inner_scope.SplitBy('}') + FString("}");
										}
									}
								}

								//Repeat parameter scope
								const NNLayer *b_layer = before_expand_structure.GetLayerByID(layer.GetLayerID());
								if (b_layer) if (b_layer->GetRepeatIDs().GetSize()){
									path_str = property_parameter_scope->GetDisplayValue();
									for (int ir = 0 ; ir < b_layer->GetRepeatIDs().GetSize() ; ir++){
										if (before_expand_structure.GetRepeatType(b_layer->GetRepeatIDs().GetAt(ir)) != NNRepeatType::Recurrent){
											path_str += FString("{") + b_layer->GetRepeatIDs().GetAt(ir) + FString("}");
										}
									}
									path_str += "/";
								}
								inner_network.SetLayerPropertyValue(u_layer.GetLayerID(), FString("ParameterScope"), path_str + parameter_scope, false);
							}

							//Set repeat id in the unit
							const NNLayerProperty *u_property_repeat_id = u_layer.GetPropertyByName("ID");
							if (u_property_repeat_id){
								inner_network.SetLayerPropertyValue(u_layer.GetLayerID(), FString("ID"), parameter_scope_prefix + u_property_repeat_id->GetDisplayValue(), false, true);
							}

							if (inner_network.EnumLinkIDByDestLayerID(u_layer.GetLayerID(), 0).GetSize() &&
								inner_network.EnumLinkIDBySourceLayerID(u_layer.GetLayerID(), 0).GetSize() == 0){
								outputs_layer_name_ = layer_name;
							}
						}
                    }

					//Paste unit to current structure
					Paste(inner_network, false, FString("/"));
					expanded = true;

					if (is_unit) {
						//Connect input
						for (int ic = 0 ; ic < layer.GetNumSideConnector(0) + 1; ic++) {
							FListT<int> link_ids = EnumLinkIDByDestLayerID(layer.GetLayerID(), ic);
							if (link_ids.GetSize() == 1){
								const NNLink *existing_link = GetLinkByID(link_ids.GetAt(0));
								assert(existing_link);

								//Connect to the Input layer in the unit
								FString layer_name = parameter_scope_prefix + input_layer_names.GetAt(ic);

								const int unit_input_layer_id = GetLayerIDByName(layer_name);
								FListT<int> link_ids2 = EnumLinkIDBySourceLayerID(unit_input_layer_id, 0);
								FListT<int> dest_layer_ids, dest_pin_indexes;
								for (int ili = 0 ; ili < link_ids2.GetSize() ; ili++){
									const NNLink *input_link = GetLinkByID(link_ids2.GetAt(ili));
									assert(input_link);
									dest_layer_ids.Add(input_link->GetDestLayerID());
									dest_pin_indexes.Add(input_link->GetDestPinIndex());

								}
								//delete original input layer
								ClearSelection();
								SetLayerSelected(unit_input_layer_id, true, false);
								DeleteSelected();

								//Connect
								for (int ili = 0 ; ili < dest_layer_ids.GetSize() ; ili++){
									AddLink(
										existing_link->GetSourceLayerID(), existing_link->GetSourcePinIndex(),
										dest_layer_ids.GetAt(ili), dest_pin_indexes.GetAt(ili));
								}
							}
						}


						//Connect Output
						{
							FListT<int> link_ids = EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
							for (int il = 0 ; il < link_ids.GetSize() ; il++){
								const NNLink *existing_link = GetLinkByID(link_ids.GetAt(il));
								assert(existing_link);
								const int dest_layer_Id = existing_link->GetDestLayerID();
								const int dest_layer_pin_index = existing_link->GetDestPinIndex();


								//Delete existing link
								ClearSelection();
								SetCurrentLinkID(link_ids.GetAt(il));
								DeleteSelected();

								//Add link
								const int output_layer_id = GetLayerIDByName(outputs_layer_name_);
								assert(output_layer_id >= 0);
								AddLink(
									output_layer_id, 0,
									dest_layer_Id, dest_layer_pin_index);
							}
						}
					}

					//Delete network layer
					ClearSelection();
					SetLayerSelected(layer.GetLayerID(), true, false);
					DeleteSelected();
					il -= 1;// to expand next layer of the expanded unit layer
				}
			}
		}
	}

	if (expanded){
		RefreshDisplayValue();
	}
	RefreshStatistics();

	//For debug
	if (!GetCompleteNetwork()){
		for (int i = 0 ; i < errors_.GetSize() ; i++){
			const NNError &error = errors_.GetAt(i);
			error.GetString();
		}
	}

	FixProperties();

	//Repeat parameter scope
	for (int il = 0 ; il < GetNumLayer() ; il++) {
		const NNLayer &layer = GetLayer(il);
		//apply repeat_id to path_str
		const NNLayer *b_layer = before_expand_structure.GetLayerByID(layer.GetLayerID());
		if (b_layer) if (b_layer->GetRepeatIDs().GetSize()){
			const NNLayerProperty *property_parameter_scope = layer.GetPropertyByName("ParameterScope");
			if (property_parameter_scope){
                FString parameter_scope = property_parameter_scope->GetValue();
				FString new_parameter_scope = parameter_scope.SplitBy('/');
				for (int ir = 0 ; ir < b_layer->GetRepeatIDs().GetSize() ; ir++){
					if (before_expand_structure.GetRepeatType(b_layer->GetRepeatIDs().GetAt(ir)) != NNRepeatType::Recurrent){
						new_parameter_scope += FString("{") + b_layer->GetRepeatIDs().GetAt(ir) + FString("}");
                    }
				}
				if (parameter_scope != ""){
					new_parameter_scope += FString("/") + parameter_scope;
				}
				SetLayerPropertyValue(layer.GetLayerID(), FString("ParameterScope"), new_parameter_scope, false);
            }
		}
	}

	return result;
}
//---------------------------------------------------------------------------
int NNStructure::IsSameInOutSetting(const NNStructure &source) const
{
	FListT<int> check_layer_id = EnumInputLayerID();
	check_layer_id.Add(EnumTerminalLayerID());
	check_layer_id.Sort();

	FListT<int> check_layer_id_source = source.EnumInputLayerID();
	check_layer_id_source.Add(source.EnumTerminalLayerID());
	check_layer_id_source.Sort();

	if (!(check_layer_id == check_layer_id_source)){
		return false;
	}

	for (int il = 0 ; il < check_layer_id.GetSize() ; il++) {
		const NNLayer *layer = GetLayerByID(check_layer_id.GetAt(il));
		assert(layer);
		const NNLayer *source_layer = source.GetLayerByID(layer->GetLayerID());
		assert(source_layer);

		const NNLayerComponent *layer_component = layer->GetComponent();
		const int is_loss_function = (layer_component ? layer_component->GetOutputType() == NNLayerComponentIOType::None : false) || layer->GetPropertyIndexByName("IsLossFunction") >= 0;
		if (is_loss_function){
			//loss function must be same
			if (!layer->IsSame(*source_layer)) {
				return false;
			}
		}else {
			//Check if output size is same
			const NNLayerProperty *output = layer->GetPropertyByName("Output");
			const NNLayerProperty *output_source = source_layer->GetPropertyByName("Output");
			if (output && output_source){
				if (output->GetDisplayValue() != output_source->GetDisplayValue()){
					return false;
				}
			}
		}

		/*
		//Check if input link structure is same
		FListT<int> input_link = EnumLinkIDByDestLayerID(layer->GetLayerID(), -1);
		FListT<int> input_link_source = EnumLinkIDByDestLayerID(source_layer->GetLayerID(), -1);
		if (input_link.GetSize() != input_link_source.GetSize()){
			return false;
		}

		//Check if output size of previous layer is same
		*/
	}
	/*
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		const NNLayer &layer = layers_.GetAt(il);
		const NNLayerComponent *layer_component = layer.GetComponent();
		if (layer_component){
			if (layer.GetType() == "Input" ||
				layer_component->GetOutputType() == NNLayerComponentIOType::None ||
				layer.GetPropertyIndexByName("IsLossFunction") >= 0) {
				const NNLayer *source_layer = source.GetLayerByID(layer.GetLayerID());
				if (source_layer) {
					if (!layer.IsSame(*source_layer)) {
						return false;
					}
				}else {
					return false;
				}
			}
		}
	}
	*/
	return true;
}
//---------------------------------------------------------------------------
void NNStructure::FixProperties()
{
	for (int il = 0 ; il < layers_.GetSize() ; il++) {
		layers_.GetAt(il).FixProperties();
	}
}
//---------------------------------------------------------------------------
void NNStructure::EmbedParameters()
{
	ClearSelection();
	for (int il = 0 ; il < GetNumLayer() ; il++) {
		const NNLayer &layer = GetLayer(il);
		if (layer.GetType() == "Parameter"){
			const FListT<int> link_ids = EnumLinkIDBySourceLayerID(layer.GetLayerID(), -1);
			if (link_ids.GetSize() == 1){
				const NNLink *link = GetLinkByID(link_ids.GetAt(0));
				if (link){
					const int layer_id = link->GetDestLayerID();
					const NNLayer *dest_layer = GetLayerByID(layer_id);
					if (link->GetDestPinIndex() > 0 && dest_layer && dest_layer->GetNumSideConnector(0) >= link->GetDestPinIndex()){
						if (dest_layer->GetSideConnector(0, link->GetDestPinIndex() - 1).GetKind() == NNLayerComponentInputKind::Parameter){
							FString side_connector_name = dest_layer->GetSideConnector(0, link->GetDestPinIndex() - 1).GetName();
							SetLayerPropertyValue(layer_id, side_connector_name + FString(".File"), layer.GetPropertyValue(FString("File")));
							SetLayerPropertyValue(layer_id, side_connector_name + FString(".Initializer"), layer.GetPropertyValue(FString("Initializer")));
							SetLayerPropertyValue(layer_id, side_connector_name + FString(".InitializerMultiplier"), layer.GetPropertyValue(FString("InitializerMultiplier")));
							SetLayerPropertyValue(layer_id, side_connector_name + FString(".LRateMultiplier"), layer.GetPropertyValue(FString("LRateMultiplier")));
							SetLayerSelected(layer.GetLayerID(), true, false);
						}
					}
				}
			}
		}
	}
	DeleteSelected();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
NNStructures &NNStructures::operator=(const NNStructures &source)
{
    Clear();
	structures_.Clear();

	for (int i = 0; i < source.structures_.GetSize() ; i++) {
		structures_.Add(source.structures_.GetAt(i));
	}
	active_structure_index_ = source.active_structure_index_;
    NNStructure::operator=(source);

	return *this;
}
//---------------------------------------------------------------------------
void NNStructures::Clear()
{
	structures_.Clear();
	structures_.Add();

	NNStructure::Clear();
	active_structure_index_ = 0;
}
//---------------------------------------------------------------------------
int NNStructures::SetName(const FString &_name)
{
	FString name = EscapeNameStr(_name);
	if (GetName() == name || name != _name) {
		return false;
	}
	if (name == "") {
		return false;
	}
	for (int i = 0; i < structures_.GetSize() ; i++) if (i != active_structure_index_){
		if (structures_.GetAt(i).GetName() == name) {
			return false;
		}
	}
	NNStructure::SetName(name);
	return true;
}
//---------------------------------------------------------------------------
const NNStructure &NNStructures::GetStructure(int structure_index) const
{
	if (structure_index == active_structure_index_) {
		return *this;
	}else {
		return structures_.GetAt(structure_index);
    }
}
//---------------------------------------------------------------------------
NNStructure &NNStructures::GetStructure_(int structure_index)
{
	if (structure_index == active_structure_index_) {
		return *this;
	}else {
		NNStructure &structure = structures_.GetAt(structure_index);
		structure.SetNNLayerComponents(nn_layer_components_);
		structure.SetNNStructureSearchPolicies(nn_structure_search_policies_);
		return structure;
	}
}
//---------------------------------------------------------------------------
void NNStructures::SetActiveStructure(int structure_index)
{
	if (active_structure_index_ == structure_index){
		return;
	}

	structures_.GetAt(active_structure_index_) = *this;
	active_structure_index_ = structure_index;

	int update_count_bak = update_count_;
	const NNLayerComponents *nn_layer_components_bak = nn_layer_components_;
	const NNStructureSearchPolicies *nn_structure_search_policies_bak = nn_structure_search_policies_;

	*dynamic_cast<NNStructure*>(this) = structures_.GetAt(active_structure_index_);

	update_count_ = update_count_bak;
	nn_layer_components_ = nn_layer_components_bak;
	nn_structure_search_policies_ = nn_structure_search_policies_bak;

	structures_.GetAt(active_structure_index_).Clear();
	ClearSelection();
	ClearInitialOffset();
	Update();

	RefreshDisplayValue();
    RefreshStatistics();
}
//---------------------------------------------------------------------------
int NNStructures::GetStructureIndexByName(const FString &structure_name) const
{
	if (GetName() == structure_name) {
		return active_structure_index_;
	}
	for (int i = 0; i < structures_.GetSize() ; i++) if (i != active_structure_index_){
		if (structures_.GetAt(i).GetName() == structure_name) {
			return i;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
void NNStructures::SetActiveStructure(const FString &structure_name)
{
	const int structure_index = GetStructureIndexByName(structure_name);
	if (structure_index >= 0) {
        SetActiveStructure(structure_index);
	}
}
//---------------------------------------------------------------------------
void NNStructures::AddStructure()
{
	FString new_structure_name;
	int index = 2;
	while (true){
		new_structure_name = FString("Network_") + FString(index);
		index++;
		int found = GetName() == new_structure_name;
		for (int i = 0 ; i < structures_.GetSize() && !found ; i++) {
			found |= structures_.GetAt(i).GetName() == new_structure_name;
		}
		if (!found) {
            break;
		}
	}
	NNStructure &structure = structures_.Add();
	structure.SetName(new_structure_name);
	SetActiveStructure(structures_.GetSize() - 1);
}
//---------------------------------------------------------------------------
void NNStructures::DeleteStructure(int structure_index)
{
	if (structures_.GetSize() == 1) {
		Clear();
		return;
	}

	const int active_structure_index_bak = active_structure_index_;
	if (structure_index == active_structure_index_) {
		SetActiveStructure(structure_index == 0);
	}
	structures_.Delete(structure_index);
	if (active_structure_index_ >= structure_index){
		active_structure_index_--;
	}
	SetActiveStructure(active_structure_index_bak < GetNumStructure() ? active_structure_index_bak : GetNumStructure() - 1);
	Update();
}
//---------------------------------------------------------------------------
void NNStructures::MoveStructure(int new_structure_index)
{
	if (active_structure_index_ == new_structure_index){
		return;
	}

	structures_.Delete(active_structure_index_);
	structures_.Insert(new_structure_index, NNStructure());
	active_structure_index_ = new_structure_index;
}
//---------------------------------------------------------------------------
void NNStructures::RefreshDisplayValue(const NNStructures *related_networks, int test)
{
	SetRelatedNetworks(related_networks);
	NNStructure::RefreshDisplayValue(test);
}
//---------------------------------------------------------------------------
void NNStructures::CheckError(const FListT<int> &structure_indexes)
{
	/*
	FListT<int> structure_indexes = _structure_indexes;
	if (!structure_indexes.GetSize()){
		for (int i = 0; i < structures_.GetSize() ; i++){
			structure_indexes.Add(i);
		}
	}
	*/

	FListP<NNError> errors;
	for (int ii = 0; ii < structure_indexes.GetSize() ; ii++) {
		const int i = structure_indexes.GetAt(ii);
		NNStructure &structure = GetStructure_(i);
        structure.RefreshDisplayValue();
		structure.CheckError();
		complete_network_ &= structure.GetCompleteNetwork();
		errors.Add(structure.GetErrors());
	}
	errors_ = errors;

	{
		//Check variable size between networks
		FStringList layer_names;
		FListT<int> network_index;
		FListT<const NNLayer*> all_layers;

		for (int ii = 0; ii < structure_indexes.GetSize() ; ii++) {
			const int in = structure_indexes.GetAt(ii);
			const NNStructure &structure = GetStructure(in);
			for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
				const NNLayer &layer = structure.GetLayer(il);
				FString layer_name = layer.GetPropertyValue(FString("Name"));
				const int index = layer_names.IndexOf(layer_name);
				if (index >= 0) {
					FString output = layer.GetDisplayPropertyValue(FString("Output"));
					if (all_layers.GetAt(index)->GetDisplayPropertyValue(FString("Output")) != output){
						NNError &new_error = errors_.Add();
						new_error.SetNetworkName(structure.GetName());
						new_error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
						new_error.SetString(
							structure.GetName() +
							FString("->") +
							layer_name +
							FString(" : Differs between network \"") +
							GetStructure(network_index.GetAt(index)).GetName() +
							FString("\" and network \"") +
							structure.GetName() +
							FString("\"")
							);
						complete_network_ = false;
					}
				}else {
					layer_names.Add(layer_name);
					network_index.Add(in);
					all_layers.Add(&layer);
				}
			}
		}
	}
	{
		//Check parameter_scope between networks
		FStringList parameter_names;
		FListT<int> network_index;
		FListT<const NNLayer*> all_layers;

		for (int ii = 0; ii < structure_indexes.GetSize() ; ii++) {
			const int in = structure_indexes.GetAt(ii);
			const NNStructure &structure = GetStructure(in);
			for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
				const NNLayer &layer = structure.GetLayer(il);
				FString parameter_name = layer.GetDisplayPropertyValue(FString("ParameterScope"));
				if (parameter_name != ""){
					for (int is = 0 ; is < layer.GetNumSideConnector(0) ; is++) {
						if (!layer.GetInputPinConnected(is + 1)){
							FString side_connector_name = layer.GetSideConnector(0, is).GetName();
							FString param_connector_name = parameter_name + FString("/") + side_connector_name;
							const int index = parameter_names.IndexOf(param_connector_name);
							if (index >= 0) {
								const NNLayer &layer2 = *all_layers.GetAt(index);
								if (layer.GetType() == layer2.GetType() && layer.GetNumProperty() == layer2.GetNumProperty()){
									int same = true;
									FString key_value;
									for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++){
										if (layer.GetProperty(ip).GetName() == layer2.GetProperty(ip).GetName() &&
											layer.GetProperty(ip).GetName().SubString(1, side_connector_name.Length() + 1) == side_connector_name + FString(".") &&
											layer.GetProperty(ip).GetName().Pos("LRateMultiplier") <= 0){
											if (layer.GetProperty(ip).GetDisplayValue() != layer2.GetProperty(ip).GetDisplayValue()) {
												if (!same){
													key_value += FString(", ");
												}
												same = false;
												key_value +=
													layer.GetProperty(ip).GetName() +
													FString(" : ") +
													layer.GetProperty(ip).GetDisplayValue() +
													FString(" != ") +
													layer2.GetProperty(ip).GetDisplayValue();
											}
										}
									}
									if (!same){
										NNError &new_error = errors_.Add();
										new_error.SetNetworkName(structure.GetName());
										new_error.SetLayerName(layer.GetDisplayPropertyValue(FString("Name")));
										//The parameter C of B in A does not match the parameter F of E in D
										new_error.SetString(
											structure.GetName() +
											FString("->") +
											layer.GetPropertyValue(FString("Name")) +
											FString(" : Parameter \"") +
											side_connector_name +
											FString("\" does not match between \"") +
											layer.GetPropertyValue(FString("Name")) +
											(&structure != &GetStructure(network_index.GetAt(index)) ? FString("\" in network \"") + structure.GetName() :  FString("")) +
											FString("\" and \"") +
											layer2.GetPropertyValue(FString("Name")) +
											(&structure != &GetStructure(network_index.GetAt(index)) ? FString("\" in network \"") + GetStructure(network_index.GetAt(index)).GetName() :  FString("")) +
											FString("\" (") +
											key_value +
											FString(")")
											);
										complete_network_ = false;
									}
								}
							}else {
								parameter_names.Add(param_connector_name);
								network_index.Add(in);
								all_layers.Add(&layer);
							}
						}
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void NNStructures::GlobalEscapeLayerName(int layer_id, FString allowed_char, int structure_index)
{
	if (structure_index < 0){
		structure_index = GetActiveStructureIndex();
	}
    NNStructure &current_structure = GetStructure_(structure_index);

	FStringList all_layer_name;
	for (int in = 0 ; in < structures_.GetSize() ; in++) {
		const NNStructure &structure = GetStructure(in);
		for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
			const NNLayer &layer2 = structure.GetLayer(il);
			if (in != structure_index || layer2.GetLayerID() != layer_id) {
				all_layer_name.Add(layer2.GetPropertyValue(FString("Name")));
			}
		}
	}

	const NNLayer *layer = current_structure.GetLayerByID(layer_id);
	assert(layer);
	FString layer_name = layer->GetPropertyValue(FString("Name"));
	if (layer_name == "") {
		layer_name = layer->GetType();
	}
	layer_name = layer_name.ReplaceStr("#", "_");
	layer_name = EscapeNameStr(layer_name, allowed_char);

	FString base_layer_name = layer_name;
	base_layer_name = base_layer_name.SplitBy('_');
	int name_index = 1;
	while (true){
		int same_name_layer = false;
		for (int i = 0 ; i < all_layer_name.GetSize() && !same_name_layer ; i++) {
			if (all_layer_name.GetAt(i) == layer_name) {
				name_index++;
				layer_name = base_layer_name + FString("_") + FString(name_index);
				same_name_layer = true;
			}
		}
		if (!same_name_layer) {
			break;
		}
	}
	current_structure.SetLayerPropertyValue(layer_id, FString("Name"), layer_name);
}
//---------------------------------------------------------------------------
void NNStructures::SetRelatedNetworks(const NNStructures *related_networks)
{
	for (int in = 0 ; in < structures_.GetSize() ; in++) {
		structures_.GetAt(in).SetRelatedNetworks(related_networks ? related_networks : this);
	}
	NNStructure::SetRelatedNetworks(related_networks ? related_networks : this);
}
//---------------------------------------------------------------------------
int NNStructures::Replace(FString from, FString to, int target_all_structure, int target_selection)
{
	int result = 0;
	if (target_all_structure) {
		//Replace in current structure
		result = NNStructure::Replace(from, to, target_selection);
	}else {
        //Replace in all structures
		for (int in = 0 ; in < GetNumStructure() ; in++) {
			result += GetStructure_(in).Replace(from, to, target_selection);
		}
	}
	return result;
}
//---------------------------------------------------------------------------
int NNStructures::Mutate(FListT<int> used_structure_index)
{
	if (!used_structure_index.GetSize()){
		for (int i = 0; i < GetNumStructure() ; i++){
            used_structure_index.Add(i);
		}
	}

	//Enum structure to mutate
	static const FString setting = FString("StructureSearch");
	FListT<int> structure_indices;
	for (int in = 0 ; in < GetNumStructure() ; in++) {
		const NNStructure &structure = GetStructure(in);
		FListT<NNLayer*> setting_layer = structure.GetLayerByType(setting, true);
		int disable = false;//Default : Search = true
		if (setting_layer.GetSize()){
			const NNLayerProperty *property = setting_layer.GetAt(0)->GetPropertyByName(FString("Search"));
			if (property){
				disable = !property->GetIntArrayProduct();
			}
		}
		if (!disable) {
			structure_indices.Add(in);
		}
	}

	if (!structure_indices.GetSize()){
		return false;
	}

	//mutate
	SetRelatedNetworks();
	NNStructures backup;
	backup = *this;

	int result = false;
	int trial = 0;
	int structure_to_mutate = structure_indices.GetAt(rand() % structure_indices.GetSize());
		while (trial < 100){
		FListT<int> added_layer_id;
		int result_1 = GetStructure_(structure_to_mutate).MutateSub(added_layer_id);
		if (result_1) {
			if (added_layer_id.GetSize()){
				for (int il = 0 ; il < added_layer_id.GetSize() ; il++){
					GlobalEscapeLayerName(added_layer_id.GetAt(il), FString(""), structure_to_mutate);
				}
			}
			for (int in = 0 ; in < GetNumStructure() ; in++) {
				NNStructure &structure = GetStructure_(in);
				structure.ClearCache();
				structure.RefreshDisplayValue();
			}
			for (int in2 = 0 ; in2 < used_structure_index.GetSize() && result_1 ; in2++) {
				const int in = used_structure_index.GetAt(in2);
				const NNStructure &structure = GetStructure(in);
				const NNStructure &structure_bak = backup.GetStructure(in);
				result_1 &= structure.GetCompleteNetwork() && structure_bak.IsSameInOutSetting(structure);
			}
			if (result_1) {
				result = true;
				break;
			}
		}

		*this = backup;
		trial++;
		if (trial % 20 == 0){
			structure_to_mutate = structure_indices.GetAt(rand() % structure_indices.GetSize());
		}
	}

	if (result && structure_to_mutate != GetActiveStructureIndex()){
		SetActiveStructure(structure_to_mutate);
	}

	return result;
}
//---------------------------------------------------------------------------
void NNStructures::ArrangeAllStructure()
{
	for (int is = 0 ; is < GetNumStructure() ; is++){
		GetStructure_(is).ArrangeLayers(true);
	}
}
//---------------------------------------------------------------------------
void NNStructures::ClearParameterFiles()
{
	for (int in = 0 ; in < GetNumStructure() ; in++) {
		GetStructure_(in).ClearParameterFiles();
	}
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
