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

#ifndef csv_datasetH
#define csv_datasetH
//---------------------------------------------------------------------------
#include <flist.h>
#include <fstring.h>

#if ( defined(_WIN32) || defined (_WIN64) || defined(WIN32) || defined (WIN64) )
#include <io.h>
#else
#include <unistd.h>
#include <signal.h>
#endif
//---------------------------------------------------------------------------
namespace elfe{

#define NAN_VALUE 1.0e+38
#define NAN_VALUE_THRESHOLD 1.0e+37

enum class CSVDatasetError : int {
None,
CantOpenFile,
NotEnoughLines,
DifferentColumnNum,
NotEnoughMemory,
CantWriteLine,
};

class CSVDataset
{
public:
	CSVDataset() : num_column_(0), update_count_(0),
				   last_error_(CSVDatasetError::None), read_only_(false),
				   buf_(NULL), buf_size_(0) {}
	CSVDataset &operator = (const CSVDataset& source);
	virtual ~CSVDataset() {Clear();}
	void Clear();
private:
	CSVDataset(const CSVDataset &source);
public:
	int LoadFromFile(const char *file_name, int has_header = true, int read_only = false);
	int SaveToFile(const char *file_name);
	int SaveToFile_(const char *file_name) const;
	CSVDatasetError GetLastError() const {return last_error_;}
	void Update() {update_count_++;};
	int GetUpdateCount() const {return update_count_;}
public:
	void SetFileName(FString file_name) {file_name_ = file_name;}
	const FString &GetFileName() const {return file_name_;}

	int GetNumData() const {return read_only_ ? ptr_.GetSize() : data_.GetSize();}
	int GetNumColumn() const {return num_column_;}

	const FStringList &GetColumnNames() const {return header_[0];}
	const FString &GetColumnName(int column_index) const {return header_[0].GetAt(column_index);}
	const FStringList &GetRowNames() const {return header_[1];}
	const FString &GetRowName(int row_index) const {return header_[1].GetAt(row_index);}
	void SetTopLeftCell(FString top_left_cell) {top_left_cell_ = top_left_cell;}
	const FString &GetTopLeftCell() const {return top_left_cell_;}

	const char* GetValuePtr(int data_index, int column_index) const;
	FString GetValue(int data_index, int column_index) const;
	FString GetValue(int data_index, FString column_name) const;
	int GetIntValue(int data_index, int column_index) const;
	int GetIntValue(int data_index, FString column_name) const;
	double GetFloatValue(int data_index, int column_index) const;
	double GetFloatValue(int data_index, FString column_name) const;
	FStringList GetData(int data_index) const;

	int GetColumnIndex(FString column_name) const;

	int GetColumnMinMax(int column_index, double &min, double &max) const;
	int GetColumnMinMax(FString column_name, double &min, double &max) const;
	int GetColumnMeanVariance(int column_index, double &mean, double &variance) const;
	double GetColumnCorrel(int column_index1, int column_index2) const;

    int GetModal(const FString &modal_name, int &col_start, int &col_end) const;
public:
	void SetColumnNames(const FString &column_names);
	void SetColumnNames(const FStringList &column_names);
	void SetRowNames(const FString &row_names);
	void SetRowNames(const FStringList &row_names);

	void SetValue(int data_index, int column_index, FString value);
	int SetValue(int data_index, FString column_name, FString value);
	void SetData(int data_index, const FStringList &data);

	void DeleteData(int data_index);
	void DeleteCol(int column_index);
private:
	FString file_name_;
	int num_column_;
	FStringList header_[2];
	FString top_left_cell_;

	int read_only_;

	//read only
	char *buf_;
	long long buf_size_;
	FListP<FListT<char *> > ptr_;

	//read write
	FListP<FStringList> data_;
private:
	int update_count_;
	CSVDatasetError last_error_;
};
//---------------------------------------------------------------------------
}
#endif
