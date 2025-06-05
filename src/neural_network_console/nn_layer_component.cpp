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

#if ( defined(_WIN32) || defined (_WIN64) || defined(WIN32) || defined (WIN64) )
#include <io.h>
#else
#include <unistd.h>
#endif

#pragma hdrstop

#include "nn_layer_component.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
int NNLayerComponentProperty::LoadFromIniSection(FFastIni &ini, const char *section, FStringList &errors)
{
	int result = true;

	short_name_ = ini.ReadString(section, "ShortName", name_);
	argument_name_ = ini.ReadString(section, "ArgumentName", FString(""));

	FString editable_str = ini.ReadString(section, "Editable", FString("True"));
	if (editable_str == "True") {
		editable_ = true;
	}else if (editable_str == "False") {
		editable_ = false;
	}else {
		//Error
		errors.Add(FString("Invalid value: Editable=") + editable_str);
		result = false;
	}

	FString important_str = ini.ReadString(section, "Important", FString("False"));
	if (important_str == "True") {
		important_ = true;
	}else if (important_str == "False") {
		important_ = false;
	}else {
		//Error
		errors.Add(FString("Invalid value: Important=") + important_str);
		result = false;
	}

	FString required_str = ini.ReadString(section, "Required", FString("True"));
	if (required_str == "True") {
		required_ = true;
	}else if (required_str == "False") {
		required_ = false;
	}else {
		//Error
		errors.Add(FString("Invalid value: Required=") + required_str);
		result = false;
	}

	FString search_parameter_str = ini.ReadString(section, "SearchParameter", FString("False"));
	if (search_parameter_str == "True") {
		search_parameter_ = true;
	}else if (search_parameter_str == "False") {
		search_parameter_ = false;
	}else {
		//Error
		errors.Add(FString("Invalid value: SearchParameter=") + search_parameter_str);
		result = false;
	}

	FString type_str = ini.ReadString(section, "Type", FString("Text"));
	if (type_str == "Boolean") {
		type_ = NNLayerComponentPropertyType::Boolean;
	}else if (type_str == "BooleanOrMacro") {
		type_ = NNLayerComponentPropertyType::BooleanOrMacro;
	}else if (type_str == "Int") {
		type_ = NNLayerComponentPropertyType::Int;
	}else if (type_str == "IntArray") {
		type_ = NNLayerComponentPropertyType::IntArray;
	}else if (type_str == "PInt") {
		type_ = NNLayerComponentPropertyType::PInt;
	}else if (type_str == "PIntArray") {
		type_ = NNLayerComponentPropertyType::PIntArray;
	}else if (type_str == "PIntArrays") {
		type_ = NNLayerComponentPropertyType::PIntArrays;
	}else if (type_str == "UInt") {
		type_ = NNLayerComponentPropertyType::UInt;
	}else if (type_str == "UIntArray") {
		type_ = NNLayerComponentPropertyType::UIntArray;
	}else if (type_str == "Float") {
		type_ = NNLayerComponentPropertyType::Float;
	}else if (type_str == "FloatArray") {
		type_ = NNLayerComponentPropertyType::FloatArray;
	}else if (type_str == "FloatArrays") {
		type_ = NNLayerComponentPropertyType::FloatArrays;
	}else if (type_str == "Option") {
		type_ = NNLayerComponentPropertyType::Option;
		ini.ReadSectionKeys((FString(section) + FString("_Option")).PChar(), &options_);
		for (int i = options_.GetSize() - 1; i >= 0; i--) {
			if (options_.GetAt(i) == "") {
				options_.Delete(i);
			}
		}
	}else if (type_str == "Dataset") {
		type_ = NNLayerComponentPropertyType::Dataset;
	}else if (type_str == "Text") {
		type_ = NNLayerComponentPropertyType::Text;
	}else if (type_str == "TextArray") {
		type_ = NNLayerComponentPropertyType::TextArray;
	}else if (type_str == "File") {
		type_ = NNLayerComponentPropertyType::File;
	}else if (type_str == "Model") {
		type_ = NNLayerComponentPropertyType::Model;
	}else {
		//Error
		result = false;
		errors.Add(FString("Invalid value: Type=") + type_str);
	}

	default_value_ = ini.ReadString(section, "Value", FString(""));

	return result;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int NNLayerComponentSideConnector::LoadFromIniSection(FFastIni &ini, const char *section)
{
	short_name_ = ini.ReadString(section, "ShortName", FString(""));
	color_ = ini.ReadInteger(section, "Color", color_);
	enable_ = ini.ReadString(section, "Enable", enable_);
	shape_ = ini.ReadString(section, "Shape", shape_);
	FString kind_str = ini.ReadString(section, "Kind", FString(""));
	if (kind_str == "Data") {
		kind_ = NNLayerComponentInputKind::Data;
	}else if (kind_str == "Parameter") {
		kind_ = NNLayerComponentInputKind::Parameter;
	}
	parameter_scope_ = ini.ReadString(section, "ParameterScope", FString(""));
	return true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
NNLayerComponent::NNLayerComponent() :
	color_(0xb2b2b2),
	hidden_(false),
	component_type_(NNLayerComponentComponentType::Normal)
{
	io_type_[0] = NNLayerComponentIOType::Single;
	io_type_[1] = NNLayerComponentIOType::Multiple;

	default_position_[0] = NNLayerComponentDefaultPosition::None;
	default_position_[1] = NNLayerComponentDefaultPosition::None;
}
//---------------------------------------------------------------------------
NNLayerComponent::~NNLayerComponent()
{

}
//---------------------------------------------------------------------------
int NNLayerComponent::LoadFromTextFile(const FString &category, const FString &type, const char *file_name, const FString &default_output, FStringList &errors)
{
	if (access(file_name, 0) != 0) {
		errors.Add(FString("File not found.") + FString(file_name));
		return false;
	}

	int result = true;
	category_ = category;
	type_ = type;

	FFastIni ini = FFastIni(file_name);
	parameter_scope_ = ini.ReadString("Global", "ParameterScope", FString(""));
	function_name_ = ini.ReadString("Global", "FunctionName", FString(""));
	reference_ = ini.ReadString("Global", "Reference", FString(""));

	if (function_name_ == "") {
		//Create lower_case from type string as default function name
		for (int i = 0 ; i < type_.Length(); i++) {
			if (type_.PChar()[i] >= 'A' && type_.PChar()[i] <= 'Z') {
				if (i > 0) {
					function_name_ += "_";
				}
				char c[2] = {0};
				c[0] = type_.PChar()[i] + 32;
				function_name_ += FString(c);
			}else {
				function_name_ += type_.SubString(i + 1, 1);
			}
		}
	}

	color_ = ini.ReadInteger("Display", "Color", 0xbebebe);
	FString component_type_str = ini.ReadString("Display", "ComponentType", FString("Normal"));

	if (component_type_str == "Comment"){
		component_type_ =  NNLayerComponentComponentType::Comment;
	}else if (component_type_str == "Argument"){
		component_type_ =  NNLayerComponentComponentType::Argument;
	}else if (component_type_str == "Setting"){
		component_type_ =  NNLayerComponentComponentType::Setting;
	}else {
		component_type_ = NNLayerComponentComponentType::Normal;
	}

	//IO
	FString io_type_str[2];
	io_type_str[0] = ini.ReadString("IO", "Input", FString("Single"));
	io_type_str[1] = ini.ReadString("IO", "Output", default_output);
	for (int i = 0 ; i < 2 ; i++) {
		if (io_type_str[i] == "None"){
			io_type_[i] = NNLayerComponentIOType::None;
		}else if (io_type_str[i] == "Single"){
			io_type_[i] = NNLayerComponentIOType::Single;
		}else if (io_type_str[i] == "SingleOrNone"){
			io_type_[i] = NNLayerComponentIOType::SingleOrNone;
		}else if (io_type_str[i] == "Multiple"){
			io_type_[i] = NNLayerComponentIOType::Multiple;
		}else if (io_type_str[i] == "Any"){
			io_type_[i] = NNLayerComponentIOType::Any;
		}else {
			//Error
			errors.Add(FString("IO type ") + FString(io_type_str[i]) + FString(" is not supported."));
			result = false;
		}
	}

	//Loop for side connector
	for (int io = 0 ; io < 2 ; io++) {
		FStringList side_connectors;
		FString section = FString(io ? "Output" : "Input") + FString("SideConnector");
		ini.ReadSectionKeys(section.PChar(), &side_connectors);
		if (side_connectors.GetSize()) {
			for (int is = 0 ; is < side_connectors.GetSize() ; is++) if (side_connectors.GetAt(is) != ""){
				NNLayerComponentSideConnector &new_side_connector = side_connectors_[io].Add();
				new_side_connector.SetName(side_connectors.GetAt(is));
				if (new_side_connector.LoadFromIniSection(ini, (section + FString("_") + side_connectors.GetAt(is)).PChar())){
					if (new_side_connector.GetShortName() == "") {
						new_side_connector.SetShortName(side_connectors.GetAt(is).SubString(1, 1));
					}
					if (new_side_connector.GetParameterScope() == "") {
						new_side_connector.SetParameterScope(parameter_scope_ + FString("/") + side_connectors.GetAt(is));
					}
				}else {
					//Error
					errors.Add(FString("Illigal side connector at ") + FString(section) + FString("."));
					result = false;
				}

			}
		}
	}

	//Global
	FString hidden_str = ini.ReadString("Global", "Hidden", FString("False"));
	if (hidden_str == "True") {
		hidden_ = true;
	}else if (hidden_str == "False") {
		hidden_ = false;
	}

	//Layout
	FString default_position_str[2];
	default_position_str[0] = ini.ReadString("Layout", "Default_Position_Horizontal", FString("None"));
	default_position_str[1] = ini.ReadString("Layout", "Default_Position_Vertical", FString("None"));
	for (int i = 0 ; i < 2 ; i++) {
		if (default_position_str[i] == "None"){
			default_position_[i] = NNLayerComponentDefaultPosition::None;
		}else if (default_position_str[i] == "Top"){
			default_position_[i] = NNLayerComponentDefaultPosition::Top;
		}else {
			//Error
			errors.Add(FString("Default position") + default_position_str[i] + FString(" is not supported."));
			result = false;
		}
	}

	//Name property
	NNLayerComponentProperty &name_property = properties_.Add();
	name_property.SetName(FString("Name"));
	name_property.SetShortName(FString("Name"));
	name_property.SetArgumentName(FString("name"));
	name_property.SetType(NNLayerComponentPropertyType::Text);
	name_property.SetDefaultValue(type_);
	name_property.SetRequired(false);

	//Is Parameter Property
	NNLayerComponentProperty &is_param_property = properties_.Add();
	is_param_property.SetName(FString("IsParam"));
	is_param_property.SetType(NNLayerComponentPropertyType::Boolean);
	is_param_property.SetDefaultValue(ini.ReadString("Global", "IsParam", FString("*IsParam")));
	is_param_property.SetEditable(false);
	is_param_property.SetRequired(false);
	is_param_property.SetVisible(false);

	//Loop for property
	FStringList properties;
	ini.ReadSectionKeys("Property", &properties);
	if (properties.GetSize()) {
		for (int ip = 0 ; ip < properties.GetSize() ; ip++) if (properties.GetAt(ip) != ""){
			NNLayerComponentProperty &new_property = properties_.Add();
			new_property.SetName(properties.GetAt(ip));
			if (!new_property.LoadFromIniSection(ini, (FString("Property_") + properties.GetAt(ip)).PChar(), errors)){
				//Error
				errors.Add(FString("Failed to load property ") + properties.GetAt(ip) + FString("."));
				result = false;
			}
		}
	}else {
		//Error
		errors.Add(FString("Property not found."));
		result = false;
	}

	FString unit_file_name = FString(file_name);
    unit_file_name = unit_file_name.SubString(1, unit_file_name.Length() - 4) + FString(".sdcproj");
	if (access(unit_file_name.PChar() , 0) == 0) {
		unit_file_name_ = unit_file_name;
	}

	return result;
}
//---------------------------------------------------------------------------
const NNLayerComponentSideConnector *NNLayerComponent::GetSideConnectorByName(int output, const FString &name) const
{
	const int side_connector_index = GetSideConnectorIndexByName(output, name);

	if (side_connector_index >= 0) {
		return &GetSideConnector(output, side_connector_index);
	}else {
		return NULL;
	}
}
//---------------------------------------------------------------------------
int NNLayerComponent::GetSideConnectorIndexByName(int output, const FString &name) const
{
	for (int is = 0 ; is < side_connectors_[output].GetSize() ; is++) {
		const NNLayerComponentSideConnector &side_connector = side_connectors_[output].GetAt(is);
		if (side_connector.GetName() == name) {
			return is;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
const NNLayerComponentProperty *NNLayerComponent::GetPropertyByName(const FString &name) const
{
	const int property_index = GetPropertyIndexByName(name);

	if (property_index >= 0) {
        return &GetProperty(property_index);
	}else {
        return NULL;
    }
}
//---------------------------------------------------------------------------
int NNLayerComponent::GetPropertyIndexByName(const FString &name) const
{
	for (int ip = 0 ; ip < properties_.GetSize() ; ip++) {
		const NNLayerComponentProperty &property = properties_.GetAt(ip);
		if (property.GetName() == name) {
			return ip;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
int NNLayerComponent::GetPropertyIndexByArgumentName(const FString &argument_name) const
{
	for (int ip = 0 ; ip < properties_.GetSize() ; ip++) {
		const NNLayerComponentProperty &property = properties_.GetAt(ip);
		if (property.GetArgumentName() == argument_name) {
			return ip;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void NNLayerComponents::Clear()
{
	FListP<NNLayerComponent>::Clear();
	compatible_component_group_.Clear();
	fold_category_.Clear();
}
//---------------------------------------------------------------------------
int NNLayerComponents::LoadFromFolder(const char *path, FStringList &error)
{
    int result = true;

	Clear();

	FString layer_component_fn = FString(path) + FString("layer_component.txt");
	FFastIni layer_component = FFastIni(layer_component_fn.PChar());

	//Loop for category
	{
		FString default_output = layer_component.ReadString("Global", "DefaultOutput", FString("Multiple"));

		FStringList categories;
		layer_component.ReadSectionKeys("Category", &categories);

		if (categories.GetSize()) {
			for (int ic = 0 ; ic < categories.GetSize() ; ic++) if (categories.GetAt(ic) != ""){
				//Enum layer
				FString category_path =
					FString(path) +
					categories.GetAt(ic) +
					FString("/");
				FStringList layers;
				layer_component.ReadSectionKeys(categories.GetAt(ic).PChar() , &layers);

				//Loop for layers in category folder
				for (int il = 0 ; il < layers.GetSize() ; il++) {
					const FString &layer_name = layers.GetAt(il);
					if (layer_name.SubString(1, 1) == "*") {
						if (layer_name == "*Fold") {
							fold_category_.Add(categories.GetAt(ic));
						}else {
							//Error (Unknown Macro)
							error.Add(FString("* layer name contains unknown macro\t") + layer_name);
							result = false;
                        }
					}else if (layer_name.SubString(1, 1) != ";") {
						//Create new layer_component
						NNLayerComponent &new_layer_component = Add();
						FString layer_component_fn_1 =
							category_path +
							layer_name +
							FString(".txt");
						if (!new_layer_component.LoadFromTextFile(categories.GetAt(ic), layer_name, layer_component_fn_1.PChar(), default_output, error)){
							error.Add(FString("* Failed to load layer component file.\t") + layer_component_fn_1);
							result = false;
						}
					}
				}
			}
		}else {
			error.Add(FString("* Error layer_component.txt is not found.\t") + layer_component_fn);
			result = false;
		}
	}

	//Loop for Compatible Component Group
	{
		FStringList compatible;
		layer_component.ReadSectionKeys("Compatible", &compatible);
		for (int ic = 0 ; ic < compatible.GetSize() ; ic++) if (compatible.GetAt(ic) != ""){
			FListT<int> &group = compatible_component_group_.Add();
			FString s = compatible.GetAt(ic);
			while (s != ""){
				FString name = s.SplitBy(',');
				const int component_index = GetComponentIndexByType(name);
				if (component_index >= 0) {
                    group.Add(component_index);
				}
			}
		}
	}

	return result;
}
//---------------------------------------------------------------------------
int NNLayerComponents::GetComponentIndexByType(const FString &type) const
{
	for (int ic = 0 ; ic < GetSize() ; ic++) {
		const NNLayerComponent &component = GetAt(ic);
		if (component.GetType() == type){
			return ic;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
const NNLayerComponent *NNLayerComponents::GetComponentByType(const FString &type) const
{
	const int component_index = GetComponentIndexByType(type);
	if (component_index >= 0) {
		return &GetAt(component_index);
	}
	return NULL;
}
//---------------------------------------------------------------------------
FListT<int> NNLayerComponents::EnumCompatibleComponentIndexes(int component_index) const
{
	FListT<int> result;
	for (int ig = 0 ; ig < compatible_component_group_.GetSize() ; ig++) {
		FListT<int> &group = compatible_component_group_.GetAt(ig);
		if (group.IndexOf(component_index) >= 0) {
			result = group;
			result.Remove(component_index);
			break;
		}
	}
	return result;
}
//---------------------------------------------------------------------------
};
#pragma package(smart_init)
