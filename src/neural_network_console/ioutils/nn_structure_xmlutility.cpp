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

#include "nn_structure_xmlutility.h"
#include <Xml.XmlDoc.hpp>
namespace sdeep_console{
//---------------------------------------------------------------------------
int NNStructureToXML(const NNStructure &structure, TXMLDocument *xml, AnsiString output_path)
{
    xml->DOMVendor = GetDOMVendor("ADOM XML v4");
	xml->Active = true;

	xml->Encoding = "UTF-8";
	IXMLNode *ffnn = xml->AddChild("feedForwardNeuralNet");
	IXMLNode *version = ffnn->AddChild("version");
	IXMLNode *layers = ffnn->AddChild("layers");
	IXMLNode *nodes = ffnn->AddChild("nodes");
	IXMLNode *inputs = ffnn->AddChild("inputs");
	IXMLNode *outputs = ffnn->AddChild("outputs");

    //Version
	version->Text = "1.5";

	//Layers
	for (int il = 0 ; il < structure.GetNumLayer() ; il++){
		const NNLayer &layer = structure.GetLayer(il);
		const NNLayerComponent *component = structure.GetNNLayerComponents()->GetComponentByType(layer.GetType());
		if (component &&
			layer.GetType() != "Input" && layer.GetType() != "Output" && layer.GetType() != "Loss" && layer.GetType() != "VertexLoss") {
			IXMLNode *_layer = layers->AddChild("layer");
			_layer->SetAttribute("type", layer.GetType().PChar());

			const FListT<int> &input_links = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
			IXMLNode *layer_inputs = _layer->AddChild("inputs");
			for (int ili = 0 ; ili < input_links.GetSize() ; ili++) {
				const NNLink *link = structure.GetLinkByID(input_links.GetAt(ili));
				if (link) {
					IXMLNode *node_id = layer_inputs->AddChild("nodeId");
					node_id->Text = UnicodeString("n") + IntToStr(link->GetSourceLayerID());
				}
			}

			//const FListT<int> &output_links = structure.EnumLinkIDBySourceLayerID(layer.GetLayerID());
			IXMLNode *layer_outputs = _layer->AddChild("outputs");
			IXMLNode *node_id = layer_outputs->AddChild("nodeId");
			node_id->Text = UnicodeString("n") + IntToStr(layer.GetLayerID());

			IXMLNode *layer_args = _layer->AddChild("args");
			for (int ip = 0 ; ip < component->GetNumProperty() ; ip++) {
				const NNLayerComponentProperty &component_property = component->GetProperty(ip);
				const NNLayerProperty *property = layer.GetPropertyByName(component_property.GetName());
				if (property && component_property.GetArgumentName() != "") {
					if (property->GetDisplayValue() != "") {
						IXMLNode *arg = layer_args->AddChild("arg");
						arg->SetAttribute("name", component_property.GetArgumentName().PChar());
						switch (property->GetType()) {
						case NNLayerPropertyType::PIntArrays:
							//No output
							break;
						case NNLayerPropertyType::PIntArray:
						case NNLayerPropertyType::UIntArray:
							{
								const FListT<__int64> &intarray = property->GetIntArray();
								for (int id = intarray.GetSize() - 1 ; id >= 0 ; id--) {
									IXMLNode *d = arg->AddChild("d");
									d->Text = IntToStr(intarray.GetAt(id));
								}
							}
							break;
						case NNLayerPropertyType::File:
							{
								FString uri = property->GetDisplayValue();
								const int inv_kernel = uri.Pos("*inv_kernel") && output_path != "";
								const int refresh_affine_weight_shape = uri.Pos("*refresh_affine_weight_shape") && output_path != "";
								const int clear_shape = uri.Pos("*clear_shape") && output_path != "";
								const int sqrt_weight = uri.Pos("*sqrt") && output_path != "";
								const int sqr_weight = uri.Pos("*sqr") && output_path != "";
								NNWeight weight;
								if (inv_kernel || refresh_affine_weight_shape || clear_shape || sqrt_weight || sqr_weight) {
									uri = uri.SplitBy("*");
									AnsiString tmp_dir = output_path + "initial_parameters";
									if (!DirectoryExists(tmp_dir)) {
										ForceDirectories(tmp_dir);
									}
									weight.Load(uri.PChar());
									uri = FString(AnsiString(tmp_dir + AnsiString("\\") + ExtractFileName(uri.PChar())).c_str());
								}
								if (inv_kernel) {
									int kernel_size = 1;
									const NNLayerProperty *property_kernel = layer.GetPropertyByName("KernelShape");
									if (property_kernel) {
										kernel_size = property_kernel->GetIntArrayProduct();
									}
									weight.InverseKernel(kernel_size);
									weight.Save(uri.PChar());
								}else if (refresh_affine_weight_shape) {
									FListT<long long> shape;
									const NNLayerProperty *property_input = layer.GetPropertyByName("Input");
									if (property_input) {
										for (int i = property_input->GetIntArray().GetSize() - 1; i >= 0; i--) {
											shape.Add(property_input->GetIntArray().GetAt(i));
										}
									}
									const NNLayerProperty *property_output = layer.GetPropertyByName("Output");
									if (property_output) {
										for (int i = property_output->GetIntArray().GetSize() - 1; i >= 0; i--) {
											shape.Add(property_output->GetIntArray().GetAt(i));
										}
									}
									weight.SetShape(shape);
									weight.Save(uri.PChar());
								}else if (clear_shape) {
									weight.SetShape(FListT<long long>());
									weight.Save(uri.PChar());
								}else if (sqrt_weight) {
									weight.Sqrt();
									weight.Save(uri.PChar());
								}else if (sqr_weight) {
									weight.Sqr();
									weight.Save(uri.PChar());
								}
								arg->SetAttribute("uri", uri.PChar());
								arg->SetAttribute("fileFormat", "txt");
								arg->SetAttribute("dtype", "xsd:float");
							}
							break;
						default:
							arg->SetAttribute("value", property->GetDisplayValue().PChar());
							break;
						}
					}
				}
			}
		}
	}

	//Nodes
	for (int il = 0 ; il < structure.GetNumLayer() ; il++){
		const NNLayer &layer = structure.GetLayer(il);
		const NNLayerProperty *property = layer.GetPropertyByName("Output");
		if (property) {
			IXMLNode *node = nodes->AddChild("node");
			node->SetAttribute("nodeId", UnicodeString("n") + IntToStr(layer.GetLayerID()));

			//Node name (= Dataset name)
			FString node_name;
			const NNLayerProperty *dataset_property = layer.GetPropertyByName("Dataset");
			if (dataset_property) {
				//Dataset for the Input Layer
				node_name = dataset_property->GetValue();
			}else {
				FListT<int> links = structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0);
				for (int ili = 0 ; ili < links.GetSize() ; ili++) {
					const NNLink *link = structure.GetLinkByID(links.GetAt(ili));
					if (link) {
						const NNLayer *dest_layer = structure.GetLayerByID(link->GetDestLayerID());
						if (dest_layer) {
							if (dest_layer->GetType() == "Output" || dest_layer->GetType() == "Loss" ) {
								//Dataset for the Output Layer
								const NNLayerProperty *output_dataset_property = dest_layer->GetPropertyByName("Dataset");
								node_name = output_dataset_property->GetValue();
							}
						}
					}
				}
			}
			if (node_name != "") {
				node->SetAttribute("name", node_name.PChar());
			}

			node->SetAttribute("dtype", "xsd:float");
			IXMLNode *shape = node->AddChild("shape");
			const FListT<__int64> &intarray = property->GetIntArray();
			for (int id = intarray.GetSize() - 1 ; id >= 0 ; id--) {
				IXMLNode *d = shape->AddChild("d");
				d->Text = IntToStr(intarray.GetAt(id));
			}
		}
	}

	//Inputs
	for (int il = 0 ; il < structure.GetNumLayer() ; il++){
		const NNLayer &layer = structure.GetLayer(il);
		if (layer.GetType() == "Input" || layer.GetType() == "Delay") {
			IXMLNode *node_id = inputs->AddChild("nodeId");
			node_id->Text = UnicodeString("n") + IntToStr(layer.GetLayerID());
		}
	}

	//Outputs
	for (int il = 0 ; il < structure.GetNumLayer() ; il++){
		const NNLayer &layer = structure.GetLayer(il);
		if (layer.GetType() == "Output") {
			const FListT<int> &links = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), 0);
			if (links.GetSize() == 1) {
				const NNLink *link = structure.GetLinkByID(links.GetAt(0));
				if (link) {
					IXMLNode *node_id = outputs->AddChild("nodeId");
					node_id->Text = UnicodeString("n") + IntToStr(link->GetSourceLayerID());
				}
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------------
int NNStructureToXMLString(TComponent *AOwner, const NNStructure &structure, AnsiString &xml_string)
{
	TXMLDocument *xml = new TXMLDocument(AOwner);
	if (!xml) {
		return false;
	}
	const int result = NNStructureToXML(structure, xml, "");
	xml_string = xml->XML->Text;

	delete xml;
	return result;
}
//---------------------------------------------------------------------------
int NNStructureToXMLFile(TComponent *AOwner, const NNStructure &structure, UnicodeString file_name)
{
	TXMLDocument *xml = new TXMLDocument(AOwner);
	if (!xml) {
		return false;
	}
	const int result = NNStructureToXML(structure, xml, ExtractFilePath(file_name));
	xml->SaveToFile(file_name);

	delete xml;
	return result;
}
//---------------------------------------------------------------------------
int XMLToNNStructure(TXMLDocument *xml, UnicodeString file_name, NNStructure &structure)
{
	structure.Clear();

	//xml->DOMVendor = DOMVendors->Find("ADOM XML v4");
	xml->Active = true;

	const NNLayerComponents *components = structure.GetNNLayerComponents();

	if (xml->ChildNodes->GetCount() < 2) {
		return false;
	}

	//Check format
	const UnicodeString xml_version = xml->ChildNodes->Get(0)->GetAttribute("version");
	if (xml_version != "1.0") {
		return false;
	}
	/*
	const UnicodeString xml_encoding = xml->ChildNodes->Get(0)->GetAttribute("encoding");
	if (xml_encoding != "UTF-8") {
		return false;
	}
	*/

	IXMLNode *ffnn = xml->ChildNodes->FindNode("feedForwardNeuralNet");
	if (!ffnn) {
		return false;
	}
	IXMLNode *layers = ffnn->ChildNodes->FindNode("layers");
	IXMLNode *nodes = ffnn->ChildNodes->FindNode("nodes");
	IXMLNode *inputs = ffnn->ChildNodes->FindNode("inputs");
	IXMLNode *outputs = ffnn->ChildNodes->FindNode("outputs");

	if (!layers || !nodes || !inputs || !outputs) {
		return false;
	}

	//Enum nodes
	FListT<int> node_ids;
	FStringList node_names;
	FStringList node_sizes;
	for (int in = 0 ; in < nodes->ChildNodes->Count ; in++) {
		IXMLNode *node = nodes->ChildNodes->Get(in);
		const AnsiString node_id_str = node->GetAttribute("nodeId");
		AnsiString node_name;
		if (node->HasAttribute("nodeName")){//for backup compatibility
			node_name = node->GetAttribute("nodeName");
		}
		if (node->HasAttribute("name")){
			node_name = node->GetAttribute("name");
		}
		const int node_id = StrToIntDef(node_id_str.SubString(2, node_id_str.Length() - 1), -1);
		if (node_id >= 0) {
			node_ids.Add(node_id);
			node_names.Add(FString(node_name.c_str()));
			IXMLNode *shape = node->ChildNodes->FindNode("shape");
			if (shape) {
				FString size_str;
				for (int id = 0 ; id < shape->ChildNodes->Count ; id++) {
					IXMLNode *d = shape->ChildNodes->Get(id);
					AnsiString d_str = d->Text;
					if (id > 0) {
						size_str += ",";
					}
					size_str += FString(d_str.c_str());
				}
				node_sizes.Add(size_str);
			}else {
				return false;
			}
		}else {
			return false;
		}
	}

	//Add Inputs
	for (int in = 0 ; in < inputs->ChildNodes->Count ; in++) {
		IXMLNode *input = inputs->ChildNodes->Get(in);
		const AnsiString node_id_str = input->Text;
		const int node_id = StrToIntDef(node_id_str.SubString(2, node_id_str.Length() - 1), -1);

		const int node_index = node_ids.IndexOf(node_id);
		if (node_index >= 0) {
			const int component_index = components->GetComponentIndexByType(FString("Input"));
			NNLayer &new_layer = structure.AddLayer(component_index);
			new_layer.SetLayerID(node_id);
			if (node_names.GetAt(node_index) != "") {
				new_layer.SetPropertyValue(FString("Dataset"), node_names.GetAt(node_index));
            }
			new_layer.SetPropertyValue(FString("Size"), node_sizes.GetAt(node_index));
		}else {
			return false;
		}
	}

	//Add Layers
	for (int il = 0 ; il < layers->ChildNodes->Count ; il++) {
		IXMLNode *layer = layers->ChildNodes->Get(il);
		const AnsiString name = layer->GetAttribute("type");

		IXMLNode *outputs = layer->ChildNodes->FindNode("outputs");
		if (!outputs){
			return false;
		}
		if (!outputs->ChildNodes->Count) {
			return false;
		}
		IXMLNode *output = outputs->ChildNodes->Get(0);
		const AnsiString node_id_str = output->Text;
		const int node_id = StrToIntDef(node_id_str.SubString(2, node_id_str.Length() - 1), -1);

		const int component_index = components->GetComponentIndexByType(FString(name.c_str()));
		const NNLayerComponent &component = components->GetAt(component_index);
		NNLayer &new_layer = structure.AddLayer(component_index);
		new_layer.SetLayerID(node_id);

		IXMLNode *args = layer->ChildNodes->FindNode("args");
		if (args) for (int ip = 0 ; ip < args->ChildNodes->Count ; ip++) {
			IXMLNode *arg = args->ChildNodes->Get(ip);
			if (arg) {
				OleVariant attribute = arg->GetAttribute("name");
				if (!attribute.IsNull()){
					AnsiString argment_name = attribute;
					const int component_property_index = component.GetPropertyIndexByArgumentName(FString(argment_name.c_str()));
					if (component_property_index >= 0 && component_property_index < component.GetNumProperty()) {
						const NNLayerComponentProperty &component_property = component.GetProperty(component_property_index);
						if (component_property.GetEditable()) {
							/*
							const int property_index = new_layer.GetPropertyIndexByName(component_property.GetName());
							if (property_index >= 0 && property_index < new_layer.GetNumProperty()) {
							}
							*/
							if (component_property.GetType() == NNLayerComponentPropertyType::PIntArray ||
								component_property.GetType() == NNLayerComponentPropertyType::UIntArray) {
								FString size_str;
								for (int id = 0 ; id < arg->ChildNodes->Count ; id++) {
									IXMLNode *d = arg->ChildNodes->Get(id);
									AnsiString d_str = d->Text;
									if (id > 0) {
										size_str += ",";
									}
									size_str += FString(d_str.c_str());
								}
								new_layer.SetPropertyValue(component_property.GetName(), size_str);
							}else {
								OleVariant attribute = arg->GetAttribute("value");
								if (!attribute.IsNull()){
									AnsiString argment_value = attribute;
									new_layer.SetPropertyValue(component_property.GetName(), FString(argment_value.c_str()));
								}else {
									//Todo : sdnx support
									attribute = arg->GetAttribute("uri");
									if (!attribute.IsNull()) {
										AnsiString argment_value = ExtractFilePath(file_name) + attribute;
										new_layer.SetPropertyValue(component_property.GetName(), FString(argment_value.c_str()));
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//Add Links
	for (int il = 0 ; il < layers->ChildNodes->Count ; il++) {
		IXMLNode *layer = layers->ChildNodes->Get(il);

		IXMLNode *inputs = layer->ChildNodes->FindNode("inputs");
		if (!inputs){
			return false;
		}
		IXMLNode *outputs = layer->ChildNodes->FindNode("outputs");
		IXMLNode *output = outputs->ChildNodes->Get(0);
		const AnsiString node_id_str = output->Text;
		const int node_id = StrToIntDef(node_id_str.SubString(2, node_id_str.Length() - 1), -1);

		for (int ii = 0 ; ii < inputs->ChildNodes->Count ; ii++) {
			IXMLNode *input = inputs->ChildNodes->Get(ii);
			const AnsiString input_node_id_str = input->Text;
			const int input_node_id = StrToIntDef(input_node_id_str.SubString(2, input_node_id_str.Length() - 1), -1);
			structure.AddLink(input_node_id, 0, node_id, 0);
		}
	}

	//Add Outputs
	structure.CheckNextID();
	for (int io = 0 ; io < outputs->ChildNodes->Count ; io++) {
		IXMLNode *output = outputs->ChildNodes->Get(io);
		const AnsiString node_id_str = output->Text;
		const int node_id = StrToIntDef(node_id_str.SubString(2, node_id_str.Length() - 1), -1);

		const int component_index = components->GetComponentIndexByType(FString("Output"));
		NNLayer &new_layer = structure.AddLayer(component_index);

		structure.AddLink(node_id, 0, new_layer.GetLayerID(), 0);

		//Let Dataset nodeName
		const int node_index = node_ids.IndexOf(node_id);
		if (node_index >= 0 && node_names.GetAt(node_index) != "") {
			new_layer.SetPropertyValue(FString("Dataset"), node_names.GetAt(node_index));
		}

		//Update loss function
		const NNLayer *previous_layer = structure.GetLayerByID(node_id);
		if (previous_layer) {
			const int loss_function_property_index = new_layer.GetPropertyIndexByName("LossFunction");
			if (loss_function_property_index >= 0) {
				if (previous_layer->GetType() == "Sigmoid") {
					structure.SetLayerPropertyValue(
						new_layer.GetLayerID(),
						loss_function_property_index,
						FString("BinaryCrossentropy"));
				}else if (previous_layer->GetType() == "Softmax") {
					structure.SetLayerPropertyValue(
						new_layer.GetLayerID(),
						loss_function_property_index,
						FString("CategoricalCrossentropy"));
				}//else default value
			}
		}
	}

	structure.ClearSelection();
	structure.ArrangeLayers();
	structure.RefreshDisplayValue();
	structure.RefreshStatistics();

	/*
	ShowMessage(
		UnicodeString("layers : ") + IntToStr(layers->ChildNodes->Count) +
		UnicodeString("\nnodes : ") + IntToStr(nodes->ChildNodes->Count) +
		UnicodeString("\ninputs : ") + IntToStr(inputs->ChildNodes->Count) +
		UnicodeString("\noutputs : ") + IntToStr(outputs->ChildNodes->Count));
	*/
	return true;
}
//---------------------------------------------------------------------------
int XMLFileToNNStructure(TComponent *AOwner, UnicodeString file_name, NNStructure &structure)
{
	TXMLDocument *xml = new TXMLDocument(AOwner);
	xml->LoadFromFile(file_name);

	const int result = XMLToNNStructure(xml, file_name, structure);

	delete xml;
	return result;
}
//---------------------------------------------------------------------------
int XMLStringToNNStructure(TComponent *AOwner, AnsiString xml_string, NNStructure &structure)
{
	TXMLDocument *xml = new TXMLDocument(AOwner);
	xml->LoadFromXML(xml_string);

	const int result = XMLToNNStructure(xml, "", structure);

	delete xml;
	return result;
}
//---------------------------------------------------------------------------
};
#pragma package(smart_init)
