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

#include <fmx.h>
#pragma hdrstop

#include "list_frame.h"
#include "color_util.h"
#include "color_defines.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TFr_List *Fr_List;
//---------------------------------------------------------------------------
//Common
static const int default_grid_text_hspace = 16;
static const int default_grid_text_vspace = 1;

static const double sdeep_console_transition_coef_ = 0.2;

//List
static const int col_space = 0;
//---------------------------------------------------------------------------
int ListItem::Contains(const FString &search_text) const
{
	for (int i = 0 ; i < values_.GetSize() ; i++) {
		if (values_.GetAt(i).Pos(search_text) >= 1) {
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
void ListItems::SaveToFile()
{
	FStringList sl;
	for (int ii = 0; ii < GetSize() ; ii++) {
		if (ii > 0) {
			sl.Add(FString(""));
		}
		ListItem &item = GetAt(ii);

		const int num_attributes = item.GetNumAttribute();
		for (int ia = 0 ; ia < num_attributes ; ia++) {
			sl.Add(
				item.GetAttributeName(ia) +
				FString("=") +
				item.GetAttributeValue(ia)
				);
		}
	}

	sl.SaveToFile(file_name_.PChar());
}
//---------------------------------------------------------------------------
void ListItems::LoadFromFile()
{
	Clear();
	FStringList sl;

	sl.LoadFromFile(file_name_.PChar());
	int line = 0;
	int index = 0;
	while (line < sl.GetSize()){
		if (sl.GetAt(line) == "") {
			index++;
		}else {
			while (index >= GetSize()) {
				Add();
			}
			FString s = sl.GetAt(line);
			FString attribute_name = s.SplitBy("=");
			s = s.ReplaceStr("{SDCPATH}", AnsiString(ExtractFilePath(ParamStr(0))).c_str());
			GetAt(index).AddAttribute(attribute_name, s);
		}
		line++;
	}
}
//---------------------------------------------------------------------------
void ListItems::AddListItem(FString key_attribute, const ListItem &list_item)
{
	const FString &value = list_item.GetAttribute(key_attribute);
	for (int ii = 0 ; ii < GetSize() ; ii++) {
		ListItem &item = GetAt(ii);
		if (item.GetAttribute(key_attribute) == value) {
			Delete(ii);
			break;
		}
	}
	Insert(0, list_item);
}
//---------------------------------------------------------------------------
void ListItems::AllocateOriginalIndex()
{
	for (int ii = 0 ; ii < GetSize() ; ii++) {
		ListItem &item = GetAt(ii);
		item.SetOriginalIndex(ii);
	}
}
//---------------------------------------------------------------------------
__fastcall TFr_List::TFr_List(TComponent* Owner)
	: TFrame(Owner),
	settings_(NULL),
	sdeep_console_transition_coef_(0.2f),
	popup_menu_(NULL),
	max_cols_(0),
	col_width_(512),
	row_height_(128),
	header_height_(56),
	allow_not_selected_(true),
    search_align_right_(true),
	top_(0.0),
	current_top_(0.0),
	enter_mode_(0),
	item_index_(-1),
	click_item_index_(-1),
	item_index_2_(-1),
	last_mouse_down_(false),
	last_mouse_x_(0.0f),
	last_mouse_y_(0.0f),

	internal_update_count_(0),
	update_count_(-1)
{
	OnApplyRenameListItem = NULL;
	OnItemChanged = NULL;
	OnItemClick = NULL;
	OnItemDblClick = NULL;
	OnItemDelete = NULL;
	OnSearchTextChanged = NULL;
	OnSearch = NULL;
	OnMouseDown = NULL;
	OnMouseMove = NULL;
	OnMouseUp = NULL;

	PB_List->Align = TAlignLayout::Client;
	Bu_List->Visible = false;
	Sc_ListY->Visible = false;
	Im_Search->Visible = false;
	Ed_ListSearch->Visible = false;
	Ed_ListSearch->Text = "";
	Ed_ListRename->Visible = false;
	popup_requested_ = false;
	wheel_enabled_ = true;

	color_background_ = SDEEP_CONSOLE_COLOR_BACKGROUND;
	color_cursor_ = SDEEP_CONSOLE_COLOR_LISTITEM1;
	color_selection_ = SDEEP_CONSOLE_COLOR_TEXTSELECT;
	color_selection2_ = SDEEP_CONSOLE_COLOR_TEXTSELECT2;
	color_line_ = SDEEP_CONSOLE_COLOR_LINE;
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::PB_ListPaint(TObject *Sender, TCanvas *Canvas)
{
	TCanvas *C = PB_List->Canvas;
	if (Bu_Menu->Visible) {
		Bu_Menu->Position->X = Width - Bu_Menu->Width;
		Bu_Menu->Position->Y = (header_height_ - Bu_Menu->Height) / 2;
	}

	if (Ed_ListSearch->Visible){
		const int x = search_align_right_ ?
			Width - Ed_ListSearch->Width - 16 - Bu_Menu->Visible * Bu_Menu->Width :
			48;
		const int y = (header_height_ - Ed_ListSearch->Height) / 2;;
		if (Ed_ListSearch->Position->X != x || Ed_ListSearch->Position->Y != y){
			Ed_ListSearch->Position->X = x;
			Ed_ListSearch->Position->Y = y;
		}
	}

	C->BeginScene();
	C->SetMatrix(PB_List->AbsoluteMatrix);

	//Background
	C->Clear(color_background_);
	C->Stroke->Color = color_line_;
	C->Stroke->Thickness = 1;
	C->Stroke->Kind = TBrushKind::Solid;
	/*
	C->DrawRect(
		RectF(0, 0, PB_List->Width, PB_List->Height),
		0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
    */

	//Items
	int last_item_index = item_index_;
	if (last_mouse_down_) {
		if (Ed_ListRename->Visible && item_index_ >= 0) {
			if (OnApplyRenameListItem) {
				OnApplyRenameListItem(item_index_, FString(AnsiString(Ed_ListRename->Text).c_str()));
			}
			last_item_index = -1;
		}
		Ed_ListRename->Visible = false;
		item_index_ = -1;
	}
	int focus_item_index = -1;
	const int cols = max_cols_ ? max_cols_ : (PB_List->Width > col_width_ ? PB_List->Width / col_width_ : 1);
	for (int ii = 0 ; ii < items_.GetSize() ; ii++) {
		ListItem &item = items_.GetAt(ii);
		item.SetPosition(0, (ii % cols) * col_width_ + col_space);
		item.SetPosition(1, (ii / cols) * row_height_ + col_space);
		item.SetPosition(2, ((ii % cols) + 1) * (col_width_ > 0 ? col_width_ : PB_List->Width) - col_space);
		item.SetPosition(3, ((ii / cols) + 1) * row_height_ - col_space);

		if (item.GetCurrentPosition(0) == 0.0f) {
			item.SetCurrentPosition(item.GetPosition());
		}

		TRectF item_rect =
			RectF(
				item.GetCurrentPosition(0),
				item.GetCurrentPosition(1) + header_height_ - current_top_,
				item.GetCurrentPosition(2),
				item.GetCurrentPosition(3) + header_height_ - current_top_);
		if (last_mouse_y_ >= header_height_ && item_rect.Contains(TPointF(last_mouse_x_, last_mouse_y_))) {
			focus_item_index = ii;
			if (last_mouse_down_) {
				item_index_ = ii;
				if (item_index_ == last_item_index && item.GetRenamable() && last_mouse_down_ < 2) {//left click
					StartRenameListItem(item_index_);
				}
			}
		}
		if (Ed_ListRename->Visible && Ed_ListRename->Tag == ii) {
			for (int ia = 0 ; ia < attributes_.GetSize() ; ia++) {
				ListUIAttribute &attribute = attributes_.GetAt(ia);
				if (attribute.GetAttributeName() == "Name"){
					Ed_ListRename->SetBounds(
						item_rect.left + attribute.GetPosition(0) - 8, item_rect.top + attribute.GetPosition(1) - 8,
						attribute.GetPosition(2) - attribute.GetPosition(0) + 16, attribute.GetPosition(3) - attribute.GetPosition(1) + 16);
					if (item_rect.top < header_height_ - 1) {
						Ed_ListRename->Visible = false;
					}
					break;
				}
			}
		}
		if (item_rect.bottom > header_height_ && item_rect.top < PB_List->Height) {
			C->Fill->Color =
				(focus_item_index == ii && color_cursor_ != color_background_) ? color_cursor_ :
					(item_index_ == ii ? color_selection_ :
						(item_index_2_ == ii ? color_selection2_ : 0));
			const int selected = (C->Fill->Color == color_selection_) && (item_index_ == ii);
			if (C->Fill->Color && (C->Fill->Color != color_background_)){
				C->FillRect(item_rect,
					0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);
			}


			if (color_line_ != color_background_){
				C->Stroke->Color = color_line_;
			}
			C->Stroke->Thickness = 1;
			C->DrawLine(
				Point(item.GetCurrentPosition(0) + default_grid_text_hspace, item.GetCurrentPosition(3) + header_height_ - current_top_),
				Point(item.GetCurrentPosition(2) - default_grid_text_hspace, item.GetCurrentPosition(3) + header_height_ - current_top_),
                1.0f);


			for (int ia = 0 ; ia < attributes_.GetSize() ; ia++) {
				ListUIAttribute &attribute = attributes_.GetAt(ia);
				FString value = item.GetAttribute(attribute.GetAttributeName());
				if (attribute.GetAttributeName().SubString(1, 1) != "*"){
					// Text attribute
					if (attribute.GetWordWrap()){
						value = value.ReplaceStr("\\n", "\n");
					}
					C->Font->Size = attribute.GetFontSize();
					C->Fill->Color = selected ? attribute.GetFontColorSelected() : attribute.GetFontColor();

					int offset = 0;
					if (value.PChar()) if (value.PChar()[0] == '*'){
						if (value.Pos("* ") == 1){
							offset = attribute.GetFontSize() * 1.5;
							value = value.SubString(3, value.Length() - 2);
						}else if (value.Pos("** ") == 1){
							offset = attribute.GetFontSize() * 2.5;
							value = value.SubString(4, value.Length() - 3);
						}
					}

					TRectF text_rect = RectF(
						item_rect.Left + attribute.GetPosition(0) + offset,
						item_rect.Top + attribute.GetPosition(1),
						item_rect.Left + attribute.GetPosition(2),
						item_rect.Top + attribute.GetPosition(3)
						);

					C->FillText(
						text_rect,
						value.PChar(),
						attribute.GetWordWrap(),
						1.0f,
						TFillTextFlags(),
						attribute.GetTextAlign(),
						TTextAlign::Leading);
				}else if (attribute.GetAttributeName() == "*PieGraph"){
					const int resolution = 120;
					double num = value.SplitBy("/").AsFloat();
					double den = value.AsFloat();
					if (num > den){
						num = den;
					}
					int rad = (num * resolution) / den;

					//PieGraph
					TRectF graph_rect = RectF(
						item_rect.Left + attribute.GetPosition(0),
						item_rect.Top + attribute.GetPosition(1),
						item_rect.Left + attribute.GetPosition(2),
						item_rect.Top + attribute.GetPosition(3)
						);
					C->Fill->Color = rad == resolution ? attribute.GetFontColorSelected() : attribute.GetFontColor();
					C->FillEllipse(graph_rect, 1.0f);

					if (rad > 0 && rad < resolution){
						C->Fill->Color = attribute.GetFontColorSelected();
						TPathData *path = new TPathData();
						const double cx = (graph_rect.left + graph_rect.right) * 0.5;
						const double cy = (graph_rect.top + graph_rect.bottom) * 0.5;
						const double r = (graph_rect.right - graph_rect.left) * 0.5;
						const double unit = 2 * 3.1415926 / resolution;
						path->MoveTo(PointF(cx, cy));
						path->LineTo(PointF(cx, graph_rect.top));
						for (int ir = 1 ; ir <= rad ; ir++){
							path->LineTo(PointF(cx + sin(ir * unit) * r, cy - cos(ir * unit) * r));
						}
						C->FillPath(path, 1.0f);
						delete path;
					}
				}else if (attribute.GetAttributeName() == "*CheckMark"){
					if (value == "true"){
						TRectF graph_rect = RectF(
							item_rect.Left + attribute.GetPosition(0),
							item_rect.Top + attribute.GetPosition(1),
							item_rect.Left + attribute.GetPosition(2),
							item_rect.Top + attribute.GetPosition(3)
							);
						C->Stroke->Color = attribute.GetFontColor();
						C->Stroke->Thickness = 3;
						TPathData *path = new TPathData();
						path->MoveTo(TPointF(graph_rect.Left + graph_rect.Width() * 0.29, graph_rect.Top + graph_rect.Height() * 0.5));
						path->LineTo(TPointF(graph_rect.Left + graph_rect.Width() * 0.42, graph_rect.Top + graph_rect.Height() * 0.67));
						path->LineTo(TPointF(graph_rect.Left + graph_rect.Width() * 0.71, graph_rect.Top + graph_rect.Height() * 0.42));
                        C->DrawPath(path, 1.0f);
						delete path;
					}
				}
			}
		}
	}
	if (last_mouse_down_){
		click_item_index_ = item_index_;
	}
	if (item_index_ < 0) {
		SelectDefaultItem();
	}
	if (last_item_index != item_index_) {
		if (OnItemChanged) {
			OnItemChanged(item_index_);
		}
		internal_update_count_++;
	}

	if (popup_menu_ && last_mouse_down_ == 2){
		popup_requested_ = true;
	}
	last_mouse_down_ = false;

	//Header (List Name)
	C->Fill->Color = color_background_;
	C->FillRect(
		RectF(0, 0, PB_List->Width, header_height_),
		0.0f, 0.0f, AllCorners, 1.0f, TCornerType::Round);

	C->Fill->Color = SDEEP_CONSOLE_COLOR_LIGHTFG;
	C->Fill->Kind = TBrushKind::Solid;
	C->Font->Size = 16;
	C->FillText(
		RectF(16, default_grid_text_vspace, PB_List->Width - default_grid_text_hspace, header_height_ - default_grid_text_vspace),
		name_,
		false,
		1.0f,
		TFillTextFlags(),
		TTextAlign::Leading,
		TTextAlign::Center);

	/*
	C->Stroke->Color = SDEEP_CONSOLE_COLOR_FOREGROUND;
	C->Stroke->Thickness = 1;

	C->DrawLine(
		Point(0, header_height_),
		Point(PB_List->Width, header_height_),
		1.0f);
	*/

	if (Ed_ListSearch->Visible) {
		static const int icon_size = 27;
		C->DrawBitmap(
			Im_Search->Bitmap,
			RectF(0, 0, Im_Search->Bitmap->Width, Im_Search->Bitmap->Height),
			RectF(
				Ed_ListSearch->Position->X - icon_size - 8,
				Ed_ListSearch->Position->Y + (Ed_ListSearch->Height - icon_size) / 2,
				Ed_ListSearch->Position->X - 8,
				Ed_ListSearch->Position->Y + (Ed_ListSearch->Height + icon_size) / 2),
			1.0,
			false);
	}

	C->EndScene();
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::Ed_ListRenameKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	switch (Key) {
	case 13://Enter
		Ed_ListRename->Tag = 1;
		Ed_ListRename->Visible = false;
		if (OnApplyRenameListItem) {
			OnApplyRenameListItem(item_index_, FString(AnsiString(Ed_ListRename->Text).c_str()));
		}
		Ed_ListRename->Tag = 0;
		Bu_List->SetFocus();
		break;
	case 27://Esc
		Ed_ListRename->Tag = 1;
		Ed_ListRename->Visible = false;
		Ed_ListRename->Tag = 0;
		Bu_List->SetFocus();
		break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::PB_ListClick(TObject *Sender)
{
	Sc_ListY->Tag = 0;
	PB_List->Repaint();
	Application->ProcessMessages();//need to update item_index
	if (click_item_index_ >= 0 && click_item_index_ < items_.GetSize() && click_item_index_ == item_index_) {
		if (OnItemClick) {
			OnItemClick(item_index_);
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::PB_ListDblClick(TObject *Sender)
{
	Ed_ListRename->Visible = false;
	last_mouse_down_ = false;
	Application->ProcessMessages();//need to update item_index
	if (click_item_index_ >= 0 && click_item_index_ < items_.GetSize()) {
		if (OnItemDblClick) {
			OnItemDblClick(click_item_index_);
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::PB_ListMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
		  float X, float Y)
{
	SetFocus();
	if (!Ed_ListRename->Visible){
		Bu_List->SetFocus();
	}
	last_mouse_x_ = X;
	last_mouse_y_ = Y;
	if (Y > header_height_) {
		last_mouse_down_ = Button == TMouseButton::mbRight ? 2 : 1;
		internal_update_count_++;
		if (OnMouseDown) {
			OnMouseDown(Button, Shift, X, Y);
		}
	}else {
        click_item_index_ = -1;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::PB_ListMouseMove(TObject *Sender, TShiftState Shift, float X,
		  float Y)
{
	if (wheel_enabled_){
		SetFocus();
		if (!Ed_ListRename->Visible && !Ed_ListSearch->IsFocused){
			Bu_List->SetFocus();
		}
	}
	last_mouse_x_ = X;
	last_mouse_y_ = Y;
	internal_update_count_++;
	if (OnMouseMove) {
		OnMouseMove(TMouseButton(), Shift, X, Y);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::PB_ListMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift,
		  float X, float Y)
{
	if (OnMouseUp) {
		OnMouseUp(Button, Shift, X, Y);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::PB_ListMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
		  bool &Handled)
{
	if (!wheel_enabled_){
		return;
	}

	const int cols = max_cols_ ? max_cols_ : (PB_List->Width > col_width_ ? PB_List->Width / col_width_ : 1);
	if (Sc_ListY->Visible) {
		//Scroll
		const int rows = (items_.GetSize() - 1) / cols + 1;
		const int scroll_amount = rows < Sc_ListY->Max * 2 ? (Sc_ListY->Max * 2) / rows : 2;
		int scroll_y = Sc_ListY->Value + scroll_amount * (WheelDelta < 0 ? 1 : -1);
		if (scroll_y < Sc_ListY->Min) {
			scroll_y = Sc_ListY->Min;
		}
		if (scroll_y > Sc_ListY->Max) {
			scroll_y = Sc_ListY->Max;
		}
		Sc_ListY->Value = scroll_y;
	}else {
        //Move Item
		Sc_ListY->Tag = 0;

		int last_item_index = item_index_;
		if (WheelDelta < 0) {
			item_index_ += cols;
		}
		if (WheelDelta > 0) {
			item_index_ -= cols;
		}
		if (item_index_ < 0) {
			item_index_ = 0;
		}
		if (item_index_ > items_.GetSize() - 1) {
			item_index_ = items_.GetSize() - 1;
		}
		internal_update_count_++;
		if (OnItemChanged && last_item_index != item_index_) {
			OnItemChanged(item_index_);
		}
		PB_List->Repaint();
	}
	Handled = true;
}
//---------------------------------------------------------------------------
void TFr_List::UIUpdate()
{
	if (settings_){
		sdeep_console_transition_coef_ = settings_->GetGUITransition() ? 0.2 : 1.0;
	}

    La_Search->Visible = Ed_ListSearch->Text == "" && !Ed_ListSearch->IsFocused;

	double sum_diff = 0.0;

	if (update_count_ != internal_update_count_) {
		update_count_ = internal_update_count_;
		sum_diff += 1.0;
	}
	for (int ii = 0 ; ii < items_.GetSize() ; ii++) {
		ListItem &item = items_.GetAt(ii);
		for (int id = 0 ; id < 4 ; id++) {
			const double last_position = item.GetCurrentPosition(id);
			item.SetCurrentPosition(
				id,
				(1.0 - sqrt(sdeep_console_transition_coef_)) * last_position +
				sqrt(sdeep_console_transition_coef_) * item.GetPosition(id));
			sum_diff += fabs(last_position - item.GetCurrentPosition(id));
		}
	}

	const int cols = max_cols_ ? max_cols_ : (PB_List->Width > col_width_ ? PB_List->Width / col_width_ : 1);
	if (Sc_ListY->Tag == 0) {
		if (item_index_ >= 0 && item_index_ < items_.GetSize()) {
			ListItem &item = items_.GetAt(item_index_);
			if (item.GetPosition(0) != item.GetPosition(2) && item.GetPosition(1) != item.GetPosition(3)) {
				if (top_ + PB_List->Height < item.GetPosition(3) + header_height_) {
					top_ = item.GetPosition(3) + header_height_ - PB_List->Height;
				}
				if (top_ > item.GetPosition(1)) {
					top_ = item.GetPosition(1);
				}
			}
		}
		const double last_top = current_top_;
		current_top_ = (1.0 - sdeep_console_transition_coef_) * current_top_ + sdeep_console_transition_coef_ * top_;
		sum_diff += fabs(last_top - current_top_);
	}

	int sc_y_visible = false;
	const int rows = (items_.GetSize() - 1) / cols + 1;
	const double view_size = rows * row_height_;
	if (view_size > PB_List->Height - header_height_) {
		if (Sc_ListY->Tag == 0) {
			Sc_ListY->Tag = -1;
			Sc_ListY->ViewportSize = (PB_List->Height - header_height_) * Sc_ListY->Max / view_size;
			Sc_ListY->SmallChange = Sc_ListY->ViewportSize;
			Sc_ListY->Value = (top_ * Sc_ListY->Max) / view_size;

			Sc_ListY->Tag = 0;
		}
		sc_y_visible = true;
	}
	if (Sc_ListY->Visible != sc_y_visible) {
		Sc_ListY->Visible = sc_y_visible;
		if (!sc_y_visible) {
			top_ = 0.0;
			Sc_ListY->Value = 0;
		}
	}
	if (sum_diff > 0.5) {
		PB_List->Repaint();
	}
	if (popup_requested_){
		popup_requested_ = false;
		TPointF screen_pos = LocalToScreen(TPointF(last_mouse_x_, last_mouse_y_));
		popup_menu_->Popup(screen_pos.X, screen_pos.Y);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::Bu_ListKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	int refresh = false;
	Sc_ListY->Tag = 0;
	const int last_item_index = item_index_;
	switch (Key) {
	case 13://Enter
        click_item_index_ = item_index_;
		if (enter_mode_){
			PB_ListDblClick(Sender);
		}else {
			PB_ListClick(Sender);
		}
		Key = 0;
		break;
	case 27://Esc
		break;
	case 33://PgUp
	case 34://PgDown
		{
			const int cols = max_cols_ ? max_cols_ : (PB_List->Width > col_width_ ? PB_List->Width / col_width_ : 1);
			const int rows = (PB_List->Height - header_height_) / row_height_;
			switch (Key) {
			case 33:
				//PgUp
				item_index_ -= cols * rows;
				if (item_index_ < 0) {
					item_index_ = 0;
				}
				refresh = true;
				break;
			case 34:
				//PgDown
				item_index_ += cols * rows;
				if (item_index_ >= items_.GetSize() - 1) {
					item_index_ = items_.GetSize() - 1;
				}
				refresh = true;
				break;
			}
		}
		Key = 0;
		break;
	case 35:
		//End
		item_index_ = items_.GetSize() - 1;
		refresh = true;
		break;
	case 36:
		//Home
		item_index_ = items_.GetSize() ? 0 : -1;
		refresh = true;
		break;
	case 37://Left
	case 38://Up
	case 39://Right
	case 40://Down
		switch (Key) {
		case 37://Left
			item_index_--;
			if (item_index_ < 0) {
				item_index_ = 0;
			}
			refresh = true;
			break;
		case 38://Up
			{
				const int cols = max_cols_ ? max_cols_ : (PB_List->Width > col_width_ ? PB_List->Width / col_width_ : 1);
				item_index_ -= cols;
				if (item_index_ < 0) {
					item_index_ = 0;
				}
			}
			refresh = true;
			break;
		case 39://Right
			item_index_++;
			if (item_index_ >= items_.GetSize() - 1) {
				item_index_ = items_.GetSize() - 1;
			}
			refresh = true;
			break;
		case 40://Down
			{
				const int cols = max_cols_ ? max_cols_ : (PB_List->Width > col_width_ ? PB_List->Width / col_width_ : 1);
				item_index_ += cols;
				if (item_index_ >= items_.GetSize() - 1) {
					item_index_ = items_.GetSize() - 1;
				}
			}
			refresh = true;
		}
		break;
	case 46:
		//Delete
		if (item_index_ >= 0 && item_index_ < items_.GetSize() && OnItemDelete) {
			OnItemDelete(item_index_);
		}
		Key = 0;
		break;
	case 48://Ctrl + 0
	case 96://Ctrl + 0
		break;
	case 107://Ctrl + +
	case 187://Ctrl + +
		break;
	case 109://Ctrl + -
	case 189://Ctrl + -
		break;
	case 113://F2(Rename)
		StartRenameListItem(item_index_);
		refresh = true;
		break;
	}
	if (OnItemChanged && last_item_index != item_index_) {
		OnItemChanged(item_index_);
	}
	if (refresh) {
		last_mouse_x_ = -1;
		last_mouse_y_ = -1;
		internal_update_count_++;
		PB_List->Repaint();
		Key = 0;
	}
}
//---------------------------------------------------------------------------
int TFr_List::StartRenameListItem(int item_index)
{
	if (item_index < 0 || item_index >= items_.GetSize()) {
		return false;
	}

	//Rename
	ListItem &item = items_.GetAt(item_index);
	if (item.GetRenamable()) {
		Ed_ListRename->Visible = true;
		Ed_ListRename->Text = item.GetAttributeValue(0).PChar();
		Ed_ListRename->Tag = item_index;
		Ed_ListRename->SelectAll();
		Ed_ListRename->SetFocus();
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------
void TFr_List::ResetScroll()
{
	Sc_ListY->Tag = 1;
	Sc_ListY->Value = 0;
	Sc_ListY->Tag = 0;
	last_mouse_x_ = -1.0f;
	last_mouse_y_ = -1.0f;
	PB_List->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::Sc_ListYChange(TObject *Sender)
{
	const int cols = max_cols_ ? max_cols_ : (PB_List->Width > col_width_ ? PB_List->Width / col_width_ : 1);
	const int rows = (items_.GetSize() - 1) / cols + 1;
	if (Sc_ListY->Tag >= 0 && rows) {
		Sc_ListY->Tag = 1;
		const double view_size = rows * row_height_;
		current_top_ = top_ = (Sc_ListY->Value * view_size) / Sc_ListY->Max;
		last_mouse_x_ = -1.0f;
		last_mouse_y_ = -1.0f;
		PB_List->Repaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::Sc_ListYExit(TObject *Sender)
{
	Sc_ListY->Tag = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFr_List::Bu_MenuClick(TObject *Sender)
{
	if (popup_menu_) {
		TPointF screen_pos = LocalToScreen(TPointF(Bu_Menu->Position->X, Bu_Menu->Position->Y + Bu_Menu->Height));
		popup_menu_->Popup(screen_pos.X, screen_pos.Y);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFr_List::Ed_ListSearchChangeTracking(TObject *Sender)
{
	if (Ed_ListSearch->Visible) {
		if (OnSearchTextChanged) {
			OnSearchTextChanged();
		}
	}
}
//---------------------------------------------------------------------------


void __fastcall TFr_List::Ed_ListSearchKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	if (Key == 27){
		//Esc
		Ed_ListSearch->Text = "";
	}else if (Key == 13){
		if (OnSearch) {
			OnSearch();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFr_List::Ed_ListRenameExit(TObject *Sender)
{
	if (Ed_ListRename->Tag == 0) {
		WORD key = 13;
		System::WideChar c;
		Ed_ListRenameKeyDown(Sender, key, c, TShiftState());
	}
}
//---------------------------------------------------------------------------

