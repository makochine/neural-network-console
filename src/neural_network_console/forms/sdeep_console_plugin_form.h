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

#ifndef sdeep_console_plugin_formH
#define sdeep_console_plugin_formH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <nncplugin.h>
#include <FMX.Dialogs.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.Memo.hpp>
#include <FMX.ScrollBox.hpp>
#include <FMX.Memo.Types.hpp>

using namespace sdeep_console;
//---------------------------------------------------------------------------
class TFo_Plugin : public TForm
{
__published:	// IDE �ŊǗ������R���|�[�l���g
	TEdit *Ed_Value;
	TLabel *La_NameAndDescription;
	TStyleBook *StyleBook;
	TOpenDialog *OD_Open;
	TButton *Bu_Browse;
	TSaveDialog *SD_Save;
	TLang *Lang;
	TLayout *Lo_OKCancel;
	TButton *Bu_OK;
	TButton *Bu_Cancel;
	TButton *Bu_Reset;
	TMemo *Me_Description;
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall Bu_BrowseClick(TObject *Sender);
	void __fastcall Bu_ResetClick(TObject *Sender);
	void __fastcall Bu_OKClick(TObject *Sender);
private:	// ���[�U�[�錾
public:		// ���[�U�[�錾
	explicit __fastcall TFo_Plugin(TComponent* Owner);

public:
	void PrepareGUI(const NNCPlugin &plugin);
	void SetImageArgument(FString image_file_name);
	void SetDefaultArgument(FString sub_string_in_description, FString value);
	void SetBaseDir(UnicodeString base_dir) {base_dir_ = base_dir;}
	void SetSampleDatasetDir(UnicodeString sample_dataset_dir);
	AnsiString GetArgs();
	AnsiString GetOutputFileName();
	AnsiString GetOutputDir();
	FStringList GetDatasetFileNames();//For dataset creation pre process
	void SetLang(UnicodeString lang) {lang_ = lang;}
private:
	void Reset();
	void SaveLastArgs();
	FStringList RestoreLastArgs();
	NNCPlugin plugin_;
	FListP<TControl*> controls_;
	UnicodeString base_dir_;
	UnicodeString lang_;
    int gui_width_, gui_height_;
};
//---------------------------------------------------------------------------
extern PACKAGE TFo_Plugin *Fo_Plugin;
//---------------------------------------------------------------------------
#endif
