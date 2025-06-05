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

#ifndef nn_structureH
#define nn_structureH

#include "flist.h"
#include "fstring.h"
#include "ffastini.h"
#include "nn_layer_component.h"
#include "nn_script.h"
#include "nn_structure_search_policy.h"

#ifdef _WIN32
#else
#include <stdint.h>
typedef int64_t __int64;
#endif

namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
const float NNLayerDefaultWidth = 240.0f;
const float NNLayerDefaultHeight = 48.0f;
const float NNLayerDefaultSpace = 24.0f;
const float NNLayerDefaultR = 12.0f;
const float NNLayerDefaultHSpace = 96.0f;
const float NNLayerDefaultVSpace = 24.0f;
const float NNLayerDefaultGrid = 24.0f;
const int NNStructureMaxPinNum = 12;
//---------------------------------------------------------------------------
//You also need to edit NNLayerComponentPropertyType in nn_layer_component.h
//and function NNLayerProperty::DecodePropertyType to add new type
enum class NNLayerPropertyType : int{
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
class NNLayer;
class NNStructure;
class NNStructures;
//---------------------------------------------------------------------------
class NNLayerProperty
{
public:
	NNLayerProperty() :
		editable_(true),
		important_(false),
		required_(true),
		search_parameter_(false),
		type_(NNLayerPropertyType::Int),
		error_(false),
		update_requested_(false),
		visible_(true),
		auto_export_(true),
		dynamic_(false) {};
	virtual ~NNLayerProperty() {};

	NNLayerProperty &operator=(const NNLayerProperty &source);
	int IsSame(const NNLayerProperty &source) const;
private:
	NNLayerProperty(const NNLayerProperty &source);
public:
	void SetName(const FString &name) {name_ = name;}
	const FString &GetName() const {return name_;}
	void SetShortName(const FString &short_name) {short_name_ = short_name.SubString(1, 12);}
	const FString &GetShortName() const {return short_name_;}
	void SetEditable(int editable) {editable_ = editable;}
	int GetEditable() const {return editable_;}
	void SetImportant(int important) {important_ = important;}
	int GetImportant() const {return important_;}
	void SetRequired(int required) {required_ = required;}
	int GetRequired() const {return required_;}
	void SetSearchParameter(int search_parameter) {search_parameter_ = search_parameter;}
	int GetSearchParameter() const {return search_parameter_;}
	void SetError(int error, const FString &error_message) {error_ = error; error_message_ = error_message;}
	int GetError() const {return error_;}
	const FString &GetErrorMessage() const {return error_message_;}
	int GetUpdateRequested() const {return update_requested_;}
	void SetType(NNLayerPropertyType type) {type_ = type;}
	NNLayerPropertyType GetType() const {return type_;}
	void SetValue(const FString &value) {
		value_ = value;
		ResetDisplayValue();
	}
	const FString &GetValue() const {return value_;}
	void SetDefaultValue(const FString &default_value) {default_value_ = default_value;}
	const FString &GetDefaultValue() const {return default_value_;}
	void SetDisplayValue(const FString &display_value);
	void ResetDisplayValue();
	const FString &GetDisplayValue() const {return display_value_;}
	const FListT<__int64> &GetIntArray() const {return int_array_;}
	__int64 GetIntArrayProduct() const;
	const FListT<double> &GetFloatArray() const {return float_array_;}

	void SetVisible(int visible) {visible_ = visible;}
	int GetVisible() const {return visible_;}
	void SetAutoExport(int auto_export) {auto_export_ = auto_export;}
	int GetAutoExport() const {return auto_export_;}
	void SetDynamic(int dynamic) {dynamic_ = dynamic;}
	int GetDynamic() const {return dynamic_;}
private:
	FString name_;
	FString short_name_;
	int editable_;
	int important_;
	int required_;
	int search_parameter_;
	NNLayerPropertyType type_;
	FString value_;
    FString default_value_;
	int visible_;
    int auto_export_;
	int dynamic_;
private:
	//Temporary
	FString display_value_;
	int error_;
	int update_requested_;
	FString error_message_;
	FListT<__int64> int_array_;
	FListT<double> float_array_;
public:
	static NNLayerPropertyType DecodePropertyType(const FString &type);
	static FString EncodePropertyType(NNLayerPropertyType layer_property_type);
};
//---------------------------------------------------------------------------
class NNLayerSideConnector
{
public:
	NNLayerSideConnector() :
		color_(0), kind_(NNLayerComponentInputKind::Data), dynamic_(0), order_(-1) {};
	virtual ~NNLayerSideConnector() {};

	NNLayerSideConnector &operator=(const NNLayerSideConnector &source);
private:
	NNLayerSideConnector(const NNLayerSideConnector &source);
public:
	void SetName(const FString &name) {name_ = name;}
	const FString &GetName() const {return name_;}
	void SetShortName(const FString &short_name) {short_name_ = short_name;}
	const FString &GetShortName() const {return short_name_;}
	void SetColor(int color) {color_ = color;}
	int GetColor() const {return color_;}
	void SetEnable(const FString &enable) {enable_ = enable;}
	const FString &GetEnable() const {return enable_;};
	void SetKind(NNLayerComponentInputKind kind) {kind_ = kind;}
	NNLayerComponentInputKind GetKind() const {return kind_;}
	void SetShape(const FString &shape){shape_ = shape;}
	const FString &GetShape() const {return shape_;}
	void SetParameterScope(const FString &parameter_scope) {parameter_scope_ = parameter_scope;}
	const FString &GetParameterScope() const {return parameter_scope_;}
	void SetDynamic(int dynamic) {dynamic_ = dynamic;}
	int GetDynamic() const {return dynamic_;}
public:
	void SetOrder(int order) {order_ = order;}
	int GetOrder() const {return order_;}
	void SetCurrentShape(const FString &current_shape){current_shape_ = current_shape;}
	const FString &GetCurrentShape() const {return current_shape_;}
private:
	FString name_;
	FString short_name_;
	int color_;
	FString enable_;
	NNLayerComponentInputKind kind_;
	FString shape_;
	FString parameter_scope_;
	int dynamic_;
private:
	//Temporary
	int order_;
    FString current_shape_;
};
//---------------------------------------------------------------------------
class NNError
{
public:
	NNError() {};
	virtual ~NNError() {};
	NNError &operator =(const NNError &source){
		string_ = source.string_;
        network_name_ = source.network_name_;
		layer_name_ = source.layer_name_;
		property_name_ = source.property_name_;
		return *this;
	}
private:
	NNError(const NNError &source);
public:
	void SetString(const FString &string) {string_ = string;}
	const FString &GetString() const {return string_;}

	void SetNetworkName(const FString &network_name) {network_name_ = network_name;}
	const FString &GetNetworkName() const {return network_name_;}
	void SetLayerName(const FString &layer_name) {layer_name_ = layer_name;}
	const FString &GetLayerName() const {return layer_name_;}
	void SetPropertyName(const FString &property_name) {property_name_ = property_name;}
	const FString &GetPropertyName() const {return property_name_;}
private:
	FString string_;
	FString network_name_;
	FString layer_name_;
	FString property_name_;
};
//---------------------------------------------------------------------------
class NNWeight
{
public:
	NNWeight() {};
	virtual ~NNWeight() {};
	NNWeight &operator = (const NNWeight &source){
		name_ = source.name_;
		shape_ = source.shape_;
		weight_ = source.weight_;
		contribution_ratio_ = source.contribution_ratio_;
		return *this;
	}
private:
	NNWeight(const NNWeight &source);
public:
	void Clear() {
		name_  = FString("");
		shape_.Clear();
		weight_.Clear();
		contribution_ratio_.Clear();
	}
	int Load(const char *file_name);
	int Save(const char *file_name) const;
	void RemoveSingleSizeDim();
	void TransposeRight();
	void CalcContributionRatio();
	void InverseKernel(int kernel_size = 0);
	void Sqrt();
	void Sqr();
	void SetShape(FListT<long long> shape) {shape_ = shape;}
public:
	const FListT<long long> &GetShape() const {return shape_;}
	const FListT<float> &GetWeight() const {return weight_;}
	const FListT<float> &GetContributionRatio() const {return contribution_ratio_;}
	void SetName(const FString &name) {name_ = name;}
	const FString &GetName() {return name_;}
private:
	FString name_;
	FListT<long long> shape_;
	FListT<float> weight_;
	FListT<float> contribution_ratio_;
};
//---------------------------------------------------------------------------
class NNLayer
{
public:
	NNLayer();
	virtual ~NNLayer() {ClearCache();}

	NNLayer &operator=(const NNLayer &source);
	int IsSame(const NNLayer &source, const FStringList *exception_property = NULL) const;
private:
	NNLayer(const NNLayer &source);
public:
	//Setter / Getter
	void SetLayerID(int layer_id) {layer_id_ = layer_id;}
	int GetLayerID() const {return layer_id_;}
	FString GetType() const {return component_->GetType();}
	FString GetName() const {return GetProperty(0).GetValue();}
	void SetComponent(const NNLayerComponent *component) {component_ = component;}
    const NNLayerComponent *GetComponent() const {return component_;}

	void SetPosition(const float *position) {memcpy(position_, position, sizeof(position_));}
	const float* GetPosition() const {return position_;}
	void SetPosition(int position_index, float value) {position_[position_index] = value;}
	float GetPosition(int position_index) const {return position_[position_index];}

	int GetNumProperty() const {return properties_.GetSize();}
	const NNLayerProperty &GetProperty(int index) const {return properties_.GetAt(index);}
	NNLayerProperty &GetProperty_(int index) {return properties_.GetAt(index);}
	int SetPropertyValue(int property_index, const FString &value, int force = false);
	int SetPropertyValue(const FString &property_name, const FString &value, int force = false);
	int SetPropertyValueFrom(const NNLayer &source);
	FString GetPropertyValue(const FString &name) const;
	FString GetDisplayPropertyValue(const FString &name) const;
	void ResetPropertyDisplayValue(int *update_required_property_num);
	void UpdatePropertyLocalDisplayValue(const NNStructure *current_network, const NNStructures *related_networks, int *update_required_property_num);
	double EvalMath(NNLayerPropertyType type, const NNStructure *current_network, const NNStructures *related_networks, const FString &math, int &success, FString &error) const;
	int EvalPropertyMacro(NNLayerPropertyType type, const NNStructure *current_network, const NNStructures *related_networks, const NNLayerProperty *property, const FString &macro, FString &value, FString &error);
	NNLayerProperty *AddProperty(int index, const FString &name, const FString &short_name, int editable, int important, int required, int search_parameter, NNLayerPropertyType type, const FString &value, int visible, int dynamic = false, int update_property_index=true);
	int GetPropertyIndexByName(const FString &name) const;
	int GetPropertyIndexByName(const char *name) const;
	const NNLayerProperty *GetPropertyByName(const FString &name) const;
	const NNLayerProperty *GetPropertyByName(const char *name) const;
	int GetPropertyIndexByValue(const FString &value) const;
	int SetPropertyDisplayValue(int property_index, const FString &value);
	void ClearProperty() {properties_.Clear(); UpdatePropertyIndex();}
	void ClearDynamicProperty();
	void FixProperties();

	int GetNumSideConnector(int output) const {return side_connectors_[output].GetSize();}
	const NNLayerSideConnector &GetSideConnector(int output, int side_connector_index) const {return side_connectors_[output].GetAt(side_connector_index);}
	const NNLayerSideConnector *GetSideConnectorByName(int output, const FString &name) const;
	int GetSideConnectorIndexByName(int output, const FString &name) const;
	void AddSideConnector(int output, int order, const FString &name, const FString &short_name, int color, const FString &enable, NNLayerComponentInputKind kind, const FString &shape, const FString &parameter_scope, int dynamic = false);
	void SetSideConnectorCurrentShape(int output, int side_connector_index, const FString &current_shape) {side_connectors_[output].GetAt(side_connector_index).SetCurrentShape(current_shape);};
	void ClearDynamicSideConnector();

	int GetIsParamPropertyIndex() const {return is_param_property_index_;}
	int GetInputPropertyIndex() const {return input_property_index_;}
	int GetOutputPropertyIndex() const {return output_property_index_;}
	int GetInPlacePropertyIndex() const {return in_place_property_index_;}

	const NNLayerProperty *GetAvailableInputProperty() const;
private:
	int layer_id_;
	const NNLayerComponent *component_;
	FListP<NNLayerProperty> properties_;
	FListP<NNLayerSideConnector> side_connectors_[2];//[0]:input, [1]:output

	float position_[4];//left, top, width, height

public:
	//Utility
	int IsContainedInRect(const float *rect) const;
	int GetPinEnable(int output, int pin_index) const;
	int GetPinVisible(int output, int pin_index) const;
public:
	//Temporary
	void SetSelected(int selected) {selected_ = selected;}
	int GetSelected() const {return selected_;}
	void SetDragged(int dragged) {dragged_ = dragged;}
	int GetDragged() const {return dragged_;}
	void SetCurrentPosition(const float *current_position) {memcpy(current_position_, current_position, sizeof(current_position_));}
	const float* GetCurrentPosition() const {return current_position_;}
	void SetCurrentPosition(int current_position_index, float value) {current_position_[current_position_index] = value;}
	float GetCurrentPosition(int current_position_index) const {return current_position_[current_position_index];}

	void SetInitialOffset(int initial_offset_index, float value) {initial_offset_[initial_offset_index] = value;}
	float GetInitialOffset(int initial_offset_index) const {return initial_offset_[initial_offset_index];}

	void ResetInputPinConnected();
	void SetInputPinConnected(int pin_index, int connected);
	int GetInputPinConnected(int pin_index) const;

	void CheckError();
	NNError &AddError() {return errors_.Add();}
	const FListP<NNError> &GetErrors() const {return errors_;}

	void SetRepeatIDs(const FStringList &repeat_ids) {repeat_ids_ = repeat_ids;}
	const FStringList &GetRepeatIDs() const {return repeat_ids_;}

	void UpdatePropertyIndex();
	void ClearCache();
public:
	//Operation
	void ClearParameterFiles();
private:
	int selected_;
	int dragged_;
	float current_position_[4];//current position for drag&drop
	float initial_offset_[2];//x, y offset for initial transition
	FListT<int> input_pin_connected_;//0x40000000 | link.GetSourceLayerID() if connected

	FListP<NNError> errors_;

	FStringList repeat_ids_;

	//Cache
	NNLayer *inner_structure_args_;
	NNStructure *inner_structure_;

	int is_param_property_index_;
	int input_property_index_;
	int output_property_index_;
	int in_place_property_index_;
};
//---------------------------------------------------------------------------
class NNLink
{
public:
	NNLink() : link_id_(0), source_layer_id_(0), source_pin_index_(0), dest_layer_id_(0), dest_pin_index_(0) {};
	virtual ~NNLink() {};

	NNLink &operator=(const NNLink &source) {
		link_id_ = source.link_id_;
		source_layer_id_ = source.source_layer_id_;
		source_pin_index_ = source.source_pin_index_;
		dest_layer_id_ = source.dest_layer_id_;
		dest_pin_index_ = source.dest_pin_index_;

		return *this;
	};
private:
	NNLink(const NNLink &source);
public:
	//Setter / Getter
	void SetLinkID(int link_id) {link_id_ = link_id;}
	int GetLinkID() const {return link_id_;}
	void SetSourceLayerID(int source_layer_id) {source_layer_id_ = source_layer_id;}
	int GetSourceLayerID() const {return source_layer_id_;}
	void SetSourcePinIndex(int source_pin_index) {source_pin_index_ = source_pin_index;}
	int GetSourcePinIndex() const {return source_pin_index_;}
	void SetDestLayerID(int dest_layer_id) {dest_layer_id_ = dest_layer_id;}
	int GetDestLayerID() const {return dest_layer_id_;}
	void SetDestPinIndex(int dest_pin_index) {dest_pin_index_ = dest_pin_index;}
	int GetDestPinIndex() const {return dest_pin_index_;}
private:
	int link_id_;

	int source_layer_id_;
	int source_pin_index_;
	int dest_layer_id_;
	int dest_pin_index_;
};
//---------------------------------------------------------------------------
class NNStatistics
{
public:
	NNStatistics() : sum_(0.0), max_(0.0){};
	virtual ~NNStatistics() {};

	NNStatistics &operator=(const NNStatistics &source) {
		label_ = source.label_;
		sum_ = source.sum_;
		max_ = source.max_;

		return *this;
	};
private:
	NNStatistics(const NNStatistics &source);
public:
	void Set(const FString &label, double sum, double max) {
		label_ = label;
		sum_ = sum;
		max_ = max;
	};
	const FString &GetLabel() const {return label_;}
	double GetSum() const {return sum_;}
	double GetMax() const {return max_;}
private:
	FString label_;
	double sum_;
	double max_;
};
//---------------------------------------------------------------------------
enum class NNRepeatType : int{
    None,
	Repeat,
	Recurrent
};
//---------------------------------------------------------------------------
class NNRepeatInfo
{
public:
	NNRepeatInfo() : type_(NNRepeatType::Repeat), num_repeat_(0) {} ;
	virtual ~NNRepeatInfo() {};

	NNRepeatInfo &operator=(const NNRepeatInfo &source) {
		id_ = source.id_;
        type_ = source.type_;
		num_repeat_ = source.num_repeat_;

		return *this;
	}
private:
	NNRepeatInfo(const NNRepeatInfo &source);
public:
	void Set(FString id, NNRepeatType type, int num_repeat) {
		id_ = id;
		type_ = type;
		num_repeat_ = num_repeat;
	}
	const FString &GetID() const {return id_;}
	int GetNumRepeat() const {return num_repeat_;}
	NNRepeatType GetType() const {return type_;}
private:
	FString id_;
	NNRepeatType type_;
	int num_repeat_;
};
//---------------------------------------------------------------------------
enum class NNStructureUseType_Obsolete : int{
	Training = 0,
	Validation = 1,
	Runtime = 2,
};
//---------------------------------------------------------------------------
class NNStructure
{
public:
	NNStructure();
	virtual ~NNStructure();

	NNStructure &operator=(const NNStructure &source);
	int IsSame(const NNStructure &source) const;
private:
	NNStructure(const NNStructure &source);
public:
	//Setup
	void SetNNLayerComponents(const NNLayerComponents *nn_layer_components) {
		nn_layer_components_ = nn_layer_components;
	}
	const NNLayerComponents *GetNNLayerComponents() const {return nn_layer_components_;}

	void SetNNStructureSearchPolicies(const NNStructureSearchPolicies *nn_structure_search_policies) {
		nn_structure_search_policies_ = nn_structure_search_policies;
	}
	const NNStructureSearchPolicies *GetNNStructureSearchPolicies() const {return nn_structure_search_policies_;}
public:
	void Clear();
	void ClearCache();
public:
	//Access
	const FString &GetName() const {return name_;}
	void SetName(const FString &name) {name_ = name; Update();}

	int GetNumLayer() const {return layers_.GetSize();}
	int GetNumSelectedLayer() const;
	const NNLayer &GetLayer(int layer_index) const {return layers_.GetAt(layer_index);}
	int GetLayerIndexByID(int layer_id) const;
	const NNLayer *GetLayerByID(int layer_id) const;
	FListT<NNLayer *> GetLayerByPropertyValue(const FString &property, const FString &value, int first_one) const;
	FListT<NNLayer *> GetLayerByDisplayPropertyValue(const FString &property, const FString &value, int first_one) const;
	FListT<NNLayer *> GetLayerByType(const FString &type, int first_one) const;
	int GetLayerIDByName(const FString &name) const;
	FListT<int> EnumInputLayerID() const;
	FListT<int> EnumTerminalLayerID() const;

	int GetNumLink() const {return links_.GetSize();}
	const NNLink &GetLink(int link_index) const {return links_.GetAt(link_index);}
	const NNLink *GetLinkByID(int link_id) const;
	int GetLinkIndexByDestLayerID(int dest_layer_id, int dest_pin_index) const;
	const NNLink *GetLinkByDestLayerID(int dest_layer_id, int dest_pin_index) const;
	int GetLinkIndex(int link_id) const;
	FListT<int> EnumLinkIDBySourceLayerID(int source_layer_id, int source_pin_index) const;
	FListT<int> EnumLinkIDByDestLayerID(int dest_layer_id, int dest_pin_index) const;
	int SearchLinkID(int source_layer_id, int source_pin_index, int dest_layer_id, int dest_pin_index) const;
	int GetPinEnable(int layer_id, int output, int pin_index) const;
	int GetPinVisible(int layer_id, int output, int pin_index) const;
	int CanLinkFrom(int source_layer_id, int source_pin_index) const;
	int CanLinkTo(int dest_layer_id, int dest_pin_index) const;
	int CanLink(int source_layer_id, int source_pin_index, int dest_layer_id, int dest_pin_index) const;

	int GetLayerPropertyVisible(const NNLayer &layer, int property_index) const;

	int CanUnitize() const {return can_unitize_;}
	FString GetArgument(const NNStructures *related_networks, const FString &name, FStringList &visit_network, int &found, FString &error) const;
private:
	//Internal Access
	NNLayer &GetLayer_(int layer_index) {return layers_.GetAt(layer_index);}
	NNLayer *GetLayerByID_(int layer_id);
	NNLink *GetLinkByID_(int link_id);
	void SetRankRecursive(int layer_id, FListT<int> &rank, int current_rank) const;//used in ArrangeLayer
	FListT<int> GetMaxRankLayerIndexSequence(const FListT<int> &rank) const;//used in ArrangeLayer
public:
	//UI operation
	void ClearSelection();
	void InverseLayerSelection();
	int GetLayerSelected(int layer_id) const;
	void SetLayerSelected(int layer_id, int selected, int group);
	void SelectAllLayer();
	void SetSelectionRect(const float *rect);
	void ResetSelectionRect();
	const float *GetSelectionRect() const {return selection_rect_;}
	int MoveCursor(int direction, int add_selection);
	int GetCurrentCursorPosition(float *cursor_position);

	int GetCurrentLayerID() const;
	void SetCurrentLayerID(int current_layer_id);
	int GetCurrentLinkID() const;
	void SetCurrentLinkID(int current_link_id);
	int GetCurrentPropertyIndex() const {return current_property_index_;}
	int SetCurrentPropertyIndex(int current_property_index);
	int GetCurrentStatisticsIndex() const {return current_statistics_index_;}
	int SetCurrentStatisticsIndex(int current_statistics_index);
	const FString &GetCurrentPropertyName() const {return current_property_name_;}
	void SetCurrentHoverLayerIDs(FListT<int> hover_layer_ids) {hover_layer_ids_ = hover_layer_ids; ViewUpdate();}
	const FListT<int> &GetCurrentHoverLayerIDs() const {return hover_layer_ids_;}

    void ClearInitialOffset();
	int GetDragging() const {return dragging_;}
	void StartDrag();
	int EndDrag();
	void Drag(float dx, float dy);

	int GetLinkEditing() const {return link_editing_;}
	int GetLinkEditingDest() const {return link_editing_dest_;}
	int GetEditingLinkID() const {return editing_link_id_;}
	const float *GetEditingLinkPos() const {return editing_link_pos_;}
	const int *GetEditingLinkLayerID() const {return editing_link_layer_id_;}
	const int *GetEditingLinkPinIndex() const {return editing_link_pin_index_;}
	int StartLinkEdit_New(int source_layer_id, int source_pin_index, float x, float y);
	int StartLinkEdit_Existing(int link_id, int dest, float x, float y);
	void DragLink(float x, float y, int snap_layer_id = -1, int snap_input_pin_index = 0, int snap_output_pin_index = 0);
	int EndLinkEdit();

	void ArrangeLayers(int sort = false);
	int Replace(FString from, FString to, int target_selection);
public:
	//Operation
	NNLayer& AddLayer(int component_index, int auto_link = false, float x = -1.0f, float y = -1.0f, int escape = true);
	NNLink* AddLink(int source_layer_id, int source_pin_index, int dest_layer_id, int dest_pin_index, int force = false);
	int DeleteSelected();
	int SetLayerPropertyValue(int layer_id, int property_index, const FString &value, int escape=true, int force = false);
	int SetLayerPropertyValue(int layer_id, const FString &property_name, const FString &value, int escape=true, int force = false);
	int Paste(const NNStructure &structure, bool escape = true, FString allowed_char = FString(""), bool escape_position = true);

	int RemoveLayer(int layer_id);//Remove the layer and connect previous and next layer with link
	NNLayer *InsertLayer(int layer_id, int component_index);//Insert nwe layer and connect previous and next layer with link

	//Evolutionary Operation
	int Mutate(FListT<int> &added_layer_id);
	int MutateSub(FListT<int> &added_layer_id);
	int MutateByReplace();
	int MutateParameter();
	int MutateStructure(FListT<int> &added_layer_id);
	void ClearParameterFiles();
private:
	//Document
	FString name_;

	int next_layer_id_;
	int next_link_id_;

	FListP<NNLayer> layers_;
	FListP<NNLink> links_;
public:
	const NNStructures* related_networks_;
    FStringList used_structure_;
protected:
	//Component Info
	const NNLayerComponents *nn_layer_components_;

	//Structure Search Policy
	const NNStructureSearchPolicies *nn_structure_search_policies_;

public:
	//Temporary
	void CheckNextID();
	int GetUpdateCount() const {return update_count_;}//Need to update display value
	int GetViewUpdateCount() const {return view_update_count_;}//Need to redraw graphs
	int GetCompleteNetwork() const {return complete_network_;}
	void RefreshDynamicProperties();
	void RefreshDisplayValue(int test = false);
	void CheckInplaceRecursive(int layer_index, int &in_place, int &success, FListP<FListP<FListT<int> > > &cache_source_layer_indexes);
	void CheckError();
	void RefreshStatistics();
	void CheckRepeatID(
		int layer_index,
		FStringList current_repeat_ids,
		FStringList &repeat_ids,
		FListT<NNRepeatType> &repeat_types,
		FListT<int> &repeat_length,
		FListP<FStringList> &layer_repeat_ids,
		FListT<int> &layer_processed);
	const FListP<NNStatistics> &GetStatistics() const {return statistics_;}
	const NNStatistics *GetStatistics(const FString &label) const;
	int GetStatisticsIndexByLabel(const FString &label) const;
	double GetStatistics_Sum(const FString &label) const;
	const FListP<NNError> &GetErrors() const {return errors_;}

	const FListP<NNRepeatInfo> &GetRepeatInfo() const {return repeat_info_;}
	int GetRepeatInfoIndex(const FString &repeat_id) const;
	NNRepeatType GetRepeatType(const FString &repeat_id) const;
	int GetRepeatNum(const FString &repeat_id) const;
	FStringList GetRepeatExtensions(const NNLayer &layer, int weight) const;

	void SetEditString(const FString &edit_string) {edit_string_ = edit_string;}
	const FString &GetEditString() const {return edit_string_;}

	float GetNetworkViewSizeX() const {return max_x_ + NNLayerDefaultHSpace > selection_rect_[2] ? max_x_ + NNLayerDefaultHSpace : selection_rect_[2];}
	float GetNetworkViewSizeY() const {return max_y_ + NNLayerDefaultVSpace > selection_rect_[3] ? max_y_ + NNLayerDefaultVSpace : selection_rect_[3];}

	void SetRelatedNetworks(const NNStructures *structures) {related_networks_ = structures;}
	void SetUsedStructure(const FStringList &used_structure) {used_structure_ = used_structure;}
	const FStringList &GetUsedStructure() const {return used_structure_;}
	FStringList SetExportedArgs(const FStringList &input_size, const NNLayer &layer);
	void FixProperties();
	void EmbedParameters();
protected:
	int update_count_;
	int view_update_count_;
	void Update();//Increment update_count_
	void ViewUpdate();//Increment view_update_count_

	int complete_network_;
	FListP<NNError> errors_;
private:
	int current_layer_id_;
	int current_link_id_;
	int current_property_index_;
	FString current_property_name_;
	int current_statistics_index_;
	FListT<int> hover_layer_ids_;

	float max_x_;
	float max_y_;

	FListP<NNStatistics> statistics_;
	FListP<NNRepeatInfo> repeat_info_;

	FString edit_string_;//for undo,redo

public:
	int GetGridSize() const;
private:
	int dragging_;
	int editted_through_drag_;
	float selection_rect_[4];

	int link_editing_;
	int link_editing_dest_;
	int editing_link_id_;
	float editing_link_pos_[2];
	int editing_link_layer_id_[2];
	int editing_link_pin_index_[2];
	int can_unitize_;

public:
	//Helper
	void EscapeLayerName(FString allowed_char = FString(""));
	int ExpandAndFix(const FStringList &used_networks);

	int IsSameInOutSetting(const NNStructure &source) const;//Constraint for structure search
	FString EscapeNameStr(const FString &name_str, FString allowed_char = FString(""));
private:
	void Escape(int layer_index);
	void EscapeLayerName(int layer_index, FString allowed_char = FString(""));
	int CountDuplication() const;

public:
	//Obsolete
	const int GetUse_Obsolete(NNStructureUseType_Obsolete use_type) const {return use_[(int)use_type];}
	void SetUse_Obsolete(NNStructureUseType_Obsolete use_type, int use) {use_[(int)use_type] = use; Update();}
private:
	int use_[3];//0=training, 1=validation, 2=runtime

};
//---------------------------------------------------------------------------
class NNStructures : public NNStructure
{
public:
	NNStructures() {Clear();}
	virtual ~NNStructures() {Clear();}

	NNStructures &operator=(const NNStructures &source);
	void Clear();
private:
	NNStructures(const NNStructures &source);
public:
	int SetName(const FString &name);
	int GetNumStructure() const {return structures_.GetSize();}
	const NNStructure &GetStructure(int structure_index) const;
	NNStructure &GetStructure_(int structure_index);
	int GetActiveStructureIndex() const {return active_structure_index_;}
	void SetActiveStructure(int structure_index);
	int GetStructureIndexByName(const FString &structure_name) const;
	void SetActiveStructure(const FString &structure_name);
	void AddStructure();//added structure will be activated
	void DeleteStructure(int structure_index);
	void MoveStructure(int new_structure_index);//Active structure is will be moved to new_structure_index
	void GlobalEscapeLayerName(int layer_id, FString allowed_char=FString(""), int structure_index = -1);

	void RefreshDisplayValue(const NNStructures *related_networks = NULL, int test = false);
	void CheckError(const FListT<int> &structure_indexes);

	void SetRelatedNetworks(const NNStructures *related_networks = NULL);
public:
	//Operation
	int Replace(FString from, FString to, int target_all_structure, int target_selection);
	int Mutate(FListT<int> used_structure_index);
	void ArrangeAllStructure();
	void ClearParameterFiles();
//---------------------------------------------------------------------------
private:
	FListP<NNStructure> structures_;
	int active_structure_index_;
};
//---------------------------------------------------------------------------
};
#endif
