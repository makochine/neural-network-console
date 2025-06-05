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
#include "color_util.h"
#include "geometric_util.h"
#include "nn_graph_mapper.h"
#include "color_defines.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
const int NNGraphMapperLeftArea = 48;
const int NNGraphMapperShadowShift = 5;
const int NNGraphMapperHSpace = 8;
const int NNGraphMapperVSpace = 8;
const int NNGraphMapperFontVSpace = 8;
const int NNGraphMapperFontHSpace = 8;
const int NNGraphMapperFontSize = 16;
const int NNGraphMapperCapitalFontSize = 42;
const int NNGraphMapperFontSizeSmall = 12;
const int NNGraphMapperSelectionWidth = 3;
const int NNGraphMapperSocketSize = 8;
const int NNGraphMapperRangeSelectionWidth = 2;
const int NNGraphMapperGraphWidth = 72;
const int NNGraphMapperGraphHeight = 6;
const int NNGraphMapperLineWidth = 1;
const float NNGraphMapperPinSpace = NNLayerDefaultHeight / 12;
const float NNGraphMapperPinXStep = (NNLayerDefaultHeight * 4) / 12;
const float NNGraphMapperPinXSize = (NNLayerDefaultHeight * 6) / 12;
const float NNGraphMapperPinYSize = (NNLayerDefaultHeight * 6) / 12;
const float NNGraphMapperPinYStep = NNLayerDefaultHeight - NNGraphMapperPinSpace * 2 - NNGraphMapperPinYSize;

/*
const float NNGraphMapperPinHShift = NNLayerDefaultWidth / 11;
const float NNGraphMapperPinVShift = NNLayerDefaultHeight / 9;
*/

//---------------------------------------------------------------------------
NNGraphItem::NNGraphItem() :
	z(NNGraphMapperZ::None),
	id(0),
	sub_id(NNGraphMapperLayerSubID::None),

	//rect[4](),
	//current_rect[4](),
	//initial_rect[4](),
	//clear_rect[4](),
	color(0x000000),

	pen_width(1.0),
	alpha(1.0),
	scale(1.0),
	shape(NNGraphItemShape::None),

	parent_type(NNGraphParentType::None),
	//parent_id[2](),
	//parent_sub_id[2](),

	can_focus(false),

	//FString text(),
	font_size(NNGraphMapperFontSize),

	transition(true),
	option(0.0f),
	optioni(0)
{
	memset(rect, 0, sizeof(rect));
	memset(current_rect, 0, sizeof(current_rect));
	memset(initial_rect, 0, sizeof(initial_rect));
	memset(clear_rect, 0, sizeof(clear_rect));

	memset(parent_id, 0, sizeof(parent_id));
	parent_name[0] = "";
	parent_name[1] = "";
	memset(parent_sub_id, 0, sizeof(parent_sub_id));
}
//---------------------------------------------------------------------------
int NNGraphItem::Contains(float x, float y)  const
{
	switch (shape) {
	case NNGraphItemShape::None:
	case NNGraphItemShape::Grid:
		return false;

	case NNGraphItemShape::Ellipse:
		{
			float cx = (current_rect[0] + current_rect[2]) * 0.5f;
			float cy = (current_rect[1] + current_rect[3]) * 0.5f;
			float rh = (current_rect[2] - current_rect[0]) * 0.5f;
			float rv = (current_rect[3] - current_rect[1]) * 0.5f;
			float dx = x - cx;
			if (rh > 0.0f) {
				dx /= rh;
			}
			float dy = y - cy;
			if (rv > 0.0f) {
				dy /= rv;
			}
			return dx * dx + dy * dy <= 1.0f;
		}
	case NNGraphItemShape::Line:
		{
			return DistanceToLine(x, y, current_rect[0], current_rect[1], current_rect[2], current_rect[3]) <= pen_width + 5;
		}
	case NNGraphItemShape::Connector:
		{
			const float distance_x = fabs(current_rect[0] - current_rect[2]);
			const float distance_y = fabs(current_rect[1] - current_rect[3]);
			const float distance = sqrt(distance_x * distance_x + distance_y * distance_y);
			if (current_rect[1] < current_rect[3] - NNLayerDefaultR &&
				fabs(current_rect[0] - current_rect[2]) >= NNLayerDefaultR &&
				(parent_sub_id[1] >= NNGraphMapperLayerSubID::InputPin2 &&
				(int)parent_sub_id[1] <= (int)NNGraphMapperLayerSubID::InputPin + NNGraphMapperMaxConnectorNum) &&
				optioni) {
				//Parameter Input Pin
				const float y1 = current_rect[3] - NNLayerDefaultR;
				if (y < current_rect[3] - NNLayerDefaultR) {
					//vertical straight line
					return DistanceToLine(x, y, current_rect[0], current_rect[1], current_rect[0], y1) <= pen_width + 5;
				}else {
					const float x1 = current_rect[0] + NNLayerDefaultR * (1 - (current_rect[0] > current_rect[2]) * 2);
					if (fabs(current_rect[0] - x) >= NNLayerDefaultR) {
						//horizontal straight line
						return DistanceToLine(x, y, x1, current_rect[3], current_rect[2], current_rect[3]) <= pen_width + 5;
					}else {
						x -= x1;
						y -= y1;
						return fabs(sqrt((x * x) + (y * y)) - NNLayerDefaultR) <= pen_width + 5;
					}
				}
			}else if (current_rect[1] <= current_rect[3] || distance <= NNLayerDefaultSpace) {
				//Top down
				if (distance <= NNLayerDefaultR || distance_x <= NNLayerDefaultR * 2 || distance_y <= NNLayerDefaultR * 2) {
					//straight line
					return DistanceToLine(x, y, current_rect[0], current_rect[1], current_rect[2], current_rect[3]) <= pen_width + 5;
				}else {
					if (y < (current_rect[1] + current_rect[3]) * 0.5 - NNLayerDefaultR){
						//vertical straight line(top half)
						return DistanceToLine(
							x, y,
							current_rect[0], current_rect[1],
							current_rect[0], (current_rect[1] + current_rect[3]) * 0.5 - NNLayerDefaultR) <= pen_width + 5;
					}else if (y < (current_rect[1] + current_rect[3]) * 0.5 + NNLayerDefaultR){
						if (fabs(x - current_rect[0]) < NNLayerDefaultR){
							//first r
							x -= current_rect[0] + (current_rect[0] < current_rect[2] ? NNLayerDefaultR : -NNLayerDefaultR);
							y -= (current_rect[1] + current_rect[3]) * 0.5 - NNLayerDefaultR;
						}else if (fabs(x - current_rect[2]) >= NNLayerDefaultR){
							//horizontal straight line
							return DistanceToLine(
								x, y,
								current_rect[0], (current_rect[1] + current_rect[3]) * 0.5,
								current_rect[2], (current_rect[1] + current_rect[3]) * 0.5) <= pen_width + 5;
						}else {
							//second r
							x -= current_rect[2] + (current_rect[0] < current_rect[2] ? -NNLayerDefaultR : NNLayerDefaultR);
							y -= (current_rect[1] + current_rect[3]) * 0.5 + NNLayerDefaultR;
						}
						return fabs(sqrt((x * x) + (y * y)) - NNLayerDefaultR) <= pen_width + 5;
					}else {
						//vertical straight line(bottom half)
						return DistanceToLine(
							x, y,
							current_rect[2], (current_rect[1] + current_rect[3]) * 0.5 + NNLayerDefaultR,
							current_rect[2], current_rect[3]) <= pen_width + 5;
					}
					return false;//Todo
				}
			}else if (current_rect[2] < current_rect[0] + NNLayerDefaultWidth / 2 + NNLayerDefaultHSpace + NNLayerDefaultSpace * 2 - 1){
				//Bottom up
				const float escape_x =
					(current_rect[0] > current_rect[2] ? current_rect[0] : current_rect[2]) +
					NNLayerDefaultWidth / 2 + NNLayerDefaultHSpace;
				if (y >= current_rect[1]) {
					//start at bottom
					if (x <= current_rect[0] + NNLayerDefaultR) {
						//first r
						x -= current_rect[0] + NNLayerDefaultR;
					}else if (x <= escape_x - NNLayerDefaultR) {
						//bottom horizontal straight line
						const float yl = current_rect[1] + NNLayerDefaultR;
						return fabs(y - yl) <= pen_width * 2.0f;
					}else {
						//second r
						x -= escape_x - NNLayerDefaultR;
					}
					y -= current_rect[1];
					return fabs(sqrt((x * x) + (y * y)) - NNLayerDefaultR) <= pen_width * 2.0f;
				}else if (y >= current_rect[3] ) {
					//escaped vertical straight line
					return fabs(x - escape_x) <= pen_width * 2.0f;
				}else {// y <= current_rect[3]
					//end at top
					if (x <= current_rect[2] + NNLayerDefaultR) {
						//last r
						x -= current_rect[2] + NNLayerDefaultR;
					}else if (x <= escape_x - NNLayerDefaultR) {
						//top horizontal straight line
						const float yl = current_rect[3] - NNLayerDefaultR;
						return fabs(y - yl) <= pen_width * 2.0f;
					}else {
						//third r
						x -= escape_x - NNLayerDefaultR;
					}
					y -= current_rect[3];
					return fabs(sqrt((x * x) + (y * y)) - NNLayerDefaultR) <= pen_width * 2.0f;
				}
			}else {
				//Bottom up�Aleft to right
				const float center_x = current_rect[0] + NNLayerDefaultWidth / 2 + NNLayerDefaultHSpace;
				if (y >= current_rect[1]) {
					//start at bottom
					if (x <= current_rect[0] + NNLayerDefaultR) {
						//first r
						x -= current_rect[0] + NNLayerDefaultR;
					}else if (x <= center_x - NNLayerDefaultR) {
						//bottom horizontal straight line
						const float yl = current_rect[1] + NNLayerDefaultR;
						return fabs(y - yl) <= pen_width * 2.0f;
					}else {
						//second r
						x -= center_x - NNLayerDefaultR;
					}
					y -= current_rect[1];
					return fabs(sqrt((x * x) + (y * y)) - NNLayerDefaultR) <= pen_width * 2.0f;
				}else if (y >= current_rect[3] ) {
					//center vertical straight line
					return fabs(x - center_x) <= pen_width * 2.0f;
				}else {// y <= current_rect[3]
					//end at top
					if (x >= current_rect[2] - NNLayerDefaultR) {
						//last r
						x -= current_rect[2] - NNLayerDefaultR;
					}else if (x >= center_x + NNLayerDefaultR) {
						//top horizontal straight line
						const float yl = current_rect[3] - NNLayerDefaultR;
						return fabs(y - yl) <= pen_width * 2.0f;
					}else {
						//third r
						x -= center_x + NNLayerDefaultR;
					}
					y -= current_rect[3];
					return fabs(sqrt((x * x) + (y * y)) - NNLayerDefaultR) <= pen_width * 2.0f;
				}
			}
		}

	case NNGraphItemShape::Rect:
	case NNGraphItemShape::RectFrame:
	case NNGraphItemShape::RoundRect:
	case NNGraphItemShape::RoundRectFrame:
	case NNGraphItemShape::DownTriangle:
	case NNGraphItemShape::InvertedTrapezoid:
	case NNGraphItemShape::Text:
	default:
		return
			x >= current_rect[0] &&
			x < current_rect[2] &&
			y >= current_rect[1] &&
			y < current_rect[3];
	}
}
//---------------------------------------------------------------------------
int NNGraphItem::operator == (const NNGraphItem &source) const {
	return sub_id == source.sub_id && name == source.name;
}
//---------------------------------------------------------------------------
int NNGraphItem::operator < (const NNGraphItem &source) const {
	if (z < source.z) {
		return true;
	}else if (z > source.z) {
		return false;
	}else if (name < source.name) {
		return true;
	}else if (name > source.name) {
		return false;
	}else if (sub_id < source.sub_id) {
		return true;
	}else {
		return false;
	}
};
//---------------------------------------------------------------------------
int NNGraphItem::operator > (const NNGraphItem &source) const {
	if (z > source.z) {
		return true;
	}else if (z < source.z) {
		return false;
	}else if (name > source.name) {
		return true;
	}else if (name < source.name) {
		return false;
	}else if (sub_id > source.sub_id) {
		return true;
	}else {
		return false;
	}
};
//---------------------------------------------------------------------------
NNGraphItem &NNGraphItem::operator = (const NNGraphItem &source){
	z = source.z;
	name = source.name;
    id = source.id;
	sub_id = source.sub_id;

	memcpy(rect, source.rect, sizeof(rect));
	memcpy(current_rect, source.current_rect, sizeof(current_rect));
	memcpy(initial_rect, source.initial_rect, sizeof(initial_rect));
	memcpy(clear_rect, source.clear_rect, sizeof(clear_rect));
	color = source.color;

	pen_width = source.pen_width;
	alpha = source.alpha;
	scale = source.scale;
	shape = source.shape;

	parent_type = source.parent_type;
	memcpy(parent_id, source.parent_id, sizeof(parent_id));
	parent_name[0] = source.parent_name[0];
	parent_name[1] = source.parent_name[1];
	memcpy(parent_sub_id, source.parent_sub_id, sizeof(parent_sub_id));

	can_focus = source.can_focus;

	text = source.text;
	font_size = source.font_size;

	transition = source.transition;
	option = source.option;
	optioni = source.optioni;

	return *this;
}
//---------------------------------------------------------------------------
void NNGraphItem::SetLayerRects(const NNLayer &layer)
{
	memcpy(initial_rect, rect, sizeof(initial_rect));
	initial_rect[0] += layer.GetInitialOffset(0);
	initial_rect[1] += layer.GetInitialOffset(1);
	initial_rect[2] += layer.GetInitialOffset(0);
	initial_rect[3] += layer.GetInitialOffset(1);
	memcpy(current_rect, initial_rect, sizeof(current_rect));
	memcpy(clear_rect, rect, sizeof(clear_rect));
}
//---------------------------------------------------------------------------
void NNGraphMapper::Clear()
{
	graph_.Clear();
	target_graph_.Clear();
}
//---------------------------------------------------------------------------
int NNGraphMapper::Update(NNStructure &structure, int refresh_display_value, int display_property_value)
{
	if (refresh_display_value){
		structure.RefreshDisplayValue();
		structure.RefreshStatistics();
	}
	target_graph_.Clear();

	//Drag grid
	if (structure.GetDragging()) {
		NNGraphItem &new_item = target_graph_.Add();
		new_item.z = NNGraphMapperZ::DragGrid;

		new_item.rect[0] = 0.0;
		new_item.rect[1] = 0.0;
		new_item.rect[2] = structure.GetGridSize();
		new_item.rect[3] = structure.GetGridSize();
		memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));
		memcpy(new_item.current_rect, new_item.rect, sizeof(new_item.current_rect));
		memcpy(new_item.clear_rect, new_item.rect, sizeof(new_item.clear_rect));

		new_item.pen_width = 1.5f;
		new_item.color = 0x808080;
		new_item.shape = NNGraphItemShape::Grid;
		new_item.transition = true;
	}

	//Drag shaddow
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);
		if (fabs(layer.GetCurrentPosition(0) - layer.GetPosition(0)) > 0.5 ||
			fabs(layer.GetCurrentPosition(1) - layer.GetPosition(1)) > 0.5) {
			NNGraphItem &new_item = target_graph_.Add();
			new_item.z = NNGraphMapperZ::DragShaddow;
			new_item.name = layer.GetPropertyValue(FString("Name"));

			new_item.rect[0] = layer.GetPosition(0);
			new_item.rect[1] = layer.GetPosition(1);
			new_item.rect[2] = new_item.rect[0] + layer.GetPosition(2);
			new_item.rect[3] = new_item.rect[1] + layer.GetPosition(3);
			new_item.SetLayerRects(layer);

			new_item.color = 0xc0c0c0;
			new_item.shape = NNGraphItemShape::RectFrame;
			new_item.transition = false;
		}
	}

	//Layer
	const FString extra_text_property =
		structure.GetCurrentStatisticsIndex() >= 0 ?
		structure.GetStatistics().GetAt(structure.GetCurrentStatisticsIndex()).GetLabel() :
		FString("Output");

	// const NNLayerComponents *nn_layer_components_ = structure.GetNNLayerComponents();
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);
		const NNLayerComponent *component = layer.GetComponent();
		assert(component);
		FString layer_name = layer.GetPropertyValue(FString("Name"));

		if (component->GetComponentType() != NNLayerComponentComponentType::Comment) {
			//OuterRectRepeatShadow
			const FStringList &repeat_ids = layer.GetRepeatIDs();
			if (repeat_ids.GetSize()) {
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerShadow;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::OuterRectRepeatShadow;

				new_item.rect[0] = layer.GetCurrentPosition(0) + NNGraphMapperShadowShift;
				new_item.rect[1] = layer.GetCurrentPosition(1) + NNGraphMapperShadowShift;
				new_item.rect[2] = new_item.rect[0] + layer.GetCurrentPosition(2);
				new_item.rect[3] = new_item.rect[1] + layer.GetCurrentPosition(3);
				new_item.SetLayerRects(layer);

				new_item.color = 0xff000000 | FadeColor(0xac99bf, 0xffffff, 0.25);
				new_item.shape = NNGraphItemShape::Rect;
				new_item.transition = !layer.GetDragged();
			}
			//Rect
			{
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::Layer;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::LayerRect;

				new_item.rect[0] = layer.GetCurrentPosition(0);
				new_item.rect[1] = layer.GetCurrentPosition(1);
				new_item.rect[2] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2);
				new_item.rect[3] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3);
				new_item.SetLayerRects(layer);

				new_item.color = component->GetColor();
				new_item.shape = NNGraphItemShape::Rect;
				new_item.can_focus = true;
				new_item.transition = !layer.GetDragged();
			}
			//Text
			{
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerFG;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::Text;

				new_item.text = layer_name;
				new_item.font_size = NNGraphMapperFontSize;

				new_item.rect[0] = layer.GetCurrentPosition(0) + NNGraphMapperLeftArea + NNGraphMapperFontHSpace;
				new_item.rect[1] = layer.GetCurrentPosition(1) + NNGraphMapperFontVSpace - 5;
				new_item.rect[2] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2) - NNGraphMapperFontHSpace;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperFontSize * 1.5;
				new_item.SetLayerRects(layer);

				new_item.color = SDEEP_CONSOLE_COLOR_LIGHTEST;
				new_item.shape = NNGraphItemShape::Text;
				new_item.transition = !layer.GetDragged();
			}
			//Text (Capital letter)
			{
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerFG;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::CapitalText;

				new_item.text = layer.GetType().SubString(1, 1);
				new_item.font_size = NNGraphMapperCapitalFontSize;

				new_item.rect[0] = layer.GetCurrentPosition(0);
				new_item.rect[1] = layer.GetCurrentPosition(1) - 3;
				new_item.rect[2] = layer.GetCurrentPosition(0) + NNGraphMapperLeftArea;
				new_item.rect[3] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - 3;
				new_item.SetLayerRects(layer);

				new_item.color = SDEEP_CONSOLE_COLOR_LIGHTEST;//FadeColor(component->GetColor(), 0xffffff, 0.5);
				//new_item.shape = NNGraphItemShape::TextCenterBold;
				new_item.shape = NNGraphItemShape::TextCenter;
				new_item.transition = !layer.GetDragged();
			}
			//PropertyText
			{
				FListT<int> important_property_index;
				if (display_property_value) for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
					if (ip > 0 && structure.GetCurrentPropertyName() == layer.GetProperty(ip).GetName()) {
						important_property_index.Add(ip);
                        break;
					}
				}
				for (int ip = 0 ; ip < layer.GetNumProperty() ; ip++) {
					if (layer.GetProperty(ip).GetImportant() && structure.GetLayerPropertyVisible(layer, ip) && important_property_index.IndexOf(ip) < 0) {
						important_property_index.Add(ip);
					}
				}

				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerFG;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::PropertyText;

				for (int ip2 = 0 ; ip2 < important_property_index.GetSize() ; ip2++) {
					const int property_index = important_property_index.GetAt(ip2);
					const NNLayerProperty &property = layer.GetProperty(property_index);
					FString display_value =
						(property.GetType() == NNLayerPropertyType::PInt || property.GetType() == NNLayerPropertyType::UInt) ?
						property.GetDisplayValue().InsertThousandSeparator() :
						property.GetDisplayValue().ReplaceStr(",", ", ");
					if (new_item.text != ""){
						new_item.text += ", ";
					}
					new_item.text += property.GetShortName() + FString(" : ") + display_value;
				}
				new_item.font_size = NNGraphMapperFontSizeSmall;

				new_item.rect[0] = layer.GetCurrentPosition(0) + NNGraphMapperLeftArea + NNGraphMapperFontHSpace;
				new_item.rect[1] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - NNGraphMapperFontVSpace - 2 - NNGraphMapperFontSizeSmall;
				new_item.rect[2] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2) - NNGraphMapperFontHSpace;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperFontSizeSmall * 1.5;
				new_item.SetLayerRects(layer);

				new_item.color = SDEEP_CONSOLE_COLOR_LIGHTEST;//FadeColor(SDEEP_CONSOLE_COLOR_LIGHTEST, component->GetColor(), 0.33);
				new_item.shape = NNGraphItemShape::Text;
				new_item.transition = !layer.GetDragged();
			}

			//Input Pin

			// repeat shadow
			if (repeat_ids.GetSize() && component->GetInputType() != NNLayerComponentIOType::None) {
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerShadow;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::InputPinRepeatShadow;

				new_item.rect[0] = layer.GetCurrentPosition(0) + (layer.GetCurrentPosition(2) - NNGraphMapperSocketSize) / 2.0 + NNGraphMapperShadowShift;
				new_item.rect[1] = layer.GetCurrentPosition(1) - NNGraphMapperSocketSize / 2.0 + NNGraphMapperShadowShift;
				new_item.rect[2] = new_item.rect[0] + NNGraphMapperSocketSize;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperSocketSize;
				new_item.SetLayerRects(layer);

				new_item.color = SDEEP_CONSOLE_COLOR_LIGHT;
				new_item.shape = NNGraphItemShape::Ellipse;
				new_item.transition = !layer.GetDragged();
			}
			// input pin
			if (component->GetInputType() != NNLayerComponentIOType::None) {
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerBG;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::InputPin;

				new_item.rect[0] = layer.GetCurrentPosition(0) + (layer.GetCurrentPosition(2) - NNGraphMapperSocketSize) / 2.0;
				new_item.rect[1] = layer.GetCurrentPosition(1) - NNGraphMapperSocketSize / 2.0;
				new_item.rect[2] = new_item.rect[0] + NNGraphMapperSocketSize;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperSocketSize;
				new_item.SetLayerRects(layer);

				new_item.color = SDEEP_CONSOLE_COLOR_GRAPHLINK;
				new_item.shape = NNGraphItemShape::Ellipse;
				new_item.transition = !layer.GetDragged();
			}

			//Output Pin

			//repeat shadow
			if (repeat_ids.GetSize() && component->GetOutputType() != NNLayerComponentIOType::None) {
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerShadow;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::OutputPinRepeatShadow;

				new_item.rect[0] = layer.GetCurrentPosition(0) + (layer.GetCurrentPosition(2) - NNGraphMapperSocketSize) / 2.0 + NNGraphMapperShadowShift;
				new_item.rect[1] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - NNGraphMapperSocketSize / 2.0 + NNGraphMapperShadowShift;
				new_item.rect[2] = new_item.rect[0] + NNGraphMapperSocketSize;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperSocketSize;
				new_item.SetLayerRects(layer);

				new_item.color = SDEEP_CONSOLE_COLOR_LIGHT;
				new_item.shape = NNGraphItemShape::Ellipse;
				new_item.transition = !layer.GetDragged();
			}
			//output pin
			if (component->GetOutputType() != NNLayerComponentIOType::None) {
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerBG;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::OutputPin;

				new_item.rect[0] = layer.GetCurrentPosition(0) + (layer.GetCurrentPosition(2) - NNGraphMapperSocketSize) / 2.0;
				new_item.rect[1] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - NNGraphMapperSocketSize / 2.0;
				new_item.rect[2] = new_item.rect[0] + NNGraphMapperSocketSize;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperSocketSize;
				new_item.SetLayerRects(layer);

				new_item.color = SDEEP_CONSOLE_COLOR_GRAPHLINK;
				new_item.shape = NNGraphItemShape::Ellipse;
				new_item.can_focus = true;
				new_item.transition = !layer.GetDragged();
			}

			const int hover = structure.GetCurrentHoverLayerIDs().IndexOf(layer.GetLayerID()) >= 0;
			//Rect
			{
				//Shade
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerShade;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::LayerShade;

				new_item.rect[0] = layer.GetCurrentPosition(0);
				new_item.rect[1] = layer.GetCurrentPosition(1) + 0.5;
				new_item.rect[2] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2);
				new_item.rect[3] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - 0.5;
				new_item.SetLayerRects(layer);

				new_item.color = SDEEP_CONSOLE_COLOR_UIBG;
				new_item.shape = NNGraphItemShape::Rect;
				new_item.can_focus = false;
				new_item.transition = !layer.GetDragged();
				new_item.alpha = hover ? 0.5 : 0.0;
			}

			//InputSideConnector
			int pin_index = 0;
			for (int is = layer.GetNumSideConnector(0) - 1 ; is >= 0 ; is--){
				const NNLayerSideConnector &side_connector = layer.GetSideConnector(0, is);
				if (structure.GetPinVisible(layer.GetLayerID(), 0, is + 1)) {

					const float right = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2) - NNGraphMapperPinSpace - NNGraphMapperPinXStep * pin_index;
					const float top = layer.GetCurrentPosition(1) + NNGraphMapperPinSpace + ((pin_index + 1) % 2) * NNGraphMapperPinYStep;
					const float left = right - NNGraphMapperPinXSize;
					const float bottom = top + NNGraphMapperPinYSize;
					//Parameter Pin
					{
						NNGraphItem &new_item = target_graph_.Add();
						new_item.z = NNGraphMapperZ::Layer;
						new_item.name = layer_name;
						new_item.id = layer.GetLayerID();
						new_item.sub_id = NNGraphMapperLayerSubID((int)NNGraphMapperLayerSubID::InputPin2 + is);

						new_item.rect[0] = left;
						new_item.rect[1] = top;
						new_item.rect[2] = right;
						new_item.rect[3] = bottom;
						new_item.SetLayerRects(layer);
						new_item.color = FadeColor(component->GetColor(), SDEEP_CONSOLE_COLOR_DARK, 0.25);
						new_item.shape = NNGraphItemShape::Ellipse;
						new_item.can_focus = true;
						new_item.transition = !layer.GetDragged();
						new_item.alpha = (float)!hover;
					}
					//Parameter Name
					{
						NNGraphItem &new_item = target_graph_.Add();
						new_item.z = NNGraphMapperZ::LayerFG;
						new_item.name = layer_name;
						new_item.id = layer.GetLayerID();
						new_item.sub_id = NNGraphMapperLayerSubID((int)NNGraphMapperLayerSubID::ConnectorText + is);

						new_item.text = side_connector.GetShortName();
						new_item.font_size = NNGraphMapperFontSizeSmall - 1;

						new_item.rect[0] = left;
						new_item.rect[1] = top;
						new_item.rect[2] = right;
						new_item.rect[3] = bottom;
						new_item.SetLayerRects(layer);

						new_item.color = FadeColor(component->GetColor(), SDEEP_CONSOLE_COLOR_LIGHTEST, 0.33);
						new_item.shape = NNGraphItemShape::TextCenterBold;
						new_item.transition = !layer.GetDragged();
						new_item.alpha = (float)!hover;
					}
					//Parameter Pin
					{
						NNGraphItem &new_item = target_graph_.Add();
						new_item.z = NNGraphMapperZ::LayerFG2;
						new_item.name = layer_name;
						new_item.id = layer.GetLayerID();
						new_item.sub_id = NNGraphMapperLayerSubID((int)NNGraphMapperLayerSubID::InputPin2 + is);

						new_item.rect[0] = left;
						new_item.rect[1] = top;
						new_item.rect[2] = right;
						new_item.rect[3] = bottom;
						new_item.SetLayerRects(layer);
						new_item.color = FadeColor(component->GetColor(), SDEEP_CONSOLE_COLOR_DARK, 0.25);
						new_item.shape = NNGraphItemShape::Ellipse;
						new_item.can_focus = true;
						new_item.transition = !layer.GetDragged();
						new_item.alpha = hover;
					}
					//Parameter Name
					{
						NNGraphItem &new_item = target_graph_.Add();
						new_item.z = NNGraphMapperZ::Top;
						new_item.name = layer_name;
						new_item.id = layer.GetLayerID();
						new_item.sub_id = NNGraphMapperLayerSubID((int)NNGraphMapperLayerSubID::ConnectorText + is);

						new_item.text = side_connector.GetShortName();
						new_item.font_size = NNGraphMapperFontSizeSmall - 1;

						new_item.rect[0] = left;
						new_item.rect[1] = top;
						new_item.rect[2] = right;
						new_item.rect[3] = bottom;
						new_item.SetLayerRects(layer);

						new_item.color = SDEEP_CONSOLE_COLOR_LIGHTEST;
						new_item.shape = NNGraphItemShape::TextCenterBold;
						new_item.transition = !layer.GetDragged();
						new_item.alpha = hover;
					}
					pin_index++;
				}
			}
			//ExtraInfoText
			const NNLayerProperty *property = layer.GetPropertyByName(extra_text_property);
			if (property){
				FString extra_info = property->GetDisplayValue();
				if (extra_info != ""){
					float value = layer.GetPropertyByName(extra_text_property)->GetIntArrayProduct();
					for (int ir = 0 ; ir < repeat_ids.GetSize(); ir++) {
						const int len = structure.GetRepeatNum(repeat_ids.GetAt(ir));
						value *= len;
					}
					const int index = structure.GetStatisticsIndexByLabel(extra_text_property);
					if (index >= 0){
						const double max = structure.GetStatistics().GetAt(index).GetMax();
						if (max > 0.0) {
							{
								value /= max;
								NNGraphItem &new_item = target_graph_.Add();
								new_item.z = NNGraphMapperZ::Layer;
								new_item.name = layer_name;
								new_item.id = layer.GetLayerID();
								new_item.sub_id = NNGraphMapperLayerSubID::ExtraInfoGraph;

								new_item.rect[0] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2) + NNGraphMapperHSpace;
								new_item.rect[3] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - NNGraphMapperVSpace - NNGraphMapperFontSizeSmall + NNGraphMapperFontSize;
								new_item.rect[2] = new_item.rect[0] + NNGraphMapperGraphWidth * value;
								new_item.rect[1] = new_item.rect[3] - NNGraphMapperGraphHeight;
								new_item.SetLayerRects(layer);
								memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));
								new_item.initial_rect[2] = new_item.initial_rect[0];

								new_item.color = SDEEP_CONSOLE_COLOR_GRAPHBAR;
								new_item.shape = NNGraphItemShape::Rect;
								new_item.transition = !layer.GetDragged();
							}
							{
								NNGraphItem &new_item = target_graph_.Add();
								new_item.z = NNGraphMapperZ::Layer;
								new_item.name = layer_name;
								new_item.id = layer.GetLayerID();
								new_item.sub_id = NNGraphMapperLayerSubID::ExtraInfoText;

								new_item.text =
									(property->GetType() == NNLayerPropertyType::PInt || property->GetType() == NNLayerPropertyType::UInt) ?
									extra_info.InsertThousandSeparator() :
									extra_info.ReplaceStr(",", ", ");
								new_item.font_size = NNGraphMapperFontSizeSmall;

								new_item.rect[0] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2) + NNGraphMapperHSpace;
								new_item.rect[1] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - NNGraphMapperVSpace - NNGraphMapperFontSizeSmall - NNGraphMapperGraphHeight;
								new_item.rect[2] = new_item.rect[0] + NNGraphMapperGraphWidth;
								new_item.rect[3] = new_item.rect[1] + NNGraphMapperFontSize;
								new_item.SetLayerRects(layer);
								memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));

								new_item.color = SDEEP_CONSOLE_COLOR_FOREGROUND;
								if (layer.GetIsParamPropertyIndex() >= 0){
									if (layer.GetProperty(layer.GetIsParamPropertyIndex()).GetIntArray().GetSize() == 1){
										if (layer.GetProperty(layer.GetIsParamPropertyIndex()).GetIntArray().GetAt(0)){
											new_item.color = SDEEP_CONSOLE_COLOR_PARAMETER;
										}
									}
								}
								new_item.shape = NNGraphItemShape::Text;
								new_item.transition = !layer.GetDragged();
							}
						}
					}
				}
			}
			//Repeat Text
			if (repeat_ids.GetSize()){
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::Layer;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::RepeatText;

				FString repeat_text = FString("x");
				for (int ir = 0 ; ir < repeat_ids.GetSize(); ir++) {
					if (ir > 0) {
						repeat_text += ", ";
					}
					const int len = structure.GetRepeatNum(repeat_ids.GetAt(ir));
					repeat_text += FString(len);
				}
				new_item.text = repeat_text;
				new_item.font_size = NNGraphMapperFontSizeSmall;

				new_item.rect[0] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2) + NNGraphMapperHSpace;
				new_item.rect[1] = layer.GetCurrentPosition(1) + NNGraphMapperVSpace;
				new_item.rect[2] = new_item.rect[0] + NNGraphMapperGraphWidth;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperFontSize;
				new_item.SetLayerRects(layer);
				memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));

				new_item.color = SDEEP_CONSOLE_COLOR_LIGHTFG2;
				new_item.shape = NNGraphItemShape::Text;
				new_item.transition = !layer.GetDragged();
			}
			if (layer.GetErrors().GetSize() && !layer.GetSelected()){
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerFG;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::WarningImage;

				new_item.text = "Warning";

				new_item.rect[0] = layer.GetCurrentPosition(0) - NNGraphMapperFontSize * 0.75;
				new_item.rect[1] = layer.GetCurrentPosition(1) + (layer.GetCurrentPosition(3) - NNGraphMapperFontSize * 1.5) * 0.5;
				new_item.rect[2] = new_item.rect[0] + NNGraphMapperFontSize * 1.5;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperFontSize * 1.5;
				new_item.SetLayerRects(layer);
				memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));

				new_item.shape = NNGraphItemShape::Image;
				new_item.transition = !layer.GetDragged();
			}
		}else {//if (component->GetComponentType() == NNLayerComponentComponentType::Comment) {
			//OuterRect
			{
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::Layer;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::OuterRect;

				new_item.rect[0] = layer.GetCurrentPosition(0);
				new_item.rect[1] = layer.GetCurrentPosition(1);
				new_item.rect[2] = new_item.rect[0] + layer.GetCurrentPosition(2);
				new_item.rect[3] = new_item.rect[1] + layer.GetCurrentPosition(3);
				new_item.SetLayerRects(layer);

				new_item.color = 0xf0f0f0;
				new_item.shape = NNGraphItemShape::Rect;
				new_item.can_focus = true;
				new_item.transition = !layer.GetDragged();
			}
			//InnerRect
			{
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::Layer;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::InnerRect;

				new_item.rect[0] = layer.GetCurrentPosition(0) + 2;
				new_item.rect[1] = layer.GetCurrentPosition(1) + 2;
				new_item.rect[2] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2) - 2;
				new_item.rect[3] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - 2;
				new_item.SetLayerRects(layer);

				new_item.color = 0xffffff;
				new_item.shape = NNGraphItemShape::Rect;
				new_item.transition = !layer.GetDragged();
			}
			//Text
			{
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::Layer;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::Text;

				new_item.text = layer.GetDisplayPropertyValue(FString("Comment"));;
				new_item.font_size = NNGraphMapperFontSize;

				new_item.rect[0] = layer.GetCurrentPosition(0) + 4;
				new_item.rect[1] = layer.GetCurrentPosition(1);
				new_item.rect[2] = layer.GetCurrentPosition(0) + layer.GetCurrentPosition(2) - 4;
				new_item.rect[3] = layer.GetCurrentPosition(1) + layer.GetCurrentPosition(3) - 2;
				new_item.SetLayerRects(layer);

				new_item.color = 0xff000000;
				new_item.shape = NNGraphItemShape::TextWordWrap;
				new_item.transition = !layer.GetDragged();
			}
		}
	}

	//Link
	int *dest_link_num = new int[structure.GetNumLayer()];
	memset(dest_link_num, 0, structure.GetNumLayer() * sizeof(int));
	FListT<int> dest_layer_indices;
	FListT<NNGraphItem *> graph_items;
	for (int ili = 0 ; ili < structure.GetNumLink() ; ili++) {
		const NNLink &link = structure.GetLink(ili);
		const int source_layer_index = structure.GetLayerIndexByID(link.GetSourceLayerID());
		FString source_layer_name = structure.GetLayer(source_layer_index).GetPropertyValue(FString("Name"));
		const int dest_layer_index = structure.GetLayerIndexByID(link.GetDestLayerID());
		dest_layer_indices.Add(dest_layer_index);
		if (link.GetDestPinIndex() == 0){
			dest_link_num[dest_layer_index]++;
		}
		FString dest_layer_name = structure.GetLayer(dest_layer_index).GetPropertyValue(FString("Name"));
		FString link_name = source_layer_name + FString(link.GetSourcePinIndex()) + FString("->") + dest_layer_name + FString(link.GetDestPinIndex());
		// const int hover = structure.GetCurrentHoverLayerIDs().IndexOf(link.GetDestLayerID()) >= 0;
		if (!structure.GetLinkEditing() || structure.GetEditingLinkID() != link.GetLinkID()) {
			{
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::Link;
				new_item.name = link_name;
				new_item.id = link.GetLinkID();
				new_item.sub_id = NNGraphMapperLayerSubID::Background;
				new_item.parent_type = NNGraphParentType::Connector;
				new_item.parent_id[0] = link.GetSourceLayerID();
				new_item.parent_name[0] = source_layer_name;
				new_item.parent_sub_id[0] = (NNGraphMapperLayerSubID)((int)NNGraphMapperLayerSubID::OutputPin + link.GetSourcePinIndex());
				new_item.parent_id[1] = link.GetDestLayerID();
				new_item.parent_name[1] = dest_layer_name;
				new_item.parent_sub_id[1] = (NNGraphMapperLayerSubID)((int)NNGraphMapperLayerSubID::InputPin + link.GetDestPinIndex());

				new_item.color = structure.GetCurrentLinkID() == link.GetLinkID() ? SDEEP_CONSOLE_COLOR_LINKSELECT : SDEEP_CONSOLE_COLOR_GRAPHLINK;
				new_item.pen_width = NNGraphMapperLineWidth;
				new_item.shape = NNGraphItemShape::Connector;
				new_item.can_focus = true;
				new_item.transition = false;
				new_item.option = NNGraphMapperSocketSize * 0.5f;
				if (link.GetDestPinIndex() == 0){
					new_item.optioni = dest_link_num[dest_layer_index];
				}
				graph_items.Add(&new_item);
			}
		}else {
			graph_items.Add(NULL);
		}
	}
	for (int ili = 0 ; ili < structure.GetNumLink() ; ili++) {
		const NNLink &link = structure.GetLink(ili);
		if (graph_items.GetAt(ili) && link.GetDestPinIndex() == 0){
			/*
			//Brightness
			if (dest_link_num[dest_layer_indices.GetAt(ili)] > 1 && graph_items.GetAt(ili)->optioni > 1 && structure.GetCurrentLinkID() != graph_items.GetAt(ili)->id){
				const double brightness = 1.0 - (graph_items.GetAt(ili)->optioni - 1) * 0.5 / (dest_link_num[dest_layer_indices.GetAt(ili)] - 1);
				graph_items.GetAt(ili)->color = FadeColor(SDEEP_CONSOLE_COLOR_BACKGROUND, graph_items.GetAt(ili)->color, brightness);
			}
			*/
			if (dest_link_num[dest_layer_indices.GetAt(ili)] > 1){
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::LayerFG;
				new_item.name = graph_items.GetAt(ili)->name + FString("_idx");
				new_item.id = graph_items.GetAt(ili)->id;
				new_item.sub_id = NNGraphMapperLayerSubID::PropertyText;
				new_item.text = FString(graph_items.GetAt(ili)->optioni);
				new_item.font_size = NNGraphMapperFontSizeSmall;

				const NNLayer *layer = structure.GetLayerByID(graph_items.GetAt(ili)->parent_id[0]);
				assert(layer);

				new_item.rect[0] = layer->GetCurrentPosition(0);
				new_item.rect[1] = layer->GetCurrentPosition(1) + layer->GetCurrentPosition(3) + 1;
				new_item.rect[2] = layer->GetCurrentPosition(0) + layer->GetCurrentPosition(2) * 0.5 - 5;
				new_item.rect[3] = new_item.rect[1] + NNGraphMapperFontSizeSmall * 1.5;
				new_item.SetLayerRects(*layer);

				new_item.color = SDEEP_CONSOLE_COLOR_FOREGROUND;
				new_item.shape = NNGraphItemShape::TextRight;
				new_item.transition = !layer->GetDragged();
			}
		}
	}
	delete[] dest_link_num;

	//Editing Link
	if (structure.GetLinkEditing()) {
		FString source_layer_name =
			structure.GetEditingLinkLayerID()[0] >= 0 ?
			structure.GetLayerByID(structure.GetEditingLinkLayerID()[0])->GetPropertyValue(FString("Name")) :
			FString("");
		FString dest_layer_name =
			structure.GetEditingLinkLayerID()[1] >= 0 ?
			structure.GetLayerByID(structure.GetEditingLinkLayerID()[1])->GetPropertyValue(FString("Name")) :
			FString("");
		// const int hover = structure.GetCurrentHoverLayerIDs().IndexOf(structure.GetEditingLinkLayerID()[1]) >= 0;
		{
			NNGraphItem &new_item = target_graph_.Add();
			new_item.z = NNGraphMapperZ::Link;
			new_item.id = structure.GetEditingLinkID();
			new_item.sub_id = NNGraphMapperLayerSubID::Background;
			new_item.parent_type = NNGraphParentType::Connector;
			new_item.parent_id[0] = structure.GetEditingLinkLayerID()[0];
			new_item.parent_name[0] = source_layer_name;
			new_item.parent_sub_id[0] = (NNGraphMapperLayerSubID)((int)NNGraphMapperLayerSubID::OutputPin + structure.GetEditingLinkPinIndex()[0]);
			new_item.parent_id[1] = structure.GetEditingLinkLayerID()[1];
			new_item.parent_name[1] = dest_layer_name;
			new_item.parent_sub_id[1] = (NNGraphMapperLayerSubID)((int)NNGraphMapperLayerSubID::InputPin + structure.GetEditingLinkPinIndex()[1]);
			new_item.rect[0] = new_item.rect[2] = structure.GetEditingLinkPos()[0];
			new_item.rect[1] = new_item.rect[3] = structure.GetEditingLinkPos()[1];
			memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));
			memcpy(new_item.clear_rect, new_item.rect, sizeof(new_item.clear_rect));
			memcpy(new_item.current_rect, new_item.rect, sizeof(new_item.current_rect));

			new_item.color = SDEEP_CONSOLE_COLOR_LINKSELECT;
			new_item.pen_width = NNGraphMapperLineWidth;
			new_item.shape = NNGraphItemShape::Connector;
			new_item.can_focus = false;
			new_item.transition = false;
			new_item.option = NNGraphMapperSocketSize * 0.5f;
			// new_item.optioni = hover;
		}
	}

	//Layer Selection
	for (int il = 0 ; il < structure.GetNumLayer() ; il++) {
		const NNLayer &layer = structure.GetLayer(il);

		if (layer.GetSelected()){
			FString layer_name = layer.GetPropertyValue(FString("Name"));
			if (structure.GetCurrentLayerID() == layer.GetLayerID()){
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::Selection;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::Foreground;

				new_item.rect[0] = layer.GetCurrentPosition(0);
				new_item.rect[1] = layer.GetCurrentPosition(1);
				new_item.rect[2] = new_item.rect[0] + layer.GetCurrentPosition(2);
				new_item.rect[3] = new_item.rect[1] + layer.GetCurrentPosition(3);
				memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));
				memcpy(new_item.clear_rect, new_item.rect, sizeof(new_item.clear_rect));
				new_item.clear_rect[0] -= 8;
				new_item.clear_rect[1] -= 8;
				new_item.clear_rect[2] += 8;
				new_item.clear_rect[3] += 8;
				new_item.initial_rect[0] -= 8;
				new_item.initial_rect[1] -= 8;
				new_item.initial_rect[2] += 8;
				new_item.initial_rect[3] += 8;
				memcpy(new_item.current_rect, new_item.initial_rect, sizeof(new_item.current_rect));

				new_item.color = SDEEP_CONSOLE_COLOR_GRAPHSELECT;
				new_item.pen_width = NNGraphMapperSelectionWidth * 2;
				new_item.alpha = 0.4;
				new_item.shape = NNGraphItemShape::Rect;
				new_item.transition = !layer.GetDragged();
			}
			{
				NNGraphItem &new_item = target_graph_.Add();
				new_item.z = NNGraphMapperZ::Selection;
				new_item.name = layer_name;
				new_item.id = layer.GetLayerID();
				new_item.sub_id = NNGraphMapperLayerSubID::Background;

				new_item.rect[0] = layer.GetCurrentPosition(0);
				new_item.rect[1] = layer.GetCurrentPosition(1);
				new_item.rect[2] = new_item.rect[0] + layer.GetCurrentPosition(2);
				new_item.rect[3] = new_item.rect[1] + layer.GetCurrentPosition(3);
				memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));
				memcpy(new_item.clear_rect, new_item.rect, sizeof(new_item.clear_rect));
				new_item.clear_rect[0] -= 8;
				new_item.clear_rect[1] -= 8;
				new_item.clear_rect[2] += 8;
				new_item.clear_rect[3] += 8;
				new_item.initial_rect[0] -= 8;
				new_item.initial_rect[1] -= 8;
				new_item.initial_rect[2] += 8;
				new_item.initial_rect[3] += 8;
				memcpy(new_item.current_rect, new_item.initial_rect, sizeof(new_item.current_rect));

				new_item.color = SDEEP_CONSOLE_COLOR_GRAPHSELECT;
				new_item.alpha = 0.4;
				new_item.pen_width = NNGraphMapperSelectionWidth;
				new_item.shape = NNGraphItemShape::Rect;
				new_item.transition = !layer.GetDragged();
			}
		}
	}

	//Range selection
	const float *selection_rect = structure.GetSelectionRect();
	if (selection_rect[0] != selection_rect[2] || selection_rect[1] != selection_rect[3]) {
		NNGraphItem &new_item = target_graph_.Add();
		new_item.z = NNGraphMapperZ::RangeSelection;
		new_item.sub_id = NNGraphMapperLayerSubID::None;

		memcpy(new_item.rect, selection_rect, sizeof(new_item.rect));
		memcpy(new_item.initial_rect, new_item.rect, sizeof(new_item.initial_rect));
		memcpy(new_item.clear_rect, new_item.rect, sizeof(new_item.clear_rect));
		memcpy(new_item.current_rect, new_item.initial_rect, sizeof(new_item.current_rect));

		new_item.color = 0x9808080;
		new_item.alpha = 0.5;
		new_item.pen_width = NNGraphMapperRangeSelectionWidth;
		new_item.shape = NNGraphItemShape::RectFrame;
		new_item.transition = false;
	}

	return true;
}
//---------------------------------------------------------------------------
double NNGraphMapper::Animation(double coef)
{
	double sum_diff = 0.0;
	graph_.Sort();
	target_graph_.Sort();

	int index = 0, target_index = 0;
	while (index < graph_.GetSize() || target_index< target_graph_.GetSize()) {
		int compare = 0;
		if (index >= graph_.GetSize()) {
			compare = 1;
		}else if (target_index >= target_graph_.GetSize()) {
			compare = -1;
		}else {
			NNGraphItem &graph = graph_.GetAt(index);
			NNGraphItem &target_graph = target_graph_.GetAt(target_index);
			if (graph < target_graph){
				compare = -1;
			}else if (graph > target_graph){
				compare = 1;
			}
		}

		switch (compare) {
		case -1://Deleted
			{
				NNGraphItem &item = graph_.GetAt(index);
				item.alpha = item.alpha * (1.0 - coef);
				for (int i = 0 ; i < 4 ; i++) {
					item.current_rect[i] = item.current_rect[i] * (1.0 - coef) + item.clear_rect[i] * coef;
				}
				if (item.alpha < 1.0 / 255.0) {
					graph_.Delete(index);
				}else {
					index++;
				}
				sum_diff += 1.0;
			}
			break;
		case 0://Exists
			{
				NNGraphItem item_bak;
				item_bak = graph_.GetAt(index);
				NNGraphItem &item = graph_.GetAt(index);
				const NNGraphItem &target_item = target_graph_.GetAt(target_index);

				if (item.text != target_item.text) {
                    sum_diff += 1.0;
				}

				item = target_item;
				switch (item.parent_type) {
				case NNGraphParentType::Connector:
					{
						for (int i = 0 ; i < 2 ; i++) {
							const NNGraphItem *source_item = SearchGraphItem(item.parent_id[i], item.parent_sub_id[i], item.parent_name[i]);
							if (source_item) {
								item.rect[i * 2] = (source_item->current_rect[0] + source_item->current_rect[2]) * 0.5f;
								item.rect[i * 2 + 1] = (source_item->current_rect[1] + source_item->current_rect[3]) * 0.5f;
							}
						}
						memcpy(item.clear_rect, item.rect, sizeof(item.clear_rect));
						memcpy(item.initial_rect, item.rect, sizeof(item.initial_rect));
					}
					break;
				default:
					break;
				}

				if (item.transition) {
					const double alpha_bak = item.alpha;
					item.alpha = item_bak.alpha * (1.0 - coef) + target_item.alpha * coef;
					sum_diff += fabs(alpha_bak - item.alpha) * 64;
					for (int i = 0 ; i < 4 ; i++) {
						item.current_rect[i] = item_bak.current_rect[i] * (1.0 - coef) + item.rect[i] * coef;
						if (fabs(item.current_rect[i] - item.rect[i]) < 1.0) {
							item.current_rect[i] = item.rect[i];
						}
					}
				}else {
					memcpy(item.current_rect, item.rect, sizeof(item.current_rect));
				}
				for (int i = 0 ; i < 4 ; i++) {
					sum_diff += fabs(item_bak.current_rect[i] - item.current_rect[i]);
                }
			}
			index++;
			target_index++;
			break;
		case 1://New Item
			{
				NNGraphItem &item = graph_.Insert(index, target_graph_.GetAt(target_index));
				item.alpha = 0.0;
				memcpy(item.current_rect, item.initial_rect, sizeof(item.rect));
				sum_diff += 1.0;
			}
			break;
		}
	}
	sum_diff += repaint_request_;
    repaint_request_ = 0;


	return sum_diff;
}
//---------------------------------------------------------------------------
NNGraphItem *NNGraphMapper::SearchGraphItem(int id, NNGraphMapperLayerSubID sub_id, const FString &name)
{
	for (int ig =  0 ; ig < graph_.GetSize() ; ig++) {
		NNGraphItem &item = graph_.GetAt(ig);
		if (item.sub_id == sub_id && item.id == id) {
			if (item.name == name) {
				return &item;
			}
		}
	}
	for (int ig =  0 ; ig < target_graph_.GetSize() ; ig++) {
		NNGraphItem &item = target_graph_.GetAt(ig);
		if (item.sub_id == sub_id && item.id == id) {
			if (item.name == name) {
				return &item;
            }
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
