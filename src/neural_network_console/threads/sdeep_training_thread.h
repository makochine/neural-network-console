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

#ifndef sdeep_training_threadH
#define sdeep_training_threadH

#include "nn_document.h"
#include "sdeep_console_global_settings.h"
#include "sdeep_process_thread.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
class SDeepTrainingThread : public SDeepProcessThread
{
public:
	SDeepTrainingThread(
		FString name,
		FString result_dir,
		TComponent *AOwner,
		NNDocument *nn_document,
		SDeepConsoleGlobalSettings *settings,
		int schedule = false,
		int resume = false);
	virtual __fastcall ~SDeepTrainingThread() {};
public:
	virtual void __fastcall Execute() = 0;
public:
	int GetLastEpoch() const {return last_epoch_;};
	int GetMaxEpoch() const {return max_epoch_;};
protected:
	//Setup
	int training_completed_;
	int max_epoch_;
	int last_epoch_;
	double last_training_;
	double last_validation_;
	int best_epoch_;
	double best_validation_;

    int resume_;

protected:
	void WriteResultIni();
};
//---------------------------------------------------------------------------
}
#endif
