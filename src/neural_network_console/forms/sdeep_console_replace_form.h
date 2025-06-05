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

#ifndef sdeep_console_replace_formH
#define sdeep_console_replace_formH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <FMX.ListBox.hpp>
//---------------------------------------------------------------------------
class TFo_Replace : public TForm
{
__published:	// IDE �ŊǗ������R���|�[�l���g
	TButton *Bu_Replace;
	TButton *Bu_Cancel;
	TLabel *La_From;
	TStyleBook *StyleBook;
	TComboBox *Co_From;
	TLabel *La_To;
	TComboBox *Co_To;
	TRadioButton *RB_TargetNetworkAll;
	TLabel *La_TargetNetwork;
	TRadioButton *RB_TargetNetworkCurrent;
	TLabel *Label1;
	TRadioButton *RB_TargetLayerAll;
	TRadioButton *RB_TargetLayerSelection;
	TLang *Lang;
	void __fastcall Bu_ReplaceClick(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall RB_TargetNetworkAllClick(TObject *Sender);
	void __fastcall RB_TargetNetworkCurrentClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
private:	// ���[�U�[�錾
	UnicodeString lang_;
public:		// ���[�U�[�錾
	explicit __fastcall TFo_Replace(TComponent* Owner);
	void Set(TStrings *component_types, UnicodeString current_type = "", int selected = false);
	int replace_;
	UnicodeString from_;
	UnicodeString to_;
	int target_current_network_;
	int target_selection_;
	void SetLang(UnicodeString lang) {lang_ = lang;}
};
//---------------------------------------------------------------------------
extern PACKAGE TFo_Replace *Fo_Replace;
//---------------------------------------------------------------------------
#endif
