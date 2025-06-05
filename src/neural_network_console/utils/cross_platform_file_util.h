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

#ifndef cross_platform_file_utilH
#define cross_platform_file_utilH

#include "fstring.h"
//---------------------------------------------------------------------------
using namespace elfe;

class PipeIO
{
public:
	explicit PipeIO(const char *command);
	virtual ~PipeIO();
	void WaitProcessFinished();
public:
	int WriteString(const FString string);
	FString ReadString();
	FString ReadErrorString();

	void SendCtrlC();

	int GetSuccess() const {return success_;};
	int GetFinished() const;
	FString GetLog() const {return log_;};
private:
	#ifdef _WIN32
	HANDLE process_, thread_;
	HANDLE stdout_read_, stdout_write_;
	HANDLE stdin_read_, stdin_write_;
	DWORD process_id_;
	#endif

	//ToDo

	int success_;
	FString log_;
};
//---------------------------------------------------------------------------
void OpenFile(const char *file_name);
void RunApp(const char *file_name, const char *option);
void OpenFileLocation(const char *file_location);
void DeleteDir(const char *dir);
int RenameDir(const char *from, const char *to);
FString GetBinaryFileVersion(const char *file_name);
//---------------------------------------------------------------------------
#endif
