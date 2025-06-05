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

#include "csvcache.h"
#include "flist.h"
#include "fstring.h"

//---------------------------------------------------------------------------
void CSVMatrix::FromCSVDataset(const CSVDataset &csv_dataset)
{
	Clear();
	num_row_ = csv_dataset.GetNumData();
	num_col_ = csv_dataset.GetNumColumn();
	min_ = -1.0f;
	max_ = 1.0f;
	for (int ir = 0 ; ir < num_row_ ; ir++) {
		for (int ic = 0 ; ic < num_col_ ; ic++) {
			const float v = (float)csv_dataset.GetFloatValue(ir, ic);
			Add(v);

			if (ir == 0 && ic == 0) {
				max_ = min_ = v;
			}else {
				if (v < min_) {
					min_ = v;
				}
				if (v > max_) {
					max_ = v;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void CSVMatrix::FromWavFile(const TWaveFile &wave_file)
{
    Clear();
	num_row_ = wave_file.GetLength();
	num_col_ = wave_file.GetCh();
	for (int ir = 0 ; ir < num_row_ ; ir++) {
		for (int ic = 0 ; ic < num_col_ ; ic++) {
			const float v = wave_file.GetFloatValue(ir, ic);
			Add(v);
		}
	}
}
//---------------------------------------------------------------------------
int CSVCache::Clear()
{
	file_name_ = "";
	dataset_path_ = "";
	enabled_ = false;
	requested_ = false;

	return true;
}
//---------------------------------------------------------------------------
int CSVCache::LoadCSV()
{
	if (enabled_ || !requested_) {
		return false;
	}

	FString path;
	if (FileExists((dataset_path_ + file_name_).PChar())) {
		//Relative path
		path = dataset_path_ + file_name_;
	}else if (FileExists(file_name_.PChar())) {
		//Absolute path
		path =file_name_;;
	}

	switch (type_) {
	case CSVCacheFileType::Wav:
		{
			TWaveFile wav;
			enabled_ = wav.LoadFromFile(path.PChar());
			if (enabled_) {
				csv_.FromWavFile(wav);
			}
		}
        break;
	default:
		//CSV file type;
		{
			CSVDataset csv_dataset;
			enabled_ = csv_dataset.LoadFromFile(path.PChar(), false, true);
			if (enabled_) {
				csv_.FromCSVDataset(csv_dataset);
			}
		}
	}

	return enabled_;
}
//---------------------------------------------------------------------------
int CSVCacheList::Clear()
{
	Terminate();
	while (running_){
		Sleep(1);
	}
	csv_list_.Clear();

	return true;
}
//---------------------------------------------------------------------------
int CSVCacheList::ClearCache()
{
	StartUpdate();
	EndUpdate();
	while (csv_list_.GetSize()){
		Sleep(1);
	}

	return true;
}
//---------------------------------------------------------------------------
void CSVCacheList::StartUpdate()
{
	updating_ = true;
	for (int i = 0 ; i < csv_list_.GetSize() ; i++) {
		csv_list_.GetAt(i).SetRequest(false);
	}
}
//---------------------------------------------------------------------------
void CSVCacheList::RequestCSV(const char *dataset_path, const char *file_name, CSVCacheFileType type)
{
	for (int i = 0 ; i < csv_list_.GetSize() ; i++) {
		if (csv_list_.GetAt(i).GetDatasetPath() == dataset_path &&
			csv_list_.GetAt(i).GetFileName() == file_name) {
			csv_list_.GetAt(i).SetRequest(true);
			return;
		}
	}
	CSVCache &csv_cache = csv_list_.Add();
	csv_cache.SetDatasetPath(dataset_path);
	csv_cache.SetFileName(file_name);
	csv_cache.SetCSVCacheFileType(type);
	csv_cache.SetRequest(true);
}
//---------------------------------------------------------------------------
void CSVCacheList::EndUpdate()
{
	updating_ = false;
}
//---------------------------------------------------------------------------
const CSVMatrix *CSVCacheList::GetCSV(const char *file_name) const
{
	for (int i = 0 ; i < csv_list_.GetSize() ; i++) {
		const CSVCache &csv_cache = csv_list_.GetAt(i);
		if (csv_cache.GetEnabled()) {
			if (csv_cache.GetFileName() == file_name) {
				return csv_cache.GetCSV();
			}
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
void __fastcall CSVCacheList::Execute()
{
	running_ = true;
	while (!Terminated){
		for (int i = 0 ; i < csv_list_.GetSize() && !updating_ && !Terminated ; i++) {
			CSVCache &cache = csv_list_.GetAt(i);
			if (!cache.GetRequest()) {
				csv_list_.Delete(i);
				i--;
			}else if (!cache.GetEnabled()) {
				load_index_ = i;
				Synchronize(LoadCSV);
				if (cache.GetEnabled()) {
                    update_count_++;
				}
				Sleep(1);
			}
		}
		Sleep(1);
	}
	running_ = false;
}
//---------------------------------------------------------------------------
void __fastcall CSVCacheList::LoadCSV()
{
	csv_list_.GetAt(load_index_).LoadCSV();
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
