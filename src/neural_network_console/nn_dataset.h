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

#ifndef nn_datasetH
#define nn_datasetH
//---------------------------------------------------------------------------
#include "flist.h"
#include "fstring.h"
#include "ffastini.h"
#include "csv_dataset.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
class NNDataset : public CSVDataset
{
public:
	NNDataset() : CSVDataset(), shuffle_(true), enable_cache_(false), image_normalization_(true), num_data_(0) {}
public:
    void SetName(FString name) {name_ = name;}
	FString GetName() const {return name_;}
	void SetShuffle(int shuffle) {shuffle_ = shuffle;}
	int GetShuffle() const {return shuffle_;}
	void SetEnableCache(int enable_cache) {enable_cache_ = enable_cache;}
	int GetEnableCache() const {return enable_cache_;}
	void SetImageNormalization(int image_normalization) {image_normalization_ = image_normalization;}
	int GetImageNormalization() const {return image_normalization_;}
	void SetNumData(int num_data) {num_data_ = num_data;}
	int GetNumData() const {return CSVDataset::GetNumData() ? CSVDataset::GetNumData() : num_data_;}
	int GetLoaded() const {return CSVDataset::GetNumData() > 0;}
private:
	FString name_;
	int shuffle_;
	int enable_cache_;
	int image_normalization_;
	int num_data_;
};
//---------------------------------------------------------------------------
class NNDatasets
{
public:
	NNDatasets() {Clear();};
	virtual ~NNDatasets() {Clear();};
	void Clear();
private:
	NNDatasets(const NNDatasets &source);
	NNDatasets &operator=(const NNDatasets &source);
protected:
	int LoadDatasetFromIni(FFastIni *ini);
	void SaveDatasetToIni(FFastIni *ini);
public:
	int GetNumDataset() const {return dataset_.GetSize();}
	int LoadDatasetFromFile(int dataset_index, FString file_name);

	void ClearDataset(int dataset_index) {dataset_.GetAt(dataset_index).Clear();}
	int AddDataset();
	int DeleteDataset(int dataset_index);

	const NNDataset &GetDataset(int dataset_index) const {return dataset_.GetAt(dataset_index);}
	NNDataset &GetDataset_(int dataset_index) {return dataset_.GetAt(dataset_index);}

	int GetDatasetIndexByName(const FString &name) const;

	void SetMainDatasetIndex(int index) {main_dataset_index_ = index;}
	int GetMainDatasetIndex() const {return main_dataset_index_;}

	void SwapDataset(int index, int index2);

public:
	//for compatibility
	void SetBasicDataset(FString training_uri, FString validation_uri, int shuffle, int enable_cache, int image_normalization);
private:
	FListP<NNDataset> dataset_;//0=Training, 1=Validation
	int main_dataset_index_;
};
//---------------------------------------------------------------------------
};
#endif
