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

#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include "flist.h"
#include "fstring.h"

#if ( defined(_WIN32) || defined (_WIN64) || defined(WIN32) || defined (WIN64) )
#include <io.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

namespace elfe {

/*****************************************************************************/
//コンストラクタ
FString::FString() : _buffer("")
{
}
/*****************************************************************************/
//コンストラクタ
FString::FString(const char *str) : _buffer(str ? str : "")
{
}
/*****************************************************************************/
//コンストラクタ
FString::FString(const bool value) : _buffer(std::to_string(value))
{
}
/*****************************************************************************/
//コンストラクタ
FString::FString(const int value) : _buffer(std::to_string(value))
{
}
/*****************************************************************************/
//コンストラクタ
FString::FString(const unsigned int value) : _buffer(std::to_string(value))
{
}
/*****************************************************************************/
//コンストラクタ(16進数)
FString::FString(const int value, const int digit)
{
	int maxDigit = (digit > 16) ? 16 : digit;
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%x", value);
	int len = std::strlen(buf);
	int pad = maxDigit - len;
	_buffer = (pad > 0 && pad < 16) ? std::string(pad, '0') + buf : buf;
}
/*****************************************************************************/
//コンストラクタ
FString::FString(const float value)
{
	char buf[48];
	std::snprintf(buf, sizeof(buf), "%.10g", value);
	_buffer = buf;
}
/*****************************************************************************/
//コンストラクタ
FString::FString(const double value)
{
	char buf[64];
	std::snprintf(buf, sizeof(buf), "%.16g", value);
	_buffer = buf;
}
/*****************************************************************************/
//コンストラクタ
FString::FString(const FString &Str) : _buffer(Str._buffer)
{
}
/*****************************************************************************/
//デストラクタ
FString::~FString()
{
}
/*****************************************************************************/
//代入
FString &FString::operator =(const FString &Str)
{
	_buffer = Str._buffer;
	return *this;
}
/*****************************************************************************/
//代入
FString &FString::operator =(const char *str)
{
	_buffer = str ? str : "";
    return *this;
}
/*****************************************************************************/
//追加
FString FString::operator +(const FString &Str) const
{
    FString result;
	result._buffer = _buffer + Str._buffer;
    return result;
}
/*****************************************************************************/
//追加
FString FString::operator +(const char *str) const
{
    FString result;
	result._buffer = _buffer + (str ? str : "");
    return result;
}
/*****************************************************************************/
//追加代入
FString &FString::operator +=(const FString &Str)
{
	_buffer += Str._buffer;
	return *this;
}
/*****************************************************************************/
//追加代入
FString &FString::operator +=(const char *str)
{
	_buffer += str ? str : "";
    return *this;
}
/*****************************************************************************/
//同じ
int FString::operator ==(const FString &Str) const
{
	return _buffer == Str._buffer;
}
/*****************************************************************************/
//同じ
int FString::operator ==(const char *str) const
{
	return _buffer == (str ? str : "");
}
/*****************************************************************************/
//違う
int FString::operator !=(const FString &Str) const
{
	return _buffer != Str._buffer;
}
/*****************************************************************************/
//違う
int FString::operator !=(const char *str) const
{
	return _buffer != (str ? str : "");
}
/*****************************************************************************/
//>
int FString::operator >(const FString &Str) const
{
	return _buffer > Str._buffer;
}
/*****************************************************************************/
//>
int FString::operator >(const char *str) const
{
	return _buffer > (str ? str : "");
}
/*****************************************************************************/
//<
int FString::operator <(const FString &Str) const
{
	return _buffer < Str._buffer;
}
/*****************************************************************************/
//<
int FString::operator <(const char *str) const
{
	return _buffer < (str ? str : "");
}
/*****************************************************************************/
//検索
int FString::Pos(const FString &Str) const
{
	size_t pos = std::string::npos;

	if (0 < Str._buffer.length()) {
		pos = _buffer.find(Str._buffer);
	}
	return (pos != std::string::npos) ? static_cast<int>(pos + 1) : 0;
}
/*****************************************************************************/
//検索
int FString::Pos(const char *str) const
{
	size_t pos = std::string::npos;
	if (str) {
		pos = _buffer.find(str);
	}
	return (pos != std::string::npos) ? static_cast<int>(pos + 1): 0;
}
/*****************************************************************************/
//検索
int FString::PosChar(const char c) const
{
	size_t pos = _buffer.find(c);
	return (pos != std::string::npos) ? static_cast<int>(pos + 1) : 0;
}
/*****************************************************************************/
//検索
int FString::PosCharR(const char c) const
{
	size_t pos = _buffer.rfind(c);
	return (pos != std::string::npos) ? static_cast<int>(pos + 1) : 0;
}
/*****************************************************************************/
//参照
const char *FString::PChar() const
{
	return _buffer.c_str();
}
/*****************************************************************************/
//長さ
int FString::Length() const
{
	return static_cast<int>(_buffer.length());
}
/*****************************************************************************/
//部分文字列
FString FString::SubString(size_t start , size_t len) const
{
	if (start > 0) {
		start--;
	}
	else {
		len = len + start - 1;
        start = 0;
    }

	FString result;
	if ((start < _buffer.length()) && (1 <= len)) {
		result._buffer = _buffer.substr(start, len);
	}
	return result;
}
/*****************************************************************************/
FString FString::SplitBy(const char *splitter)
{
	return SplitStrBy(*this, splitter);
}
/*****************************************************************************/
FString FString::SplitBy(const FString &splitter)
{
	const char* ptr = nullptr;

	if (0 <splitter._buffer.length()) {
		ptr = splitter.PChar();
	}
	return SplitStrBy(*this, ptr);
}
/*****************************************************************************/
FString FString::SplitBy(char c)
{
	return SplitStrBy(*this, c);
}
/*****************************************************************************/
FString FString::ReplaceStr(const char *old_str, const char *new_str) const
{

	return ReplaceStr(FString(old_str),FString(new_str));
}
/*****************************************************************************/
FString FString::ReplaceStr(const FString &From, const FString &To) const
{
	FString result;
	FString S = *this;
	while ( true ) {
		result += S.SplitBy(From);
		if (S == "") {
			break;
		}
		result += To;
	}

	return result;
}
/*****************************************************************************/
FString FString::ToLower() const
{
	FString result(_buffer.c_str());

	std::transform(result._buffer.begin(), result._buffer.end(), result._buffer.begin(), ::tolower);
	return result;
}
/*****************************************************************************/
FString FString::ToUpper() const
{
	FString result(_buffer.c_str());

	std::transform(result._buffer.begin(), result._buffer.end(), result._buffer.begin(), ::toupper);
	return result;
}
/*****************************************************************************/
FString FString::InsertThousandSeparator() const
{
	FString source = *this;
	FString result;

	while (source != ""){
		const int p = source.Length() > 3 ? source.Length() - 2 : 1;
		FString thousand = source.SubString(p, 3);
		result = thousand + result;
		if (p > 1) {
			source = source.SubString(1, p - 1);
			result = FString(",") + result;
		}else {
			source = FString("");
		}
	}

	return result;
}
/*****************************************************************************/
//値として
int FString::AsInt() const
{
	int result = 0;
	size_t pos = this->PosChar('x');
	try {
		if (0 < pos) {
			pos--;
			result = std::stoi(_buffer, &pos, 16);
		}
		else {
			result = std::stoi(_buffer);
		}
	} catch (const std::invalid_argument& e) {
		;
	}
	return result;
}
/*****************************************************************************/
unsigned int FString::AsUnsignedInt() const
{
	unsigned long result = 0;
	size_t pos = this->PosChar('x');
	try {
		if (0 < pos) {
			pos--;
			result = std::stoul(_buffer, &pos, 16);
		}
		else {
			result = std::stoul(_buffer);
		}
	} catch (const std::invalid_argument& e) {
		;
	}
	return (unsigned int)result;
}
/*****************************************************************************/
double FString::AsFloat() const
{
	double result = 0.0;
	try {
		result = stod(_buffer);
	} catch (const std::invalid_argument& e) {
		;
	}
	return result;
}
FString FString::SplitStrBy(FString &S, const char *splitter)
{
	FString result;
	int p = S.Pos(splitter);
	if ( p > 0 ) {
		result = S.SubString(1, p - 1);
		S = S.SubString(p + strlen(splitter), S.Length());
	}else{
		result = S;
		S = FString("");
	}
	return result;
}
/*****************************************************************************/
FString FString::SplitStrBy(FString &S, char c)
{
	FString result = FString("");
	int p = S.PosChar(c);
	if ( p > 0 ) {
		result = S.SubString(1, p - 1);
		S = S.SubString(p + 1, S.Length() - p);
	}else{
		result = S;
		S = FString("");
	}
	return result;
}
/*****************************************************************************/


/*****************************************************************************/
/** class FStringList */
/*****************************************************************************/

/*****************************************************************************/
//参照
//FString &FStringList::GetAt(int nindex) const
//{
//	return *pointers_[nindex];
//}

/*****************************************************************************/
//Key=Valueとの書式の列を探し、Valueを得る
FString FStringList::Values(FString key) const
{
	FString key_str = key + FString("=");
	int length = key_str.Length();
	for ( int i = 0; i < GetSize(); ++i ) {
		if ( GetAt(i).SubString(1, length) == key_str ) {
			return GetAt(i).SubString(length + 1, GetAt(i).Length());
		}
	}
	return FString("");
}

/*****************************************************************************/
bool FStringList::LoadFromFile(const char *filepath, unsigned int max_file_size)
{
	Clear();

	std::ifstream ifs(filepath, std::ios::binary);
	if (ifs.fail()) {
		return false;
	}

	ifs.seekg(0, ifs.end);
	size_t size = ifs.tellg();
	ifs.seekg(0, ifs.beg);
	if (0 < max_file_size && max_file_size < size) {
		size = max_file_size;
	}

	std::string str;
	str.resize(size);
	ifs.read(&str[0], size);

	size_t startline = 0;
	while (startline < size) {
		size_t endline = str.find('\n', startline);
		if (endline == std::string::npos) {
			endline = size;
		}
		std::string line = str.substr(startline, endline - startline);
		while (!line.empty() && iscntrl(line.back())) {
			line.pop_back();
		}
		Add(FString(line.c_str()));
		startline = endline + 1;
	}

	// Delete UTF-8 BOM
	static const unsigned char bom[4] = {0xef, 0xbb, 0xbf, 0x00};
	if (GetSize() && GetAt(0).SubString(1, 3) == (const char*)bom) {
		GetAt(0) = GetAt(0).SubString(4, GetAt(0).Length() - 3);
	}
	return true;
}

/*****************************************************************************/
int FStringList::SaveToFile(const char *filepath) const
{
	std::ofstream ofs(filepath);
	if (ofs.fail()) {
		return false;
	}
	ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	try {
		for ( int i = 0; i < GetSize(); ++i ) {
			ofs << GetAt(i).PChar() << std::endl;
		}
	} catch (...) {
		return false;
	}
	return true;
}

/*****************************************************************************/
//Input文字列をSplitterで分割し、Outputに入れる。GroupingStrで囲まれた範囲は分割されない（"()"など）
void FStringList::SplitStr(const FString &Input, const FString &splitter, const FString &GroupingStr, int remove_grouping_str)
{
	const int input_length = Input.Length();
	const int splitter_length = splitter.Length();

	int pos = 0;
	int start = 0, len = 0;
	int groupcount = 0;
	const char splitter_1stchr = splitter.PChar()[0];
	const char groupingchr_s = GroupingStr.Length() ? GroupingStr.PChar()[0] : 0;
	const char groupingchr_e = GroupingStr.Length() ? GroupingStr.PChar()[GroupingStr.Length() - 1] : 0;
	while ( pos < input_length ) {
		int split = false;
		if (splitter_length > 1) {
			const FString S1 = Input.SubString(pos + 1, splitter_length);
			split = S1 == splitter;
		}else {
			split = Input.PChar()[pos] == splitter_1stchr;
		}
		if (split && groupcount == 0) {
			this->Add(Input.SubString(start + 1, len));
			pos += splitter_length;
			len = 0;
			start = pos;
		}else{
			const char c1 = Input.PChar()[pos];
			if (c1 == groupingchr_s) {
				if (groupingchr_s != groupingchr_e || groupcount == 0) {
					groupcount++;
					if (remove_grouping_str) {
						start++;
						len--;
					}
				}
				else {
					groupcount--;
					if (remove_grouping_str) len--;
				}
			}
			else if (c1 ==groupingchr_e) {
				groupcount--;
				if (remove_grouping_str) len--;
			}
			else {
				;
			}
			len++;
			pos++;
		}
	}
	this->Add(Input.SubString(start + 1, len));
}

/*****************************************************************************/
void FStringList::SplitStr(const FString &_src, const char *splitter)
{
	FString src = _src;
	this->Clear();
	int pos;
	while ( (pos = src.Pos(splitter)) != 0 ) {
		this->Add(src.SubString(1, pos - 1));
		src = src.SubString(pos + strlen(splitter), src.Length());
	}
	this->Add(src);
}
/*****************************************************************************/
FString FStringList::GetText(char separator) const
{
	if (GetSize()) {
		FListT<int> length;
		int sum = 0;
		for (int i = 0 ; i < GetSize() ; i++) {
			const int len = GetAt(i).Length();
			length.Add(len);
			sum += len;
		}
		const int size = sum + GetSize();
		char *result_s = new char[size];
		int pos = 0;
		for (int i = 0 ; i < GetSize() ; i++) {
			const int len = length.GetAt(i);
			memcpy(result_s + pos, GetAt(i).PChar(), sizeof(char) * len);
			pos += len;
			result_s[pos++] = i < GetSize() - 1 ? separator : '\0';
		}
		FString result = FString(result_s);
		delete[] result_s;
		return result;
	}
	return FString("");
}
/*****************************************************************************/
int FStringList::operator ==(const FStringList &list) const
{
	if (list.GetSize() != GetSize()) {
		return false;
	}
	for (int i = 0 ; i < GetSize() ; i++) {
		if (list.GetAt(i) != GetAt(i)) {
			return false;
		}
	}
    return true;
}
/*****************************************************************************/

}	//namespace elfe

