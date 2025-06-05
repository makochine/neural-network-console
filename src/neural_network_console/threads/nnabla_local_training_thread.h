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

#ifndef nnabla_local_training_threadH
#define nnabla_local_training_threadH

#include "nnabla_training_thread.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
class NNablaLocalTrainingThread : public NNablaTrainingThread
{
public:
	NNablaLocalTrainingThread(
		FString name,
		FString result_dir,
		TComponent *AOwner,
		NNDocument *nn_document,
		SDeepConsoleGlobalSettings *settings,
		int schedule = false,
		int terminate_with_ctrl_c = false,
		int resume = false);
	virtual __fastcall ~NNablaLocalTrainingThread() {};
public:
	FString virtual GetCommand(int initial_parameter_exists, AnsiString protobuf_file_name) override;
};
//---------------------------------------------------------------------------
};
#endif
