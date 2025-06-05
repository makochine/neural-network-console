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

#include "solver_component.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
int SolverComponent::LoadFromSection(FFastIni *ini, FString section)
{
	name_ = section;
	reference_ = ini->ReadString(section, FString("reference"), FString(""));
	argument_name_ = ini->ReadString(section, FString("argument_name"), FString(""));

	const int num_param = ini->ReadInteger(section, FString("num_param"), 0);
	param_names_.Clear();
	param_arg_names_.Clear();
	default_values_.Clear();

	for (int ip = 0 ; ip < num_param ; ip++) {
		FString s = ini->ReadString(section, FString("param_") + FString(ip), FString(""));
		param_names_.Add(s.SplitBy(";"));
		param_arg_names_.Add(s.SplitBy(";"));
		default_values_.Add(s.AsFloat());
	}
	default_ = ini->ReadBool(section, FString("default"), false);

	return param_names_.GetSize() > 0;
}
//---------------------------------------------------------------------------
int SolverComponents::GetSolverComponentIndexByName(FString name) const
{
	for (int i = 0; i < GetSize(); i++) {
		if (GetAt(i).GetName() == name) {
			return i;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
int SolverComponents::LoadFromTextFile(const char *file_name)
{
	int result = true;
	FFastIni ini = FFastIni(file_name);

	FStringList solvers;
	ini.ReadSections(&solvers);

	for (int i = 0 ; i < solvers.GetSize() ; i++) {
		SolverComponent &solver = Add();
		result &= solver.LoadFromSection(&ini, solvers.GetAt(i));
		if (solver.GetDefault()) {
			default_solver_index_ = i;
		}
	}
	return result;
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
