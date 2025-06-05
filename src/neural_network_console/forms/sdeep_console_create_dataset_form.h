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

#ifndef sdeep_console_create_dataset_formH
#define sdeep_console_create_dataset_formH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <FMX.Objects.hpp>
#include <FMX.Dialogs.hpp>
//---------------------------------------------------------------------------
class TFo_CreateDataset : public TForm
{
__published:	// IDE �ŊǗ������R���|�[�l���g
	TStyleBook *StyleBook;
	TLabel *La_Title;
	TLabel *Label2;
	TLabel *La_OutputDir;
	TLabel *La_OutputFileName1;
	TLabel *La_OutputFileName2;
	TLabel *Label1;
	TLabel *Label3;
	TEdit *Ed_SourceDir;
	TEdit *Ed_OutputDir;
	TEdit *Ed_OutputFileName1;
	TEdit *Ed_OutputFileName2;
	TEdit *Ed_OutputFileRatio1;
	TEdit *Ed_OutputFileRatio2;
	TLabel *La_ShapingMode;
	TRadioButton *RB_Trimming;
	TRadioButton *RB_Padding;
	TLabel *La_OutputColorCh;
	TRadioButton *RB_Monochrome;
	TRadioButton *RB_RGB;
	TLabel *Label4;
	TLabel *Label5;
	TEdit *Ed_OutputWidth;
	TEdit *Ed_OutputHeight;
	TButton *Bu_SourceDir;
	TImage *Im_SourceDir;
	TButton *Bu_OutputDir;
	TImage *Im_OutputDir;
	TButton *Bu_Apply;
	TButton *Bu_Cancel;
	TSaveDialog *SD_OutputDir;
	TOpenDialog *OD_SourceDir;
	TCheckBox *Ch_Shuffle;
	TRadioButton *RadioButton1;
	TLang *Lang;
	void __fastcall Bu_SourceDirClick(TObject *Sender);
	void __fastcall Bu_OutputDirClick(TObject *Sender);
	void __fastcall Bu_ApplyClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
private:	// ���[�U�[�錾
    UnicodeString lang_;
public:		// ���[�U�[�錾
	explicit __fastcall TFo_CreateDataset(TComponent* Owner);
	void SetLang(UnicodeString lang) {lang_ = lang;}

    int ok_;
};
//---------------------------------------------------------------------------
extern PACKAGE TFo_CreateDataset *Fo_CreateDataset;
//---------------------------------------------------------------------------
#endif
