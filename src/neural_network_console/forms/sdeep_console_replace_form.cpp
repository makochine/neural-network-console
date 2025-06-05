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

#include "sdeep_console_replace_form.h"
#include "translation.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TFo_Replace *Fo_Replace;

using namespace elfe;
//---------------------------------------------------------------------------
__fastcall TFo_Replace::TFo_Replace(TComponent* Owner)
	: TForm(Owner), replace_(false), target_current_network_(true), target_selection_(false)
{
}
//---------------------------------------------------------------------------
void TFo_Replace::Set(TStrings *component_types, UnicodeString current_type, int selected)
{
    replace_ = false;
    Co_From->Items->Assign(component_types);
	Co_To->Items->Assign(component_types);
	int index = component_types->IndexOf(current_type);
	if (index < 0) {
        index = 0;
	}
	if (selected) {
		RB_TargetNetworkCurrent->IsChecked = true;
		RB_TargetNetworkCurrentClick(this);
		RB_TargetLayerSelection->IsChecked = true;
	}else {
		RB_TargetNetworkAll->IsChecked = true;
		RB_TargetNetworkAllClick(this);
	}
	Co_From->ItemIndex = index;
	Co_To->ItemIndex = index;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Replace::Bu_ReplaceClick(TObject *Sender)
{
	replace_ = true;
	from_ = Co_From->Items->Strings[Co_From->ItemIndex];
	to_ = Co_To->Items->Strings[Co_To->ItemIndex];
	target_current_network_ = RB_TargetNetworkCurrent->IsChecked;
	target_selection_ = RB_TargetLayerSelection->IsChecked;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Replace::FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift)
{
	if (Key == 27) {
		ModalResult = mrCancel;
		Close();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Replace::RB_TargetNetworkAllClick(TObject *Sender)
{
	RB_TargetLayerAll->Enabled = false;
	RB_TargetLayerSelection->Enabled = false;
	RB_TargetLayerAll->IsChecked = true;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Replace::RB_TargetNetworkCurrentClick(TObject *Sender)
{
	RB_TargetLayerAll->Enabled = true;
	RB_TargetLayerSelection->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TFo_Replace::FormShow(TObject *Sender)
{
	SetupTranslation(Lang);
	if (lang_ != "auto") {
		Lang->AutoSelect = false;
		Lang->Lang = lang_;
	}
}
//---------------------------------------------------------------------------

