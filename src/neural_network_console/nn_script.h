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

#ifndef nn_scriptH
#define nn_scriptH
//---------------------------------------------------------------------------
#include "flist.h"
#include "fstring.h"
#include "ffastini.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
class NNScript
{
public:
	NNScript() {};
	virtual ~NNScript() {};
public:
	int LoadFromIniSection(FFastIni &ini, const char *section);
public:
	const FString &GetName() const {return name_;};
	const FString &GetCLI() const {return cli_;};
	const FString &GetType() const {return type_;};
private:
	FString name_;
	FString cli_;
    FString type_;
};

class NNScripts : public FListP<NNScript>
{
public:
	NNScripts() {};
	virtual ~NNScripts() {Clear(); };
public:
	int LoadFromTextFile(const char *file_name);
};

//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
#endif
