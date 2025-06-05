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

#include "nn_dataset.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
void NNDatasets::Clear()
{
	dataset_.Clear();
	for (int i = 0 ; i < 2 ; i++) {
		NNDataset &dataset = dataset_.Add();
		dataset.SetName(FString(i == 0 ? "Training" : "Validation"));
		dataset.SetShuffle(i == 0);
	}
	main_dataset_index_ = 0;
}
//---------------------------------------------------------------------------
int NNDatasets::LoadDatasetFromIni(FFastIni *ini)
{
	dataset_.Clear();
	const int num_dataset = ini->ReadInteger("Dataset", "Num", -1);
	if (num_dataset < 0) {
		return false;
	}
	SetMainDatasetIndex(ini->ReadInteger("Dataset", "MainIndex", 0));
	for (int i = 0 ; i < num_dataset; i++) {
		FString key = FString("Dataset_") + FString(i);
		NNDataset &dataset = dataset_.Add();
		dataset.SetName(ini->ReadString(key.PChar(), "Name", FString("")));
		dataset.SetFileName(ini->ReadString(key.PChar(), "URI", FString("")));
		dataset.SetShuffle(ini->ReadInteger(key.PChar(), "Shuffle", dataset.GetShuffle()));
		dataset.SetEnableCache(ini->ReadBool(key.PChar(), "EnableCache", dataset.GetEnableCache()));
		dataset.SetImageNormalization(ini->ReadInteger(key.PChar(), "ImageNormalization", dataset.GetImageNormalization()));
		dataset.SetNumData(ini->ReadInteger(key.PChar(), "NumData", dataset.GetNumData()));
	}
	return true;
}
//---------------------------------------------------------------------------
void NNDatasets::SaveDatasetToIni(FFastIni *ini)
{
	ini->Write<int>("Dataset", "Num", dataset_.GetSize());
	ini->Write<int>("Dataset", "MainIndex", GetMainDatasetIndex());
	for (int i = 0 ; i < dataset_.GetSize(); i++) {
		FString key = FString("Dataset_") + FString(i);
		const NNDataset &dataset = dataset_.GetAt(i);
		ini->Write<FString>(key.PChar(), "Name", dataset.GetName());
		ini->Write<FString>(key.PChar(), "URI", dataset.GetFileName());
		ini->Write<bool>(key.PChar(), "Shuffle", dataset.GetShuffle());
		ini->Write<bool>(key.PChar(), "EnableCache", dataset.GetEnableCache());
		ini->Write<bool>(key.PChar(), "ImageNormalization", dataset.GetImageNormalization());
		ini->Write<int>(key.PChar(), "NumData", dataset.GetNumData());
	}
}
//---------------------------------------------------------------------------
int NNDatasets::LoadDatasetFromFile(int dataset_index, FString file_name)
{
	// SetDatasetURI(dataset_index, file_name);
	return dataset_.GetAt(dataset_index).LoadFromFile(file_name.PChar(), true, true);
}
//---------------------------------------------------------------------------
int NNDatasets::GetDatasetIndexByName(const FString &name) const
{
	for (int id = 0; id < dataset_.GetSize() ; id++) {
		if (dataset_.GetAt(id).GetName() == name) {
			return id;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
void NNDatasets::SwapDataset(int index, int index2)
{
	dataset_.Swap(index, index2);
	if (index == main_dataset_index_){
		main_dataset_index_ = index2;
	}else if (index2 == main_dataset_index_){
		main_dataset_index_ = index;
	}
}
//---------------------------------------------------------------------------
void NNDatasets::SetBasicDataset(FString training_uri, FString validation_uri, int shuffle, int enable_cache, int image_normalization)
{
	dataset_.Clear();
	for (int i = 0 ; i < 2; i++) {
		NNDataset &dataset = dataset_.Add();
		dataset.SetName(FString(i == 0 ? "Training" : "Validation"));
		dataset.SetFileName(i == 0 ? training_uri : validation_uri);
		dataset.SetShuffle(i == 0 ? shuffle : false);
		dataset.SetEnableCache(enable_cache);
		dataset.SetImageNormalization(image_normalization);
	}
}
//---------------------------------------------------------------------------
int NNDatasets::AddDataset()
{
	int index = 1;
	while (true){
		FString name = FString("Dataset_") + FString(index++);
		int found = false;
		for (int i = 0 ; i < dataset_.GetSize() && !found ; i++) {
			if (dataset_.GetAt(i).GetName() == name){
				found = true;
			}
		}
		if (!found) {
			dataset_.Add().SetName(name);
			break;
		}
	}
	return true;
}
//---------------------------------------------------------------------------
int NNDatasets::DeleteDataset(int dataset_index)
{
	if (dataset_index < 0 || dataset_index >= dataset_.GetSize() || dataset_.GetSize() <= 1){
		return false;
	}
	dataset_.Delete(dataset_index);

	if (main_dataset_index_ > 0  && main_dataset_index_ >= dataset_index) {
		main_dataset_index_--;
	}
	return true;
}
//---------------------------------------------------------------------------
};
#pragma package(smart_init)
