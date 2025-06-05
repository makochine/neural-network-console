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

#pragma hdrstop

#include "nn_script.h"

#include "ffastini.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
int NNScript::LoadFromIniSection(FFastIni &ini, const char *section)
{
	name_ = FString(section);
	cli_ = ini.ReadString(section, "CLI", FString(""));
	type_ = ini.ReadString(section, "Type", FString(""));

	return true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int NNScripts::LoadFromTextFile(const char *file_name)
{
	FFastIni ini = FFastIni(file_name);
	for (int is = 0 ; is < ini.GetSectionSize() ; is++) {
		NNScript &script = Add(NNScript());
		script.LoadFromIniSection(ini, ini.GetSectionName(is));
	}

	return true;
}
//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
#pragma package(smart_init)
