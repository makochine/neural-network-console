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

#ifndef __ELFE_FSTRING_H__
#define __ELFE_FSTRING_H__

#include <stdio.h>
#include <string>
#include "flist.h"

namespace elfe{


static const int MINFSTRINGBUFSIZE = 2;

/*****************************************************************************/
/** FString */
/*****************************************************************************/

class FString
{
// init
public:
	/** constructor */
	FString();
	explicit FString(const char *str);
	explicit FString(const bool value);
	explicit FString(const int value);
	explicit FString(const unsigned int value);
	explicit FString(const int value, const int digit);	// HEX
	explicit FString(const float value);
	explicit FString(const double value);
	/** copy constructor */
	FString(const FString &Str);
	/** destructor */
	virtual ~FString();
// operands
public:
	FString &operator =(const char *str);
	FString &operator =(const FString &Str);
	FString operator +(const char *str) const;
	FString operator +(const FString &Str) const;
	FString &operator +=(const char *str);
	FString &operator +=(const FString &Str);
	int operator ==(const char *str) const;
	int operator ==(const FString &Str) const;
	int operator !=(const char *str) const;
	int operator !=(const FString &Str) const;
	int operator >(const char *str) const;
	int operator >(const FString &Str) const;
	int operator <(const char *str) const;
	int operator <(const FString &Str) const;
// get
public:
	const char *PChar() const;
	int AsInt() const;
	unsigned int AsUnsignedInt() const;
	double AsFloat() const;
// attribute
public:
	int Length() const;
	int Pos(const char *str) const;
	int Pos(const FString &Str) const;
	int PosChar(char c) const;
	int PosCharR(char c) const;
// manipulate
public:
	FString SubString(size_t start , size_t len) const;					/** sub string */
	FString SplitBy(const char *splitter);								/** tokenize */
	FString SplitBy(const FString &splitter);							/** tokenize */
	FString SplitBy(char c);											/** tokenize */
	FString ReplaceStr(const FString &From, const FString &To) const;					/** replace */
	FString ReplaceStr(const char *old_str, const char *new_str) const;	/** replace */
	FString ToLower() const;										    /** lower case */
	FString ToUpper() const;                                            /** upper case */
	FString InsertThousandSeparator() const;							/** insert thousand separator */
// static funcs
public:
	//特定の文字列で文字列分割
	static FString SplitStrBy(FString &S, const char *splitter);
	//特定の文字で文字列分割
	static FString SplitStrBy(FString &S, char c);
private:
	std::string _buffer;
};

/*****************************************************************************/
/** FStringList */
/*****************************************************************************/

class FStringList : public FListP<FString>
{
public:
//	FString Add(FString &item) { return FListP<FString>::Add(item); }
//	FString Add(const char *item) { return FListP<FString>::Add(FString(item)); }
	//Key=Valueとの書式の列を探し、Valueを得る
	FString Values(FString Key) const;
	//ファイルから読み込み
	bool LoadFromFile(const char *filepath, unsigned int max_file_size = 0);
	//バイナリファイルから読み込み
	int LoadFromFile(FILE* handle);
	//ファイルへ書き込み
	int SaveToFile(const char *filepath) const;
	//バイナリファイルへ書き込み
	int SaveToFile(FILE *handle) const;
	//Input文字列をSplitterで分割し、Outputに入れる。GroupingStrで囲まれた範囲は分割されない（"()"など）
	void SplitStr(const FString &Input, const FString &splitter, const FString &GroupingStr, int remove_grouping_str = true);
	void SplitStr(const FString &src, const char *splitter);
	//1行のテキストを得る
	FString GetText(char separator = '\n') const;

	//Operator
public:
	int operator ==(const FStringList &list) const;
	int operator !=(const FStringList &list) const {return !(*this == list);}
};

}	// namespace elfe {

#endif	//__ELFE_FSTRING_H__
