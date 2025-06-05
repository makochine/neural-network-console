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

#ifndef solver_componentH
#define solver_componentH

#include "flist.h"
#include "fstring.h"
#include "ffastini.h"

namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
class SolverComponent
{
public:
	SolverComponent() : default_(0) {};
	virtual ~SolverComponent() {};
public:
	int LoadFromSection(FFastIni *ini, FString section);

	const FString &GetName() const {return name_;};
	const FString &GetReference() const {return reference_;};
	const FString &GetArgumentName() const {return argument_name_;};
	const FStringList &GetParamNames() const {return param_names_;}
	const FStringList &GetParamArgNames() const {return param_arg_names_;}
	const FListT<double> &GetDefaultValues() const {return default_values_;}
	int GetDefault() const {return default_;}
private:
	FString name_;
	FString reference_;
	FString argument_name_;
	FStringList param_names_;
	FStringList param_arg_names_;
	FListT<double> default_values_;
	int default_;
};
//---------------------------------------------------------------------------
class SolverComponents : public FListP<SolverComponent>
{
public:
	SolverComponents() : default_solver_index_(0) {};
	virtual ~SolverComponents() {};
public:
	int LoadFromTextFile(const char *file_name);
public:
	int GetDefaultSolverIndex() const {return default_solver_index_;};
	int GetSolverComponentIndexByName(FString name) const;
private:
	int default_solver_index_;
};
//---------------------------------------------------------------------------
}
#endif
