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

#include <fmx.h>
#pragma hdrstop

#include <System.JSON.hpp>

#include "sdeep_console_setup_form.h"
#include "sdeep_console_main_form.h"
#include "translation.h"

#include "cross_platform_file_util.h"
#include "translation.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"

TFo_Setup *Fo_Setup;

using namespace elfe;

//---------------------------------------------------------------------------
__fastcall TFo_Setup::TFo_Setup(TComponent* Owner)
	: TForm(Owner), settings_(NULL),
	setting_file_(NULL)
{
	Left = (Application->MainForm->Left + Application->MainForm->Width - Width) / 2;
	Top = (Application->MainForm->Top + Application->MainForm->Height - Height) / 2;

	apply_ = false;
}

//-------------------------------------------------------------------------
void __fastcall TFo_Setup::FormShow(TObject *Sender)
{
	SetupTranslation(Lang);
	if (lang_ != "auto") {
		Lang->AutoSelect = false;
		Lang->Lang = lang_;
	}

	TC_Setup->Align = TAlignLayout::Client;
	Bu_EngineTabClick(Sender);

	ClientHeight = Bu_EngineTab->Height + TC_Setup->Height + Pa_SetupBottom->Height;
	Me_About->Height = TC_Setup->Height - 32;

	//Engine : Todo
	switch (settings_->GetLocalSDeepProcessorType()) {
	case LocalSDeepProcessorType::CPU:
		RB_LocalSDeepProcessorTypeCPU->IsChecked = true;
		break;
	case LocalSDeepProcessorType::CUDA:
	case LocalSDeepProcessorType::cuDNN_CUDA:
		RB_LocalSDeepProcessorTypeCUDA->IsChecked = true;
		break;
	}
	Co_GPUDeviceIndex->ItemIndex = settings_->GetLocalSDeepGPUIndex() + 1;

	Ed_LocalNNablaSDPCommand->Text = settings_->GetLocalNNablaSDPCommand().PChar();
	Me_LocalEnvironmentVariable->Lines->Clear();
	const FStringList &local_environment_variable = settings_->GetLocalEnvironmentVariable();
	for (int i = 0 ; i < local_environment_variable.GetSize() ; i++) {
		Me_LocalEnvironmentVariable->Lines->Add(local_environment_variable.GetAt(i).PChar());
	}

	//View
	TB_GUIScale->Tag = 1;
	TB_GUIScale->Value = settings_->GetGUIScale();
	TB_GUIScale->Tag = 0;
	Ch_GUITransition->IsChecked = settings_->GetGUITransition();
	Ch_PlayAudioFile->IsChecked = settings_->GetPlayAudioFile();
	FStringList languages = EnumLanguages();
	for (int i = 0 ; i < languages.GetSize() ; i++) {
		if (languages.GetAt(i).SubString(1, 2) != "en") {
			Co_Language->Items->Add(languages.GetAt(i).PChar());
		}
	}
	UnicodeString language = settings_->GetLanguage().PChar();
	for (int i = 0 ; i < Co_Language->Items->Count ; i++) {
		if (Co_Language->Items->Strings[i].Pos(language) == 1) {
			Co_Language->ItemIndex = i;
            break;
		}
	}
	switch(settings_->GetDefaultCSVGraphType()){
	case CSVGraphType::Summary:
		Co_CSVGraphType->ItemIndex = 0;
		break;
	case CSVGraphType::TwoDim:
		Co_CSVGraphType->ItemIndex = 2;
		break;
	case CSVGraphType::TwoDim_Transposed:
		Co_CSVGraphType->ItemIndex = 3;
		break;
	default:
		Co_CSVGraphType->ItemIndex = 1;
		break;
	}

	//About tab
	Me_About->Lines->LoadFromFile(ExtractFilePath(ParamStr(0)) + "settings\\about.txt");
	Me_About->Lines->Insert(0, "");
	Me_About->Lines->Insert(0, (
		FString(PRODUCT_NAME) +
		FString(" version ") +
		GetBinaryFileVersion(AnsiString(ParamStr(0)).c_str())
		).PChar());

	Transparency = false;
}
//---------------------------------------------------------------------------
void __fastcall TFo_Setup::Bu_ApplyClick(TObject *Sender)
{
	if (RB_LocalSDeepProcessorTypeCPU->IsChecked) {
		settings_->SetLocalSDeepProcessorType(LocalSDeepProcessorType::CPU);
	}
	if (RB_LocalSDeepProcessorTypeCUDA->IsChecked) {
		settings_->SetLocalSDeepProcessorType(LocalSDeepProcessorType::cuDNN_CUDA);
	}
	settings_->SetLocalSDeepGPUIndex(Co_GPUDeviceIndex->ItemIndex - 1);

	settings_->SetLocalNNablaSDPCommand(FString(AnsiString(Ed_LocalNNablaSDPCommand->Text).c_str()));
	FStringList environment_variable;
	for (int i = 0 ; i < Me_LocalEnvironmentVariable->Lines->Count ; i++) {
		environment_variable.Add(FString(AnsiString(Me_LocalEnvironmentVariable->Lines->Strings[i]).c_str()));
	}
	settings_->SetLocalEnvironmentVariable(environment_variable);

	//View Tab
	if (Co_Language->ItemIndex >= 0) {
		FString s = FString(AnsiString(Co_Language->Items->Strings[Co_Language->ItemIndex]).c_str());
		settings_->SetLanguage(s.SplitBy(":"));
	}
	settings_->SetGUIScale(TB_GUIScale->Value);
	settings_->SetGUITransition(Ch_GUITransition->IsChecked);
	settings_->SetPlayAudioFile(Ch_PlayAudioFile->IsChecked);
	switch(Co_CSVGraphType->ItemIndex){
	case 0:
		settings_->SetDefaultCSVGraphType(CSVGraphType::Summary);
		break;
	case 2:
		settings_->SetDefaultCSVGraphType(CSVGraphType::TwoDim);
		break;
	case 3:
		settings_->SetDefaultCSVGraphType(CSVGraphType::TwoDim_Transposed);
		break;
	default://1
		settings_->SetDefaultCSVGraphType(CSVGraphType::All);
		break;
	}

	apply_ = true;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFo_Setup::FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift)
{
	if (Key == 27) {
		ModalResult = mrCancel;
		Close();
	}
}
//---------------------------------------------------------------------------
void __fastcall TFo_Setup::TB_GUIScaleChange(TObject *Sender)
{
    const float scale = settings_->GUIScaleToGUIScaleFactor(TB_GUIScale->Value);
	La_GUIScaleValue->Text = FormatFloat("0.0", scale) + UnicodeString("x");
	Fo_Main->SetScale(TB_GUIScale->Value);
}
//---------------------------------------------------------------------------
void __fastcall TFo_Setup::FormClose(TObject *Sender, TCloseAction &Action)
{
	Fo_Main->SetScale(settings_->GetGUIScale());
}
//---------------------------------------------------------------------------

void __fastcall TFo_Setup::Bu_EngineTabClick(TObject *Sender)
{
	SetActiveTab(TC_Setup, TI_Engine);
}
//---------------------------------------------------------------------------

void __fastcall TFo_Setup::Bu_ViewTabClick(TObject *Sender)
{
	SetActiveTab(TC_Setup, TI_View);
}
//---------------------------------------------------------------------------

void __fastcall TFo_Setup::Bu_AboutTabClick(TObject *Sender)
{
	SetActiveTab(TC_Setup, TI_About);
}
//---------------------------------------------------------------------------

void TFo_Setup::SetActiveTab(TTabControl *TC, TTabItem *TI)
{
	for (int it = 0 ; it < TC->TabCount ; it++){
		if (TC->Tabs[it] == TI){
			TC->Tabs[it]->Visible = true;
			TC->ActiveTab = TI;
		}else {
			TC->Tabs[it]->Visible = false;
		}
	}
	Re_EngineTab->Visible = TI == TI_Engine;
	Re_ViewTab->Visible = TI == TI_View;
	Re_AboutTab->Visible = TI == TI_About;
}
//---------------------------------------------------------------------------


void __fastcall TFo_Setup::FormActivate(TObject *Sender)
{
}
//---------------------------------------------------------------------------

void __fastcall TFo_Setup::FormDestroy(TObject *Sender)
{
}
//---------------------------------------------------------------------------
