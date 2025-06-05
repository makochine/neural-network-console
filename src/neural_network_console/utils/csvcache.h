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

#ifndef csvcacheH
#define csvcacheH

#include <System.Classes.hpp>
#include "flist.h"
#include "fstring.h"
#include "csv_dataset.h"
#include "wavefile.h"

using namespace elfe;
//---------------------------------------------------------------------------
enum class CSVCacheFileType : int {
CSV,
Wav,
};
//---------------------------------------------------------------------------
class CSVMatrix : FListT<float>
{
public:
	CSVMatrix() :
		num_col_(0), num_row_(0), max_(1.0f), min_(-1.0f) {}
private:
	CSVMatrix(const CSVMatrix &source);
public:
	void FromCSVDataset(const CSVDataset &csv_dataset);
	void FromWavFile(const TWaveFile &wave_file);
	int GetNumCol() const {return num_col_;}
	int GetNumRow() const {return num_row_;}

	float GetValue(int row, int col) const {return GetAt(col + row * num_col_);}
	float GetMin() const {return min_;}
	float GetMax() const {return max_;}
private:
	int num_col_;
	int num_row_;
	float max_, min_;
};
//---------------------------------------------------------------------------
class CSVCache
{
public:
	CSVCache() : enabled_(false), requested_(false), type_(CSVCacheFileType::CSV){};
	__fastcall virtual ~CSVCache() { Clear();};
	int Clear();
public:
	int LoadCSV();
public:
	void SetFileName(const char *file_name) {file_name_ = file_name;};
	const FString &GetFileName() const {return file_name_;};
	void SetDatasetPath(const char *dataset_path) {dataset_path_ = dataset_path;};
	const FString &GetDatasetPath() const {return dataset_path_;};
	void SetCSVCacheFileType(CSVCacheFileType type) {type_ = type;};
	const CSVCacheFileType GetCSVCacheFileType() const {return type_;};
	void SetRequest(int requested) {requested_ = requested;};
	int GetRequest() const {return requested_;};
	int GetEnabled() const {return enabled_;}
	const CSVMatrix *GetCSV() const {return enabled_ ? &csv_ : NULL;};
private:
	FString file_name_;
	FString dataset_path_;
	CSVCacheFileType type_;
	CSVMatrix csv_;
	int enabled_;
	int requested_;
};
//---------------------------------------------------------------------------
class CSVCacheList : public TThread
{
public:
	CSVCacheList() : TThread(false), updating_(false), update_count_(0), running_(false), load_index_(0) {};
	virtual __fastcall ~CSVCacheList() {Clear(); };
	int Clear();
	int ClearCache();
public:
	void StartUpdate();
	void RequestCSV(const char *dataset_path, const char *file_name, CSVCacheFileType type);
	void EndUpdate();
	int GetUpdateCount() const {return update_count_;};

	const CSVMatrix *GetCSV(const char *file_name) const;
public:
	void __fastcall Execute();
private:
	void __fastcall LoadCSV();
private:
	FListP<CSVCache> csv_list_;
	int updating_;
	int running_;
	int load_index_;
	int update_count_;
};
//---------------------------------------------------------------------------
#endif
