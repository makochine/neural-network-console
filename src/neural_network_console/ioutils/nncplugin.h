// Copyright 2019,2020,2021 Sony Corporation.
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

#ifndef nncpluginH
#define nncpluginH

#include "flist.h"
#include "fstring.h"
namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
class NNCPlugin
{
public:
	NNCPlugin() : date_modified_(0.0), found_(false), need_update_(false), original_index_(0) {};
	virtual ~NNCPlugin() {};
	NNCPlugin &operator=(const NNCPlugin &source);
	int operator ==(const NNCPlugin &plugin) const;//Check if the file name is same
	int operator <(const NNCPlugin &plugin) const;//Check if the file name is same
	int operator >(const NNCPlugin &plugin) const;//Check if the file name is same
private:
	NNCPlugin (const NNCPlugin &source);
public:
	FString name_;
	FString file_name_;
	FString description_;

	FStringList arg_option_;
	FStringList arg_name_;
	FStringList arg_description_;
	FStringList arg_default_;

	double date_modified_;
	int found_;
	int need_update_;

    int original_index_;
};
//---------------------------------------------------------------------------
class NNCPlugins : public FListP<NNCPlugin>
{
public:
	NNCPlugins(){};
	virtual ~NNCPlugins() {};
public:
	int LoadFromIni(FString file_name);
	int RemoveNotFoundPlugins();
	int SaveToIni(FString file_name) const;
public:
	const NNCPlugin &GetPlugin(FString name) const;
	NNCPlugins GetPlugins(FString dir) const;
};
//---------------------------------------------------------------------------
};
#endif
