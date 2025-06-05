// Copyright 2016,2017,2018,2019,2020,2021 Sony Corporation.
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

#ifndef sdeep_process_threadH
#define sdeep_process_threadH

#include <System.Classes.hpp>
#include "nn_document.h"
#include "sdeep_console_global_settings.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
class SDeepProcessThread : public TThread
{
public:
	SDeepProcessThread(
		FString name,
		FString result_dir,
		TComponent *AOwner,
		NNDocument *nn_document,
		SDeepConsoleGlobalSettings *settings,
		int schedule = false);
	virtual __fastcall ~SDeepProcessThread() {};
public:
	virtual void __fastcall Execute() = 0;
public:
	//Monitor
	const FString &GetName() const {return name_;}
	const FString &GetResultDir() const {return result_dir_;}
	int GetNumLatestLog() const {return log_.GetSize() - log_index_;}
	const FString &GetLatestLog();
	const FStringList &GetLog() {return log_;}
	int GetNumIndicatorLog() const {return indicator_log_.GetSize();}
	FString GetIndicatorLog();
	void SetStopped(int stopped) {stopped_ = stopped;}
	int GetRemote() {return remote_;}
	int GetProcessCreated() {return process_created_;}
	int GetStopped() {return stopped_;}
	int GetSuccessfullyFinished() {return successfully_finished_;}
	int GetUpdated() const {return updated_;}
	void ResetUpdated() {updated_ = false;}

	const FString GetOption() const {return option_;}
	void SetOption(const FString &option) {option_ = option;}
protected:
	//Setup
	FString name_;//Name of training thread (it is also name of result dir)
	FString result_dir_;//Directory to save result (full path without last '\')
	TComponent *owner_;//(Need to create xml file)
	NNDocument *nn_document_;
	SDeepConsoleGlobalSettings *settings_;
	int schedule_;
	int remote_;
	FString option_;

	//State (need to set in sub class)
	int process_created_;
	int stopped_;
	int successfully_finished_;
protected:
	//Log
	void Log(FString s, int add_time_code = false, FString fn = FString(""));
	void Log(FStringList sl, int add_time_code = false, FString fn = FString(""));
	void CommandLog(FString s);
public:
	void IndicatorLog(FString s) {indicator_log_.Add(s);}
protected:
	//Log
	FStringList log_;
	int log_index_;

	FStringList indicator_log_;

	//Log
	UnicodeString log_file_name_;
	FString log_file_name_sync_;
	FStringList log_str_;
	int add_time_code_;
	void __fastcall LogSynchronize();

	int updated_;

	FString progress_txt_;
};
//---------------------------------------------------------------------------
}
#endif
