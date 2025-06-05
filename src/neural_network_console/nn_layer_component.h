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

#ifndef nn_layer_componentH
#define nn_layer_componentH
//---------------------------------------------------------------------------
#include "flist.h"
#include "fstring.h"
#include "ffastini.h"

#include "fstring.h"

namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
enum class NNLayerComponentIOType : int{
None,
Single,
SingleOrNone,
Multiple,
Any,
};
//---------------------------------------------------------------------------
enum class NNLayerComponentDefaultPosition : int{
None,
Top,
};
//---------------------------------------------------------------------------
enum class NNLayerComponentComponentType : int{
Normal,
Comment,
Argument,
Setting,
};
//---------------------------------------------------------------------------
//You also need to edit NNLayerPropertyType in nn_structure.h to add new type
enum class NNLayerComponentPropertyType : int{
Boolean,
BooleanOrMacro,
Int,
IntArray,
PInt,			//Positive Int (not include 0)
PIntArray,
PIntArrays,
UInt,           //Unsigned Int (include 0)
UIntArray,
Float,
FloatArray,
FloatArrays,
Option,
Dataset,
Text,
TextArray,
File,
Model,
Any,
Error
};
//---------------------------------------------------------------------------
enum class NNLayerComponentInputKind : int{
Data,
Parameter,
};
//---------------------------------------------------------------------------
class NNLayerComponentProperty
{
public:
	NNLayerComponentProperty() :
		editable_(true),
		important_(false),
		required_(true),
		search_parameter_(false),
		type_(NNLayerComponentPropertyType::PInt),
		visible_(true) {}
	virtual ~NNLayerComponentProperty() {}
public:
	int LoadFromIniSection(FFastIni &ini, const char *section, FStringList &errors);
public:
	void SetName(const FString &name) {name_ = name;}
	const FString &GetName() const {return name_;}
	void SetShortName(const FString &short_name) {short_name_ = short_name.SubString(1, 12);}
	const FString &GetShortName() const {return short_name_;}
	void SetArgumentName(const FString &argument_name) {argument_name_ = argument_name;}
	const FString &GetArgumentName() const {return argument_name_;}
	void SetEditable(int editable) {editable_ = editable;}
	int GetEditable() const {return editable_;}
	void SetImportant(int important) {important_ = important;}
	int GetImportant() const {return important_;}
	void SetRequired(int required) {required_ = required;}
	int GetRequired() const {return required_;}
	void SetSearchParameter(int search_parameter) {search_parameter_ = search_parameter;}
	int GetSearchParameter() const {return search_parameter_;}
	void SetType(NNLayerComponentPropertyType type) {type_ = type;}
	NNLayerComponentPropertyType GetType() const {return type_;}
	void SetDefaultValue(FString default_value) {default_value_ = default_value;}
	const FString &GetDefaultValue() const {return default_value_;}
	const FStringList &GetOptions() const {return options_;}
	void SetVisible(int visible) {visible_ = visible;}
	int GetVisible() const {return visible_;}
private:
	FString name_;
	FString short_name_;
	FString argument_name_;
	int editable_;
	int important_;
	int required_;
	int search_parameter_;
	NNLayerComponentPropertyType type_;
	FString default_value_;
	FStringList options_;
    int visible_;
};
//---------------------------------------------------------------------------
class NNLayerComponentSideConnector
{
public:
	NNLayerComponentSideConnector() :
		color_(0), kind_(NNLayerComponentInputKind::Data) {}
	virtual ~NNLayerComponentSideConnector() {}
public:
	int LoadFromIniSection(FFastIni &ini, const char *section);
public:
	void SetName(const FString &name) {name_ = name;}
	const FString &GetName() const {return name_;}
	void SetShortName(const FString &short_name) {short_name_ = short_name;}
	const FString &GetShortName() const {return short_name_;}
	int GetColor() const {return color_;}
	const FString &GetEnable() const {return enable_;}
	NNLayerComponentInputKind GetKind() const {return kind_;}
	const FString &GetShape() const {return shape_;}
	void SetParameterScope(FString parameter_scope) {parameter_scope_ = parameter_scope;}
	const FString &GetParameterScope() const {return parameter_scope_;}
private:
	FString name_;
	FString short_name_;
	int color_;
	FString enable_;
	NNLayerComponentInputKind kind_;
	FString shape_;
	FString parameter_scope_;
};
//---------------------------------------------------------------------------
class NNLayerComponent
{
public:
	NNLayerComponent();
	virtual ~NNLayerComponent();
public:
	int LoadFromTextFile(const FString &category, const FString &type, const char *file_name, const FString &default_output, FStringList &errors);
public:
	const FString &GetCategory() const {return category_;}
	const FString &GetType() const {return type_;}
	NNLayerComponentComponentType GetComponentType() const {return component_type_;}
	const FString &GetFunctionName() const {return function_name_;}
	const FString &GetParameterScope() const {return parameter_scope_;}

	int GetColor() const {return color_;}
	int GetHidden() const {return hidden_;}
	NNLayerComponentIOType GetInputType() const {return io_type_[0];}
	NNLayerComponentIOType GetOutputType() const {return io_type_[1];}
	NNLayerComponentDefaultPosition GetDefaultPositionHorizontal() const {return default_position_[0];}
	NNLayerComponentDefaultPosition GetDefaultPositionVertical() const {return default_position_[1];}

	int GetNumSideConnector(int output) const {return side_connectors_[output].GetSize();}
	const NNLayerComponentSideConnector &GetSideConnector(int output, int side_connector_index) const {return side_connectors_[output].GetAt(side_connector_index);}
	const NNLayerComponentSideConnector *GetSideConnectorByName(int output, const FString &name) const;
	int GetSideConnectorIndexByName(int output, const FString &name) const;

	int GetNumProperty() const {return properties_.GetSize();}
	const NNLayerComponentProperty &GetProperty(int property_index) const {return properties_.GetAt(property_index);}
	const NNLayerComponentProperty *GetPropertyByName(const FString &name) const;
	int GetPropertyIndexByName(const FString &name) const;
	int GetPropertyIndexByArgumentName(const FString &argument_name) const;

	const FString &GetUnitFileName() const {return unit_file_name_;}
	const FString &GetReference() const {return reference_;}
private:
	FString category_;
	FString type_;//'Affine', 'Convolution', etc...
	NNLayerComponentComponentType component_type_;
	FString function_name_;//Python function name for NNabla, 'affine', 'convolution', etc...
	FString parameter_scope_;//Parameter scope for NNabla. 'affine', 'conv', etc...
	int color_;
	int hidden_;
	NNLayerComponentIOType io_type_[2];//[0]:input, [1]:output
	NNLayerComponentDefaultPosition default_position_[2];
	FListP<NNLayerComponentSideConnector> side_connectors_[2];//[0]:input, [1]:output

	FListP<NNLayerComponentProperty> properties_;

	FString reference_;

	FString unit_file_name_;
};
//---------------------------------------------------------------------------
class NNLayerComponents : public FListP<NNLayerComponent>
{
public:
	NNLayerComponents() {}
	virtual ~NNLayerComponents() {}
	void Clear();
public:
	int LoadFromFolder(const char *path, FStringList &error);
public:
	int GetComponentIndexByType(const FString &type) const;
	const NNLayerComponent *GetComponentByType(const FString &type) const;
	FListT<int> EnumCompatibleComponentIndexes(int component_index) const;
	const FStringList &GetFoldCategory() const {return fold_category_;}
private:
	FListP<FListT<int> > compatible_component_group_;
	FStringList fold_category_;
};

}
//---------------------------------------------------------------------------
#endif
