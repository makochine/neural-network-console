// Copyright 2018,2019,2020,2021 Sony Corporation.
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

#ifndef sdeep_console_create_unit_formH
#define sdeep_console_create_unit_formH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
//---------------------------------------------------------------------------
class TFo_CreateUnit : public TForm
{
__published:	// IDE �ŊǗ������R���|�[�l���g
	TButton *Bu_Create;
	TButton *Bu_Cancel;
	TEdit *Ed_NetworkName;
	TLabel *La_NetworkName;
	TStyleBook *StyleBook;
	TLang *Lang;
	void __fastcall Bu_CreateClick(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall FormShow(TObject *Sender);

private:	// ���[�U�[�錾
	UnicodeString lang_;
public:		// ���[�U�[�錾
	explicit __fastcall TFo_CreateUnit(TComponent* Owner);
	void SetLang(UnicodeString lang) {lang_ = lang;}
	int create_;
};
//---------------------------------------------------------------------------
extern PACKAGE TFo_CreateUnit *Fo_CreateUnit;
//---------------------------------------------------------------------------
#endif
