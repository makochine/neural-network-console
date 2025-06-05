// Copyright 2025 Sony Group Corporation.
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

#if ( defined(_WIN32) || defined (_WIN64) || defined(WIN32) || defined (WIN64) )
#include <io.h>
#else
#include <unistd.h>
#endif

#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/optional.hpp>
#include "flist.h"
#include "fstring.h"
#include "ffastini.h"

namespace pt = boost::property_tree;

namespace elfe {

/*****************************************************************************/
/** FFastIni */
/*****************************************************************************/
/** constructor */
FFastIni::FFastIni(const char *file_name)
	: file_name_(file_name), is_modified_(false), is_ready_(false)
{
	FFastIni::Open(file_name);
}

/** destructor */
FFastIni::~FFastIni()
{
	FFastIni::Flush();      // write
	FFastIni::Close();      // close
}

/** clear */
void FFastIni::Clear(int modify)
{
	FFastIni::Close();	// make sure to free current
	pt_.clear();
	is_ready_ = true;
	is_modified_ = modify;
}

/** open */
int FFastIni::Open(const char *file_name)
{
	Close();
	try {
		if (file_name != nullptr) {
			pt::read_ini(file_name, pt_);  // fast load if normal ini format
			is_ready_ = true;
			is_modified_ = false;
		}
	} catch (const std::exception& e) {
		;
	}

	if (!is_ready_) {
		// retry for loading layer_component
		FStringList string_list;
		if (string_list.LoadFromFile(file_name)) {
			LoadFromStringList(string_list);
			is_ready_ = true;
			is_modified_ = false;
	}
	}

	if (!is_ready_) {
		//success if file is not found
		is_ready_ = access(file_name, 0) != 0;
	}
	return is_ready_;
}
void FFastIni::LoadFromStringList(const FStringList &string_list)
{
	std::string section = "";

	for ( int i = 0; i < string_list.GetSize(); ++i ) {
		FString str = string_list.GetAt(i);
		if (str.SubString(1, 1) == "[") {
			section = str.SubString(2, str.Length() - 2).PChar();
			section += "/";
		}
		else {
			std::string key = str.SplitBy("=").PChar();
			std::string value = str.PChar();
			if ((key[0] != ';') && (key[0] != '#')) {
				pt_.put(pt::ptree::path_type(section + key, '/'), value);
			}
		}
	}
	is_modified_ = false;
}

/** close */
void FFastIni::Close()
{
	pt_.clear();
	is_ready_ = false;
}

/** reload */
int FFastIni::Reload()
{
	return FFastIni::Open(file_name_.PChar());
}

/** flush to file */
int FFastIni::Flush()
{
	bool result = true;
	if ( is_ready_ && is_modified_ ) {
		try {
			pt::write_ini(file_name_.PChar(), pt_);
			is_modified_ = false;
		} catch (const std::exception& e) {
			result = false;
		}
	}
	return result;
}
/** read string */
FString FFastIni::ReadString(const char *section, const char *key, const FString &default_val)
{
	if (section != nullptr && key != nullptr) {
		const std::string sectionkey = std::string(section) + "/" + key;
		std::string val = pt_.get(pt::ptree::path_type(sectionkey, '/'), default_val.PChar());
		return FString(val.c_str());
	}
	else {
		return default_val;
	}
}

/** read boolean */
bool FFastIni::ReadBool(const char *section, const char *key, const bool &default_val)
{
	bool result = default_val;
	if (section != nullptr && key != nullptr) {
		const std::string sectionkey = std::string(section) + "/" + key;
		try {
			std::string value = pt_.get<std::string>(pt::ptree::path_type(sectionkey, '/'));
			result = FString(value.c_str()).AsInt() ? true : false;
		} catch (pt::ptree_error& e) {
			;
		}
	}
	return result;
}

/** read signed integer */
int FFastIni::ReadInteger(const char *section, const char *key, const int &default_val)
{
	int result = default_val;
	if (section != nullptr && key != nullptr) {
		const std::string sectionkey = std::string(section) + "/" + key;
		try {
			std::string value = pt_.get<std::string>(pt::ptree::path_type(sectionkey, '/'));
			result = FString(value.c_str()).AsInt();
		} catch (pt::ptree_error& e) {
			;
		}
	}
	return result;
}

/** read unsigned integer */
unsigned int FFastIni::ReadUnsignedInteger(const char *section, const char *key, const unsigned int &default_val)
{
	unsigned int result = default_val;
	if (section != nullptr && key != nullptr) {
		const std::string sectionkey = std::string(section) + "/" + key;
		try {
			std::string value = pt_.get<std::string>(pt::ptree::path_type(sectionkey, '/'));
			result = FString(value.c_str()).AsUnsignedInt();
		} catch (pt::ptree_error& e) {
			;
		}
	}
	return result;
}

/** read float */
float FFastIni::ReadFloat(const char *section, const char *key, const float &default_val)
{
	float result = default_val;
	if (section != nullptr && key != nullptr) {
		const std::string sectionkey = std::string(section) + "/" + key;
		try {
			std::string value = pt_.get<std::string>(pt::ptree::path_type(sectionkey, '/'));
			result = static_cast<float>(FString(value.c_str()).AsFloat());
		} catch (pt::ptree_error& e) {
			;
		}
	}
	return result;
}

/** read double */
double FFastIni::ReadDouble(const char *section, const char *key, const double &default_val)
{
	double result = default_val;
	if (section != nullptr && key != nullptr) {
		const std::string sectionkey = std::string(section) + "/" + key;
		try {
			std::string value = pt_.get<std::string>(pt::ptree::path_type(sectionkey, '/'));
			result = FString(value.c_str()).AsFloat();
		} catch (pt::ptree_error& e) {
			;
		}
	}
	return result;
}

/** read section names */
void FFastIni::ReadSections(FStringList* string_list)
{
	string_list->Clear();

	for (const auto& item : pt_) {
		const std::string& section = item.first;
		if (!item.second.empty()) {
			// This item has direct children
			string_list->Add(FString(section.c_str()));
		}
	}
}

/** read section values */
void FFastIni::ReadSectionValues(const char *section, FStringList* string_list)
{
	if (string_list != nullptr) {
		string_list->Clear();

		if (section != nullptr) {
			if (auto items = pt_.get_child_optional(pt::ptree::path_type(section, '/'))) {
				for (const auto& item : *items) {
					const std::string keyvalue = item.first + "=" + item.second.data();
					string_list->Add(FString(keyvalue.c_str()));
				}
			}
		}
	}
}

/** read section keys */
void FFastIni::ReadSectionKeys(const char *section, FStringList* string_list)
{
	if (string_list != nullptr) {
		string_list->Clear();

		if (section != nullptr) {
			if (auto items = pt_.get_child_optional(pt::ptree::path_type(section, '/'))) {
				for (const auto& item : *items) {
					string_list->Add(FString(item.first.c_str()));
				}
			}
		}
	}
}

/** get section and its children */
FStringList FFastIni::GetStringList()
{
	FStringList sl;

	for (const auto& item : pt_) {
		if (!item.second.empty()) {
			std::string section = "[" + item.first + "]";
			sl.Add(FString(section.c_str()));
			for (const auto& child : item.second) {
				std::string keyvalue = child.first + "=" + child.second.data();
				sl.Add(FString(keyvalue.c_str()));
			}
		}
		else {
			std::string keyvalue = item.first + "=" + item.second.data();
			sl.Add(FString(keyvalue.c_str()));
		}
	}
	return sl;
}

/** erase section */
bool FFastIni::DeleteSection(const char *section)
{
	bool deleted = false;

	if (section != nullptr) {
		if (pt_.erase(section) != 0) {
			is_modified_ = true;
			deleted = true;
		}
	}
	return deleted;
}

/** copy section */
bool FFastIni::CopySection(const char *section_dest, const char *section_from)
{
	bool copied = false;

	if (section_dest != nullptr && section_from != nullptr) {
		if ((pt_.find(section_from) != pt_.not_found()) &&
			(pt_.find(section_dest) != pt_.not_found())) {
			pt_.erase(section_dest);
			pt_.add_child(pt::ptree::path_type(section_dest, '/'), pt_.get_child(pt::ptree::path_type(section_from, '/')));
			is_modified_ = true;
			copied = true;
		}
	}
	return copied;
}

/** delete key */
bool FFastIni::DeleteKey(const char *section, const char *key)
{
	bool deleted = false;

	if (section != nullptr && key != nullptr) {
		if (pt_.find(section) != pt_.not_found()) {
			if (pt_.get_child(pt::ptree::path_type(section, '/')).erase(key) != 0) {
				is_modified_ = true;
				deleted = true;
			}
		}
	}
	return deleted;
}

/** delete key */
bool FFastIni::DeleteKey(const char *section, const FString &key)
{
	return DeleteKey(section,key.PChar());
}

/** get section size */
int FFastIni::GetSectionSize() const
{
	int size = 0;

	// Identical to pt_.size() in the normal case with no root item.
	for (const auto& item : pt_) {
		if (!item.second.empty()) {
			// This item has direct children
			size++;
		}
	}
	return size;
}

/** get section name */
const char *FFastIni::GetSectionName(int index) const
{
	const char* section = NULL;

	if (0 <= index && index < (int)pt_.size()) {
		for (const auto& item : pt_) {
			if (!item.second.empty()) {
				// This item has direct children
				if (index-- == 0) {
					section = item.first.c_str();
					break;
				}
			}
		}
	}
	return section;
}
/*=======================================================*/

/*****************************************************************************/
}	//namespace elfe


