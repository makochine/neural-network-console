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

#ifndef nnabla_training_threadH
#define nnabla_training_threadH

#include "sdeep_training_thread.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
class NNablaTrainingThread : public SDeepTrainingThread
{
public:
	NNablaTrainingThread(
		FString name,
		FString result_dir,
		TComponent *AOwner,
		NNDocument *nn_document,
		SDeepConsoleGlobalSettings *settings,
		int schedule = false,
		int terminate_with_ctrl_c = false,
		int resume = false);
	virtual __fastcall ~NNablaTrainingThread() {};
public:
	void __fastcall Execute() override;
	FString virtual GetCommand(int initial_parameter_exists, AnsiString protobuf_file_name) {return FString("");}
protected:
	void PostLog(FString log_str);

	UnicodeString prototxt_file_name_;
	UnicodeString prototxt_file_name_2_;
    int terminate_with_ctrl_c_;
private:
	int CreateNNablaProtoBufForTraining(
		const NNDocument &nn_document, const SDeepConsoleGlobalSettings &settings, //in
		const char *file_name); //out

};
//---------------------------------------------------------------------------
};
#endif
