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
#include "nnabla_utility.h"

#if _WIN32
  typedef long long _int_array_type;
#else
  # ifdef __APPLE__
    typedef long long _int_array_type;
  #else
    typedef long _int_array_type;
  #endif
#endif

namespace sdeep_console{
//---------------------------------------------------------------------------
FString replace_input_shape(const NNLayer &layer, const FString &inputs, const FString &s){
	FString str = s;
	if (str.Pos("/")){
		str = str.ReplaceStr("/", "//");
	}
	while (str.Pos("Input[")){
		FString left = str.SplitBy("Input[");
		FString value = str.SplitBy("]");
		const NNLayerProperty &is_param_property = layer.GetProperty(layer.GetIsParamPropertyIndex());
		if (is_param_property.GetIntArray().GetSize() == 1){
			if (!is_param_property.GetIntArray().GetAt(0)){
				value += " + 1";
			}
		}
		str = left + inputs + FString(".shape[") + value + FString("]") + str;
	}
	return str;
}
//---------------------------------------------------------------------------
FStringList decode_arg_value(const NNLayer &layer, const NNLayerProperty &property, const FString &argument_name_str, const FString &inputs)
{
	FStringList str_array;
	FString property_str = property.GetValue();
	FListT<_int_array_type> int_array = property.GetIntArray();
	for (int id = int_array.GetSize() - 1 ; id >= 0 ; id--) {
		FString s = property_str.SplitBy(",");
		str_array.Insert(0, s.SubString(1,1) != "*" && s.AsInt() != int_array.GetAt(id) ? s : FString(""));
	}

	if (argument_name_str.Pos("-a") && !layer.GetProperty(layer.GetIsParamPropertyIndex()).GetIntArray().GetAt(0)) {
		//Axis Option : increnemt axis index to add batch_size axis
		for (int id = int_array.GetSize() - 1 ; id >= 0 ; id--) {
			int_array.SetAt(id, int_array.GetAt(id) + 1);
			if (str_array.GetAt(id) != ""){
				str_array.GetAt(id) += FString("+1");
			}
		}
	}
	if (argument_name_str.Pos("-0") && !layer.GetProperty(layer.GetIsParamPropertyIndex()).GetIntArray().GetAt(0)) {
		//Add 0th axis for batch_size (used in Transpose layer)
		int_array.Add(0);
		str_array.Add(FString(""));
	}

	if (property.GetType() >= NNLayerPropertyType::Int && property.GetType() <= NNLayerPropertyType::UIntArray){
		for (int id = int_array.GetSize() - 1 ; id >= 0 ; id--) {
			if (str_array.GetAt(id) != ""){
				str_array.GetAt(id) = replace_input_shape(layer, inputs, str_array.GetAt(id));
				continue;
			}
			str_array.GetAt(id) = FString((int)int_array.GetAt(id));
		}
	}
	return str_array;
}
//---------------------------------------------------------------------------
FString GetPropertyArgStr(const NNLayerProperty &property, const FStringList &network_args_list, const FString &argument_name_str, const NNLayer &layer, const FString &inputs, const FString &input1)
{
	FString result;
	if (network_args_list.IndexOf(property.GetValue().SubString(2, property.GetValue().Length())) >= 0){
		//network argument ("*" + argname)
		result = property.GetValue().SubString(2, property.GetValue().Length());
	}else {
		//normal argument (including macro)
		FListT<_int_array_type> int_array = property.GetIntArray();

		//preparer str array
		FStringList str_array = decode_arg_value(layer, property, argument_name_str, inputs);

		if (argument_name_str.Pos("-test=")) {
			FString arg = argument_name_str;
			arg.SplitBy("-test=");
			if (arg == "true") {
				result = FString("test");
			}else {
				result = FString("not test");
			}
		}else switch (property.GetType()) {
		case NNLayerPropertyType::Boolean:
			if (int_array.GetSize()){
				result = FString(int_array.GetAt(0) ? "True" : "False");
			}
			break;
		case NNLayerPropertyType::Int:
		case NNLayerPropertyType::PInt:
		case NNLayerPropertyType::UInt:
			if (int_array.GetSize()){
				result = str_array.GetAt(0);
			}
			break;
		case NNLayerPropertyType::IntArray:
		case NNLayerPropertyType::PIntArray:
		case NNLayerPropertyType::UIntArray:
		case NNLayerPropertyType::FloatArray:
			{
				FString tuple = FString("(");
				if (argument_name_str.Pos("-b") && !layer.GetProperty(layer.GetIsParamPropertyIndex()).GetIntArray().GetAt(0)){
					// Add batch size to shape if it is not parameter
					tuple += input1 + FString(".shape[0],");
				}
				for (int id = str_array.GetSize() - 1 ; id >= 0; id--) {
					tuple += str_array.GetAt(id);
					if (id > 0 || str_array.GetSize() <= 1) {
						tuple += ",";
					}
				}
				tuple += FString(")");
				result = tuple;
			}
			break;
		case NNLayerPropertyType::Dataset:
		case NNLayerPropertyType::Option:
		case NNLayerPropertyType::Text:
		case NNLayerPropertyType::Model:
			result = FString("'") + property.GetDisplayValue() + FString("'");
			break;
		case NNLayerPropertyType::File:
			result = FString("'") + property.GetDisplayValue().ReplaceStr("\\","\\\\") + FString("'");
			break;
		default:
			result = property.GetDisplayValue();
			break;
		}
	}
	if (argument_name_str.Pos("-n")){
		FString s = argument_name_str;
		if (s.Pos("-pyargname=")) {
			s.SplitBy("-pyargname=");
			s = s.SplitBy(" ");
		}else {
			s = s.SplitBy(" ");
			while (s.Pos(".")){
				s.SplitBy(".");
			}
        }
		result = s + FString("=") + result;
	}
    return result;
}
//---------------------------------------------------------------------------
FStringList GetNNablaPythonCode(const NNDocument &document)
{
	const NNStructures &structures = document;

	//Generate code
	FStringList sl;
	sl.Add(FString("import nnabla as nn"));
	sl.Add(FString("import nnabla.functions as F"));
	sl.Add(FString("import nnabla.parametric_functions as PF"));
	sl.Add(FString(""));

	//Export structures
	for (int is = 0 ; is < structures.GetNumStructure() ; is++){
		//original structure
		const NNStructure &structure = structures.GetStructure(is);

		//arranged structure
		NNStructure net;
		net = structure;
		net.ArrangeLayers(true);

		//Encode network args
		FString network_args;
		FStringList network_args_list;
		for (int il = 0; il <net.GetNumLayer() ; il++) {
			const NNLayer &layer = net.GetLayer(il);
			if (layer.GetType() == "Argument" ){
				network_args += FString(", ") + layer.GetPropertyValue(FString("Name"));
				const NNLayerProperty *property = layer.GetPropertyByName(FString("Value"));
				if (property) {
					FString value = property->GetValue();
					if (value == "*Default") {
						value = property->GetDefaultValue();
					}
					network_args += FString("=") + value;
				}
				network_args_list.Add(layer.GetPropertyValue(FString("Name")));
			}
		}

		//Remove unnecessory layers
		net.ClearSelection();
        FString test_network = FString("False");
		for (int il = 0; il <net.GetNumLayer() ; il++) {
			const NNLayer &layer = net.GetLayer(il);
			if (layer.GetType() == "Comment" || layer.GetType() == "Argument" || layer.GetType() == "TestNetwork" || layer.GetType() == "StructureSearch" || layer.GetType() == "Import"){
				net.SetLayerSelected(layer.GetLayerID(), true, false);
				if (layer.GetType() == "TestNetwork") {
                    test_network = "True";
				}
			}
		}
		net.DeleteSelected();

		//Enum network inputs
		FStringList input_variables;
		FString network_inputs_str;
		FStringList network_inputs;
		FStringList variables;
		FStringList output_variable_names;
		FString terminal_variable_names_str;
		int new_name_index = 0;
		for (int il = 0; il <net.GetNumLayer() ; il++) {
			const NNLayer &layer = net.GetLayer(il);

			const NNLayerProperty *dataset = NULL;
			int input_dataset = false;
			if (layer.GetType() == "Input") {
				dataset = layer.GetPropertyByName("Dataset");
				input_dataset = true;
			}else {
				dataset = layer.GetPropertyByName("T.Dataset");
			}

			FString output_variable_name;
			if (dataset) {
				if (input_variables.IndexOf(dataset->GetValue()) < 0){
					if (network_inputs_str != "") {
						network_inputs_str += FString(", ");
					}
					network_inputs_str += dataset->GetValue();
					input_variables.Add(dataset->GetValue());
					network_inputs.Add(dataset->GetValue());
					if (variables.IndexOf(dataset->GetValue()) < 0) {
						variables.Add(dataset->GetValue());
					}
				}
				if (input_dataset){
					output_variable_name = dataset->GetValue();
				}
			}
			if (output_variable_name == "") {
				FListT<int> source = net.EnumLinkIDByDestLayerID(layer.GetLayerID(), -1);

				//Use last variable name if there is no branch
				int use_last_variable_name = true;
				for (int is = 0 ; is < source.GetSize() ; is++) {
					const NNLink *source_link = net.GetLinkByID(source.GetAt(is));
					if (source_link) {
						FListT<int> sibling = net.EnumLinkIDBySourceLayerID(source_link->GetSourceLayerID(), 0);
						use_last_variable_name &= sibling.GetSize() == 1;
					}
				}
				if (use_last_variable_name && source.GetSize()) {
					const NNLink *source_link = net.GetLinkByID(source.GetAt(0));
					if (source_link) {
						int layer_index = net.GetLayerIndexByID(source_link->GetSourceLayerID());
						if (layer_index < il) {
							if (network_inputs.IndexOf(output_variable_names.GetAt(layer_index)) < 0) {
								output_variable_name = output_variable_names.GetAt(layer_index);
							}
						}
					}
				}

				while (output_variable_name == "") {
					//new output variable name
					FString new_name = FString("h") + (new_name_index > 0 ? FString(new_name_index) : FString(""));
					new_name_index++;
					if (variables.IndexOf(new_name) < 0) {
						output_variable_name = new_name;
						variables.Add(new_name);
					}
				}
			}
			output_variable_names.Add(output_variable_name);
		}

		FString network_name = net.GetName() == "Main" ? FString("") : FString("_") + net.GetName();
		sl.Add(FString("def network") + network_name + FString("(") + network_inputs_str + network_args + FString(", test=") + test_network + FString("):"));
		FString indent = FString("    ");
		FStringList repeat_ids;
		FStringList repeat_ids_inputs;
		//layer loop
		for (int il = 0; il <net.GetNumLayer() ; il++) {
			const NNLayer &layer = net.GetLayer(il);
			const NNLayerComponent *layer_component = net.GetNNLayerComponents()->GetComponentByType(layer.GetType());
			FString layer_name = layer.GetPropertyByName("Name")->GetValue();
			FString parameter_scope_name = layer.GetDisplayPropertyValue(FString("ParameterScope"));
			FString function_name = layer_component ? layer_component->GetFunctionName() : layer_name;

			//Enum input variable names
			FString input1, inputs;
			FListT<int> source = net.EnumLinkIDByDestLayerID(layer.GetLayerID(), -1);
			int need_space = false;
			for (int is = 0 ; is < source.GetSize() ; is++) {
				const NNLink *source_link = net.GetLinkByID(source.GetAt(is));
				if (source_link) {
					const int source_layer_index = net.GetLayerIndexByID(source_link->GetSourceLayerID());
					if (is > 0) {
						inputs += FString(", ");
					}
					inputs += output_variable_names.GetAt(source_layer_index);
					if (is == 0) {
						input1 = inputs;
					}

					//Check space in original network layout
					const NNLayer *source_layer = structure.GetLayerByID(source_link->GetSourceLayerID());
					const NNLayer *dest_layer = structure.GetLayerByID(source_link->GetDestLayerID());
					if (source_layer && dest_layer) {
						if (fabs(source_layer->GetPosition()[0] - dest_layer->GetPosition()[0]) > 0.1 ||
							fabs(source_layer->GetPosition()[1] + NNLayerDefaultHeight - dest_layer->GetPosition()[1]) > 0.1) {
							need_space = true;
						}
					}
				}
			}

			if (need_space) {
				sl.Add();
			}

			const NNLayerProperty *dataset = NULL;
			if (layer.GetType() == "Delay") {
				dataset = layer.GetPropertyByName("initial.Dataset");
			}else {
				dataset = layer.GetPropertyByName("T.Dataset");
			}
			if (dataset) {
				inputs += FString(", ") + dataset->GetDisplayValue();
			}

			FString outputs = output_variable_names.GetAt(il);
			if (layer.GetType() == "Parameter") {

			}else {
				//Output comment
				FString out_size_comment;
				const NNLayerProperty *input = layer.GetPropertyByName("Input");
				const NNLayerProperty *output = layer.GetPropertyByName("Output");
				if (output) {
					out_size_comment = FString(" -> ") + output->GetDisplayValue();
					if (input) {
						if (input->GetDisplayValue() == output->GetDisplayValue()) {
							out_size_comment = FString("");
						}
					}

					// input variable name
					if (layer.GetType() == "Input"){
						out_size_comment = FString(":") + outputs + out_size_comment;
					}
				}

				sl.Add(indent + FString("# ") + layer_name + out_size_comment);

				//Encode args
				FString args;
				//int not_default = false, need_arg_name = false;
				for (int ip = layer.GetNumProperty() - 1 ; ip >= 0 ; ip--) {
					const NNLayerProperty &property = layer.GetProperty(ip);
					const NNLayerComponentProperty *component_property = layer_component ? layer_component->GetPropertyByName(property.GetName()) : NULL;
					const FString &argument_name_str = component_property ? component_property->GetArgumentName() : property.GetName();
					const FString &default_value = component_property ? component_property->GetDefaultValue() : FString("");
					if (argument_name_str != "" && argument_name_str != "name") {
						if (//not_default ||
							(!property.GetDynamic() && property.GetDisplayValue() != default_value) ||
							argument_name_str.Pos("-test=") ||
							(property.GetDynamic() && !property.GetName().Pos(".")) || //argument for unit
							argument_name_str.Pos("-f")) {//force specifying the argument

							/*
							if (argument_name_str.Pos("-n") == 0){
								//Be sure to output the arguments before this property
								not_default = true;
							}
                            */
							if (property.GetValue() != "*Default" || argument_name_str.Pos("-f")) {
								args = FString(", ") + GetPropertyArgStr(property, network_args_list, argument_name_str + FString(" -n"), layer, inputs, input1) + args;
							}
						}
					}
				}

				//Add function
				const NNLayerProperty *skip_property = layer.GetPropertyIndexByName("SkipAtTest") >= 0 ? layer.GetPropertyByName("SkipAtTest") : layer.GetPropertyByName("SkipAtInspection");
				const int skip_at_test = skip_property ? skip_property->GetIntArray().GetAt(0) : false;
				if (layer.GetType() == "Input"){

				}else if (layer.GetType() == "RepeatStart"){
					FString repeat_id;
					FString repeat_times = FString("1");
					const NNLayerProperty *repeat_id_property = layer.GetPropertyByName("ID");
					if (repeat_id_property) {
						//repeat_id = repeat_id_property->GetDisplayValue();
						repeat_id = indent.Length() > 4 ? FString(indent.Length() / 4) : FString("");
					}
					const NNLayerProperty *times_property = layer.GetPropertyByName("Times");
					if (times_property) {
						FStringList str_array = decode_arg_value(layer, *times_property, FString(""), inputs);
						if (str_array.GetSize()){
							repeat_times = str_array.GetAt(0);
						}
					}

					sl.Add(indent + FString("for i") + repeat_id + FString(" in range(") + repeat_times + FString("):"));
					indent += FString("    ");
					repeat_ids.Add(repeat_id);
					repeat_ids_inputs.Add(inputs);

					//Let data in last repeat
					FListT<int> source = net.EnumLinkIDByDestLayerID(layer.GetLayerID(), 1);// previous pin
					if (source.GetSize()) {
						const NNLink *link = net.GetLinkByID(source.GetAt(0));
						if (link) {
							const int source_layer_index = net.GetLayerIndexByID(link->GetSourceLayerID());
							FString output_variable_name = output_variable_names.GetAt(source_layer_index);
							if (output_variable_name != outputs) {
								sl.Add(indent + FString("if i") + repeat_id + FString(" > 0:"));
								sl.Add(indent + FString("    ") + outputs + (" = ") + output_variable_name);
							}
						}
					}

				}else if (layer.GetType() == "RepeatEnd"){
					if (repeat_ids.GetSize()){
						outputs = repeat_ids_inputs.GetAt(repeat_ids_inputs.GetSize() - 1);
						repeat_ids.Delete(repeat_ids.GetSize() - 1);
						repeat_ids_inputs.Delete(repeat_ids_inputs.GetSize() - 1);
						if (inputs != outputs) {
							sl.Add(indent + outputs + FString(" = ") + inputs);
						}
						indent = indent.SubString(1, indent.Length() - 4);
					}
				}else if (layer.GetType() == "Unit"){
					//Unit
					FString scope_name = FString("'") + parameter_scope_name + FString("'");
					for (int i = 0 ; i < repeat_ids.GetSize() ; i++) {
						scope_name += FString(" + '[' + str(i") + repeat_ids.GetAt(i) + FString(") + ']'");
					}
                    args += FString(", test=test");
					scope_name = scope_name.ReplaceStr("' + '", "");
					if (scope_name != "''"){
						sl.Add(indent + FString("with nn.parameter_scope(") + scope_name + FString("):"));
						sl.Add(indent + FString("    ") + outputs + FString(" = network_") + layer.GetPropertyValue(FString("Network")) + FString("(") + inputs + args + FString(")"));
					}else {
						sl.Add(indent + outputs + FString(" = network_") + layer.GetPropertyValue(FString("Network")) + FString("(") + inputs + args + FString(")"));
					}
				}else if (
					layer.GetNumSideConnector(0) &&
					layer.GetSideConnector(0, 0).GetKind() == NNLayerComponentInputKind::Parameter
					) {
					// Parametric function
					FString scope_name = FString("'") + parameter_scope_name + FString("'");
					for (int i = 0 ; i < repeat_ids.GetSize() ; i++) {
						scope_name += FString(" + '") + repeat_ids.GetAt(i) + FString("[' + str(i") + repeat_ids.GetAt(i) + FString(") + ']'");
					}
					scope_name = scope_name.ReplaceStr("' + '", "");
					sl.Add(indent + outputs + FString(" = PF.") + function_name + FString("(") +
						   inputs + args + FString(", name=") + scope_name + FString(")"));
					/*
					sl.Add(indent + FString("with nn.parameter_scope(") + scope_name + FString("):"));
					sl.Add(indent + FString("    ") + outputs + FString(" = PF.") + function_name + FString("(") + inputs + args + FString(")"));
					*/
				}else if (skip_at_test) {
					// Function (training only)
					sl.Add(indent + FString("if not test:"));
					sl.Add(indent + FString("    ") + outputs + FString(" = F.") + function_name + FString("(") + inputs + args + FString(")"));
					if (outputs != inputs) {
						sl.Add(indent + FString("else:"));
						sl.Add(indent + FString("    ") + outputs + FString(" = ") + inputs);
					}
				}else {
					// Function
					sl.Add(indent + outputs + FString(" = F.") + function_name + FString("(") + inputs + args + FString(")"));
				}

				if (!net.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0).GetSize()) {
					if (terminal_variable_names_str != "") {
						terminal_variable_names_str += ", ";
					}
					terminal_variable_names_str += outputs;
				}
			}
		}
		sl.Add(indent + FString("return ") + terminal_variable_names_str);
		sl.Add();
	}

	return sl;
}
//---------------------------------------------------------------------------
FString ShapeToStr_(FListT<_int_array_type> shape)
{
	FString result;
	for (int id = shape.GetSize() - 1 ; id >= 0; id--) {
		if (result != "") {
			result += " ";
		}
		result += FString("dim: ") + FString((int)shape.GetAt(id));
	}
	return result;
}
//---------------------------------------------------------------------------
void NetworkToNNablaProtoTxt(
	const NNStructure &structure_in, const NNConfig &config,//in
	FStringList &prototxt, FString &indent) //out
{
	NNStructure structure;
	structure = structure_in;

	//Add link from pre-RepeatEnd to RepeatStart
	for (int il = 0; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);

		if (layer.GetType() == "RepeatEnd") {
			FString repeat_start_layer_name = layer.GetPropertyByName("ID")->GetDisplayValue();
			const int repeat_start_layer_id = structure.GetLayerIDByName(repeat_start_layer_name);

			FListT<int> link_ids = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
			if (link_ids.GetSize() == 1) {
				const NNLink *link = structure.GetLinkByID(link_ids.GetAt(0));
				assert(link);
				const int pre_repeat_end_layer_id = link->GetSourceLayerID();

				structure.AddLink(pre_repeat_end_layer_id, 0, repeat_start_layer_id, 1, true);
			}
		}
	}

	prototxt.Add(indent + FString("network {"));
	indent += "  ";

	prototxt.Add(indent + FString("name: \"") + structure.GetName() + FString("\""));
	prototxt.Add(indent + FString("batch_size: ") + FString(config.GetBatchSize()));

	//  RepeatInfo
	for (int ir = 0 ; ir < structure.GetRepeatInfo().GetSize() ; ir++) {
		FStringList sl;

		sl.Add(indent + FString("repeat_info {"));
		indent += "  ";

		sl.Add(FString(indent + "id: \"") + structure.GetRepeatInfo().GetAt(ir).GetID() + FString("\""));
		sl.Add(FString(indent + "times: ") + FString(structure.GetRepeatInfo().GetAt(ir).GetNumRepeat()));

		indent = indent.SubString(1, indent.Length() - 2);
		sl.Add(indent + FString("}"));//Function

		prototxt.Add(sl);
	}

	//  Variables
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		FStringList sl;

		const NNLayer &layer = structure.GetLayer(il);
		if (layer.GetType() != "Output") {
			int success = true;
			sl.Add(indent + FString("variable {"));
			indent += "  ";

			//Name
			sl.Add(FString(indent + "name: \"") + layer.GetPropertyValue(FString("Name")) + FString("\""));
			sl.Add(FString(indent + "type: \"") +
				FString(
					layer.GetType() == "Parameter" ?// || layer.GetType() == "WorkingMemory")
					"Parameter" :
					"Buffer"
				) + FString("\""));
			const FStringList &repeat_ids = layer.GetRepeatIDs();
			for (int ir = 0 ; ir < repeat_ids.GetSize() ; ir++) {
				sl.Add(FString(indent + "repeat_id: \"") + FString(repeat_ids.GetAt(ir)) + FString("\""));
			}

			//Shape
			FString shape_str = FString("dim:-1");
			if (layer.GetIsParamPropertyIndex()){
				const NNLayerProperty &is_param_property = layer.GetProperty(layer.GetIsParamPropertyIndex());
				if (is_param_property.GetIntArray().GetSize() == 1){
					if (is_param_property.GetIntArray().GetAt(0)){
                        shape_str = "";
					}
				}
			}

			const NNLayerProperty* property = layer.GetPropertyByName("Output");
			if (property) {
				if (property->GetIntArray().GetSize()) {
					if (shape_str != "") {
						shape_str += " ";
					}
					shape_str += ShapeToStr_(property->GetIntArray());
				}
			}
			sl.Add(FString(indent + "shape: { ") + shape_str + " }");

			if (layer.GetType() == "Parameter" || layer.GetType() == "WorkingMemory") {
				sl.Add(indent + FString("initializer {"));
				indent += "  ";

				sl.Add(indent + FString("type: \"") + layer.GetDisplayPropertyValue(FString("Initializer")) + FString("\""));
				sl.Add(indent + FString("multiplier: ") + layer.GetDisplayPropertyValue(FString("InitializerMultiplier")));

				indent = indent.SubString(1, indent.Length() - 2);
				sl.Add(indent + FString("}"));//Initializer
			}

			indent = indent.SubString(1, indent.Length() - 2);
			sl.Add(indent + FString("}"));//Function

			if (success) {
				prototxt.Add(sl);
			}
		}
	}


	//  Functions
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);
		if (layer.GetType() != "Output" &&
			structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), 0).GetSize()) {
			//Has input
			FStringList sl;
			int success = true;

			sl.Add(indent + FString("function {"));
			indent += "  ";

			//Name
			sl.Add(FString(indent + "name: \"") + layer.GetPropertyValue(FString("Name")) + FString("\""));
			FString layer_type = layer.GetType();
			if (layer_type == "WorkingMemory"){
				layer_type = FString("Identity");
			}
			sl.Add(FString(indent + "type: \"") + layer_type + FString("\""));
			const FStringList &repeat_ids = layer.GetRepeatIDs();
			for (int ir = 0 ; ir < repeat_ids.GetSize() ; ir++) {
				sl.Add(FString(indent + "repeat_id: \"") + FString(repeat_ids.GetAt(ir)) + FString("\""));
			}


			//IO
			for (int ipin = 0 ; ipin < NNStructureMaxPinNum ; ipin++) {
				FListT<int> link_from = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), ipin);
				for (int ili = 0 ; ili < link_from.GetSize() ; ili++) {
					const NNLink *link = structure.GetLinkByID(link_from.GetAt(ili));
					if (link) {
						const NNLayer *source_layer = structure.GetLayerByID(link->GetSourceLayerID());
						sl.Add(FString(indent + "input: \"") + source_layer->GetPropertyValue(FString("Name")) + FString("\""));
					}
				}
			}
			sl.Add(FString(indent + "output: \"") + layer.GetPropertyValue(FString("Name")) + FString("\""));

			//Parameters
			const NNLayerComponent *component = layer.GetComponent();
			FString current_parameter;
			if (component) {
				assert (component->GetNumProperty() == layer.GetNumProperty());
				for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
					const NNLayerProperty &property = layer.GetProperty(ip);
					const NNLayerComponentProperty &component_property = component->GetProperty(ip);
					FString argument_name_str = component_property.GetArgumentName();
					FString argument_name = argument_name_str.SplitBy(" ");
					if (argument_name.Pos(".")) {
						FString parameter = argument_name.SplitBy(".");
						if (parameter != current_parameter) {
							if (current_parameter != "") {
								indent = indent.SubString(1, indent.Length() - 2);
								sl.Add(indent + FString(indent + "}"));
							}
							current_parameter = parameter;
							sl.Add(indent + parameter + FString(" {"));
							indent += "  ";
						}

						FListT<_int_array_type> int_array = property.GetIntArray();
						FListT<double> float_array = property.GetFloatArray();
						const int is_param_property_index = layer.GetIsParamPropertyIndex();
						const int is_param_num = layer.GetProperty(is_param_property_index).GetIntArray().GetSize();
						if (argument_name_str.Pos("-a") && !layer.GetProperty(is_param_property_index).GetIntArray().GetAt(0)) {
							//Axis Option
							for (int id = 0; id < int_array.GetSize() ; id++) {
								int_array.SetAt(id, int_array.GetAt(id) + 1);
							}
						}
						if (argument_name_str.Pos("-0") && !layer.GetProperty(is_param_property_index).GetIntArray().GetAt(0)) {
                            int_array.Add(0);
                        }
						switch (property.GetType()) {
						case NNLayerPropertyType::Boolean:
							sl.Add(indent + argument_name + FString(": ") + FString(int_array.GetAt(0) ? "true" : "false"));
							break;
						case NNLayerPropertyType::Int:
						case NNLayerPropertyType::PInt:
						case NNLayerPropertyType::UInt:
							sl.Add(indent + argument_name + FString(": ") + FString((int)int_array.GetAt(0)));
							break;
						case NNLayerPropertyType::IntArray:
						case NNLayerPropertyType::PIntArray:
						case NNLayerPropertyType::UIntArray:
							if (argument_name_str.Pos("-a") || argument_name_str.Pos("-r")) {
								for (int id = int_array.GetSize() - 1 ; id >= 0; id--) {
									sl.Add(indent + argument_name + FString(": ") + FString((int)int_array.GetAt(id)));
								}
							}else {
								if (argument_name_str.Pos("-b") && !layer.GetProperty(layer.GetIsParamPropertyIndex()).GetIntArray().GetAt(0)) {
									// Add batch size to shape if it is not parameter
									int_array.Add(-1);
								}
								sl.Add(indent + argument_name + FString(": { ") + ShapeToStr_(int_array) + FString(" }"));
							}
							break;
						case NNLayerPropertyType::FloatArray:
							for (int id = float_array.GetSize() - 1 ; id >= 0; id--) {
								sl.Add(indent + argument_name + FString(": ") + FString(float_array.GetAt(id)));
							}
							break;
						case NNLayerPropertyType::Dataset:
						case NNLayerPropertyType::Option:
						case NNLayerPropertyType::Text:
						case NNLayerPropertyType::Model:
							sl.Add(indent + argument_name + FString(": \"") + property.GetDisplayValue() + FString("\""));
							break;
						case NNLayerPropertyType::File:
							sl.Add(indent + argument_name + FString(": \"") + property.GetDisplayValue().ReplaceStr("\\","\\\\") + FString("\""));
							break;
						default:
							sl.Add(indent + argument_name + FString(": ") + property.GetDisplayValue());
							break;
						}
					}
				}
			}
			if (current_parameter != "") {
				indent = indent.SubString(1, indent.Length() - 2);
				sl.Add(FString(indent + "}"));
			}


			indent = indent.SubString(1, indent.Length() - 2);
			sl.Add(indent + FString("}"));//Function

			if (success) {
				prototxt.Add(sl);
			}
		}
	}
	indent = indent.SubString(1, indent.Length() - 2);
	prototxt.Add(indent + FString("}"));//Network
}
//---------------------------------------------------------------------------
void FlattenCurrentStructure(const NNDocument &nn_document, NNStructures &structures, int arrange)
{
	//Expand Network Unit
	structures.ExpandAndFix(FStringList());

	//Delete unnecessory layers
	structures.ClearSelection();
	for (int il = 0 ; il < structures.GetNumLayer() ; il++) {
		const NNLayer &layer = structures.GetLayer(il);
		const NNLayerComponent *component = layer.GetComponent();
		assert(component);
		if (component->GetComponentType() != NNLayerComponentComponentType::Normal) {
			structures.SetLayerSelected(layer.GetLayerID(), true, false);
		}
	}
	structures.DeleteSelected();

	structures.EscapeLayerName(FString("/.{}"));
	if (arrange){
		structures.ArrangeLayers(true);
	}
	structures.RefreshDisplayValue(&nn_document);
	structures.RefreshStatistics();

	//Expand old unit
	for (int il = 0 ; il < structures.GetNumLayer() ; il++) {
		const NNLayer &layer = structures.GetLayer(il);
		const NNLayerComponent *component = layer.GetComponent();
		if (component->GetUnitFileName() != "") {
			//Open unit definition
			NNDocument unit;
			unit.RegisterNNEngineInfo(SDeepEngineType::NNabla, nn_document.GetNNEngineInfo());
			unit.LoadFromFile(component->GetUnitFileName());

			FString parameter_scope_prefix = layer.GetDisplayPropertyValue(FString("ParameterScope")) + FString("/");
			FString layer_name_prefix = parameter_scope_prefix;
			while (layer_name_prefix.Pos("{")){
				const int p = layer_name_prefix.Pos("{");
				const int p2 = layer_name_prefix.Pos("}");
				layer_name_prefix = layer_name_prefix.SubString(1, p - 1) + layer_name_prefix.SubString(p2 + 1, layer_name_prefix.Length());
			}

			//Set property of layers in the unit
			FString outputs_layer_name_;
			for (int iul = 0 ; iul < unit.GetNumLayer() ; iul++) {
				const NNLayer &u_layer = unit.GetLayer(iul);
				for (int iup = 0 ; iup < u_layer.GetNumProperty() ; iup++) {
					const NNLayerProperty &u_property = u_layer.GetProperty(iup);
					if (u_property.GetValue().Pos("*UnitProperty")) {
						FString property_name = u_property.GetValue();
						property_name.SplitBy("(");
						property_name = property_name.SplitBy(")");

						const NNLayerProperty *property = layer.GetPropertyByName(property_name);
						assert(property);
						unit.SetLayerPropertyValue(u_layer.GetLayerID(), u_property.GetName(), property->GetDisplayValue());
					}
				}

				//Set arguments(weight)
				FStringList dummy;
                dummy.Add(FString(""));
				unit.SetExportedArgs(dummy, layer);

				//Set name of layer in the unit
				FString layer_name = layer_name_prefix + u_layer.GetPropertyValue(FString("Name"));
				unit.SetLayerPropertyValue(u_layer.GetLayerID(), FString("Name"), layer_name, false);

				//Set parameter scope in the unit
				const NNLayerProperty *u_property_parameter_scope = u_layer.GetPropertyByName("ParameterScope");
				if (u_property_parameter_scope){
					const NNLayerProperty *property_parameter_scope = layer.GetPropertyByName("ParameterScope");
					if (property_parameter_scope){
						FString parameter_scope =
							parameter_scope_prefix + u_property_parameter_scope->GetDisplayValue();
						unit.SetLayerPropertyValue(u_layer.GetLayerID(), FString("ParameterScope"), parameter_scope, false);
					}
				}

				//Set repeat id in the unit
				const NNLayerProperty *u_property_repeat_id = u_layer.GetPropertyByName("ID");
				if (u_property_repeat_id){
					unit.SetLayerPropertyValue(u_layer.GetLayerID(), FString("ID"), layer_name_prefix + u_property_repeat_id->GetDisplayValue(), false, true);
				}

				if (unit.EnumLinkIDBySourceLayerID(u_layer.GetLayerID(), 0).GetSize() == 0){
					outputs_layer_name_ = layer_name;
				}
			}

			//Paste unit to current structure
			structures.Paste(unit, false, FString("/"));

			//Connect inputs
			for (int ipin = 0 ; ipin < layer.GetNumSideConnector(0) + 1 ; ipin++) {
				FListT<int> link_ids = structures.EnumLinkIDByDestLayerID(layer.GetLayerID(), ipin);
				if (link_ids.GetSize() == 1){
					const NNLink *existing_link = structures.GetLinkByID(link_ids.GetAt(0));
					assert(existing_link);


					if (ipin == 0) {
						//Connect to the Input layer in the unit
						FString layer_name = layer_name_prefix + FString("Input");

						const int unit_input_layer_id = structures.GetLayerIDByName(layer_name);
                        assert(unit_input_layer_id >= 0);
						FListT<int> link_ids = structures.EnumLinkIDBySourceLayerID(unit_input_layer_id, 0);
						assert(link_ids.GetSize() == 1);
						const NNLink *input_link = structures.GetLinkByID(link_ids.GetAt(0));
						assert(input_link);
						const int dest_layer_id = input_link->GetDestLayerID();

						//delete original input layer
						structures.ClearSelection();
						structures.SetLayerSelected(unit_input_layer_id, true, false);
						structures.DeleteSelected();

						//Connect
						structures.AddLink(
							existing_link->GetSourceLayerID(), existing_link->GetSourcePinIndex(),
							dest_layer_id, 0);
					}else {
						//Search dest layer in the unit
						FString connector_name = layer.GetSideConnector(0, ipin - 1).GetName();

						FString layer_name =
							layer.GetDisplayPropertyValue(FString("ParrameterScope")) +
							FString("/") +
							connector_name.SplitBy(".");

						const NNLayer *u_layer = structures.GetLayerByID(structures.GetLayerIDByName(layer_name));
						assert(u_layer);

						for (int iupin = 0; iupin < u_layer->GetNumSideConnector(0) ; iupin++) {
							if (u_layer->GetSideConnector(0, iupin).GetName() == connector_name) {
								structures.AddLink(
									existing_link->GetSourceLayerID(), existing_link->GetSourcePinIndex(),
									u_layer->GetLayerID(), iupin + 1);
							}
						}
					}
				}
			}

			//Connect Output
			{
				FListT<int> link_ids = structures.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
				for (int il = 0 ; il < link_ids.GetSize() ; il++){
					const NNLink *existing_link = structures.GetLinkByID(link_ids.GetAt(il));
					assert(existing_link);

					const int dest_layer_id = existing_link->GetDestLayerID();

					//Delete existing link
					structures.ClearSelection();
					structures.SetCurrentLinkID(link_ids.GetAt(il));
					structures.DeleteSelected();

					//Add link
					const int output_layer_id = structures.GetLayerIDByName(outputs_layer_name_);
					assert(output_layer_id > 0);
					structures.AddLink(
						output_layer_id, 0,
						dest_layer_id, 0);
				}
			}

			//Delete unit layer
			structures.ClearSelection();
			structures.SetLayerSelected(layer.GetLayerID(), true, false);
			structures.DeleteSelected();
			il -= 1;// to expand next layer of the expanded unit layer
		}
	}

	structures.EscapeLayerName(FString("/.{}"));
	structures.ArrangeLayers(true);
	structures.RefreshDisplayValue(&nn_document);
	structures.RefreshStatistics();


	//Add integrated Parameter and Input layers
	for (int il = 0 ; il < structures.GetNumLayer() ; il++) {
		const NNLayer &layer = structures.GetLayer(il);
		for (int is = 0 ; is < layer.GetNumSideConnector(0) ; is++){
			const NNLayerSideConnector &side_connector = layer.GetSideConnector(0, is);
			if (structures.GetPinEnable(layer.GetLayerID(), 0, is + 1)) {
				if (!layer.GetInputPinConnected(is + 1)){
					const NNLayerComponent *component = layer.GetComponent();
					assert(component);
					const NNLayerSideConnector &connector = layer.GetSideConnector(0, is);

					if (connector.GetKind() == NNLayerComponentInputKind::Data) {
						//Input connector -> Create new Input layer
						NNLayer &new_layer = structures.AddLayer(structures.GetNNLayerComponents()->GetComponentIndexByType(FString("Input")),false, 0.0f, 0.0f, false);
						new_layer.SetPropertyValue(FString("Size"), FString("*GraphOutput"));
						new_layer.SetPropertyValue(FString("Name"), layer.GetDisplayPropertyValue(FString("Name")) + FString("_") + side_connector.GetName());
						new_layer.SetPropertyValue(FString("Dataset"), layer.GetDisplayPropertyValue(side_connector.GetName() + FString(".Dataset")));
						new_layer.SetPropertyValue(FString("Generator"), layer.GetDisplayPropertyValue(side_connector.GetName() + FString(".Generator")));
						new_layer.SetPropertyValue(FString("GeneratorMultiplier"), layer.GetDisplayPropertyValue(side_connector.GetName() + FString(".GeneratorMultiplier")));
						structures.AddLink(new_layer.GetLayerID(), 0, layer.GetLayerID(), is + 1);
					}else if (connector.GetKind() == NNLayerComponentInputKind::Parameter) {
						//Parameter connector
						FString layer_parameter_scope = layer.GetDisplayPropertyValue(FString("ParameterScope"));
						if (layer_parameter_scope != ""){
							layer_parameter_scope += "/";
						}
						FString parameter_name =
							layer_parameter_scope +
							(connector.GetParameterScope() != "" ? connector.GetParameterScope() : component->GetParameterScope() + FString("/") + side_connector.GetName());
						FListT<NNLayer *> layers = structures.GetLayerByPropertyValue(FString("Name"), parameter_name, true);
						if (layers.GetSize()) {
							//Parameter already exists -> Link from existing Parameter layer
							assert(layers.GetSize() == 1);
							structures.AddLink(layers.GetAt(0)->GetLayerID(), 0, layer.GetLayerID(), is + 1);
						}else {
							//Create new parameter layer
							NNLayer &new_layer = structures.AddLayer(structures.GetNNLayerComponents()->GetComponentIndexByType(FString("Parameter")),false, 0.0f, 0.0f, false);
							new_layer.SetPropertyValue(FString("Size"), FString("*GraphOutput"));
							new_layer.SetPropertyValue(FString("Name"), parameter_name);
							new_layer.SetPropertyValue(FString("File"), layer.GetDisplayPropertyValue(side_connector.GetName() + FString(".File")));
							new_layer.SetPropertyValue(FString("Initializer"), layer.GetDisplayPropertyValue(side_connector.GetName() + FString(".Initializer")));
							new_layer.SetPropertyValue(FString("InitializerMultiplier"), layer.GetDisplayPropertyValue(side_connector.GetName() + FString(".InitializerMultiplier")));
							new_layer.SetPropertyValue(FString("LRateMultiplier"), layer.GetDisplayPropertyValue(side_connector.GetName() + FString(".LRateMultiplier")));
							structures.AddLink(new_layer.GetLayerID(), 0, layer.GetLayerID(), is + 1);
						}
					}
				}
			}
		}
	}

	structures.EscapeLayerName(FString("/.{}"));
	if (arrange){
		structures.ArrangeLayers(true);
	}
	structures.RefreshDisplayValue(&nn_document);
	structures.RefreshStatistics();

}
//---------------------------------------------------------------------------
void ProcessIntoTestNetwork(NNStructure &structure)
{
	//Delete DropOut, Set BatchStat of BN to false
	for (int il = structure.GetNumLayer() - 1; il >= 0  ; il--) {
		const NNLayer &layer = structure.GetLayer(il);
		const NNLayerComponent *component = layer.GetComponent();
		const NNLayerProperty *skip_property = layer.GetPropertyIndexByName("SkipAtTest") ? layer.GetPropertyByName("SkipAtTest") : layer.GetPropertyByName("SkipAtInspection");
		if (skip_property) {
			if (skip_property->GetIntArray().GetAt(0)) {
				structure.RemoveLayer(layer.GetLayerID());
			}
		}else if (component){
			//Set test time property value
			for (int ip = 0 ; ip < layer.GetNumProperty() && ip < component->GetNumProperty() ; ip++) {
				const NNLayerComponentProperty &property = component->GetProperty(ip);
				FString s = property.GetArgumentName();
				s.SplitBy("-test=");
				if (s != "") {
					structure.SetLayerPropertyValue(layer.GetLayerID(), ip, s);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void CreateNNablaProtoTxt_DataVariable(const NNStructure &structure, FString &indent, FStringList &prototxt)
{
	FListT<const NNLayer*> data_variables;
	for (int il = 0; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);
		if (layer.GetType() == "Input") {
			data_variables.Add(&layer);
		}
	}

    // insertion sort by order
	for (int il = 1; il < data_variables.GetSize() ; il++) {
		int pos = il - 1;
		while (pos >= 0){
			const int order1 = data_variables.GetAt(pos)->GetPropertyValue(FString("Order")).AsInt();
			const int order2 = data_variables.GetAt(pos + 1)->GetPropertyValue(FString("Order")).AsInt();
			if (order2 >= 0 && (order1 > order2 || order1 < 0)) {
				data_variables.Swap(pos, pos + 1);
				pos--;
			}else{
				break;
			}
		}

	}

	for (int il = 0; il < data_variables.GetSize() ; il++) {
		const NNLayer &layer = *data_variables.GetAt(il);
		if (layer.GetDisplayPropertyValue(FString("Generator")) == "None") {
			//Data Variable
			prototxt.Add(indent + FString("data_variable {"));
			indent += "  ";

			prototxt.Add(indent + FString("variable_name: \"") + layer.GetDisplayPropertyValue(FString("Name")) + FString("\""));
			prototxt.Add(indent + FString("data_name: \"") + layer.GetDisplayPropertyValue(FString("Dataset")) + FString("\""));

			indent = indent.SubString(1, indent.Length() - 2);
			prototxt.Add(indent + FString("}"));//Data Variable
		}else {
			//Generator Variable
			prototxt.Add(indent + FString("generator_variable {"));
			indent += "  ";

			prototxt.Add(indent + FString("variable_name: \"") + layer.GetDisplayPropertyValue(FString("Name")) + FString("\""));
			prototxt.Add(indent + FString("type: \"") + layer.GetDisplayPropertyValue(FString("Generator")) + FString("\""));
			prototxt.Add(indent + FString("multiplier: ") + layer.GetDisplayPropertyValue(FString("GeneratorMultiplier")));

			indent = indent.SubString(1, indent.Length() - 2);
			prototxt.Add(indent + FString("}"));//Generator Variable
		}
	}
}
//---------------------------------------------------------------------------
void RemoveLossFunctions(NNStructure &structure, int no_output_layer)
{
	const int output_component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Output"));

	//Remove link from label data to loss function
	int loss_function_removed = false;
	for (int il = structure.GetNumLayer() - 1 ; il >= 0  ; il--) {
		const NNLayer &layer = structure.GetLayer(il);
		const NNLayerComponent *component = layer.GetComponent();
		int last_identity = false;
		if (layer.GetType() == "Identity"){
			if (structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0).GetSize() == 0){
				last_identity = true;
			}
		}

		//Loss Function or last Identity
		if (layer.GetPropertyIndexByName("IsLossFunction") >= 0 || last_identity) {
			int property_index = -1;
			const NNLayer *source_layer = NULL;
			for (int is = 0 ; is < layer.GetNumSideConnector(0) ; is++){
				const NNLayerSideConnector &side_connector = layer.GetSideConnector(0, is);
				if (structure.GetPinEnable(layer.GetLayerID(), 0, is + 1)) {
					if (layer.GetInputPinConnected(is + 1)){
						//Check dataset input pin to get source dataset layer and delete link to dataset layer
						const int link_id = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), is + 1).GetAt(0);
						const NNLink *link = structure.GetLinkByID(link_id);
						source_layer = structure.GetLayerByID(link->GetSourceLayerID());
						property_index = source_layer->GetPropertyIndexByName("Dataset");

						//Delete link from dataset to loss layer
						structure.ClearSelection();
						structure.SetCurrentLinkID(link_id);
						structure.DeleteSelected();
					}
				}
			}
			//Check loss normal input to add output layer
			const FListT<int> links = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
			if (links.GetSize() == 1) {
				const int link_id2 = links.GetAt(0);
				const NNLink *link2 = structure.GetLinkByID(link_id2);
				const NNLayer *source_layer2 = structure.GetLayerByID(link2->GetSourceLayerID());
				if (source_layer2){
					structure.SetCurrentLayerID(source_layer2->GetLayerID());
				}
				structure.SetLayerSelected(layer.GetLayerID(), true, false);
				NNLayer &output = structure.AddLayer(output_component_index, true);

				if (source_layer && property_index >= 0) {
					output.SetPropertyValue(FString("VariableName"), source_layer->GetProperty(property_index).GetValue() + FString("'"));
				}else if (source_layer2){
					output.SetPropertyValue(FString("VariableName"), source_layer2->GetDisplayPropertyValue(FString("Name")));
				}
			}

			//Delete loss layer
			structure.ClearSelection();
			structure.SetLayerSelected(layer.GetLayerID(), true, false);
			structure.DeleteSelected();
            loss_function_removed = true;
		}
	}
	//Remove unnecessary functions from runtime network
	if (loss_function_removed) while (true) {
		int removed_layer = 0;
		for (int il = structure.GetNumLayer() - 1 ; il >= 0 ; il--) {
			const NNLayer &layer = structure.GetLayer(il);
			const int num_out_link = structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0).GetSize();
			if (layer.GetType() != "Output" && num_out_link == 0) {
				//Delete layer with no destination
				structure.ClearSelection();
				structure.SetLayerSelected(layer.GetLayerID(), true, false);
				structure.DeleteSelected();
				removed_layer++;
			}
		}
		if (removed_layer == 0) {
			break;
		}
	}

	if (no_output_layer) {
		for (int il = structure.GetNumLayer() - 1 ; il >= 0 ; il--) {
			const NNLayer &layer = structure.GetLayer(il);
			if (layer.GetType() == "Output") {
				//Delete layer with no destination
				structure.ClearSelection();
				structure.SetLayerSelected(layer.GetLayerID(), true, false);
				structure.DeleteSelected();
			}
		}
	}

	structure.RefreshDisplayValue();
	structure.RefreshStatistics();
}
//---------------------------------------------------------------------------
int CreateNNablaProtoTxtForTraining(
	const NNDocument &nn_document, const SDeepConsoleGlobalSettings &settings, const char *file_name,//in
	FStringList &prototxt, FStringList &param_csv, FStringList &error_str) //out
{
	NNStructures structures;
	structures = nn_document;
	const NNConfig &config = nn_document;
	const NNDatasets &datasets = nn_document;

	FListP<NNStructure> all_networks;

	//Build complete network with parameter layer
	for (int is = 0 ; is < structures.GetNumStructure() ; is++) {
		structures.SetActiveStructure(is);

		if (nn_document.IsStructureUsed(structures.GetName()) || structures.GetName() == "Main"){
			//Check test network
			int test_network = false;//Default : Test = true
			FListT<NNLayer*> test_layer = structures.GetLayerByType(FString("TestNetwork"), true);
			if (test_layer.GetSize()){
				const NNLayerProperty *property = test_layer.GetAt(0)->GetPropertyByName(FString("ModifyForTesting"));
				if (property){
					test_network = property->GetIntArrayProduct();
				}
			}

			FlattenCurrentStructure(nn_document, structures, true);

			if (test_network) {
				ProcessIntoTestNetwork(structures);
				structures.RefreshDisplayValue();
				structures.RefreshStatistics();
			}

			if (structures.GetName() == "Main") {
				// main network -> Generate validation and runtime network automatically
				all_networks.Add(structures);

				//Build validation network
				NNStructure &structure_validation = all_networks.Add(structures);
				ProcessIntoTestNetwork(structure_validation);
				structure_validation.RefreshDisplayValue();
				structure_validation.RefreshStatistics();
				{
					//Set unique validation network name
					int index = 1;
					while (true){
						FString s = FString("MainValidation");
						if (index > 1) {
							s = FString("_") + FString(index);
						}
						if (nn_document.GetStructureIndexByName(s) < 0) {
							structure_validation.SetName(s);
							break;
						}
						index++;
					}
				}

				//Build runtime network
				NNStructure &structure_runtime = all_networks.Add(structure_validation);

				RemoveLossFunctions(structure_runtime);
				{
					//Set unique runtime network name
					int index = 1;
					while (true){
						FString s = FString("MainRuntime");
						if (index > 1) {
							s = FString("_") + FString(index);
						}
						if (nn_document.GetStructureIndexByName(s) < 0) {
							structure_runtime.SetName(s);
							break;
						}
						index++;
					}
				}
			}else {
				all_networks.Add(structures);
			}
		}
	}

	//Delete unused network
	for (int is = all_networks.GetSize() - 1 ; is >= 0 ; is--) {
		const NNStructure &structure = all_networks.GetAt(is);
		int used = nn_document.IsStructureUsed(structure.GetName());

		if (!used){
			all_networks.Delete(is);
		}else {
			// create parameter assignment csv for cli_util
			for (int il = 0 ; il < structure.GetNumLayer(); il++) {
				const NNLayer &layer = structure.GetLayer(il);
				if (layer.GetType() == "Parameter" || layer.GetType() == "WorkingMemory") {
					// Parameter layer
					FStringList repeat_extensions = structure.GetRepeatExtensions(layer, true);
					FString file_names = layer.GetPropertyValue(FString("File"));
					if (file_names != "") {
						for (int ire = 0; ire < repeat_extensions.GetSize() ; ire++) {
							FString tmp = file_names.SplitBy(";");
							FString file_name = tmp.SplitBy("*");
							FString option = tmp;
							if (file_name != ""){
								FString line = file_name + FString(",") + repeat_extensions.GetAt(ire);
								if (param_csv.IndexOf(line) <= 0){
									param_csv.Add(line);
								}
							}
						}
					}
				}
			}
		}
	}


	//Create prototxt
	FString indent;


	//GlobalConfig
	prototxt.Add(indent + FString("global_config {"));
	indent += "  ";

	//  Context
	prototxt.Add(indent + FString("default_context {"));
	indent += "  ";

	prototxt.Add(indent + FString("array_class: \"") + FString(settings.GetLocalSDeepProcessorType() == LocalSDeepProcessorType::CPU ? "CpuCachedArray" : "CudaCachedArray") + FString("\""));
	if (settings.GetLocalSDeepProcessorType() != LocalSDeepProcessorType::CPU) {
		prototxt.Add(indent + FString("device_id: \"") + FString(settings.GetLocalSDeepGPUIndex() >= 0 ? settings.GetLocalSDeepGPUIndex() : 0) + FString("\""));
	}
	switch (settings.GetLocalSDeepProcessorType()){
	case LocalSDeepProcessorType::cuDNN_CUDA:
		if (nn_document.GetTypeConfig() == TypeConfig::Half){
			prototxt.Add(indent + FString("backends: \"cudnn:half\""));
		}
		prototxt.Add(indent + FString("backends: \"cudnn:float\""));
	default:
		break;
	}
	switch (settings.GetLocalSDeepProcessorType()){
	case LocalSDeepProcessorType::cuDNN_CUDA:
	case LocalSDeepProcessorType::CUDA:
		if (nn_document.GetTypeConfig() == TypeConfig::Half){
			prototxt.Add(indent + FString("backends: \"cuda:half\""));
		}
		prototxt.Add(indent + FString("backends: \"cuda:float\""));
	default:
		break;
	}
	prototxt.Add(indent + FString("backends: \"cpu:float\""));

	indent = indent.SubString(1, indent.Length() - 2);
	prototxt.Add(indent + FString("}"));//Context

	indent = indent.SubString(1, indent.Length() - 2);
	prototxt.Add(indent + FString("}"));//GlobalConfig


	//TrainingConfig
	prototxt.Add(indent + FString("training_config {"));
	indent += "  ";

	const NNDataset &main_dataset = nn_document.GetDataset(nn_document.GetMainDatasetIndex());
	prototxt.Add(indent + FString("max_epoch: ") + FString(nn_document.GetCommandType() == SDeepCommandType::CreateTrainingResult ? 0 : config.GetMaxEpoch()));
	const int iter_per_epoch = main_dataset.GetNumData() / nn_document.GetBatchSize();
	const int max_iter = iter_per_epoch * config.GetMaxEpoch();
	prototxt.Add(indent + FString("iter_per_epoch: ") + FString(iter_per_epoch));
	prototxt.Add(indent + FString("save_best: ") + FString(config.GetSaveBest() ? "true" : "false"));
	prototxt.Add(indent + FString("monitor_interval: ") + FString(nn_document.GetMonitorInterval()));

	indent = indent.SubString(1, indent.Length() - 2);
	prototxt.Add(indent + FString("}"));//TrainingConfig


	//Networks
	for (int in = 0 ; in < all_networks.GetSize() ; in++){
		NetworkToNNablaProtoTxt(all_networks.GetAt(in), config, prototxt, indent);
	}


	//Dataset
	FStringList nntxt_dataset_names;
	for (int i = 0 ; i < datasets.GetNumDataset() ; i++) {
		const NNDataset &dataset = datasets.GetDataset(i);
		FString dataset_file_name = dataset.GetFileName();
		if (dataset_file_name != ""){
			prototxt.Add(indent + FString("dataset {"));
			indent += "  ";

			prototxt.Add(indent + FString("name: \"") + dataset.GetName() + FString("\""));
			prototxt.Add(indent + FString("uri: \"") + dataset_file_name.ReplaceStr("\\","\\\\") + FString("\""));
			FString cache_dir = dataset_file_name.SubString(1, dataset_file_name.Length() - 4).ReplaceStr("\\","\\\\") + FString(".cache");
			prototxt.Add(indent + FString("cache_dir: \"") + cache_dir + FString("\""));
			prototxt.Add(indent + FString("overwrite_cache: ") + FString(dataset.GetEnableCache() ? "false" : "true"));
			prototxt.Add(indent + FString("create_cache_explicitly: true"));

			prototxt.Add(indent + FString("shuffle: ") + FString(dataset.GetShuffle() ? "true" : "false"));
			prototxt.Add(indent + FString("no_image_normalization: ") + FString(dataset.GetImageNormalization() ? "false" : "true"));
			prototxt.Add(indent + FString("batch_size: ") + FString(nn_document.GetBatchSize()));

			indent = indent.SubString(1, indent.Length() - 2);
			prototxt.Add(indent + FString("}"));//Dataset

			nntxt_dataset_names.Add(dataset.GetName());
		}
	}

	//Optimizer
	for (int io = 0 ; io < nn_document.GetNumOptimizer() ; io++){
		const NNOptimizer &optimizer = nn_document.GetOptimizer(io);

		int structure_index = -1;
		for (int in = 0 ; in < all_networks.GetSize() ; in++) {
			if (all_networks.GetAt(in).GetName() == optimizer.GetNetworkName()) {
				structure_index = in;
			}
		}
		if (structure_index >= 0){
			prototxt.Add(indent + FString("optimizer {"));
			indent += "  ";

			const int start_iter = optimizer.GetRangeFrom() > 0 ?
				(optimizer.GetRangeUnit() == 0 ?
					(optimizer.GetRangeFrom() - 1) * iter_per_epoch + 1 : //in epoch
					optimizer.GetRangeFrom()) ://in iter
				0;
			const int end_iter = optimizer.GetRangeTo() > 0 ?
				(optimizer.GetRangeUnit() == 0 ?
					optimizer.GetRangeTo() * iter_per_epoch ://in epoch
					optimizer.GetRangeTo()) ://in iter
				0;
			prototxt.Add(indent + FString("start_iter: ") + FString(start_iter));
			prototxt.Add(indent + FString("end_iter: ") + FString(end_iter));
			prototxt.Add(indent + FString("name: \"") + FString(optimizer.GetOptimizerName()) + FString("\""));

			prototxt.Add(indent + FString("update_interval: ") + FString(optimizer.GetUpdateInterval()));

			prototxt.Add(indent + FString("network_name: \"") + optimizer.GetNetworkName() + FString("\""));
			FStringList dataset_names = optimizer.GetDatasetNames();
			for (int id = 0 ; id < dataset_names.GetSize() ; id++){
				const FString dataset_name = dataset_names.GetAt(id);
				if (nntxt_dataset_names.IndexOf(dataset_name) >= 0){
					prototxt.Add(indent + FString("dataset_name: \"") + dataset_name + FString("\""));
				}
			}

			//  Solver
			prototxt.Add(indent + FString("solver {"));
			indent += "  ";
			prototxt.Add(indent + FString("type: \"") + optimizer.GetSolverName() + FString("\""));

			prototxt.Add(indent + FString("weight_decay: ") + FString(optimizer.GetWeightDecay()));

			prototxt.Add(indent + optimizer.GetSolverArgumentName() + FString(" {"));
			indent += "  ";
			for (int ip = 0; ip < optimizer.GetSolverParameterNum(); ip++) {
				prototxt.Add(indent + optimizer.GetSolverParameterArgumentName(ip) + FString(": ") + FString(optimizer.GetSolverParameter(ip)));
			}
			indent = indent.SubString(1, indent.Length() - 2);
			prototxt.Add(indent + FString("}"));//Solver argument

			prototxt.Add(indent + FString("lr_scheduler_type: \"") + optimizer.GetLearningRateScheduler() + FString("\""));
			if (optimizer.GetLearningRateScheduler() == "Cosine"){
				prototxt.Add(indent + FString("cosine_scheduler_param {"));
				indent += "  ";
				prototxt.Add(indent + FString("max_iter: ") + FString(max_iter));
				indent = indent.SubString(1, indent.Length() - 2);
				prototxt.Add(indent + FString("}"));//cosine_scheduler_param
			}else if (optimizer.GetLearningRateScheduler() == "Exponential"){
				prototxt.Add(indent + FString("exponential_scheduler_param {"));
				indent += "  ";
				prototxt.Add(indent + FString("gamma: ") + FString(optimizer.GetLearningRateMultiplier()));
				const int lr_update_interval = optimizer.GetLearningRateUpdateIntervalUnit() ?
					optimizer.GetLearningRateUpdateInterval() : // iteration
					optimizer.GetLearningRateUpdateInterval() * iter_per_epoch;
				prototxt.Add(indent + FString("iter_interval: ") + FString(lr_update_interval));
				indent = indent.SubString(1, indent.Length() - 2);
				prototxt.Add(indent + FString("}"));//exponential_scheduler_param

				//for compatibility
				prototxt.Add(indent + FString("lr_decay: ") + FString(optimizer.GetLearningRateMultiplier()));
				prototxt.Add(indent + FString("lr_decay_interval: ") + FString(lr_update_interval));
			}else if (optimizer.GetLearningRateScheduler() == "Polynomial"){
				prototxt.Add(indent + FString("polynomial_scheduler_param {"));
				indent += "  ";
				prototxt.Add(indent + FString("max_iter: ") + FString(max_iter));
				prototxt.Add(indent + FString("power: ") + FString(optimizer.GetLearningRatePower()));
				indent = indent.SubString(1, indent.Length() - 2);
				prototxt.Add(indent + FString("}"));//polynomial_scheduler_param
			}else if (optimizer.GetLearningRateScheduler() == "Step"){
				prototxt.Add(indent + FString("step_scheduler_param {"));
				indent += "  ";
				prototxt.Add(indent + FString("gamma: ") + FString(optimizer.GetLearningRateMultiplier()));
				for (int is = 0 ; is < optimizer.GetLearningRateSteps().GetSize() ; is++){
					const int lr_update_step = optimizer.GetLearningRateUpdateIntervalUnit() ?
						optimizer.GetLearningRateSteps().GetAt(is) : // iteration
						optimizer.GetLearningRateSteps().GetAt(is) * iter_per_epoch;
					prototxt.Add(indent + FString("iter_steps: ") + FString(lr_update_step));
				}
				indent = indent.SubString(1, indent.Length() - 2);
				prototxt.Add(indent + FString("}"));//step_scheduler_param
			}

			prototxt.Add(indent + FString("lr_warmup_scheduler_type: \"") + optimizer.GetLearningRateWarmupScheduler() + FString("\""));
			if (optimizer.GetLearningRateWarmupScheduler() == "Linear"){
				prototxt.Add(indent + FString("linear_warmup_scheduler_param {"));
				indent += "  ";
				const int lr_warmup_length = optimizer.GetLearningRateWarmupLengthUnit() ?
					optimizer.GetLearningRateWarmupLength() : // iteration
					optimizer.GetLearningRateWarmupLength() * iter_per_epoch;
				prototxt.Add(indent + FString("warmup_iter: ") + FString(lr_warmup_length));
				indent = indent.SubString(1, indent.Length() - 2);
				prototxt.Add(indent + FString("}"));//polynomial_scheduler_param
			}

			indent = indent.SubString(1, indent.Length() - 2);
			prototxt.Add(indent + FString("}"));//Solver

			const NNStructure &structure = all_networks.GetAt(structure_index);

			//  Data Variable
			CreateNNablaProtoTxt_DataVariable(structure, indent, prototxt);

			//  Loss Variable
			for (int il = 0; il < structure.GetNumLayer() ; il++) {
				const NNLayer &layer = structure.GetLayer(il);
				FListT<int> output = structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
				if (output.GetSize() == 0) {//Is terminal layer
					//Loss Variable
					prototxt.Add(indent + FString("loss_variable {"));
					indent += "  ";

					prototxt.Add(indent + FString("variable_name: \"") + layer.GetDisplayPropertyValue(FString("Name")) + FString("\""));

					indent = indent.SubString(1, indent.Length() - 2);
					prototxt.Add(indent + FString("}"));//Loss Variable
				}
			}

			//  Parameter Variable
			for (int il = 0; il < structure.GetNumLayer() ; il++) {
				const NNLayer &layer = structure.GetLayer(il);
				int learning_rate_multiplier_index = layer.GetPropertyIndexByName("LRateMultiplier");
				if (learning_rate_multiplier_index >= 0) {
					const NNLayerProperty &property = layer.GetProperty(learning_rate_multiplier_index);
					//Parameter Variable
					prototxt.Add(indent + FString("parameter_variable {"));
					indent += "  ";

					prototxt.Add(indent + FString("variable_name: \"") + layer.GetDisplayPropertyValue(FString("Name")) + FString("\""));
					prototxt.Add(indent + FString("learning_rate_multiplier: ") + property.GetDisplayValue());

					indent = indent.SubString(1, indent.Length() - 2);
					prototxt.Add(indent + FString("}"));//Parameter Variable
				}
			}

			indent = indent.SubString(1, indent.Length() - 2);
			prototxt.Add(indent + FString("}"));//Optimizer
		}
	}


	//Monitor
	for (int im = 0 ; im < nn_document.GetNumMonitor() ; im++){
		const NNMonitor &monitor = nn_document.GetMonitor(im);

		int structure_index = -1;
		for (int in = 0 ; in < all_networks.GetSize() ; in++) {
			if (all_networks.GetAt(in).GetName() == monitor.GetNetworkName()) {
				structure_index = in;
			}
		}
		if (structure_index >= 0){
			prototxt.Add(indent + FString("monitor {"));
			indent += "  ";

			prototxt.Add(indent + FString("name: \"") + monitor.GetMonitorName() + FString("\""));

			prototxt.Add(indent + FString("network_name: \"") + monitor.GetNetworkName() + FString("\""));
			FStringList dataset_names = monitor.GetDatasetNames();
			for (int id = 0 ; id < dataset_names.GetSize() ; id++){
				const FString dataset_name = dataset_names.GetAt(id);
				if (nntxt_dataset_names.IndexOf(dataset_name) >= 0){
					prototxt.Add(indent + FString("dataset_name: \"") + dataset_name + FString("\""));
				}
			}

			const NNStructure &structure = all_networks.GetAt(structure_index);

			//  Data Variable
			CreateNNablaProtoTxt_DataVariable(structure, indent, prototxt);

			//  Monitor Variable
			for (int il = 0; il < structure.GetNumLayer() ; il++) {
				const NNLayer &layer = structure.GetLayer(il);
				FListT<int> output = structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
				if (output.GetSize() == 0) {//Is terminal layer
					//Monitor Variable
					prototxt.Add(indent + FString("monitor_variable {"));
					indent += "  ";

					prototxt.Add(indent + FString("type: \"Error\""));
					prototxt.Add(indent + FString("variable_name: \"") + layer.GetDisplayPropertyValue(FString("Name")) + FString("\""));

					indent = indent.SubString(1, indent.Length() - 2);
					prototxt.Add(indent + FString("}"));//Error Variable
				}
			}
			indent = indent.SubString(1, indent.Length() - 2);
			prototxt.Add(indent + FString("}"));//Monitor
        }
	}


	//Executor
	for (int ie = 0 ; ie < nn_document.GetNumExecutor() ; ie++){
		const NNExecutor &executor = nn_document.GetExecutor(ie);

		int structure_index = -1;
		for (int in = 0 ; in < all_networks.GetSize() ; in++) {
			if (all_networks.GetAt(in).GetName() == executor.GetNetworkName()) {
				structure_index = in;
			}
		}
		if(structure_index >= 0) {
			prototxt.Add(indent + FString("executor {"));
			indent += "  ";

			prototxt.Add(indent + FString("name: \"") + executor.GetExecutorName() + FString("\""));
			prototxt.Add(indent + FString("network_name: \"") + executor.GetNetworkName() + FString("\""));
			prototxt.Add(indent + FString("num_evaluations: ") + FString(executor.GetNumEvaluations()));
			FString evaluation_type_str = FString("mean");
			switch (executor.GetRepeatEvaluationType()) {
			case 1:
				evaluation_type_str = FString("last");
				break;
			case 2:
				evaluation_type_str = FString("std");
				break;
			}
			prototxt.Add(indent + FString("repeat_evaluation_type: \"") + evaluation_type_str + FString("\""));
			prototxt.Add(indent + FString("need_back_propagation: ") + FString(executor.GetNeedBackPropagation() ? "true" : "false"));

			int image_normalization = true;
			if (executor.GetDatasetNames().GetSize()){
				//Todo : multiple dataset for executor
				const FString &dataset_name = executor.GetDatasetNames().GetAt(0);
				const int dataset_index = nn_document.GetDatasetIndexByName(dataset_name);
				if (dataset_index >= 0){
					image_normalization = nn_document.GetDataset(dataset_index).GetImageNormalization();
				}
			}
			prototxt.Add(indent + FString("no_image_normalization: ") + FString(!image_normalization ? "true" : "false"));

			const NNStructure &structure = all_networks.GetAt(structure_index);

			//  Data Variable
			CreateNNablaProtoTxt_DataVariable(structure, indent, prototxt);

			//  Loss Variable
			if (executor.GetNeedBackPropagation()) {
				for (int il = 0; il < structure.GetNumLayer() ; il++) {
					const NNLayer &layer = structure.GetLayer(il);
					FListT<int> output = structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
					if (output.GetSize() == 0) {//Is terminal layer
						//Loss Variable
						prototxt.Add(indent + FString("loss_variable {"));
						indent += "  ";

						prototxt.Add(indent + FString("variable_name: \"") + layer.GetDisplayPropertyValue(FString("Name")) + FString("\""));

						indent = indent.SubString(1, indent.Length() - 2);
						prototxt.Add(indent + FString("}"));//Loss Variable
					}
				}
			}

			//  Output Variable
			{
				FStringList data_name_and_variable_name;
				for (int il = 0; il < structure.GetNumLayer() ; il++) {
					const NNLayer &layer = structure.GetLayer(il);
					if (layer.GetType() == "Output") {
						const FListT<int> links = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
						if (links.GetSize()){
							const int link_id2 = links.GetAt(0);
							const NNLink *link2 = structure.GetLinkByID(link_id2);
							const NNLayer *source_layer2 = structure.GetLayerByID(link2->GetSourceLayerID());
							data_name_and_variable_name.Add(
								layer.GetDisplayPropertyValue(FString("VariableName")) +
								FString(":") +
								source_layer2->GetDisplayPropertyValue(FString("Name")));
						}
					}else if (structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0).GetSize() == 0){
						//Terminal Variable
						data_name_and_variable_name.Add(
							layer.GetDisplayPropertyValue(FString("Name")) +
							FString(":") +
							layer.GetDisplayPropertyValue(FString("Name")));
					}
				}

				//Sort output variable with its name
				data_name_and_variable_name.Sort();
				for (int i = 0 ; i < data_name_and_variable_name.GetSize() ; i++){
					const FString data_name = data_name_and_variable_name.GetAt(i).SplitBy(":");
					const FString &variable_name = data_name_and_variable_name.GetAt(i);
					prototxt.Add(indent + FString("output_variable {"));
					indent += "  ";

					prototxt.Add(indent + FString("variable_name: \"") + variable_name + FString("\""));
					prototxt.Add(indent + FString("data_name: \"") + data_name + FString("\""));

					indent = indent.SubString(1, indent.Length() - 2);
					prototxt.Add(indent + FString("}"));//Output Variable
				}
			}

			//  Parameter Variable
			for (int il = 0; il < structure.GetNumLayer() ; il++) {
				const NNLayer &layer = structure.GetLayer(il);
				int learning_rate_multiplier_index = layer.GetPropertyIndexByName("LRateMultiplier");
				if (learning_rate_multiplier_index >= 0) {
					const NNLayerProperty &property = layer.GetProperty(learning_rate_multiplier_index);
					//Parameter Variable
					prototxt.Add(indent + FString("parameter_variable {"));
					indent += "  ";

					prototxt.Add(indent + FString("variable_name: \"") + layer.GetDisplayPropertyValue(FString("Name")) + FString("\""));
					if (executor.GetNeedBackPropagation()) {
						prototxt.Add(indent + FString("learning_rate_multiplier: ") + property.GetDisplayValue());
					}

					indent = indent.SubString(1, indent.Length() - 2);
					prototxt.Add(indent + FString("}"));//Parameter Variable
				}
			}

			indent = indent.SubString(1, indent.Length() - 2);
			prototxt.Add(indent + FString("}"));//Executor
        }
	}


	if (!prototxt.SaveToFile(file_name)){
		error_str.Add(FString("Can't write to \"") + FString(file_name) + FString("\""));
	}

	return error_str.GetSize() == 0;
}
//---------------------------------------------------------------------------
void SplitStringListToChunk(const FStringList &source, FStringList &chunk_names, FListP<FStringList> &chunks)
{
	int current_chunk_index = -1;
	for (int il = 0 ; il < source.GetSize() ; il++){
		const FString &line = source.GetAt(il);
		if (line.SubString(1, 1) == "}"){
			current_chunk_index = -1;
		}else if (line.SubString(1, 2) == "  " && current_chunk_index >= 0){
			chunks.GetAt(current_chunk_index).Add(line.SubString(3, line.Length() - 2));
		}else if (line.SubString(line.Length(), 1) == "{"){
			current_chunk_index = chunk_names.GetSize();
			chunk_names.Add(FString(line).SplitBy(" "));//remove last " {"
			chunks.Add(FStringList());
		}else {
			chunk_names.Add(FString(line));//dict etc
			chunks.Add(FStringList());
		}
	}
}
//---------------------------------------------------------------------------
void DecodeDict(const FStringList &source, FStringList &keys, FStringList &values)
{
	FStringList path;
	for (int il = 0 ; il < source.GetSize() ; il++){
		FString line = source.GetAt(il);
		int p = line.Pos(": ");
		if (p){
			//Value
			FString key = line.SubString(1, p - 1);
			for (int ip = path.GetSize() - 1 ; ip >= 0 ; ip--){
				key = path.GetAt(ip) + FString(".") + key.ReplaceStr(" ", "");
			}
			keys.Add(key);
			FString value = line.SubString(p + 2, line.Length());
			if (value.SubString(1, 1) == "\"" && value.SubString(value.Length(), 1) == "\""){
				values.Add(value.SubString(2, value.Length() - 2));
			}else {
				values.Add(value);
			}
		}else if (path.GetSize() && line.SubString(path.GetSize() * 2 - 1, 1) == "}"){
			path.Delete(path.GetSize() - 1);
		}else if (line.Pos(" {")){
			line = line.SubString(path.GetSize() * 2 + 1, line.Length());
			path.Add(line.SplitBy(" {"));
		}
	}
}
//---------------------------------------------------------------------------
FStringList GetProperty(const FStringList &keys, const FStringList &values, const FString &key)
{
	FStringList result;
	const int key_length = key.Length();
    int found = false;
	for (int ik = 0 ; ik < keys.GetSize() ; ik++){
		const FString &_key = keys.GetAt(ik);
		const int pos = _key.Pos(key);
		if (pos == 1 && (_key.SubString(key_length + 1, 1) == "." || key == _key)){
			if (values.GetAt(ik).SubString(1, 1) == "{" && values.GetAt(ik).SubString(values.GetAt(ik).Length(), 1) == "}"){
				result.Add(values.GetAt(ik).ReplaceStr("{ dim:", "").ReplaceStr(" dim: ", ",").ReplaceStr(" }","").ReplaceStr(" ",""));
			}else {
				result.Add(values.GetAt(ik));
            }
			found = true;
		}else if (found){
			break;
		}
	}
	return result;
}
//---------------------------------------------------------------------------
FString UpperPath(const FString source, char separator, int order)
{
    int count = 0;
	for (int i = source.Length() - 1 ; i >= 0 ; i--){
		if (source.PChar()[i] == separator){
			count++;
			if (count == order){
                return source.SubString(1, i);
			}
		}
	}
	return FString("");
}
//---------------------------------------------------------------------------
int NntxtToDocument(const char *file_name, NNDocument &nn_document, FStringList &error_str)
{
	// file_name(input) : file name of .nntxt to load
	// nn_document(output) : .nntxt will be loaded in nn_document

	FStringList proto_txt;
	if (!proto_txt.LoadFromFile(file_name)) {
		error_str.Add(FString("Can't read from \"") + FString(file_name) + FString("\""));
		return false;
	}

	//Split prototxt into chunk
	FStringList chunk_names;
	FListP<FStringList> chunks;
	SplitStringListToChunk(proto_txt, chunk_names, chunks);
	proto_txt.Clear();

	int network_num = 0;
	int dataset_num = 0;
	int optimizer_num = 0;
	int monitor_num = 0;
	int executor_num = 0;

	//To backup original value of properties
    FStringList property_layers_and_names, property_values_org;

	for (int ic = 0 ; ic < chunk_names.GetSize() ; ic++){
		const FString &chunk_name = chunk_names.GetAt(ic);
		if (chunk_name == "network"){
			network_num++;
			if (network_num > nn_document.GetNumStructure()) {
				nn_document.AddStructure();
			}

			const NNLayerComponents *components = nn_document.GetNNLayerComponents();
			FStringList func_var_names;
			FListP<FStringList> func_vars;
			SplitStringListToChunk(chunks.GetAt(ic), func_var_names, func_vars);
			FStringList keys, values;
			DecodeDict(func_var_names, keys, values);
			const FStringList network_name = GetProperty(keys, values, FString("name"));
			if (network_name.GetSize()){
				nn_document.SetName(network_name.GetAt(0));
			}
			const FStringList batch_size = GetProperty(keys, values, FString("batch_size"));
			if (batch_size.GetSize()){
				nn_document.SetBatchSize(batch_size.GetAt(0).AsInt());
			}

			FListP<FStringList> layer_inputs, layer_outputs;
			FStringList variable_names, variable_types, variable_shapes, variable_inittypes, variable_initmultipliers;
			FStringList repeat_ids;
			FListT<int> repeat_times;
			//Function variable loop
			for (int ifv = 0 ; ifv < func_var_names.GetSize() ; ifv++){
				FStringList keys, values;
				DecodeDict(func_vars.GetAt(ifv), keys, values);
				if (func_var_names.GetAt(ifv) == "function"){
					//Add layer
					const FStringList layer_name = GetProperty(keys, values, FString("name"));
					const FStringList layer_type = GetProperty(keys, values, FString("type"));
					if (layer_name.GetSize() && layer_type.GetSize()){
						int component_index = components->GetComponentIndexByType(layer_type.GetAt(0));
						if (component_index >= 0){
							NNLayer &layer = nn_document.AddLayer(component_index);

							//Set layer prperties
							layer.SetPropertyValue(FString("Name"), layer_name.GetAt(0));
							const NNLayerComponent &component = components->GetAt(component_index);
							for (int ip = 0 ; ip < component.GetNumProperty() ; ip++){
								const NNLayerComponentProperty &property = component.GetProperty(ip);
								if (property.GetArgumentName() != ""){
									FString s = property.GetArgumentName();
									FString argument_name = s.SplitBy(" ");
									const int ignore_first = s.Pos("-0") || s.Pos("-b");//0 or batch size inserted to the arg
									const int sub_1 = s.Pos("-a");//axis indices are incremented
									FStringList property_value = GetProperty(keys, values, argument_name);
									if (property_value.GetSize() == 1){
										if (property_value.GetAt(0).Pos(",")){
											property_value.SplitStr(FString(property_value.GetAt(0)), ",");
										}
									}
									if (property_value.GetSize()){
										//backup original property value
										FString property_value_org;
										for (int iv = 0 ; iv < property_value.GetSize() ; iv++){
											if (property_value_org != ""){
												property_value_org += FString(",");
											}
											property_value_org += property_value.GetAt(iv);
										}
										property_layers_and_names.Add(layer_name.GetAt(0) + FString("->") + property.GetName());
										property_values_org.Add(property_value_org);


										FString property_value_1;
										for (int iv = ignore_first ; iv < property_value.GetSize() ; iv++){
											if (property_value_1 != ""){
												property_value_1 += FString(",");
											}
											if (!sub_1){
												property_value_1 += property_value.GetAt(iv);
											}else {
												property_value_1 += FString(property_value.GetAt(iv).AsInt() - 1);
											}
										}
										if (property.GetType() <= NNLayerComponentPropertyType::BooleanOrMacro){
											if (property_value_1 == "true"){
												property_value_1 = "True";
											}else if (property_value_1 == "false"){
												property_value_1 = "False";
											}
										}
										layer.SetPropertyValue(property.GetName(), property_value_1);
									}else if (property.GetType() == NNLayerComponentPropertyType::Boolean || (property.GetType() == NNLayerComponentPropertyType::BooleanOrMacro && property.GetDefaultValue().SubString(1, 1) != "*")) {
										if (property.GetName() != "WithBias") {
											//Set false for boolean property as default
											layer.SetPropertyValue(property.GetName(), FString("False"));
										}
									}
								}
							}
							layer_inputs.Add(GetProperty(keys, values, FString("input")));
							layer_outputs.Add(GetProperty(keys, values, FString("output")));
						}else {
							//Unsupported
							error_str.Add(FString("Layer type '") + layer_type.GetAt(0) + FString("' is not supported at function '") + layer_name.GetAt(0) + FString("'."));
							/*
							int component_index = nn_document.GetNNLayerComponents()->GetComponentIndexByType(FString("Unsupported"));
							NNLayer &layer = nn_document.AddLayer(component_index);
							layer.SetPropertyValue(FString("Name"), layer_name.GetAt(0));
							layer.AddProperty(-1, FString("Type"), FString(""), true, false, false, false, NNLayerPropertyType::Error, layer_type.GetAt(0), true);
							layer_inputs.Add(GetProperty(keys, values, FString("input")));
							layer_outputs.Add(GetProperty(keys, values, FString("output")));
							*/
						}
					}
				}else if (func_var_names.GetAt(ifv) == "variable") {
					//variables

					const FStringList variable_name = GetProperty(keys, values, FString("name"));
					const FStringList variable_type = GetProperty(keys, values, FString("type"));
					const FStringList variable_shape = GetProperty(keys, values, FString("shape"));
					const FStringList variable_inittype = GetProperty(keys, values, FString("initializer.type"));
					const FStringList variable_initmultiplier = GetProperty(keys, values, FString("initializer.multiplier"));

					variable_names.Add(variable_name.GetSize() ? variable_name.GetAt(0) : FString(""));
					variable_types.Add(variable_type.GetSize() ? variable_type.GetAt(0) : FString(""));
					variable_shapes.Add(variable_shape.GetSize() ? variable_shape.GetText().ReplaceStr("\n", ",") : FString(""));
					variable_inittypes.Add(variable_inittype.GetSize() ? variable_inittype.GetAt(0) : FString(""));
					variable_initmultipliers.Add(variable_initmultiplier.GetSize() ? variable_initmultiplier.GetAt(0) : FString(""));
				}else if (func_var_names.GetAt(ifv) == "repeat_info") {
					const FStringList id = GetProperty(keys, values, FString("id"));
					const FStringList times = GetProperty(keys, values, FString("times"));
					if (id.GetSize() == 1 && times.GetSize() == 1){
						repeat_ids.Add(id.GetAt(0));
						repeat_times.Add(times.GetAt(0).AsInt());
					}
				}
			}

			//Add links, set parameters
			assert(nn_document.GetNumLayer() == layer_inputs.GetSize());
			assert(nn_document.GetNumLayer() == layer_outputs.GetSize());
			FListT<int> layer_id_with_bias;
			for (int il = 0 ; il < layer_inputs.GetSize() ; il++) {
				const NNLayer &layer = nn_document.GetLayer(il);
				FStringList &layer_input = layer_inputs.GetAt(il);
				for (int iin = 0 ; iin < layer_input.GetSize() ; iin++){
					const FString &input = layer_input.GetAt(iin);
					FString parameter_name = input;
					while (parameter_name.Pos("{")){
						const int p = parameter_name.Pos("{");
						const int p2 = parameter_name.Pos("}");
						parameter_name = parameter_name.SubString(1, p - 1) + parameter_name.SubString(p2 + 1, parameter_name.Length());
					}

					//Search variable
					const int variable_index = variable_names.IndexOf(input);
					if (variable_index >= 0){
						if (variable_types.GetAt(variable_index) == "Parameter"){
							FString parameter_scope = UpperPath(parameter_name, '/', 2);

							if (parameter_scope != layer.GetPropertyValue(FString("Name")) && parameter_scope != ""){
								nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("ParameterScope"), parameter_scope);
							}
							//Set property from parameter
							if (layer.GetType().Pos("ffine")){
								if (iin == layer.GetNumSideConnector(0)){
									//Bias
									layer_id_with_bias.Add(layer.GetLayerID());
								}else if (iin == 1){
									//Weight
									FString out_shape = variable_shapes.GetAt(variable_index);
									out_shape.SplitBy(",");

									nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("OutShape"), out_shape);
								}
							}else if (layer.GetType().Pos("onvolution")){
								if (iin == layer.GetNumSideConnector(0)){
									//Bias
									layer_id_with_bias.Add(layer.GetLayerID());
								}else if (iin == 1){
									//Weight
									FString s = variable_shapes.GetAt(variable_index);
									FString out_maps = s.SplitBy(",");
									if (!layer.GetType().Pos("Depthwise")){
										const int group = layer.GetPropertyValue(FString("Group")).AsInt();
										if (layer.GetType().Pos("Deconvolution")){
											if (layer.GetPropertyValue(FString("ChannelLast")) != "True") {
												out_maps = s.SplitBy(",");//outmaps is in 2nd shape
											} else{
												FString tmp = s;
												s = FString("");
												while (true){
													FString s1 = tmp.SplitBy(",");
													if (s != "") {
														s += FString(",");
													}
													s += s1;
													if (tmp.Pos(",") < 1) {
														break;
													}
												}
												out_maps = tmp; // outmaps is 4th shape
											}
											if (out_maps  != "" && group > 0) {
												out_maps = FString(out_maps.AsInt() * group);
											}
										}else {
											if (layer.GetPropertyValue(FString("ChannelLast")) != "True") {
												s.SplitBy(",");//waste in_maps
											}else{
												FString tmp = s;
												s = FString("");
												while (true){
													FString s1 = tmp.SplitBy(",");
													if (s != "") {
														s += FString(",");
													}
													s += s1;
													if (tmp.Pos(",") < 1) {
														break;
													}
												}
											}
										}
										nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("OutMaps"), out_maps);
									}
									nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("KernelShape"), s);
								}
							}
							int external_parameter = true;
							if (iin > 0 && iin <= layer.GetNumSideConnector(0)){
								external_parameter = parameter_scope + FString("/") + layer.GetSideConnector(0, iin - 1).GetParameterScope() != parameter_name;
							}
							if (external_parameter) {
								//parameter layer or working memory layer
								FListT<NNLayer *> input_layers = nn_document.GetLayerByPropertyValue(FString("Name"), parameter_name, true);
								if (input_layers.GetSize() == 1){
									//input layer already exist
									nn_document.AddLink(input_layers.GetAt(0)->GetLayerID(), 0, layer.GetLayerID(), iin <= layer.GetNumSideConnector(0) ? iin : 0);
								}else {
									FString parameter_shape = variable_shapes.GetAt(variable_index);
									FString component_type;
									if (FString(parameter_shape).SplitBy(",") == "-1"){
										//Working memory if shape has batch size
										component_type = FString("WorkingMemory");
										parameter_shape.SplitBy(",");//remove batch size(-1)
									}else {
										if (iin == 0 ||
											(input.Pos(layer.GetPropertyValue(FString("Name"))) == 0 &&
											 input.Pos(layer.GetPropertyValue(FString("ParameterScope"))) == 0)){
											component_type = FString("Parameter");
										}else {
											//parameter layer is included in neural network layer (affine, convolution, etc...)
										}
									}
									int component_index = components->GetComponentIndexByType(component_type);
									if (component_index >= 0){
										NNLayer &input_layer = nn_document.AddLayer(component_index);
										input_layer.SetPropertyValue(FString("Name"), parameter_name);
										input_layer.SetPropertyValue(FString("Size"), parameter_shape);
										nn_document.AddLink(input_layer.GetLayerID(), 0, layer.GetLayerID(), iin <= layer.GetNumSideConnector(0) ? iin : 0);
									}else {
										//component is not found
									}
								}
							}
						}else {//if (variable_types.GetAt(variable_index) == "Buffer"){
							//search output
							int layer_index = -1;
							int layer_output_index = -1;
							for (int il2 = 0 ; il2 < layer_outputs.GetSize() ; il2++) {
								const int output_index = layer_outputs.GetAt(il2).IndexOf(input);
								if (output_index >= 0){
									layer_index = il2;
									layer_output_index = output_index;
									break;
								}
							}

							if (layer_index >= 0){
								//output found
								const NNLayer &input_layer = nn_document.GetLayer(layer_index);
								nn_document.AddLink(input_layer.GetLayerID(), layer_output_index, layer.GetLayerID(), iin <= layer.GetNumSideConnector(0) ? iin : 0);
							}else {
								//output not found (input layer)
								FListT<NNLayer *> input_layers = nn_document.GetLayerByPropertyValue(FString("Name"), input, true);
								if (input_layers.GetSize() == 1){
									//input layer already exist
									nn_document.AddLink(input_layers.GetAt(0)->GetLayerID(), 0, layer.GetLayerID(), iin <= layer.GetNumSideConnector(0) ? iin : 0);
								}else {
									int component_index = components->GetComponentIndexByType(FString("Input"));
									if (component_index >= 0){
										NNLayer &input_layer = nn_document.AddLayer(component_index);
										input_layer.SetPropertyValue(FString("Name"), input);
										FString size = variable_shapes.GetAt(variable_index);
										size.SplitBy(",");//remove batch size
										input_layer.SetPropertyValue(FString("Size"), size);
										nn_document.AddLink(input_layer.GetLayerID(), 0, layer.GetLayerID(), iin <= layer.GetNumSideConnector(0) ? iin : 0);
									}
								}
							}
						}
					}else {
						//Variable not found error

						error_str.Add(FString("Variable '") + input + FString("' is not found at function '") + layer.GetPropertyValue(FString("Name")) + FString("'."));
					}
				}
			}

			//Set with bias, repeat times, size of delay layer
			for (int il = 0 ; il < layer_inputs.GetSize() ; il++) {
				const NNLayer &layer = nn_document.GetLayer(il);
				if (layer.GetType() == "RepeatStart"){
					const NNLayerProperty *property = layer.GetPropertyByName(FString("Times"));
					if (property){
						const int index = repeat_ids.IndexOf(layer.GetPropertyValue(FString("Name")));
						if (index >= 0){
							nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("Times"), FString(repeat_times.GetAt(index)));
						}
					}
				}
				const NNLayerProperty *property = layer.GetPropertyByName(FString("WithBias"));
				if (property && layer_id_with_bias.IndexOf(layer.GetLayerID()) < 0){
					nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("WithBias"), FString("False"));
				}
				if (layer.GetType() == "Delay"){
					//enum links connected to a initial value pin
					FListT<int> link_ids = nn_document.EnumLinkIDByDestLayerID(layer.GetLayerID(), 1);
					if (link_ids.GetSize() == 1){
						const NNLink *link = nn_document.GetLinkByID(link_ids.GetAt(0));
						if (link){
							const NNLayer *source_layer = nn_document.GetLayerByID(link->GetSourceLayerID());
							if (source_layer){
								const NNLayerProperty *property_size = source_layer->GetPropertyByName(FString("Size"));
								if (property_size){
									//Set size property of delay layer with input size to the initial value pin
									nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("Size"), property_size->GetValue());
								}
							}
						}
					}
				}
			}

			//Finalize network
			nn_document.ArrangeLayers(true);
			nn_document.RefreshDisplayValue();//to set parameter_scope

			//Error check and restore properties
			for (int il = 0 ; il < nn_document.GetNumLayer() ; il++){
				const NNLayer &layer = nn_document.GetLayer(il);
				for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++){
					const NNLayerProperty &property = layer.GetProperty(ip);
					if (property.GetError()){
						const int index = property_layers_and_names.IndexOf(layer.GetPropertyValue(FString("Name")) + FString("->") + property.GetName());
						if (index >= 0){
							nn_document.SetLayerPropertyValue(layer.GetLayerID(), property.GetName(), property_values_org.GetAt(index));
						}
					}
				}
			}
			nn_document.RefreshDisplayValue();

			//Set property value according to parameter size
			for (int il = 0 ; il < nn_document.GetNumLayer() ; il++) {
				const NNLayer &layer = nn_document.GetLayer(il);
				for (int iin = 1 ; iin <= layer.GetNumSideConnector(0) ; iin++){
					FListT<int> link_ids = nn_document.EnumLinkIDByDestLayerID(layer.GetLayerID(), iin);
					if (link_ids.GetSize() == 1){
						const NNLink *link = nn_document.GetLinkByID(link_ids.GetAt(0));
						if (link){
							const NNLayer *source_layer = nn_document.GetLayerByID(link->GetSourceLayerID());
							if (source_layer){
								FString out_shape = source_layer->GetDisplayPropertyValue(FString("Output"));
								//Set property from parameter
								if (layer.GetType().Pos("ffine")){
									if (iin == 1){
										//Weight
										out_shape.SplitBy(",");

										nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("OutShape"), out_shape);
									}else if (iin == layer.GetNumSideConnector(0)){
										//Bias
										layer_id_with_bias.Add(layer.GetLayerID());
									}
								}else if (layer.GetType().Pos("onvolution")){
									if (iin == 1){
										//Weight
										FString out_maps = out_shape.SplitBy(",");
										if (!layer.GetType().Pos("Depthwise")){
											const int group = layer.GetPropertyValue(FString("Group")).AsInt();
											if (layer.GetType().Pos("Deconvolution")) {
												if (layer.GetPropertyValue(FString("ChannelLast")) != "True") {
													out_maps = out_shape.SplitBy(",");//outmaps is in 2nd shape
												} else{
													FString tmp = out_shape;
													out_shape = FString("");
													while (true){
														FString s1 = tmp.SplitBy(",");
														if (out_shape != "") {
															out_shape += FString(",");
														}
														out_shape += s1;
														if (tmp.Pos(",") < 1) {
															break;
														}
													}
													out_maps = tmp; // outmaps is 4th shape
												}
												if (out_maps  != "" && group > 0) {
													out_maps = FString(out_maps.AsInt() * group);
												}
											} else {
												if (layer.GetPropertyValue(FString("ChannelLast")) != "True") {
													out_shape.SplitBy(",");//remove in_maps
												}else{
													FString tmp = out_shape;
													out_shape = FString("");
													while (true){
														FString s1 = tmp.SplitBy(",");
														if (out_shape != "") {
															out_shape += FString(",");
														}
														out_shape += s1;
														if (tmp.Pos(",") < 1) {
															break;
														}
													}
												}
											}
											nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("OutMaps"), out_maps);
										}
										nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString("KernelShape"), out_shape);
									}else if (iin == layer.GetNumSideConnector(0)){
										//Bias
										layer_id_with_bias.Add(layer.GetLayerID());
									}
								}
							}
						}
					}
				}
			}
		}else if (chunk_name == "training_config"){
			FStringList keys, values;
			DecodeDict(chunks.GetAt(ic), keys, values);

			const FStringList dataset_max_epoch = GetProperty(keys, values, FString("max_epoch"));
			if (dataset_max_epoch.GetSize()){
				nn_document.SetMaxEpoch(dataset_max_epoch.GetAt(0).AsInt());
			}
			const FStringList dataset_save_best = GetProperty(keys, values, FString("save_best"));
			if (dataset_save_best.GetSize()){
				nn_document.SetSaveBest(dataset_save_best.GetAt(0).AsInt());
			}

		}else if (chunk_name == "dataset"){
			dataset_num++;
			if (dataset_num > nn_document.GetNumDataset()) {
				nn_document.AddDataset();
			}

			FStringList keys, values;
			DecodeDict(chunks.GetAt(ic), keys, values);

			NNDataset &dataset = nn_document.GetDataset_(dataset_num - 1);
			const FStringList dataset_name = GetProperty(keys, values, FString("name"));
			const FStringList dataset_uri = GetProperty(keys, values, FString("uri"));
			const FStringList dataset_shuffle = GetProperty(keys, values, FString("shuffle"));
			const FStringList dataset_overwrite_cache = GetProperty(keys, values, FString("overwrite_cache"));
			const FStringList dataset_no_image_normalization = GetProperty(keys, values, FString("no_image_normalization"));

			dataset.SetName(dataset_name.GetSize() ? dataset_name.GetAt(0) : dataset.GetName());
			FString dataset_file_name = dataset_uri.GetSize() ? dataset_uri.GetAt(0) : FString("");
			dataset.LoadFromFile(dataset_file_name.PChar());
			dataset.SetShuffle(dataset_shuffle.GetSize() ? dataset_shuffle.GetAt(0).ToLower() == "true" : dataset.GetShuffle());
			dataset.SetEnableCache(dataset_overwrite_cache.GetSize() ? dataset_overwrite_cache.GetAt(0).ToLower() == "false" : dataset.GetEnableCache());
			dataset.SetImageNormalization(dataset_no_image_normalization.GetSize() ? dataset_no_image_normalization.GetAt(0).ToLower() == "false" : dataset.GetImageNormalization());
		}else if (chunk_name == "optimizer"){
			optimizer_num++;
			if (optimizer_num > nn_document.GetNumOptimizer()) {
				nn_document.AddOptimizer();
			}

			FStringList optimizer_names;
			FListP<FStringList> optimizer_vars;
			SplitStringListToChunk(chunks.GetAt(ic), optimizer_names, optimizer_vars);
			FStringList keys, values;
			DecodeDict(optimizer_names, keys, values);

			NNOptimizer &optimizer = nn_document.GetOptimizer(optimizer_num - 1);
			const FStringList optimizer_name = GetProperty(keys, values, FString("name"));
			const FStringList optimizer_update_interval = GetProperty(keys, values, FString("update_interval"));
			const FStringList optimizer_network_name = GetProperty(keys, values, FString("network_name"));
			const FStringList optimizer_dataset_names = GetProperty(keys, values, FString("dataset_name"));

			optimizer.SetOptimizerName(optimizer_name.GetSize() ? optimizer_name.GetAt(0) : optimizer.GetOptimizerName());
			optimizer.SetUpdateInterval(optimizer_update_interval.GetSize() ? optimizer_update_interval.GetAt(0).AsInt() : optimizer.GetUpdateInterval());
			const int structure_index = optimizer_network_name.GetSize() > 0 ? nn_document.GetStructureIndexByName(optimizer_network_name.GetAt(0)) : -1;
			optimizer.SetNetworkName(optimizer_network_name.GetSize() ? optimizer_network_name.GetAt(0) : optimizer.GetNetworkName());
			optimizer.SetDatasetNames(optimizer_dataset_names.GetSize() ? optimizer_dataset_names : optimizer.GetDatasetNames());

			//solver
			const FStringList solver_weight_decay = GetProperty(keys, values, FString("solver.weight_decay"));
			const FStringList solver_lr_decay = GetProperty(keys, values, FString("solver.lr_decay"));
			const FStringList solver_lr_decay_interval = GetProperty(keys, values, FString("solver.lr_decay_interval"));
			optimizer.SetWeightDecay(solver_weight_decay.GetSize() ? solver_weight_decay.GetAt(0).AsFloat() : optimizer.GetWeightDecay());
			optimizer.SetLearningRateMultiplier(solver_lr_decay.GetSize() ? solver_lr_decay.GetAt(0).AsFloat() : optimizer.GetLearningRateMultiplier());
			optimizer.SetLearningRateUpdateInterval(solver_lr_decay_interval.GetSize() ? solver_lr_decay_interval.GetAt(0).AsInt() : optimizer.GetLearningRateUpdateInterval());

			for (int is = 0 ; is < optimizer_names.GetSize() ; is++){
				FStringList keys, values;
				DecodeDict(optimizer_vars.GetAt(is), keys, values);

				if (optimizer_names.GetAt(is) == "solver"){
					const FStringList solver_type = GetProperty(keys, values, FString("type"));
					if (solver_type.GetSize()){
						optimizer.SetSolver(solver_type.GetAt(0), true);
						for (int ip = 0 ; ip < optimizer.GetSolverParameterNum() ; ip++){
							FString key = optimizer.GetSolverArgumentName() + FString(".") + optimizer.GetSolverParameterArgumentName(ip);
							const FStringList solver_param = GetProperty(keys, values, key);
							if (solver_param.GetSize()) {
								optimizer.SetSolverParameter(ip, solver_param.GetAt(0).AsFloat());
							}
						}
					}

				}else if (optimizer_names.GetAt(is) == "data_variable"){
					const FStringList variable_name = GetProperty(keys, values, FString("variable_name"));
					const FStringList variable_data_name = GetProperty(keys, values, FString("data_name"));
					if (structure_index >= 0 && variable_name.GetSize() && variable_data_name.GetSize()){
						NNStructure &structure = nn_document.GetStructure_(structure_index);
						const int layer_id = structure.GetLayerIDByName(variable_name.GetAt(0));
						structure.SetLayerPropertyValue(layer_id, FString("Dataset"), variable_data_name.GetAt(0));
					}

				}else if (optimizer_names.GetAt(is) == "generator_variable"){
					const FStringList variable_name = GetProperty(keys, values, FString("variable_name"));
					const FStringList generator_type = GetProperty(keys, values, FString("type"));
					const FStringList generator_multiplier = GetProperty(keys, values, FString("multiplier"));
					if (structure_index >= 0 && variable_name.GetSize() && (generator_type.GetSize() || generator_multiplier.GetSize()) ){
						NNStructure &structure = nn_document.GetStructure_(structure_index);
						const int layer_id = structure.GetLayerIDByName(variable_name.GetAt(0));
						if (generator_type.GetSize()){
							structure.SetLayerPropertyValue(layer_id, FString("Generator"), generator_type.GetAt(0));
						}
						if (generator_multiplier.GetSize()){
							structure.SetLayerPropertyValue(layer_id, FString("GeneratorMultiplier"), generator_multiplier.GetAt(0));
						}
					}
				}else if (optimizer_names.GetAt(is) == "parameter_variable"){
					const FStringList variable_name = GetProperty(keys, values, FString("variable_name"));
					const FStringList learning_rate_multiplier = GetProperty(keys, values, FString("learning_rate_multiplier"));
					if (structure_index >= 0 && variable_name.GetSize() && learning_rate_multiplier.GetSize()){
						NNStructure &structure = nn_document.GetStructure_(structure_index);
						const FString parameter_scope = UpperPath(variable_name.GetAt(0), '/', 2);
						FListT<NNLayer *> layers = structure.GetLayerByDisplayPropertyValue(FString("ParameterScope"), parameter_scope, false);
						for (int il = 0 ; il < layers.GetSize() ; il++){
                            const NNLayer *layer = layers.GetAt(il);
							for (int ic = 0 ; ic < layer->GetNumSideConnector(0) ; ic++){
								if (parameter_scope + FString("/") + layer->GetSideConnector(0, ic).GetParameterScope() == variable_name.GetAt(0)){
									structure.SetLayerPropertyValue(layer->GetLayerID(), layer->GetSideConnector(0, ic).GetName() + FString(".LRateMultiplier"), learning_rate_multiplier.GetAt(0));
								}
							}
						}
					}
				}
			}

		}else if (chunk_name == "monitor" && monitor_num < 2){
			monitor_num++;

			FStringList monitor_names;
			FListP<FStringList> monitor_vars;
			SplitStringListToChunk(chunks.GetAt(ic), monitor_names, monitor_vars);
			FStringList keys, values;
			DecodeDict(monitor_names, keys, values);

			NNMonitor &monitor = nn_document.GetMonitor(monitor_num - 1);
			const FStringList monitor_network_name = GetProperty(keys, values, FString("network_name"));
			const FStringList monitor_dataset_names = GetProperty(keys, values, FString("dataset_name"));

			const int structure_index = monitor_network_name.GetSize() > 0 ? nn_document.GetStructureIndexByName(monitor_network_name.GetAt(0)) : -1;
			monitor.SetNetworkName(monitor_network_name.GetSize() ? monitor_network_name.GetAt(0) : monitor.GetNetworkName());
			monitor.SetDatasetNames(monitor_dataset_names.GetSize() ? monitor_dataset_names : monitor.GetDatasetNames());

			for (int is = 0 ; is < monitor_names.GetSize() ; is++){
				FStringList keys, values;
				DecodeDict(monitor_vars.GetAt(is), keys, values);

				if (monitor_names.GetAt(is) == "data_variable"){
					const FStringList variable_name = GetProperty(keys, values, FString("variable_name"));
					const FStringList variable_data_name = GetProperty(keys, values, FString("data_name"));
					if (structure_index >= 0 && variable_name.GetSize() && variable_data_name.GetSize()){
						NNStructure &structure = nn_document.GetStructure_(structure_index);
						const int layer_id = structure.GetLayerIDByName(variable_name.GetAt(0));
						structure.SetLayerPropertyValue(layer_id, FString("Dataset"), variable_data_name.GetAt(0));
					}

				}else if (monitor_names.GetAt(is) == "generator_variable"){
					const FStringList variable_name = GetProperty(keys, values, FString("variable_name"));
					const FStringList generator_type = GetProperty(keys, values, FString("type"));
					const FStringList generator_multiplier = GetProperty(keys, values, FString("multiplier"));
					if (structure_index >= 0 && variable_name.GetSize() && (generator_type.GetSize() || generator_multiplier.GetSize()) ){
						NNStructure &structure = nn_document.GetStructure_(structure_index);
						const int layer_id = structure.GetLayerIDByName(variable_name.GetAt(0));
						if (generator_type.GetSize()){
							structure.SetLayerPropertyValue(layer_id, FString("Generator"), generator_type.GetAt(0));
						}
						if (generator_multiplier.GetSize()){
							structure.SetLayerPropertyValue(layer_id, FString("GeneratorMultiplier"), generator_multiplier.GetAt(0));
						}
					}
				}
			}

		}else if (chunk_name == "executor"){
			executor_num++;
			if (executor_num > nn_document.GetNumExecutor()) {
				nn_document.AddExecutor();
			}

			FStringList executor_names;
			FListP<FStringList> executor_vars;
			SplitStringListToChunk(chunks.GetAt(ic), executor_names, executor_vars);
			FStringList keys, values;
			DecodeDict(executor_names, keys, values);

			NNExecutor &executor = nn_document.GetExecutor(executor_num - 1);
			const FStringList executor_name = GetProperty(keys, values, FString("name"));
			const FStringList executor_num_evaluations = GetProperty(keys, values, FString("num_evaluations"));
			const FStringList executor_repeat_evaluation_type = GetProperty(keys, values, FString("repeat_evaluation_type"));
			const FStringList executor_need_back_propagation = GetProperty(keys, values, FString("need_back_propagation"));
			const FStringList executor_network_name = GetProperty(keys, values, FString("network_name"));
			const FStringList executor_dataset_names = GetProperty(keys, values, FString("dataset_name"));

			executor.SetExecutorName(executor_name.GetSize() ? executor_name.GetAt(0) : executor.GetExecutorName());
			executor.SetNumEvaluations(executor_num_evaluations.GetSize() ? executor_num_evaluations.GetAt(0).AsInt() : executor.GetNumEvaluations());
			executor.SetRepeatEvaluationType(0);//default = mean
			if (executor_repeat_evaluation_type.GetSize() && executor_repeat_evaluation_type.GetAt(0) == "last") {
				executor.SetRepeatEvaluationType(1);
			}else if (executor_repeat_evaluation_type.GetSize() && executor_repeat_evaluation_type.GetAt(0) == "std") {
				executor.SetRepeatEvaluationType(2);
			}
			executor.SetNeedBackPropagation(executor_need_back_propagation.GetSize() ? executor_need_back_propagation.GetAt(0).AsInt() : executor.GetNeedBackPropagation());
			const int structure_index = executor_network_name.GetSize() > 0 ? nn_document.GetStructureIndexByName(executor_network_name.GetAt(0)) : -1;
			executor.SetNetworkName(executor_network_name.GetSize() ? executor_network_name.GetAt(0) : executor.GetNetworkName());
			executor.SetDatasetNames(executor_dataset_names.GetSize() ? executor_dataset_names : executor.GetDatasetNames());

			for (int is = 0 ; is < executor_names.GetSize() ; is++){
				FStringList keys, values;
				DecodeDict(executor_vars.GetAt(is), keys, values);

				if (executor_names.GetAt(is) == "data_variable"){
					const FStringList variable_name = GetProperty(keys, values, FString("variable_name"));
					const FStringList variable_data_name = GetProperty(keys, values, FString("data_name"));
					if (structure_index >= 0 && variable_name.GetSize() && variable_data_name.GetSize()){
						NNStructure &structure = nn_document.GetStructure_(structure_index);
						const int layer_id = structure.GetLayerIDByName(variable_name.GetAt(0));
						structure.SetLayerPropertyValue(layer_id, FString("Dataset"), variable_data_name.GetAt(0));
					}

				}else if (executor_names.GetAt(is) == "generator_variable"){
					const FStringList variable_name = GetProperty(keys, values, FString("variable_name"));
					const FStringList generator_type = GetProperty(keys, values, FString("type"));
					const FStringList generator_multiplier = GetProperty(keys, values, FString("multiplier"));
					if (structure_index >= 0 && variable_name.GetSize() && (generator_type.GetSize() || generator_multiplier.GetSize()) ){
						NNStructure &structure = nn_document.GetStructure_(structure_index);
						const int layer_id = structure.GetLayerIDByName(variable_name.GetAt(0));
						if (generator_type.GetSize()){
							structure.SetLayerPropertyValue(layer_id, FString("Generator"), generator_type.GetAt(0));
						}
						if (generator_multiplier.GetSize()){
							structure.SetLayerPropertyValue(layer_id, FString("GeneratorMultiplier"), generator_multiplier.GetAt(0));
						}
					}
				}
			}
		}
	}

	bool finalized = false;
	while (!finalized){
		//Finalize
		finalized = true;
		for (int i = nn_document.GetNumStructure() - 1 ; i >= 0 ; i--) {
			nn_document.SetActiveStructure(i);
			nn_document.RefreshDisplayValue();
			nn_document.RefreshStatistics();

            //Special processing
			for (int il = 0 ; il < nn_document.GetNumLayer() ; il++){
				const NNLayer &layer = nn_document.GetLayer(il);
				if (layer.GetType() == "Slice"){
					//Delete batch dim from Start, Stop and Step properties
					const NNLayerProperty &input = layer.GetProperty(layer.GetInputPropertyIndex());
					if (input.GetIntArray().GetSize()){
						static const char *properies[3] = {
						"Start",
						"Stop",
						"Step",
						};
						for (int ip = 0 ; ip < 3 ; ip++){
							const NNLayerProperty *property = layer.GetPropertyByName(properies[ip]);
							if (property){
								if (property->GetIntArray().GetSize() > input.GetIntArray().GetSize()){
									FString value = property->GetValue();
									value.SplitBy(",");//Waste first dim
									nn_document.SetLayerPropertyValue(layer.GetLayerID(), FString(properies[ip]), value);
									finalized = false;
								}
							}
						}
					}
				}
			}
		}


	}

	return !error_str.GetSize();
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
