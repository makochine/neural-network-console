// Copyright 2023,2024,2025 Sony Group Corporation.
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

#ifndef script_threadH
#define script_threadH

#include "sdeep_training_thread.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
class ScriptThread : public SDeepTrainingThread
{
public:
	ScriptThread(
		FString name,
		FString result_dir,
		TComponent *AOwner,
		NNDocument *nn_document,
		SDeepConsoleGlobalSettings *settings,
		int schedule = false,
		int terminate_with_ctrl_c = false,
		int resume = false);
	virtual __fastcall ~ScriptThread() {};
public:
	void __fastcall Execute() override;
protected:
	void PostLog(FString log_str);

	int terminate_with_ctrl_c_;
};
//---------------------------------------------------------------------------
};
#endif
