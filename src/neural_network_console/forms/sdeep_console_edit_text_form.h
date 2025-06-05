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

#ifndef sdeep_console_edit_text_formH
#define sdeep_console_edit_text_formH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <FMX.Memo.hpp>
#include <FMX.Memo.Types.hpp>
#include <FMX.ScrollBox.hpp>
#include <FMX.Dialogs.hpp>
//---------------------------------------------------------------------------
class TFo_EditText : public TForm
{
__published:	// IDE �ŊǗ������R���|�[�l���g
	TButton *Bu_OK;
	TButton *Bu_Cancel;
	TEdit *Ed_Text;
	TLabel *La_Caption;
	TStyleBook *StyleBook;
	TLang *Lang;
	TMemo *Me_Text;
	TButton *Bu_FromFolder;
	TButton *Bu_FromFiles;
	TOpenDialog *OD_Files;
	TOpenDialog *OD_Dir;
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift);
	void __fastcall Bu_OKClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall Bu_FromFilesClick(TObject *Sender);
	void __fastcall Bu_FromFolderClick(TObject *Sender);
private:	// ���[�U�[�錾
    int mode_;
	UnicodeString result_;
	UnicodeString lang_;
public:		// ���[�U�[�錾
	explicit __fastcall TFo_EditText(TComponent* Owner);
	void Set(UnicodeString window_caption, UnicodeString caption, UnicodeString text, int mode = 0);
	void SetLang(UnicodeString lang) {lang_ = lang;}
	const UnicodeString &GetResult() const {return result_;};
	UnicodeString GetTranslatedText(UnicodeString text);
};
//---------------------------------------------------------------------------
extern PACKAGE TFo_EditText *Fo_EditText;
//---------------------------------------------------------------------------
#endif
