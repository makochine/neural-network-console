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

#ifndef __ELFE_FFASTINI_H__
#define __ELFE_FFASTINI_H__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/optional.hpp>

namespace elfe
{

/*****************************************************************************/
/** FFastIni */
/*****************************************************************************/
class FFastIni
{
public:
	explicit FFastIni(const char *file_name);
	virtual ~FFastIni();
	void Clear(int modify = false);
public:

	/** read string */
	FString ReadString(const char *section, const char *key, const FString &default_val);
	FString ReadString(const char *section, const FString &key, const FString &default_val){return ReadString(section,key.PChar(), default_val); }
	FString ReadString(const FString &section, const FString &key, const FString &default_val){ return ReadString(section.PChar(), key.PChar(), default_val); }
	FString ReadString(const FString &section, const char *key, const FString &default_val){ return ReadString(section.PChar(), key, default_val); }
	/** read boolean */
	bool ReadBool(const char *section, const char *key, const bool &default_val);
	bool ReadBool(const char *section, const FString &key, const bool &default_val){ return ReadBool(section, key.PChar(), default_val); }
	bool ReadBool(const FString &section, const FString &key, const bool &default_val){ return ReadBool(section.PChar(),key.PChar(), default_val); }
	bool ReadBool(const FString &section, const char *key, const bool &default_val){ return ReadBool(section.PChar(), key, default_val); }
	/** read signed integer */
	int ReadInteger(const char *section, const char *key, const int &default_val);
	int ReadInteger(const char *section, const FString &key, const int &default_val){ return ReadInteger(section, key.PChar(), default_val); }
	int ReadInteger(const FString &section, const FString &key, const int &default_val){ return ReadInteger(section.PChar(), key.PChar(), default_val); }
	int ReadInteger(const FString &section, const char *key, const int &default_val){ return ReadInteger(section.PChar(), key, default_val); }
	/** read unsigned integer */
	unsigned int ReadUnsignedInteger(const char *section, const char *key, const unsigned int &default_val);
	unsigned int ReadUnsignedInteger(const char *section, const FString &key, const unsigned int &default_val){ return ReadUnsignedInteger(section, key.PChar(), default_val); }
	unsigned int ReadUnsignedInteger(const FString &section, const FString &key, const unsigned int &default_val){ return ReadUnsignedInteger(section.PChar(), key.PChar(), default_val); }
	unsigned int ReadUnsignedInteger(const FString &section, const char *key, const unsigned int &default_val){ return ReadUnsignedInteger(section.PChar(), key, default_val); }
	/** read float */
	float ReadFloat(const char *section, const char *key, const float &default_val);
	float ReadFloat(const char *section, const FString &key, const float &default_val){ return ReadFloat(section, key.PChar(), default_val); }
	float ReadFloat(const FString &section, const FString &key, const float &default_val){ return ReadFloat(section.PChar(), key.PChar(), default_val); }
	float ReadFloat(const FString &section, const char *key, const float &default_val){ return ReadFloat(section.PChar(), key, default_val); }
	/** read double */
	double ReadDouble(const char *section, const char *key, const double &default_val);
	double ReadDouble(const char *section, const FString &key, const double &default_val){ return ReadDouble(section, key.PChar(), default_val); }
	double ReadDouble(const FString &section, const FString &key, const double &default_val){ return ReadDouble(section.PChar(), key.PChar(), default_val); }
	double ReadDouble(const FString &section, const char *key, const double &default_val){ return ReadDouble(section.PChar(), key, default_val); }

	/** read section names to list as string */
	void ReadSections(FStringList* string_list);
	/** read all section key-values to list as string */
	void ReadSectionValues(const char *section, FStringList* string_list);
	/** read all section keys to list as string */
	void ReadSectionKeys(const char *section, FStringList* string_list);

	/** Write. template class need to be capable of transforming into FString */
	template <typename _T> void Write(const char *section, const char *key, _T val)
	{
		if (section != nullptr && key != nullptr) {
			const std::string sectionkey = std::string(section) + "/" + key;
			pt_.put(boost::property_tree::ptree::path_type(sectionkey, '/'), FString(val).PChar());
			is_modified_ = true;
		}
	}
	/** Write. template class need to be capable of transforming into FString */
	template <typename _T> void Write(const char *section, const FString &key, _T val)
	{
		Write(section, key.PChar(),val);
	}
	/** Write. template class need to be capable of transforming into FString */
	template <typename _T> void Write(const FString &section, const FString &key, _T val)
	{
		Write(section.PChar(), key.PChar(),val);
	}
	/** Write. template class need to be capable of transforming into FString */
	template <typename _T> void Write(const FString &section, const char *key, _T val)
	{
		Write(section.PChar(), key,val);
	}

	FStringList GetStringList();

	void SetStringList(const FStringList &sl)
	{
		FFastIni::Close();		// make sure to free current
		LoadFromStringList(sl);
	}

//util
public:
	/** delete section */
	bool DeleteSection(const char *section);
	/** copy section */
	bool CopySection(const char *section_dest, const char *section_from);
	/** delete key */
	bool DeleteKey(const char *section, const char *key);
	/** delete key */
	bool DeleteKey(const char *section, const FString &key);
	/** get section info */
	int GetSectionSize() const;
	const char *GetSectionName(int index) const;
	/** check state */
	bool CheckIfReady() const { return is_ready_; }
	/** reload */
	int Reload();
	/** flush to file */
	int Flush();
// local
private:
	int Open(const char *file_name);
	void LoadFromStringList(const FStringList &string_list);
	void Close();
// member
private:
	FString							file_name_;
	bool							is_modified_;
	bool							is_ready_;
	boost::property_tree::ptree		pt_;

};

}
#endif	//__ELFE_FFASTINI_H__
