// Copyright 2019,2020,2021 Sony Corporation.
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

#include "sdeep_console_plugin_form.h"
#include "translation.h"
#include "ffastini.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TFo_Plugin *Fo_Plugin;

using namespace elfe;
//---------------------------------------------------------------------------
__fastcall TFo_Plugin::TFo_Plugin(TComponent* Owner)
	: TForm(Owner), gui_width_(640), gui_height_(240)
{
}
//---------------------------------------------------------------------------
void TFo_Plugin::PrepareGUI(const NNCPlugin &plugin)
{
	plugin_ = plugin;
	controls_.Clear();

	Caption = plugin.name_.PChar();

	Me_Description->Text = plugin.description_.ReplaceStr("\\n","\n").PChar();

	La_NameAndDescription->Visible = false;
	Ed_Value->Visible = false;
	Bu_Browse->Visible = false;

	static const int max_rows = 8;
    static const int col_width = 640;
	const int cols = (plugin.arg_option_.GetSize() - 1) / max_rows + 1;
	const int rows = (plugin.arg_option_.GetSize() + cols - 1) / cols;

	FStringList arg_default = RestoreLastArgs();

    int taborder = 1;
	for (int i = 0 ; i < plugin.arg_option_.GetSize() ; i++){
		const int col = i / rows;
        const int row = i % rows;
		TLabel *label = new TLabel(Fo_Plugin);
		label->Text = (plugin.arg_name_.GetAt(i) + FString(" - ") + plugin.arg_description_.GetAt(i)).PChar();
		label->Position->X = La_NameAndDescription->Position->X + col * col_width;
		label->Position->Y = La_NameAndDescription->Position->Y + row * 64;
		label->Width = La_NameAndDescription->Width;
		label->Height = La_NameAndDescription->Height;
		label->TextSettings->Font->Size = La_NameAndDescription->TextSettings->Font->Size;
		label->Parent = this;
		label->Visible = true;
		const FString lower_desc = plugin.arg_description_.GetAt(i).ToLower();
		if (lower_desc.Pos("(bool)")){
			//Boolean option
			TCheckBox *check_box = new TCheckBox(this);
			check_box->Text = plugin.arg_name_.GetAt(i).PChar();
			check_box->IsChecked = arg_default.GetAt(i).ToLower() == "true";
			check_box->Position->X = Ed_Value->Position->X + col * col_width;
			check_box->Position->Y = Ed_Value->Position->Y + row * 64;
			check_box->Width = Ed_Value->Width;
			check_box->Height = Ed_Value->Height;
			check_box->Parent = this;
			check_box->TabStop = true;
			check_box->TabOrder = taborder++;
			check_box->Visible = true;
			controls_.Add(check_box);
		}else if (lower_desc.Pos("(option:")){
			FString options = lower_desc;
			options.SplitBy("(option:");
			options = options.SplitBy(")");
			FStringList option_list;
			option_list.SplitStr(options, FString(" "), FString(""));
			TLayout *layout = new TLayout(this);
			layout->Position->X = Ed_Value->Position->X + col * col_width;
			layout->Position->Y = Ed_Value->Position->Y + row * 64;
			layout->Width = Ed_Value->Width;
			layout->Height = Ed_Value->Height;
			layout->Parent = this;
			layout->Visible = true;
			layout->TabStop = true;
			layout->TabOrder = taborder++;
			for (int i2 = 0 ; i2 < option_list.GetSize() ; i2++){
				const FString option = option_list.GetAt(i2);
				TRadioButton *rb = new TRadioButton(layout);
				rb->Text = option.PChar();
				rb->GroupName = plugin.arg_name_.GetAt(i).PChar();
				if (option == arg_default.GetAt(i)) {
					rb->IsChecked = true;
				}
				rb->Width = (col_width - Ed_Value->Position->X * 2) / (option_list.GetSize() > 3 ? option_list.GetSize() : 3);
				rb->Position->X = 10000;
				rb->Align = TAlignLayout::Left;
				rb->Parent = layout;
				rb->TabStop = true;
				rb->TabOrder = i2;
				rb->Visible = true;
			}
			controls_.Add(layout);
		}else if (lower_desc.Pos("(msoption:")){
			FString options = lower_desc;
			options.SplitBy("(msoption:");
			options = options.SplitBy(")");
			FStringList option_list;
			option_list.SplitStr(options, FString(" "), FString(""));
			TLayout *layout = new TLayout(this);
			layout->Position->X = Ed_Value->Position->X + col * col_width;
			layout->Position->Y = Ed_Value->Position->Y + row * 64;
			layout->Width = Ed_Value->Width;
			layout->Height = Ed_Value->Height;
			layout->Parent = this;
			layout->Visible = true;
			layout->TabStop = true;
			layout->TabOrder = taborder++;
			for (int i2 = 0 ; i2 < option_list.GetSize() ; i2++){
				const FString option = option_list.GetAt(i2);
				TCheckBox *cb = new TCheckBox(layout);
				cb->Text = option.PChar();
				cb->IsChecked = arg_default.GetAt(i).Pos(option) > 0;
				cb->Width = (col_width - Ed_Value->Position->X * 2) / (option_list.GetSize() > 3 ? option_list.GetSize() : 3);
				cb->Position->X = 10000;
				cb->Align = TAlignLayout::Left;
				cb->Parent = layout;
				cb->TabStop = true;
				cb->TabOrder = i2;
				cb->Visible = true;
			}
			controls_.Add(layout);
		}else {
			//Text
			const int need_browse =
				lower_desc.Pos("(image)") ||
				lower_desc.Pos("(csv)") ||
				lower_desc.Pos("(file)") ||
				lower_desc.Pos("(dir)");
			TEdit *edit = new TEdit(this);
			edit->Text = arg_default.GetAt(i).PChar();
			edit->Position->X = Ed_Value->Position->X + col * col_width;
			edit->Position->Y = Ed_Value->Position->Y + row * 64;
			edit->Width = need_browse ? Ed_Value->Width - 8 - Bu_Browse->Width : Ed_Value->Width;
			edit->Height = Ed_Value->Height;
			edit->Parent = this;
			edit->TabStop = true;
			edit->TabOrder = taborder++;
			edit->Visible = true;
			if (i == 0) {
                edit->SetFocus();
			}
			controls_.Add(edit);

			if (need_browse){
				TButton *button = new TButton(this);
				button->Text = Bu_Browse->Text;
				button->Position->X = Bu_Browse->Position->X + col * col_width;
				button->Position->Y = Bu_Browse->Position->Y + row * 64;
				button->Width = Bu_Browse->Width;
				button->Height = Bu_Browse->Height;
				button->StyleLookup = Bu_Browse->StyleLookup;
				button->OnClick = Bu_Browse->OnClick;
				button->Parent = this;
				button->TabStop = true;
				button->TabOrder = taborder++;
				button->Visible = true;
				button->Tag = i;
			}
		}
	}

	gui_width_ = col_width * cols;
	gui_height_ = La_NameAndDescription->Position->Y + rows * 64 + Lo_OKCancel->Height;
}
//---------------------------------------------------------------------------
void TFo_Plugin::SetImageArgument(FString image_file_name)
{
	for (int i = 0 ; i < plugin_.arg_name_.GetSize() ; i++){
		if (plugin_.arg_name_.GetAt(i) == "image" || plugin_.arg_name_.GetAt(i) == "input-data"){
			((TEdit*)controls_.GetAt(i))->Text = image_file_name.PChar();
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Plugin::SetDefaultArgument(FString sub_string_in_description, FString value)
{
	for (int i = 0 ; i < plugin_.arg_name_.GetSize() ; i++){
		if (plugin_.arg_description_.GetAt(i).Pos(sub_string_in_description)){
			((TEdit*)controls_.GetAt(i))->Text = value.PChar();
		}
	}
}
//---------------------------------------------------------------------------
void TFo_Plugin::SetSampleDatasetDir(UnicodeString sample_dataset_dir)
{
	for (int i = 0 ; i < plugin_.arg_name_.GetSize() ; i++){
		if (plugin_.arg_name_.GetAt(i) == "output-dir" || plugin_.arg_name_.GetAt(i) == "output_dir"){
			((TEdit*)controls_.GetAt(i))->Text = sample_dataset_dir + "\\" + plugin_.arg_default_.GetAt(i).PChar();
		}
	}
}
//---------------------------------------------------------------------------
AnsiString TFo_Plugin::GetArgs()
{
	AnsiString result;
	for (int i = 0 ; i < plugin_.arg_option_.GetSize() ; i++){
		const int is_bool = plugin_.arg_description_.GetAt(i).ToLower().Pos("(bool)");
		const int is_option = plugin_.arg_description_.GetAt(i).ToLower().Pos("(option:");
		const int is_msoption = plugin_.arg_description_.GetAt(i).ToLower().Pos("(msoption:");
		AnsiString s;
		if (is_bool){
			if (!((TCheckBox*)controls_.GetAt(i))->IsChecked){
				continue;
			}
		}else if (is_option){
			TLayout *layout = ((TLayout*)controls_.GetAt(i));
			for (int ic = 0 ; ic < layout->ComponentCount ; ic++) {
				if (layout->Components[ic]->ClassName() == "TRadioButton") {
					TRadioButton *rb = (TRadioButton*)layout->Components[ic];
					if (rb->IsChecked) {
						s = rb->Text;
					}
				}
			}
		}else if (is_msoption){
			TLayout *layout = ((TLayout*)controls_.GetAt(i));
			for (int ic = 0 ; ic < layout->ComponentCount ; ic++) {
				if (layout->Components[ic]->ClassName() == "TCheckBox") {
					TCheckBox *cb = (TCheckBox*)layout->Components[ic];
					if (cb->IsChecked) {
						if (s != "") {
							s += ",";
						}
						s += cb->Text;
					}
				}
			}
			if (s == "") {
                continue;
			}
		}else {
			s = ((TEdit*)controls_.GetAt(i))->Text;
			if (s == ""){
				continue;
			}
		}
		if (result != ""){
			result += " ";
		}
		result += "-";
		result += plugin_.arg_option_.GetAt(i).PChar();
		if (!is_bool){
			if (s.Pos(" ")){
				result += AnsiString(" \"") + s + AnsiString("\"");
			}else {
				result += AnsiString(" ") + s;
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------
void TFo_Plugin::Reset()
{
    // Reset to default settings

	for (int i = 0 ; i < plugin_.arg_option_.GetSize() ; i++){
		const int is_bool = plugin_.arg_description_.GetAt(i).ToLower().Pos("(bool)");
		const int is_option = plugin_.arg_description_.GetAt(i).ToLower().Pos("(option:");
		const int is_msoption = plugin_.arg_description_.GetAt(i).ToLower().Pos("(msoption:");
		AnsiString s;
		if (is_bool){
			// Checkbox
			((TCheckBox*)controls_.GetAt(i))->IsChecked = plugin_.arg_default_.GetAt(i).ToLower() == "true";
		}else if (is_option){
			// Radiobox
			TLayout *layout = ((TLayout*)controls_.GetAt(i));
			for (int ic = 0 ; ic < layout->ComponentCount ; ic++) {
				if (layout->Components[ic]->ClassName() == "TRadioButton") {
					TRadioButton *rb = (TRadioButton*)layout->Components[ic];
					if (rb->Text == plugin_.arg_default_.GetAt(i).PChar()) {
						rb->IsChecked = true;
					}
				}
			}
		}else if (is_msoption){
			// CheckBox
			TLayout *layout = ((TLayout*)controls_.GetAt(i));
			for (int ic = 0 ; ic < layout->ComponentCount ; ic++) {
				if (layout->Components[ic]->ClassName() == "TCheckBox") {
					TCheckBox *cb = (TCheckBox*)layout->Components[ic];
					cb->IsChecked = plugin_.arg_default_.GetAt(i).Pos(AnsiString(cb->Text).c_str()) > 0;
				}
			}
		}else {
			// Text
			((TEdit*)controls_.GetAt(i))->Text = plugin_.arg_default_.GetAt(i).PChar();
		}
	}
}
//---------------------------------------------------------------------------
AnsiString TFo_Plugin::GetOutputDir()
{
	for (int i = 0 ; i < plugin_.arg_option_.GetSize() ; i++){
		if (plugin_.arg_name_.GetAt(i).ToLower() == "output-dir" ||
			plugin_.arg_name_.GetAt(i).ToLower() == "output_dir"){
			return ((TEdit*)controls_.GetAt(i))->Text;
		}
	}
	return "";
}
//---------------------------------------------------------------------------
AnsiString TFo_Plugin::GetOutputFileName()
{
	for (int i = 0 ; i < plugin_.arg_option_.GetSize() ; i++){
		if (plugin_.arg_name_.GetAt(i).ToLower() == "output"){
			return ((TEdit*)controls_.GetAt(i))->Text;
		}
	}
	return "";
}
//---------------------------------------------------------------------------
FStringList TFo_Plugin::GetDatasetFileNames()
{
	FStringList result;
	for (int i = 0 ; i < plugin_.arg_option_.GetSize() ; i++){
		if (plugin_.arg_name_.GetAt(i).ToLower().Pos("output-file") == 1 ||
			plugin_.arg_name_.GetAt(i).ToLower().Pos("output_file") == 1){
			FString file_name = FString(AnsiString(((TEdit*)controls_.GetAt(i))->Text).c_str());
			if (ExtractFileExt(file_name.ToLower().PChar()) == ".csv") {
				result.Add(file_name);
			}
		}
	}
	return result;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Plugin::FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	if (Key == 27) {
		ModalResult = mrCancel;
		Close();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_Plugin::FormShow(TObject *Sender)
{
	SetupTranslation(Lang);
	if (lang_ != "auto") {
		Lang->AutoSelect = false;
		Lang->Lang = lang_;
	}

    Me_Description->WordWrap = true;

	const int h_bak = ClientHeight, w_bak = ClientWidth;
	ClientWidth = gui_width_;
	ClientHeight = gui_height_;
	Top = Top - (ClientHeight - h_bak) / 2;
	Left = Left - (ClientWidth - w_bak) / 2;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Plugin::Bu_BrowseClick(TObject *Sender)
{
	const int index = ((TButton*)Sender)->Tag;
	TEdit *edit = (TEdit*)controls_.GetAt(index);
	UnicodeString dir = ExtractFileDir(edit->Text);
	if (dir == ""){
		dir = base_dir_;
	}
	if (plugin_.arg_description_.GetAt(index).ToLower().Pos("(dir)")) {
        UnicodeString dir;
		if (SelectDirectory("Directory", "", dir)){
            edit->Text = dir;
		}
	}else if (plugin_.arg_name_.GetAt(index).ToLower().Pos("output")){
		if (edit->Text != ""){
			SD_Save->FileName = ExtractFileName(edit->Text);
		}
		SD_Save->InitialDir = dir;
		if (SD_Save->Execute()){
			edit->Text = SD_Save->FileName;
		}
	}else{
		if (edit->Text != ""){
			OD_Open->FileName = ExtractFileName(edit->Text);
		}
		OD_Open->InitialDir = dir;
		if (OD_Open->Execute()){
			edit->Text = OD_Open->FileName;
		}

	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Plugin::Bu_ResetClick(TObject *Sender)
{
	Reset();
}
//---------------------------------------------------------------------------
FStringList TFo_Plugin::RestoreLastArgs()
{
	FFastIni arg_backup_ini = FFastIni(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\plugin_args.ini").c_str());
	FStringList result;
	for (int i = 0 ; i < plugin_.arg_name_.GetSize() ; i++){
		result.Add(arg_backup_ini.ReadString(plugin_.name_, plugin_.arg_name_.GetAt(i), plugin_.arg_default_.GetAt(i)));
	}
	return result;
}
//---------------------------------------------------------------------------
void TFo_Plugin::SaveLastArgs()
{
	FFastIni arg_backup_ini = FFastIni(AnsiString(ExtractFilePath(ParamStr(0)) + "settings\\plugin_args.ini").c_str());
	for (int i = 0 ; i < plugin_.arg_option_.GetSize() ; i++){
		const int is_bool = plugin_.arg_description_.GetAt(i).ToLower().Pos("(bool)");
		const int is_option = plugin_.arg_description_.GetAt(i).ToLower().Pos("(option:");
		const int is_msoption = plugin_.arg_description_.GetAt(i).ToLower().Pos("(msoption:");
		AnsiString s;
		if (is_bool){
			s = ((TCheckBox*)controls_.GetAt(i))->IsChecked ? "true" : "false";
		}else if (is_option){
			TLayout *layout = ((TLayout*)controls_.GetAt(i));
			for (int ic = 0 ; ic < layout->ComponentCount ; ic++) {
				if (layout->Components[ic]->ClassName() == "TRadioButton") {
					TRadioButton *rb = (TRadioButton*)layout->Components[ic];
					if (rb->IsChecked) {
						s = rb->Text;
					}
				}
			}
		}else if (is_msoption){
			TLayout *layout = ((TLayout*)controls_.GetAt(i));
			for (int ic = 0 ; ic < layout->ComponentCount ; ic++) {
				if (layout->Components[ic]->ClassName() == "TCheckBox") {
					TCheckBox *cb = (TCheckBox*)layout->Components[ic];
					if (cb->IsChecked) {
						if (s != "") {
                            s += ",";
						}
						s += cb->Text;
					}
				}
			}
		}else {
			s = ((TEdit*)controls_.GetAt(i))->Text;
		}
		arg_backup_ini.Write<FString>(plugin_.name_, plugin_.arg_name_.GetAt(i), FString(s.c_str()));
	}
	arg_backup_ini.Flush();
}
//---------------------------------------------------------------------------

void __fastcall TFo_Plugin::Bu_OKClick(TObject *Sender)
{
	SaveLastArgs();
}
//---------------------------------------------------------------------------

