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

// ---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include "file_util.h"
#include "sdeep_console_create_dataset_form.h"
#include "translation.h"
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TFo_CreateDataset *Fo_CreateDataset;

using namespace elfe;

// ---------------------------------------------------------------------------
__fastcall TFo_CreateDataset::TFo_CreateDataset(TComponent* Owner)
	: TForm(Owner), ok_(false) {
}

// ---------------------------------------------------------------------------
void __fastcall TFo_CreateDataset::Bu_SourceDirClick(TObject *Sender) {
	if (OD_SourceDir->Execute()) {
		Ed_SourceDir->Text = ExtractFileDir(OD_SourceDir->FileName);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TFo_CreateDataset::Bu_OutputDirClick(TObject *Sender) {
	SD_OutputDir->FileName = Ed_OutputFileName1->Text;
	if (SD_OutputDir->Execute()) {
		Ed_OutputDir->Text = ExtractFileDir(SD_OutputDir->FileName);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TFo_CreateDataset::Bu_ApplyClick(TObject *Sender) {
	if (Ed_SourceDir->Text == "" || !DirectoryExists(Ed_SourceDir->Text)) {
		ShowMessage(GetTranslatedText(Lang, "Source directory is not found."));
		Ed_SourceDir->SetFocus();
		return;
	}

	if (Ed_OutputDir->Text == "") {
		ShowMessage(GetTranslatedText(Lang, "Output directory is empty."));
		Ed_OutputDir->SetFocus();
		return;
	}

	if (Ed_OutputDir->Text == Ed_SourceDir->Text) {
		ShowMessage(GetTranslatedText(Lang, "Source and output directories must be different."));
		Ed_OutputDir->SetFocus();
		return;
	}

	FStringList sl =
		EnumFiles((AnsiString(Ed_OutputDir->Text) + AnsiString("\\")).c_str());
	// ShowMessage(IntToStr(sl.GetSize()));

	if (sl.GetSize() > 2) {
		int result =
			MessageDlg(
			GetTranslatedText(Lang, "The output directory is not empty. Do you want to overwrite?"),
			TMsgDlgType::mtWarning, mbOKCancel, 0);
		if (result != mrOk) {
			Ed_OutputDir->SetFocus();
			return;
		}
	}

	if (StrToIntDef(Ed_OutputWidth->Text, -1) <= 0) {
		ShowMessage(GetTranslatedText(Lang, "The value of output width is invalid."));
		Ed_OutputWidth->SetFocus();
		return;
	}

	if (StrToIntDef(Ed_OutputHeight->Text, -1) <= 0) {
		ShowMessage(GetTranslatedText(Lang, "The value of output height is invalid."));
		Ed_OutputWidth->SetFocus();
		return;
	}

	const int ratio1 = StrToIntDef(Ed_OutputFileRatio1->Text, -1);
	if (ratio1 < 0) {
		ShowMessage(GetTranslatedText(Lang, "The value of output file ratio 1 is invalid."));
		Ed_OutputFileRatio1->SetFocus();
		return;
	}

	const int ratio2 = StrToIntDef(Ed_OutputFileRatio2->Text, -1);
	if (ratio2 < 0) {
		ShowMessage(GetTranslatedText(Lang, "The value of output file ratio 2 is invalid."));
		Ed_OutputFileRatio2->SetFocus();
		return;
	}

	if (ratio1 + ratio2 != 100) {
		ShowMessage(GetTranslatedText(Lang, "The sum of output file ratio 1 and 2 must be 100."));
		Ed_OutputFileRatio1->SetFocus();
		return;
	}

	if (Ed_OutputFileName1->Text == "" && ratio1 > 0) {
		ShowMessage(GetTranslatedText(Lang, "Output file name 1 is empty."));
		Ed_OutputFileName1->SetFocus();
		return;
	}

	if (Ed_OutputFileName2->Text == "" && ratio2 > 0) {
		ShowMessage(GetTranslatedText(Lang, "Output file name 2 is empty."));
		Ed_OutputFileName2->SetFocus();
		return;
	}

	ok_ = true;
	Close();
}

// ---------------------------------------------------------------------------
void __fastcall TFo_CreateDataset::FormShow(TObject *Sender) {
	Im_SourceDir->Align = TAlignLayout::Client;
	Im_OutputDir->Align = TAlignLayout::Client;

	SetupTranslation(Lang);
	if (lang_ != "auto") {
		Lang->AutoSelect = false;
		Lang->Lang = lang_;
	}
}

// ---------------------------------------------------------------------------
void __fastcall TFo_CreateDataset::FormKeyDown(TObject *Sender, WORD &Key,
	System::WideChar &KeyChar, TShiftState Shift) {
	if (Key == 27) {
		ModalResult = mrCancel;
		Close();
	}
}
// ---------------------------------------------------------------------------
