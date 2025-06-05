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

#ifndef sdeep_console_setup_formH
#define sdeep_console_setup_formH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Types.hpp>
#include <FMX.Edit.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.TabControl.hpp>

#include "sdeep_console_global_settings.h"
#include <FMX.Ani.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.Memo.hpp>
#include <FMX.ListBox.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.ScrollBox.hpp>
#include <FMX.Objects.hpp>
#include <FMX.WebBrowser.hpp>
#include <FMX.Memo.Types.hpp>

using namespace sdeep_console;
//---------------------------------------------------------------------------
class TFo_Setup : public TForm
{
__published:	// IDE �ŊǗ������R���|�[�l���g
	TStyleBook *StyleBook;
	TPanel *Pa_SetupBottom;
	TButton *Bu_Apply;
	TButton *Bu_Cancel;
	TTabControl *TC_Setup;
	TTabItem *TI_Engine;
	TTabItem *TI_About;
	TLabel *Label1;
	TMemo *Me_About;
	TRadioButton *RB_LocalSDeepProcessorTypeCPU;
	TRadioButton *RB_LocalSDeepProcessorTypeCUDA;
	TMemo *Me_LocalEnvironmentVariable;
	TTabItem *TI_View;
	TLabel *La_GUIScale;
	TTrackBar *TB_GUIScale;
	TLabel *La_GUIScaleValue;
	TEdit *Ed_LocalNNablaSDPCommand;
	TLabel *La_ProcessorType;
	TLabel *La_CommandLineInterface;
	TLabel *La_EnvironmentVariable;
	TButton *Bu_EngineTab;
	TRectangle *Re_EngineTab;
	TButton *Bu_ViewTab;
	TRectangle *Re_ViewTab;
	TButton *Bu_AboutTab;
	TRectangle *Re_AboutTab;
	TCheckBox *Ch_GUITransition;
	TComboBox *Co_GPUDeviceIndex;
	TLabel *La_GPUDeviceIndex;
	TRectangle *Re_Processing;
	TCheckBox *Ch_PlayAudioFile;
	TLabel *Label2;
	TComboBox *Co_Language;
	TLang *Lang;
	TLabel *La_CSVGraphType;
	TComboBox *Co_CSVGraphType;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall Bu_ApplyClick(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall TB_GUIScaleChange(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall Bu_EngineTabClick(TObject *Sender);
	void __fastcall Bu_ViewTabClick(TObject *Sender);
	void __fastcall Bu_AboutTabClick(TObject *Sender);
	void __fastcall FormActivate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
private:	// ���[�U�[�錾
public:		// ���[�U�[�錾
	explicit __fastcall TFo_Setup(TComponent* Owner);

public:
	void SetSDeepConsoleGlobalSettings(SDeepConsoleGlobalSettings *settings, FFastIni *setting_file) {settings_ = settings; setting_file_ = setting_file;}

	int apply_;

private:
	SDeepConsoleGlobalSettings *settings_;
	FFastIni *setting_file_;
	void ApplyMainFormScale(float scale);
	void SetActiveTab(TTabControl *TC, TTabItem *TI);

public:
	void SetLang(UnicodeString lang) {lang_ = lang;}
private:
	UnicodeString lang_;
};
//---------------------------------------------------------------------------
extern PACKAGE TFo_Setup *Fo_Setup;
//---------------------------------------------------------------------------
#endif
