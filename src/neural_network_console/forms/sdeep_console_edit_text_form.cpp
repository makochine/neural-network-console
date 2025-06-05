// Copyright 2020,2021 Sony Corporation.
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

#include "sdeep_console_edit_text_form.h"
#include "translation.h"
#include "file_util.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TFo_EditText *Fo_EditText;

using namespace elfe;
//---------------------------------------------------------------------------
__fastcall TFo_EditText::TFo_EditText(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void TFo_EditText::Set(UnicodeString window_caption, UnicodeString caption, UnicodeString text, int mode)
{
	Caption = window_caption;
    mode_ = mode;
	La_Caption->Visible = false;
	Ed_Text->Visible = false;
	Me_Text->Visible = false;
	Bu_FromFolder->Visible = false;
	Bu_FromFiles->Visible = false;

	switch (mode) {
	case 1:
	case 2:
		//multiline text mode
		ClientWidth = 1600;
		ClientHeight = 800;
		BorderStyle = TFmxFormBorderStyle::Sizeable;
		if (mode == 2) {
			//dataset edit mode
			Bu_OK->Width = Bu_OK->Width + 64;
			Bu_OK->Text = GetTranslatedText("Save as...");
			Bu_FromFolder->Visible = true;
			Bu_FromFiles->Visible = true;
		}
		Me_Text->Visible = true;
		break;
	default:
		//text edit mode
		La_Caption->Visible = true;
		Ed_Text->Visible = true;
		La_Caption->Text = caption + ":";
		Ed_Text->Text = text;
		result_ = text;
		Ed_Text->SetFocus();
		Ed_Text->SelectAll();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_EditText::FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	if (Key == 27) {
		ModalResult = mrCancel;
		Close();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_EditText::Bu_OKClick(TObject *Sender)
{
	switch (mode_) {
	case 1:
	case 2:
		result_ = Me_Text->Text;
		break;
	default:
		result_ = Ed_Text->Text;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFo_EditText::FormShow(TObject *Sender)
{
	SetupTranslation(Lang);
	if (lang_ != "auto") {
		Lang->AutoSelect = false;
		Lang->Lang = lang_;
	}
}
//---------------------------------------------------------------------------
UnicodeString TFo_EditText::GetTranslatedText(UnicodeString text)
{
	TStrings *lang_str = Lang->LangStr[Lang->Lang];
	if (lang_str) {
		UnicodeString text2 = text + "=";
		const int text2_length = text2.Length();
		for (int i2 = 0 ; i2 < lang_str->Count ; i2++) {
			if (lang_str->Strings[i2].SubString(1, text2_length) == text2) {
				return lang_str->Strings[i2].SubString(text2_length + 1, lang_str->Strings[i2].Length() - text2_length);
			}
		}
	}
	return text;
}
//---------------------------------------------------------------------------

void __fastcall TFo_EditText::FormResize(TObject *Sender)
{
	const int button_y = ClientHeight - 16 - Bu_Cancel->Height;
	Bu_OK->Position->Y = button_y;
	Bu_Cancel->Position->Y = button_y;

	Bu_Cancel->Position->X = ClientWidth - 16 - Bu_Cancel->Width;
	Bu_OK->Position->X = Bu_Cancel->Position->X - 16 - Bu_OK->Width;

	Me_Text->Width = ClientWidth - 32;
	Me_Text->Height = button_y - 16 - Me_Text->Position->Y;
}
//---------------------------------------------------------------------------

void __fastcall TFo_EditText::Bu_FromFilesClick(TObject *Sender)
{
	if (OD_Files->Execute()) {
		Me_Text->Lines->Assign(OD_Files->Files);
	}
}
//---------------------------------------------------------------------------
void ListFiles(const char *path, FStringList &files)
{
	TSearchRec LSearchRec;
	UnicodeString LSearchPath = IncludeTrailingPathDelimiter(path) + "*.*";

	if (FindFirst(LSearchPath, faAnyFile, LSearchRec) == 0)	{
		do {
			if ((LSearchRec.Name != "..") && (LSearchRec.Name != ".")) {
				UnicodeString LFullPath = IncludeTrailingPathDelimiter(path) + LSearchRec.Name;
				if ((LSearchRec.Attr & faDirectory) == faDirectory)	{
					ListFiles(AnsiString(LFullPath).c_str(), files);
				}else {
					files.Add(FString(AnsiString(LFullPath).c_str()));
				}
			}
		} while (FindNext(LSearchRec) == 0);
		FindClose(LSearchRec);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_EditText::Bu_FromFolderClick(TObject *Sender)
{
	if (OD_Dir->Execute()) {
		UnicodeString dir_name = OD_Dir->FileName;
		if (!DirectoryExists(dir_name)) {
			dir_name = ExtractFileDir(dir_name);
		}
		FStringList result_files;
		_di_IFMXCursorService cursorService;
		if (TPlatformServices::Current->SupportsPlatformService(__uuidof(IFMXCursorService), &cursorService))
		{
			cursorService->SetCursor(crHourGlass);
			ListFiles(AnsiString(dir_name + "\\").c_str(), result_files);

			for (int i = 0 ; i < result_files.GetSize() ; i++) {
				FString file_name = result_files.GetAt(i);
				if (file_name != "." && file_name != "..") {
					Me_Text->Lines->Add(file_name.PChar());
				}
			}
			cursorService->SetCursor(crDefault);
		}
	}
}
//---------------------------------------------------------------------------

