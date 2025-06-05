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

#ifndef nn_structure_search_policyH
#define nn_structure_search_policyH
//---------------------------------------------------------------------------
#include "flist.h"
#include "fstring.h"
#include "ffastini.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
class NNStructureSearchPolicy
{
public:
	NNStructureSearchPolicy() {};
	virtual ~NNStructureSearchPolicy() {};
public:
	int LoadFromIniSection(FFastIni &ini, const char *section);
public:
	const FStringList &GetConditions() const {return conditions_;};
	const FStringList &GetActions() const {return actions_;};
private:
	FString name_;
	FStringList conditions_;
	FStringList actions_;
};

class NNStructureSearchPolicies : public FListP<NNStructureSearchPolicy>
{
public:
	NNStructureSearchPolicies() {};
	virtual ~NNStructureSearchPolicies() {Clear(); };
public:
	int LoadFromTextFile(const char *file_name);
};

//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
#endif
