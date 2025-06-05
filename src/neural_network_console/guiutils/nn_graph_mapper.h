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

#ifndef nn_graph_mapperH
#define nn_graph_mapperH

#include "nn_structure.h"
namespace sdeep_console{

//---------------------------------------------------------------------------
enum class NNGraphItemShape : int{
None,
Line,
Rect,
RectFrame,
RoundRect,
RoundRectFrame,
DownTriangle,
InvertedTrapezoid,
Ellipse,
Text,
TextWordWrap,
TextRight,
TextCenter,
TextCenterBold,
Image,

Connector,
Grid
};
//---------------------------------------------------------------------------
enum class NNGraphParentType : int{
None,
Connector
};
//---------------------------------------------------------------------------
enum class NNGraphMapperZ : int {
None,
DragGrid,
DragShaddow,
LayerShadow,
LayerBG,
Layer,
LayerShade,
LayerFG,
LayerFG2,
LayerFG3,
Link,
Selection,
RangeSelection,
Top
};
//---------------------------------------------------------------------------
const int NNGraphMapperMaxConnectorNum = 256;
enum class NNGraphMapperLayerSubID : int {
None,
Foreground,
Background,
OuterRectRepeatShadow,
InputPinRepeatShadow,
OutputPinRepeatShadow,
LayerRect,
OuterRect,
InnerRect,
InvertedTrapezoid,
CapitalTextShadow,
CapitalText,
PropertyText,
PropertyText2,
ConnectorText,
ConnectorTextEnd=NNGraphMapperLayerSubID::ConnectorText + NNGraphMapperMaxConnectorNum,
LayerShade,
InputPin,
InputPin2,
InputPinEnd=NNGraphMapperLayerSubID::InputPin + NNGraphMapperMaxConnectorNum,
InputConnector2,
InputConnectorEnd=NNGraphMapperLayerSubID::InputConnector2 + NNGraphMapperMaxConnectorNum,
OutputPin,
OutputPin2,
OutputPin3,
Text,
ExtraInfoGraph,
ExtraInfoText,
RepeatText,
WarningImage,
};

class NNGraphItem
{
public:
	NNGraphItem();
	virtual ~NNGraphItem() {};
public:
	int Contains(float x, float y) const;
public:
	int operator == (const NNGraphItem &source) const;
	int operator < (const NNGraphItem &source) const;
	int operator > (const NNGraphItem &source) const;
	NNGraphItem &operator = (const NNGraphItem &source);
private:
	NNGraphItem(const NNGraphItem &source);
public:
	//Setup
	void SetLayerRects(const NNLayer &layer);
public:
	NNGraphMapperZ z;
    int id;
	FString name;
	NNGraphMapperLayerSubID sub_id;

	float rect[4];
	float current_rect[4];
	float initial_rect[4];
	float clear_rect[4];
	int color;

	float pen_width;
	float alpha;
	float scale;
	NNGraphItemShape shape;

	NNGraphParentType parent_type;
	int parent_id[2];
	FString parent_name[2];
	NNGraphMapperLayerSubID parent_sub_id[2];


	int can_focus;

	FString text;
	int font_size;

	int transition;
	float option;
    int optioni;
};
//---------------------------------------------------------------------------
class NNGraphMapper
{
public:
	NNGraphMapper() : repaint_request_(0) {};
	virtual ~NNGraphMapper() {};
public:
	void Clear();
public:
	int Update(NNStructure &structure, int refresh_display_value, int display_property_value = false);
	void RequestRepaint() {repaint_request_ = 1;};
	double Animation(double coef);
	const FListP<NNGraphItem> &GetGraph() const {return graph_;};
private:
	NNGraphItem *SearchGraphItem(int id, NNGraphMapperLayerSubID sub_id, const FString &name);
private:
	FListP<NNGraphItem> graph_;
	FListP<NNGraphItem> target_graph_;
	int repaint_request_;
};
//---------------------------------------------------------------------------
};
#endif
