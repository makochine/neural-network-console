// Copyright 2024,2025 Sony Group Corporation.
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

#ifndef torch_local_training_threadH
#define torch_local_training_threadH

#include "sdeep_training_thread.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
class TorchLocalTrainingThread : public SDeepTrainingThread
{
public:
	TorchLocalTrainingThread(
		FString name,
		FString result_dir,
		TComponent *AOwner,
		NNDocument *nn_document,
		SDeepConsoleGlobalSettings *settings,
		int schedule = false,
		int terminate_with_ctrl_c = false,
		int resume = false);
	virtual __fastcall ~TorchLocalTrainingThread() {};
public:
	void __fastcall Execute() override;
	FString GetCommand(int initial_parameter_exists, AnsiString protobuf_file_name);
protected:
	void PostLog(FString log_str);

	int terminate_with_ctrl_c_;
private:

};
//---------------------------------------------------------------------------
};
#endif