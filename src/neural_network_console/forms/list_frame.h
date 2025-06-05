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

#ifndef list_frameH
#define list_frameH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Ani.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.Menus.hpp>
#include <FMX.Objects.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
//---------------------------------------------------------------------------
#include "nn_document.h"
#include "sdeep_console_global_settings.h"
//---------------------------------------------------------------------------
using namespace elfe;
using namespace sdeep_console;
//---------------------------------------------------------------------------
class ListItem
{
public:
	ListItem() : original_index_(-1), removable_(true), renamable_(false) {
		memset(&position_, 0, sizeof(position_));
		memset(&current_position_, 0, sizeof(current_position_));
	};
	virtual ~ListItem() {};

	ListItem &operator = (const ListItem &source){
		attributes_ = source.attributes_;
		values_ = source.values_;
		original_index_ = source.original_index_;
		memcpy(position_, source.position_, sizeof(position_));
		memcpy(current_position_, source.current_position_, sizeof(current_position_));
		removable_ = source.removable_;
		renamable_ = source.renamable_;
		return *this;
	};
private:
	ListItem(const ListItem &source);
public:
	void AddAttribute(const FString attribute, const FString value) {
		attributes_.Add(attribute);
		values_.Add(value);
	};
	FString GetAttribute(const FString attribute) const {
		const int index = attributes_.IndexOf(attribute);
		if (index >= 0) {
			return values_.GetAt(index);
		}else {
			return FString("");
		}
	};
	int GetNumAttribute() const {return attributes_.GetSize();};
	FString &GetAttributeName(int index) const {return attributes_.GetAt(index);};
	FString &GetAttributeValue(int index) const {return values_.GetAt(index);};
	void SetOriginalIndex(int original_index) {original_index_ = original_index;};
	int GetOriginalIndex() const {return original_index_;};

	void SetPosition(int index, float position) {position_[index] = position;};
	float GetPosition(int index) const {return position_[index];};
	const float *GetPosition() const {return position_;};
	void SetCurrentPosition(int index, float current_position) {current_position_[index] = current_position;};
	void SetCurrentPosition(const float *current_position) {memcpy(current_position_, current_position, sizeof(current_position_));};
	float GetCurrentPosition(int index) const {return current_position_[index];};

	void SetRemovable(int removable) {removable_ = removable;};
	int GetRemovable() const {return removable_;};

	void SetRenamable(int renamable) {renamable_ = renamable;};
	int GetRenamable() const {return renamable_;};

	int Contains(const FString &search_text) const;
private:
	FStringList attributes_;
	FStringList values_;
	int original_index_;
	float position_[4];
	float current_position_[4];

	int removable_;
	int renamable_;
};
//---------------------------------------------------------------------------
class ListItems : public FListP<ListItem>
{
public:
	ListItems() {};
	virtual ~ListItems() {};

	ListItems &operator = (const ListItems &source){
		FListP<ListItem>::operator=(source);
		file_name_ = source.file_name_;
		return *this;
	};
private:
	ListItems(const ListItems &source);
public:
	void AddListItem(FString key_attribute, const ListItem &list_item);

	void AllocateOriginalIndex();

	void SetFileName(const char *file_name) {file_name_ = file_name;};
	void SaveToFile();
	void LoadFromFile();
private:
	FString file_name_;
};
//---------------------------------------------------------------------------
class ListUIAttribute
{
public:
	ListUIAttribute() {
		memset(&position_, 0, sizeof(position_));
		font_color_ = 0xffffffff;
		font_color_selected_ = 0xffffffff;
		font_size_ = 16;
		word_wrap_ = false;
		text_align_ = TTextAlign::Leading;
	}
	ListUIAttribute &operator =(const ListUIAttribute &source){
		attribute_name_ = source.attribute_name_;
		memcpy(position_, source.position_, sizeof(position_));
		font_color_ = source.font_color_;
		font_color_selected_ = source.font_color_selected_;
		font_size_ = source.font_size_;
		word_wrap_ = source.word_wrap_;
		text_align_ = source.text_align_;
		return *this;
	}
	virtual ~ListUIAttribute() {};
private:
	ListUIAttribute(const ListUIAttribute &source);
public:
	void SetAttributeName(FString attribute_name) {attribute_name_ = attribute_name;}
	const FString &GetAttributeName() const {return attribute_name_;}
	void SetPosition(int index, float position) {position_[index] = position;}
	float GetPosition(int index) const {return position_[index];}
	void SetFontColors(int font_color) {font_color_ = font_color; font_color_selected_ = font_color;}
	int GetFontColor() const {return font_color_;}
	void SetFontColorSelected(int font_color_selected) {font_color_selected_ = font_color_selected;}
	int GetFontColorSelected() const {return font_color_selected_;}
	void SetFontSize(int font_size) {font_size_ = font_size;}
	int GetFontSize() const {return font_size_;}
	void SetWordWrap(int word_wrap) {word_wrap_ = word_wrap;}
	int GetWordWrap() const {return word_wrap_;}
	void SetTextAlign(TTextAlign text_align) {text_align_ = text_align;}
	TTextAlign GetTextAlign() const {return text_align_;}
private:
	FString attribute_name_;
	float position_[4];
	int font_color_, font_color_selected_;
	int font_size_;
	int word_wrap_;
	TTextAlign text_align_;
};
//---------------------------------------------------------------------------
typedef void (__closure *ListFrameCallback)();
typedef void (__closure *ListFrameItemCallback)(int);
typedef void (__closure *ListFrameRenameItemCallback)(int, FString);
typedef void (__closure *ListFrameMouseCallback)(TMouseButton, TShiftState, float, float);
//---------------------------------------------------------------------------
class TFr_List : public TFrame
{
__published:	// IDE �ŊǗ������R���|�[�l���g
	TPaintBox *PB_List;
	TEdit *Ed_ListSearch;
	TButton *Bu_List;
	TEdit *Ed_ListRename;
	TButton *Bu_Menu;
	TImage *Im_Menu;
	TScrollBar *Sc_ListY;
	TImage *Im_Search;
	TLabel *La_Search;
	TLine *Li_TopLine;
	void __fastcall PB_ListPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall Ed_ListRenameKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall PB_ListClick(TObject *Sender);
	void __fastcall PB_ListDblClick(TObject *Sender);
	void __fastcall PB_ListMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          float X, float Y);
	void __fastcall PB_ListMouseMove(TObject *Sender, TShiftState Shift, float X, float Y);
	void __fastcall PB_ListMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
          bool &Handled);
	void __fastcall Bu_ListKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall Sc_ListYChange(TObject *Sender);
	void __fastcall Sc_ListYExit(TObject *Sender);
	void __fastcall Bu_MenuClick(TObject *Sender);
	void __fastcall Ed_ListSearchChangeTracking(TObject *Sender);
	void __fastcall PB_ListMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift,
          float X, float Y);
	void __fastcall Ed_ListSearchKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall Ed_ListRenameExit(TObject *Sender);

private:	// ���[�U�[�錾
public:		// ���[�U�[�錾
	explicit __fastcall TFr_List(TComponent* Owner);

	//Setup
	void Setup(SDeepConsoleGlobalSettings *settings,
	           UnicodeString name, int max_cols, int col_width, int row_height, int header_height,
			   int allow_not_selected, int search_enabled, int search_align_right, FListP<ListUIAttribute> attributes) {
		settings_ = settings;
		name_ = name;
		max_cols_ = max_cols;
		col_width_ = col_width;
		row_height_ = row_height;
		header_height_ = header_height;
		allow_not_selected_ = allow_not_selected;
		attributes_ = attributes;
		search_align_right_ = search_align_right;
		Ed_ListSearch->Visible = search_enabled;
	}
	void SetMenu(TPopupMenu *menu){
		popup_menu_ = menu;
		Bu_Menu->Visible = menu != NULL;
	}
	void SetColor(int background, int cursor, int selection, int selection2, int line)
	{
		color_background_ = background;
		color_cursor_ = cursor;
		color_selection_ = selection;
		color_selection2_ = selection2;
		color_line_ = line;
	}
	void SetEnterMode(int dbl_click){
		enter_mode_ = dbl_click;
	}
	void SetWheelEnabled(int enabled){
		wheel_enabled_ = enabled;
	}

	//Set list
	void SetItems(ListItems &items) {
		if (items.GetSize() != items_.GetSize() && Ed_ListRename->Visible){
            Ed_ListRename->Visible = false;
		}
		items_ = items;
		if (item_index_ < 0 || item_index_ >= items_.GetSize()) {
			SelectDefaultItem();
		}
		internal_update_count_++;
	}

	//UI Update
	void UIUpdate();

	//Setter, Getter
	int GetItemIndex() {return item_index_;}
	void SetItemIndex(int item_index) {
		if (item_index >= 0 && item_index < items_.GetSize()) {
			item_index_ = item_index;
		}else {
			SelectDefaultItem();
		}
		last_mouse_x_ = -1;
		last_mouse_y_ = -1;
		internal_update_count_++;
	}
	int GetItemIndex2() {return item_index_2_;}
	void SetItemIndex2(int item_index_2) {
		if (item_index_2 >= 0 && item_index_2 < items_.GetSize()) {
			item_index_2_ = item_index_2;
		}else {
			item_index_2_ = -1;;
		}
	}
	void SelectDefaultItem(){
		if (allow_not_selected_) {
			item_index_ = -1;
		}else {
			item_index_ = items_.GetSize() ? 0 : -1;
		}
	}

	const ListItems &GetItems(){
		return items_;
	}

	const ListItem &GetItem(int index) const {
		return items_.GetAt(index);
	}
private:
	//Setup
	SDeepConsoleGlobalSettings *settings_;
	float sdeep_console_transition_coef_;
	UnicodeString name_;
	FListP<ListUIAttribute> attributes_;
	int max_cols_;
	int col_width_;
	int row_height_;
	int header_height_;
	int allow_not_selected_;
    int search_align_right_;
	TPopupMenu *popup_menu_;

	int enter_mode_;
    int wheel_enabled_;

	//Color
	int color_background_;
	int color_cursor_;
	int color_selection_;
	int color_selection2_;
	int color_line_;

	//Items
	ListItems items_;

	//Current
	double top_;
	double current_top_;
	int item_index_;
	int item_index_2_;
	int click_item_index_;//always allow -1 (not selected)
	int last_mouse_down_;
	float last_mouse_x_;
	float last_mouse_y_;

	int internal_update_count_;
	int update_count_;
    int popup_requested_;

public:
	int StartRenameListItem(int item_index);
	void ResetScroll();

public:
	//Callbacks
	ListFrameRenameItemCallback OnApplyRenameListItem;
	ListFrameItemCallback OnItemChanged;
	ListFrameItemCallback OnItemClick;
	ListFrameItemCallback OnItemDblClick;
	ListFrameItemCallback OnItemDelete;
	ListFrameCallback OnSearchTextChanged;
	ListFrameCallback OnSearch;
	ListFrameMouseCallback OnMouseDown;
	ListFrameMouseCallback OnMouseMove;
	ListFrameMouseCallback OnMouseUp;
};
//---------------------------------------------------------------------------
extern PACKAGE TFr_List *Fr_List;
//---------------------------------------------------------------------------
#endif
