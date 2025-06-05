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

#pragma hdrstop

#include "nn_structure_search_policy.h"

#include "ffastini.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
int NNStructureSearchPolicy::LoadFromIniSection(FFastIni &ini, const char *section)
{
	name_ = FString(section);
	conditions_.SplitStr(ini.ReadString(section, "Cond", FString("")), ",");
	actions_.SplitStr(ini.ReadString(section, "Action", FString("")), FString(","), FString("()"), false);

	return true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int NNStructureSearchPolicies::LoadFromTextFile(const char *file_name)
{
	FFastIni ini = FFastIni(file_name);
	for (int is = 0 ; is < ini.GetSectionSize() ; is++) {
		NNStructureSearchPolicy &policy = Add(NNStructureSearchPolicy());
		policy.LoadFromIniSection(ini, ini.GetSectionName(is));
	}

	return true;
}
//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
#pragma package(smart_init)
