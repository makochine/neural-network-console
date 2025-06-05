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

#ifndef sdeep_evaluation_threadH
#define sdeep_evaluation_threadH

#include "nn_document.h"
#include "sdeep_console_global_settings.h"
#include "sdeep_process_thread.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
class SDeepEvaluationThread : public SDeepProcessThread
{
public:
	SDeepEvaluationThread(
		FString name,
		FString result_dir,
		TComponent *AOwner,
		NNDocument *nn_document,
		SDeepConsoleGlobalSettings *settings,
		int schedule = false);
	virtual __fastcall ~SDeepEvaluationThread() {};
public:
	int GetDataNum() const {return data_num_;};
	int GetLastData() const {return last_data_;};

	void SetAutoEvaluation(int auto_evaluation) {auto_evaluation_ = auto_evaluation;}
	int GetAutoEvaluation() const {return auto_evaluation_;}

protected:
	int data_num_;
	int last_data_;
    int auto_evaluation_;
};
//---------------------------------------------------------------------------
}
#endif
