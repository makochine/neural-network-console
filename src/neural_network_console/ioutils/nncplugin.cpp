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

#pragma hdrstop

#include "nncplugin.h"

#include "ffastini.h"

namespace sdeep_console{
//---------------------------------------------------------------------------
NNCPlugin &NNCPlugin::operator=(const NNCPlugin &source)
{
	name_ = source.name_;
	file_name_ = source.file_name_;
	description_ = source.description_;

	date_modified_ = source.date_modified_;
	found_ = source.found_;
	need_update_ = source.need_update_;

	original_index_ = source.original_index_;

	arg_option_ = source.arg_option_;
	arg_name_ = source.arg_name_;
	arg_description_ = source.arg_description_;
	arg_default_ = source.arg_default_;

	return *this;
}
//---------------------------------------------------------------------------
int NNCPlugin::operator==(const NNCPlugin &plugin) const
{
    return file_name_ == plugin.file_name_;
}
//---------------------------------------------------------------------------
int NNCPlugin::operator<(const NNCPlugin &plugin) const
{
	return file_name_ < plugin.file_name_;
}
//---------------------------------------------------------------------------
int NNCPlugin::operator>(const NNCPlugin &plugin) const
{
	return file_name_ > plugin.file_name_;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int NNCPlugins::RemoveNotFoundPlugins()
{
	int result = false;

	for (int i = GetSize() - 1 ; i >= 0 ; i--){
		if (!GetAt(i).found_){
			Delete(i);
            result = true;
		}
	}

	return result;
}
//---------------------------------------------------------------------------
int NNCPlugins::LoadFromIni(FString file_name)
{
	Clear();
	FFastIni plugin_ini(file_name.PChar());

	FStringList sections;
	plugin_ini.ReadSections(&sections);

	for (int i = 0 ; i < sections.GetSize() ; i++){
		const FString &section = sections.GetAt(i);
		NNCPlugin &plugin = Add();
		plugin.file_name_ = section;
		plugin.name_ = plugin_ini.ReadString(section, FString("Name"), FString(""));
		plugin.date_modified_ = plugin_ini.ReadDouble(section, FString("DateModified"), 0.0);
		plugin.description_ = plugin_ini.ReadString(section, FString("Description"), FString(""));

		const int num_args = plugin_ini.ReadInteger(section, FString("NumArgs"), 0);
		for (int ia = 0 ; ia < num_args ; ia++){
			FString option = plugin_ini.ReadString(section, FString("Arg") + FString(ia) + FString("_Option"), FString(""));
			if (section.Pos("_Training") == 1 && (option == "c" || option ==  "o" || option == "r")){
				// Remove c(config), o(output) and r(resume) option from training plugins
			}else {
				plugin.arg_option_.Add(option);
				plugin.arg_name_.Add(plugin_ini.ReadString(section, FString("Arg") + FString(ia) + FString("_Name"), FString("")));
				plugin.arg_description_.Add(plugin_ini.ReadString(section, FString("Arg") + FString(ia) + FString("_Desc"), FString("")));
				plugin.arg_default_.Add(plugin_ini.ReadString(section, FString("Arg") + FString(ia) + FString("_Default"), FString("")));
            }
		}
	}

	Sort();

	return true;
}
//---------------------------------------------------------------------------
int NNCPlugins::SaveToIni(FString file_name) const
{
	FFastIni plugin_ini(file_name.PChar());
	plugin_ini.Clear(true);

	for (int i = 0 ; i < GetSize() ; i++){
		const NNCPlugin &plugin = GetAt(i);
		plugin_ini.Write<FString>(plugin.file_name_, FString("Name"), plugin.name_);
		plugin_ini.Write<double>(plugin.file_name_, FString("DateModified"), plugin.date_modified_);
		plugin_ini.Write<FString>(plugin.file_name_, FString("Description"), plugin.description_);

		plugin_ini.Write<int>(plugin.file_name_, FString("NeedUpdate"), plugin.need_update_);

		plugin_ini.Write<int>(plugin.file_name_, FString("NumArgs"), plugin.arg_option_.GetSize());
		for (int ia = 0 ; ia < plugin.arg_option_.GetSize() ; ia++){
			plugin_ini.Write<FString>(plugin.file_name_, FString("Arg") + FString(ia) + FString("_Option"), plugin.arg_option_.GetAt(ia));
			plugin_ini.Write<FString>(plugin.file_name_, FString("Arg") + FString(ia) + FString("_Name"), plugin.arg_name_.GetAt(ia));
			plugin_ini.Write<FString>(plugin.file_name_, FString("Arg") + FString(ia) + FString("_Desc"), plugin.arg_description_.GetAt(ia));
			plugin_ini.Write<FString>(plugin.file_name_, FString("Arg") + FString(ia) + FString("_Default"), plugin.arg_default_.GetAt(ia));
		}
	}

	plugin_ini.Flush();

	return true;
}
//---------------------------------------------------------------------------
const NNCPlugin &NNCPlugins::GetPlugin(FString name) const
{
	for (int i = 0 ; i < GetSize() ; i++){
		if (GetAt(i).name_ == name){
			return GetAt(i);
		}
	}

	static NNCPlugin dummy;
	return dummy;
}
//---------------------------------------------------------------------------
NNCPlugins NNCPlugins::GetPlugins(FString dir) const
{
	NNCPlugins result;
	for (int i = 0 ; i < GetSize() ; i++){
		if (GetAt(i).file_name_.SubString(1, dir.Length() + 1) == dir + FString("\\")){
			NNCPlugin &plugin = result.Add(GetAt(i));
			plugin.file_name_ = plugin.file_name_.SubString(dir.Length() + 2, plugin.file_name_.Length() - 1 - dir.Length());
			plugin.original_index_ = i;
		}
	}
    return result;
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
