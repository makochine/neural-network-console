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

#include "cross_platform_file_util.h"

#ifdef _WIN32
//---------------------------------------------------------------------------
PipeIO::PipeIO(const char *command)
{
	success_ = true;

	SECURITY_ATTRIBUTES security_attributes;
	security_attributes.nLength = sizeof(security_attributes);
	security_attributes.bInheritHandle = TRUE;
	security_attributes.lpSecurityDescriptor = NULL;

	HANDLE process = GetCurrentProcess();
	success_ &= CreatePipe(&stdout_read_, &stdout_write_, &security_attributes, NULL);
	success_ &= DuplicateHandle(
		process,
		stdout_read_,
		process,
		NULL,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS);
	log_ += FString("stdout_read_ = ") + FString((int)stdout_read_, 8) + FString("\tstdout_write_ = ") + FString((int)stdout_write_, 8);
	success_ &= CreatePipe(&stdin_read_, &stdin_write_, &security_attributes, NULL);
	success_ &= DuplicateHandle(
		process,
		stdin_write_,
		process,
		NULL,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS);
	log_ += FString("\tstdin_read_ = ") + FString((int)stdin_read_, 8) + FString("\tstdout_write_ = ") + FString((int)stdin_write_, 8);

	STARTUPINFO startup_info;
	PROCESS_INFORMATION process_information;

	GetStartupInfo(&startup_info);
	startup_info.cb = sizeof(startup_info);
	startup_info.hStdInput = stdin_read_;
	startup_info.hStdOutput = stdout_write_;
	startup_info.hStdError = stdout_write_;
	startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_HIDE;

	log_ += FString("\r\ncb = ") + FString((int)startup_info.cb);
	log_ += FString("\tlpReserved = ") + FString(AnsiString(UnicodeString(startup_info.lpReserved)).c_str());
	log_ += FString("\tlpDesktop = ") + FString(AnsiString(UnicodeString(startup_info.lpDesktop)).c_str());
	log_ += FString("\tlpTitle = ") + FString(AnsiString(UnicodeString(startup_info.lpTitle)).c_str());
	log_ += FString("\r\ndwX = ") + FString((int)startup_info.dwX);
	log_ += FString("\tdwY = ") + FString((int)startup_info.dwY);
	log_ += FString("\tdwXSize = ") + FString((int)startup_info.dwXSize);
	log_ += FString("\tdwYSize = ") + FString((int)startup_info.dwYSize);
	log_ += FString("\tdwXCountChars = ") + FString((int)startup_info.dwXCountChars);
	log_ += FString("\tdwYCountChars = ") + FString((int)startup_info.dwYCountChars);
	log_ += FString("\r\ndwFillAttribute = ") + FString((int)startup_info.dwFillAttribute);
	log_ += FString("\tdwFlags = ") + FString((int)startup_info.dwFlags);
	log_ += FString("\twShowWindow = ") + FString((int)startup_info.wShowWindow);
	log_ += FString("\tcbReserved2 = ") + FString((int)startup_info.cbReserved2);
	log_ += FString("\tlpReserved2 = ") + FString((int)startup_info.lpReserved2);
	log_ += FString("\r\nhStdInput = ") + FString((int)startup_info.hStdInput);
	log_ += FString("\thStdOutput = ") + FString((int)startup_info.hStdOutput);
	log_ += FString("\thStdError = ") + FString((int)startup_info.hStdError);

	SetStdHandle(STD_INPUT_HANDLE, stdin_read_);
	SetStdHandle(STD_OUTPUT_HANDLE, stdout_write_);
	SetStdHandle(STD_ERROR_HANDLE, stdout_write_);

	success_ &= CreateProcess(
		NULL,
		UnicodeString(command).c_str(),
		NULL,
		NULL,
		true,
		0, //CREATE_NO_WINDOW,// | CREATE_NEW_PROCESS_GROUP,
		NULL,
		NULL,
		&startup_info,
		&process_information);

	unsigned long mode = PIPE_NOWAIT;
	SetNamedPipeHandleState(stdout_read_ ,&mode, NULL, NULL);

	if (!success_) {
		process_ = NULL;
		thread_ = NULL;
	}else {
		process_ = process_information.hProcess;
		thread_ = process_information.hThread;
		process_id_ = process_information.dwProcessId;
	}
	log_ += FString("\thprocess_ = ") + FString((int)process_, 8);
}
//---------------------------------------------------------------------------
PipeIO::~PipeIO()
{
	if (process_) {
		TerminateProcess(process_, 0);
	}
	CloseHandle(thread_);
	CloseHandle(process_);
	CloseHandle(stdout_read_);
	CloseHandle(stdout_write_);
	CloseHandle(stdin_read_);
	CloseHandle(stdin_write_);
}
//---------------------------------------------------------------------------
void PipeIO::WaitProcessFinished()
{
	WaitForSingleObject(process_, INFINITE);
}
//---------------------------------------------------------------------------
int PipeIO::WriteString(const FString string)
{
	unsigned long written_bytes = 0;
	int result = true;
	if (string != "") {
		const int length = string.Length() + 1;
		WriteFile(stdin_write_, string.PChar(), length, &written_bytes, NULL);
		result &= written_bytes == (unsigned)length;
	}
	FlushFileBuffers(stdin_write_);

	return result;
}
//---------------------------------------------------------------------------
FString PipeIO::ReadString()
{
	//FlushFileBuffers(read_);
	unsigned long read_bytes = 0;
	char buf[65536];
	memset(buf, 0, sizeof(buf));
	ReadFile(stdout_read_, buf, sizeof(buf), &read_bytes, NULL);
	buf[read_bytes] = '\0';

	return FString(AnsiString(UnicodeString(buf)).c_str());
}
//---------------------------------------------------------------------------
BOOL WINAPI ctrl_handler(DWORD dwCtrlType)
{
	return true;
}
//---------------------------------------------------------------------------
void PipeIO::SendCtrlC()
{
	FreeConsole();
	AttachConsole(process_id_);
	SetConsoleCtrlHandler(ctrl_handler, 1);
	GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
}
//---------------------------------------------------------------------------
int PipeIO::GetFinished() const
{
	unsigned long result = STILL_ACTIVE;
	GetExitCodeProcess(process_, &result);
	return result != STILL_ACTIVE;
}
//---------------------------------------------------------------------------
void OpenFile(const char *file_name)
{
	ShellExecute(
		0,
		UnicodeString("OPEN").c_str(),
		UnicodeString(file_name).c_str(),
		UnicodeString("").c_str(),
		UnicodeString("").c_str(),
		SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void RunApp(const char *file_name, const char *option)
{
	ShellExecute(
		0,
		UnicodeString("OPEN").c_str(),
		UnicodeString(file_name).c_str(),
		UnicodeString(option).c_str(),
		UnicodeString("").c_str(),
		SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void OpenFileLocation(const char *file_location)
{
	ShellExecute(
		0,
		UnicodeString("OPEN").c_str(),
		UnicodeString("explorer").c_str(),
		UnicodeString(UnicodeString("\"") + file_location + UnicodeString("\"")).c_str(),
		UnicodeString(UnicodeString("\"") + file_location + UnicodeString("\"")).c_str(),
		SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void DeleteDir(const char *dir)
{
	//system((FString("rmdir /s /q \"") + FString(dir) + FString("\"")).PChar());
	SHELLEXECUTEINFO shell_execute_info;
	memset(&shell_execute_info, 0, sizeof(SHELLEXECUTEINFO));
	shell_execute_info.cbSize = sizeof(SHELLEXECUTEINFO);
	static const UnicodeString file_name = "cmd";
	shell_execute_info.lpFile = file_name.c_str();
	UnicodeString parameter = UnicodeString((FString("/c rmdir /s /q \"") + FString(dir) + FString("\"")).PChar());
	shell_execute_info.lpParameters = parameter.c_str();
	shell_execute_info.fMask = SEE_MASK_NOCLOSEPROCESS;
	shell_execute_info.nShow = SW_HIDE;
	ShellExecuteEx(&shell_execute_info);
	WaitForSingleObject(shell_execute_info.hProcess, INFINITE);
}
//---------------------------------------------------------------------------
FString GetBinaryFileVersion(const char *file_name)
{
	unsigned long handle = NULL;
	int buf_size = GetFileVersionInfoSize(UnicodeString(file_name).c_str(), &handle);

	char *buf = new char[buf_size];

	AnsiString ver_str;

	if (GetFileVersionInfo(UnicodeString(file_name).c_str(), handle, buf_size, buf)) {
		unsigned int var_info_size = 0;
		char *var_info_buf = NULL;
		if (VerQueryValue(buf, UnicodeString("\\").c_str(), (VOID FAR* FAR*)&var_info_buf, &var_info_size)) {
			if (var_info_size) {
				VS_FIXEDFILEINFO *ver_info = (VS_FIXEDFILEINFO *)var_info_buf;
				if (ver_info->dwSignature == 0xfeef04bd) {
					ver_str =
						IntToStr((int)((ver_info->dwFileVersionMS >> 16 ) & 0xffff)) + AnsiString(".") +
						IntToStr((int)((ver_info->dwFileVersionMS >> 0 ) & 0xffff)) + AnsiString(".") +
						IntToStr((int)((ver_info->dwFileVersionLS >> 16 ) & 0xffff)) + AnsiString(".") +
						IntToStr((int)((ver_info->dwFileVersionLS >> 0 ) & 0xffff));
				}
			}
		}
	}
	delete[] buf;

	return FString(ver_str.c_str());
}
//---------------------------------------------------------------------------
int RenameDir(const char *from, const char *to)
{
    return MoveFileA(from, to);
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
//ToDo
//---------------------------------------------------------------------------
#pragma package(smart_init)
