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

#pragma hdrstop

#include <iostream>
#include <fstream>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "csv_dataset.h"
namespace elfe{
//---------------------------------------------------------------------------
CSVDataset &CSVDataset::operator = (const CSVDataset& source)
{
	if (this != &source){
		Clear();
		file_name_ = source.file_name_;
		num_column_ = source.num_column_;
		top_left_cell_ = source.top_left_cell_;
		header_[0] = source.header_[0];
		header_[1] = source.header_[1];
		data_ = source.data_;
		last_error_ = source.last_error_;

        read_only_ = source.read_only_;
		if (source.buf_) {
			buf_size_ = source.buf_size_;
			buf_ = new char[source.buf_size_];
			memcpy(buf_, source.buf_, source.buf_size_);
			long long offset = (long long)buf_ - (long long)source.buf_;
			ptr_ = source.ptr_;
			for (int i = 0 ; i < ptr_.GetSize(); i++) {
				FListT<char*> &ptr2 = ptr_.GetAt(i);
				for (int i2 = 0 ; i2 < ptr2.GetSize(); i2++) {
					ptr2.SetAt(i2, ptr2.GetAt(i2) + offset);
				}
			}
		}

		update_count_ = source.update_count_;
	}

	return *this;
}
//---------------------------------------------------------------------------
void CSVDataset::Clear()
{
	file_name_ = "";
	num_column_ = 0;
	if (buf_) {
		delete[] buf_;
		buf_ = NULL;
	}
	buf_size_ = 0;
	ptr_.Clear();

	header_[0].Clear();
	header_[1].Clear();
	top_left_cell_ = "";
	data_.Clear();

	read_only_ = false;

	update_count_++;
}
//---------------------------------------------------------------------------
int CSVDataset::LoadFromFile(const char *file_name, int has_header, int read_only)
{
	Clear();

	read_only_ = read_only;

	file_name_ = file_name;
	last_error_ = CSVDatasetError::None;
	if (read_only) {
		if (access(file_name, 0) != 0 ) {
			last_error_ = CSVDatasetError::CantOpenFile;
			return false;
		}
#ifdef _WIN32
		WIN32_FIND_DATAA find_data;
		HANDLE handle;
		handle = FindFirstFileA(file_name, &find_data);
		long long size = find_data.nFileSizeLow | ((long long)find_data.nFileSizeHigh << 32);
		FindClose(handle);
#endif
		std::ifstream f;
		f.open(file_name, std::ios_base::binary | std::ios_base::in);
		if (!f.good() || f.eof() || !f.is_open()) {
			last_error_ = CSVDatasetError::CantOpenFile;
			if (f.is_open()){
				f.close();
			}
			return false;
		}
#ifndef _WIN32
		f.seekg(0, std::ios_base::end);
		std::ifstream::pos_type end = f.tellg();
		f.seekg(0, std::ios_base::beg);
		std::ifstream::pos_type begin = f.tellg();
		long long size = static_cast<long long>(end - begin);
#endif
		if ( size >= 1 ) {
			buf_size_ = size + 1;
			try {
				buf_ = new char[buf_size_];
			}
			catch (...){
				last_error_ = CSVDatasetError::NotEnoughMemory;
				f.close();
				return false;
			}
			f.read(buf_, size);
			buf_[size] = '\0';
			char *buf = buf_;
			char *bufe = buf + size;
			int dqcount = 0;//double quotation count
			int flag = false;
			FListT<char*> *ptr2 = &ptr_.Add();//add row
			while (buf < bufe){
				switch (*buf) {
				case '\n':
					if (!dqcount) {
						ptr2 = &ptr_.Add();//new row
					}
				case '\r':
					if (!dqcount) {
						*buf = '\0';
						flag = false;
					}
					break;
				case '\"':
					if (dqcount) {
						dqcount = 0;
					}else {
						dqcount = 1;
					}
					*buf = '\0';
					break;
				case ',':
					if (!dqcount) {
						flag = false;
						*buf = '\0';
						break;
					}
				default:
					if (!flag) {
						ptr2->Add(buf);//add column
						if (ptr2->GetSize() > num_column_) {
							num_column_ = ptr2->GetSize();
						}
						flag = true;
					}
				}
				buf++;
			}

			//Delete empty line
			while (ptr_.GetSize()){
				FListT<char*> &ptr3 = ptr_.GetAt(ptr_.GetSize() - 1);
				if (ptr3.GetSize() == 0) {
					ptr_.Delete(ptr_.GetSize() - 1);
				}else {
					break;
				}
			}

			static const unsigned char bom[4] = {0xef, 0xbb ,0xbf, 0x00};
			if (ptr_.GetSize() && ptr_.GetAt(0).GetSize()){
				if (FString(ptr_.GetAt(0).GetAt(0)).SubString(1, 3) == (const char*)bom) {
                    ptr_.GetAt(0).SetAt(0, ptr_.GetAt(0).GetAt(0) + 3);
                }
			}

			//Fill empty col
			for (int i = 0 ; i < ptr_.GetSize(); i++) {
				FListT<char*> &ptr3 = ptr_.GetAt(i);
				for (int i2 = ptr3.GetSize() ; i2 < num_column_; i2++) {
					last_error_ = CSVDatasetError::DifferentColumnNum;
					ptr3.Add(buf_ + size);
				}

				//Extract header
				if (i == 0) {
					for (int i2 = 0 ; i2 < num_column_; i2++) {
						header_[0].Add(FString(ptr3.GetAt(i2)));
					}
				}
			}

			// remove header from cells
			if (has_header && ptr_.GetSize()) {
				ptr_.Delete(0);
			}

			// check if csv has enough lines
			if (!ptr_.GetSize()) {
				last_error_ = CSVDatasetError::NotEnoughLines;
			}
		}else {
			last_error_ = CSVDatasetError::NotEnoughLines;
			f.close();
			return false;
		}
		f.close();
	}else {
		FStringList csv;
		if (!csv.LoadFromFile(file_name)){
			last_error_ = CSVDatasetError::CantOpenFile;
			return false;
		}
		if (csv.GetSize() < 1 + has_header){
			last_error_ = CSVDatasetError::NotEnoughLines;
			return false;
		}

		FStringList first_line;
		first_line.SplitStr(csv.GetAt(0), FString(","), FString("\""));
		num_column_ = first_line.GetSize();
		if (has_header){
			header_[0] = first_line;
		}
		for (int il = has_header ; il < csv.GetSize() ; il++) {
			FStringList &line = data_.Add(FStringList());
			line.SplitStr(csv.GetAt(il), FString(","), FString("\""));
			/*
			if (num_column_ != line.GetSize()) {
				last_error_ = CSVDatasetError::DifferentColumnNum;
				return false;
			}
			*/
			if (line.GetSize() > num_column_){
				if (has_header) for (int ic = num_column_ ; ic < line.GetSize() ; ic++){
					header_[0].Add();
				}
				for (int il2 = 0 ; il2 < data_.GetSize() - 1 ; il2 ++){
					for (int ic = num_column_ ; ic < line.GetSize() ; ic++){
						data_.GetAt(il2).Add(FString(""));
					}
				}
				num_column_ = line.GetSize();
			}
		}
	}

	return last_error_ == CSVDatasetError::None;
}
//---------------------------------------------------------------------------
int CSVDataset::SaveToFile(const char *file_name)
{
	file_name_ = file_name;
	return SaveToFile_(file_name);
}
//---------------------------------------------------------------------------
int CSVDataset::SaveToFile_(const char *file_name) const
{
	CSVDatasetError error = CSVDatasetError::None;
	FILE *fp = fopen(file_name, "wt");
	if ( !fp ) {
		// error = CSVDatasetError::CantOpenFile;
		return false;
	}

	if (header_[0].GetSize()){
		FString header;
		int first = header_[1].GetSize() == 0;
		for (int ic = 0 ; ic < header_[0].GetSize() ; ic++) {
			if (!first) {
				header += top_left_cell_ + ",";
			}
			header += header_[0].GetAt(ic);
			first = false;
		}
		if (fprintf(fp, "%s\n", header.PChar()) == EOF){
			error = CSVDatasetError::CantWriteLine;
		}
	}

	for (int id = 0 ; id < GetNumData() ; id++){
		FString line;
		int first = true;
		if (header_[1].GetSize()) {
			if (id < header_[1].GetSize()) {
				line = header_[1].GetAt(id);
			}
			first = false;
		}
		for (int ic = 0 ; ic < GetNumColumn() ; ic++) {
			if (!first) {
				line += ",";
			}
			const FString &value = GetValue(id, ic);
			if (value.Pos(",")){
				line += FString("\"") + value + FString("\"");
			}else {
				line += value;
			}
			first = false;
		}
		if (fprintf(fp, "%s\n", line.PChar()) == EOF){
			error = CSVDatasetError::CantWriteLine;
		}
	}
    fclose(fp);

	return error == CSVDatasetError::None;
}
//---------------------------------------------------------------------------
const char* CSVDataset::GetValuePtr(int data_index, int column_index) const
{
	if (read_only_) {
		if (data_index < ptr_.GetSize()) {
			if (column_index < ptr_.GetAt(data_index).GetSize()) {
				return ptr_.GetAt(data_index).GetAt(column_index);
			}
		}
	}else {
		if (data_index < data_.GetSize()) {
			if (column_index < data_.GetAt(data_index).GetSize()) {
				return data_.GetAt(data_index).GetAt(column_index).PChar();
			}
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
int CSVDataset::GetIntValue(int data_index, int column_index) const
{
	int v = 0;
	const char *p = GetValuePtr(data_index, column_index);
	if (p) if (p[0] != '\0'){
		sscanf(p, "%d", &v);
	}
	return v;
}
//---------------------------------------------------------------------------
int CSVDataset::GetIntValue(int data_index, FString column_name) const
{
	const int column_index = GetColumnIndex(column_name);
	if (column_index >= 0) {
		return GetIntValue(data_index, column_index);
	}else {
		return 0.0;
	}
}
//---------------------------------------------------------------------------
double CSVDataset::GetFloatValue(int data_index, int column_index) const
{
	double v = 0.0;
	const char *p = GetValuePtr(data_index, column_index);
	if (p) if (p[0] != '\0'){
		sscanf(p, "%lf", &v);
	}
	return v;
}
//---------------------------------------------------------------------------
double CSVDataset::GetFloatValue(int data_index, FString column_name) const
{
	const int column_index = GetColumnIndex(column_name);
	if (column_index >= 0) {
		return GetFloatValue(data_index, column_index);
	}else {
		return 0.0;
	}
}
//---------------------------------------------------------------------------
FString CSVDataset::GetValue(int data_index, int column_index) const
{
	if (read_only_) {
		if (data_index < ptr_.GetSize()) {
			if (column_index < ptr_.GetAt(data_index).GetSize()) {
				return FString(ptr_.GetAt(data_index).GetAt(column_index));
			}
		}
	}else {
		if (data_index < data_.GetSize()) {
			if (column_index < data_.GetAt(data_index).GetSize()) {
				return data_.GetAt(data_index).GetAt(column_index);
			}
		}
	}
	return FString("");
}
//---------------------------------------------------------------------------
FString CSVDataset::GetValue(int data_index, FString column_name) const
{
	const int column_index = GetColumnIndex(column_name);
	if (column_index >= 0) {
		return GetValue(data_index, column_index);
	}else {
		return FString("");
	}
}
//---------------------------------------------------------------------------
FStringList CSVDataset::GetData(int data_index) const
{
	if (read_only_) {
		if (data_index < ptr_.GetSize()) {
            FStringList result;
			for (int ic = 0 ; ic < GetNumColumn() ; ic++){
				result.Add(FString(ptr_.GetAt(data_index).GetAt(ic)));
			}
            return result;
		}
	}else {
		if (data_index < data_.GetSize()) {
			return data_.GetAt(data_index);
		}
	}
	return FStringList();
}
//---------------------------------------------------------------------------
int CSVDataset::GetColumnIndex(FString column_name) const
{
	for (int ic = 0 ; ic < header_[0].GetSize() ; ic++) {
		if (header_[0].GetAt(ic) == column_name){
			return ic;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
int CSVDataset::GetColumnMinMax(int column_index, double &min, double &max) const
{
	//Do not initialize min and max in this function
	int result = false;

	for (int id = 0 ; id < GetNumData() ; id++) {
		const char *p = GetValuePtr(id, column_index);
		if (p) if (p[0] != '\0'){
			double v = 0.0;
			sscanf(p, "%lf", &v);
			if (v < NAN_VALUE_THRESHOLD) {
				result = true;
				if (min == 0.0 && max == 0.0) {
					min = max = v;
				}else {
					if (v < min) {
						min = v;
					}
					if (v > max) {
						max = v;
					}
				}
			}
		}
	}

	return result;
}
//---------------------------------------------------------------------------
int CSVDataset::GetColumnMinMax(FString column_name, double &min, double &max) const
{
	const int column_index = GetColumnIndex(column_name);
	if (column_index >= 0) {
		return GetColumnMinMax(column_index, min, max);
	}else {
		return false;
	}
}
//---------------------------------------------------------------------------
int CSVDataset::GetColumnMeanVariance(int column_index, double &mean, double &variance) const
{
	int result = false;

	FListT<double> values;
	for (int id = 0 ; id < GetNumData() ; id++) {
		FString s =	GetValue(id, column_index);
		if (s != "") {
			values.Add(s.AsFloat());
			result = true;
		}
	}

	if (result) {
		mean = values.Mean();
		variance = values.Variance(mean);
	}

	return result;
}
//---------------------------------------------------------------------------
double CSVDataset::GetColumnCorrel(int column_index1, int column_index2) const
{
	FListT<double> col[2];
	for (int i = 0 ; i < GetNumData() ; i++) {
		col[0].Add(GetValue(i, column_index1).AsFloat());
		col[1].Add(GetValue(i, column_index2).AsFloat());
	}

	double mean[2], var[2];
	for (int ic = 0 ; ic < 2 ; ic++) {
		mean[ic] = col[ic].Mean();
		var[ic] = col[ic].Variance(mean[ic]);
	}

	double cov = 0.0;
	for (int i = 0 ; i < GetNumData() ; i++) {
		cov += (col[0].GetAt(i) - mean[0]) * (col[1].GetAt(i) - mean[1]);
	}
	cov /= GetNumData();

	return var[0] > 0.0 && var[1] > 0.0 ?
		cov / (sqrt(var[0]) * sqrt(var[1])) :
		0.0;
}
//---------------------------------------------------------------------------
int CSVDataset::GetModal(const FString &modal_name, int &col_start, int &col_end) const
{
	col_start = -1;
	col_end = -1;
	for (int ic = 0 ; ic < header_[0].GetSize() ; ic++) {
		FString s = header_[0].GetAt(ic);
		s = s.SplitBy(':');
		s = s.SplitBy("__");
		if (s == modal_name) {
			if (col_start < 0) {
				col_start = ic;
			}
			col_end = ic;
		}
	}
	return col_start >= 0;
}
//---------------------------------------------------------------------------
void CSVDataset::SetColumnNames(const FString &column_names)
{
	header_[0].SplitStr(column_names, FString(","), FString("\""));
	update_count_++;
}
//---------------------------------------------------------------------------
void CSVDataset::SetColumnNames(const FStringList &column_names)
{
	header_[0] = column_names;
	update_count_++;
}
//---------------------------------------------------------------------------
void CSVDataset::SetRowNames(const FString &row_names)
{
	header_[1].SplitStr(row_names, FString(","), FString("\""));
	update_count_++;
}
//---------------------------------------------------------------------------
void CSVDataset::SetRowNames(const FStringList &row_names)
{
	header_[1] = row_names;
	update_count_++;
}
//---------------------------------------------------------------------------
void CSVDataset::SetValue(int data_index, int column_index, FString value)
{
	if (read_only_) {
		return;
	}
	while (data_index >= data_.GetSize()){
		data_.Add();
	}
	while (column_index >= data_.GetAt(data_index).GetSize()){
		data_.GetAt(data_index).Add();
	}
	data_.GetAt(data_index).GetAt(column_index) = value;
	if (column_index >= num_column_) {
		num_column_ = column_index + 1;
	}
	update_count_++;
}
//---------------------------------------------------------------------------
int CSVDataset::SetValue(int data_index, FString column_name, FString value)
{
	const int column_index = GetColumnIndex(column_name);
	if (column_index >= 0) {
		SetValue(data_index, column_index, value);
		return true;
	}else {
		return false;
	}
}
//---------------------------------------------------------------------------
void CSVDataset::SetData(int data_index, const FStringList &data)
{
	if (read_only_) {
		return;
	}
	while (data_index >= data_.GetSize()){
		data_.Add();
	}
	data_.GetAt(data_index) = data;
	if (data.GetSize() > num_column_) {
		num_column_ = data.GetSize();
	}
}
//---------------------------------------------------------------------------
void CSVDataset::DeleteData(int data_index)
{
	data_.Delete(data_index);
}
//---------------------------------------------------------------------------
void CSVDataset::DeleteCol(int column_index)
{
	if (header_[0].GetSize()) {
		assert(column_index < header_[0].GetSize());
		header_[0].Delete(column_index);
	}
	for (int i = 0 ; i < GetNumData() ; i++) {
		assert(column_index < data_.GetAt(i).GetSize());
		if (read_only_) {
			ptr_.GetAt(i).Delete(column_index);
		}else {
			data_.GetAt(i).Delete(column_index);
		}
	}
	num_column_--;
}
//---------------------------------------------------------------------------
};
#pragma package(smart_init)
