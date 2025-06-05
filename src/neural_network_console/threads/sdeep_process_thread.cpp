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

#pragma hdrstop

#include "sdeep_process_thread.h"
#include "fstring.h"
namespace sdeep_console{
//---------------------------------------------------------------------------
SDeepProcessThread::SDeepProcessThread(
	FString name,
	FString result_dir,
	TComponent *AOwner,
	NNDocument *nn_document,
	SDeepConsoleGlobalSettings *settings,
	int schedule) :
		TThread(schedule),
		name_(name),
		result_dir_(result_dir),
		owner_(AOwner),
		nn_document_(nn_document),

		settings_(settings),
		schedule_(schedule),
		remote_(false),
		stopped_(false),
		successfully_finished_(false),
		process_created_(false),

		log_index_(0),
		add_time_code_(false),

        updated_(false)
{
    FreeOnTerminate = false;
};
//---------------------------------------------------------------------------
const FString &SDeepProcessThread::GetLatestLog()
{
	static const FString null_txt = FString("");
	if (log_.GetSize() - log_index_ > 0) {
		return log_.GetAt(log_index_++);
	}else {
		return null_txt;
	}
}
//---------------------------------------------------------------------------
void SDeepProcessThread::Log(FString s, int add_time_code, FString file_name)
{
	log_str_.Add(s);
	add_time_code_ = add_time_code;
	log_file_name_sync_ = file_name;
	Synchronize(LogSynchronize);
	log_str_.Clear();
};
//---------------------------------------------------------------------------
void SDeepProcessThread::CommandLog(FString s)
{
	FStringList command_logs;
	command_logs.SplitStr(s, FString(" -"), FString(""));
	for (int i = 1 ; i < command_logs.GetSize() ; i++) {
		command_logs.GetAt(i) = FString("\t-") + command_logs.GetAt(i);
	}
	for (int i = 1 ; i < command_logs.GetSize() - 1 ; i++) {
		if (command_logs.GetAt(i).Length() + command_logs.GetAt(i + 1).Length() < 74) {
			command_logs.GetAt(i) = command_logs.GetAt(i) + command_logs.GetAt(i + 1).ReplaceStr("\t", " ");
			command_logs.Delete(i + 1);
            i--;
		}
	}

	Log(command_logs);
}
//---------------------------------------------------------------------------
void SDeepProcessThread::Log(FStringList sl, int add_time_code, FString file_name)
{
	log_str_ = sl;
	add_time_code_ = add_time_code;
	log_file_name_sync_ = file_name;
	Synchronize(LogSynchronize);
	log_str_.Clear();
}
//---------------------------------------------------------------------------
void __fastcall SDeepProcessThread::LogSynchronize()
{
	UnicodeString file_name = log_file_name_sync_ != "" ? (result_dir_ + FString("\\") + log_file_name_sync_).PChar() : log_file_name_;
	FILE *F = fopen(AnsiString(file_name).c_str(), "at");
	if (!F){
		return;
	}
	FString last_progress_txt = progress_txt_;
	for (int i = 0 ; i < log_str_.GetSize(); i++) {
		FString s = log_str_.GetAt(i);
		s = s.ReplaceStr("\x1b[A", "");
		if (add_time_code_) {
			s = FString(AnsiString(FormatDateTime("yyyy-mm-dd hh:nn:ss,zzz",Now()) + " ").c_str()) + s;
		}
		if (s.Pos("\r")) {
			progress_txt_ = s.SplitBy("]") + FString("]");//remove tqdm line
			if (progress_txt_.Pos("\r")) {
				progress_txt_.SplitBy("\r");
			}
		}
		if (s.PChar()) {
			if (log_file_name_sync_ == "") {
				log_.Add(s);
			}
			fprintf(F, "%s\n", s.PChar());
			progress_txt_ = "";
		}
	}
	fclose(F);

	if (progress_txt_ != last_progress_txt) {
		UnicodeString state_file_name = UnicodeString(result_dir_.PChar()) + UnicodeString("\\progress.txt");
		FILE *F = fopen(AnsiString(state_file_name).c_str(), "wt");
		if (progress_txt_.PChar()) {
			fprintf(F, "%s", progress_txt_.PChar());
        }
		fclose(F);
	}
};
//---------------------------------------------------------------------------
FString SDeepProcessThread::GetIndicatorLog()
{
	FString result;
	if (indicator_log_.GetSize()) {
		result = indicator_log_.GetAt(0);
		indicator_log_.Delete(0);
	}
	return result;
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
