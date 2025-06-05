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

#include "imagecache.h"
//---------------------------------------------------------------------------
int ImageCache::Clear()
{
	if (image_) {
		delete image_;
		image_ = NULL;
	}
	file_name_ = "";
	dataset_path_ = "";
	enabled_ = false;
	requested_ = false;

	return true;
}
//---------------------------------------------------------------------------
int ImageCache::LoadImage()
{
	if (enabled_ || !requested_) {
		return false;
	}
	image_ = new TBitmap(1, 1);
	if (FileExists((dataset_path_ + file_name_).PChar())) {
		//Relative path
		image_->LoadFromFile((dataset_path_ + file_name_).PChar());
		enabled_ = true;
	}else if (FileExists(file_name_.PChar())) {
		//Absolute path
		image_->LoadFromFile(file_name_.PChar());
		enabled_ = true;
	}

	return enabled_;
}
//---------------------------------------------------------------------------
int ImageCacheList::Clear()
{
	Terminate();
	while (running_){
		Sleep(1);
	}
	image_list_.Clear();

	return true;
}
//---------------------------------------------------------------------------
int ImageCacheList::ClearCache()
{
	StartUpdate();
	EndUpdate();

	bool need_proceed = true;
	while (image_list_.GetSize() && need_proceed){
		Sleep(1);
		Application->HandleMessage();

		need_proceed = false;
		for (int i=0; i<image_list_.GetSize(); i++) {
			if (!image_list_.GetAt(i).GetRequest()) {
				need_proceed = true;
				break;
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------------
void ImageCacheList::StartUpdate()
{
	updating_ = true;
	for (int i = 0 ; i < image_list_.GetSize() ; i++) {
		image_list_.GetAt(i).SetRequest(false);
	}
}
//---------------------------------------------------------------------------
void ImageCacheList::RequestImage(const char *dataset_path, const char *file_name)
{
	for (int i = 0 ; i < image_list_.GetSize() ; i++) {
		if (image_list_.GetAt(i).GetDatasetPath() == dataset_path &&
			image_list_.GetAt(i).GetFileName() == file_name) {
			image_list_.GetAt(i).SetRequest(true);
			return;
		}
	}
	ImageCache &image_cache = image_list_.Add();
	image_cache.SetDatasetPath(dataset_path);
	image_cache.SetFileName(file_name);
	image_cache.SetRequest(true);
}
//---------------------------------------------------------------------------
void ImageCacheList::EndUpdate()
{
	updating_ = false;
}
//---------------------------------------------------------------------------
const TBitmap *ImageCacheList::GetImage(const char *file_name) const
{
	for (int i = 0 ; i < image_list_.GetSize() ; i++) {
		const ImageCache &image_cache = image_list_.GetAt(i);
		if (image_cache.GetEnabled()) {
			if (image_cache.GetFileName() == file_name) {
				return image_cache.GetImage();
			}
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
void __fastcall ImageCacheList::Execute()
{
	running_ = true;
	while (!Terminated){
		for (int i = 0 ; i < image_list_.GetSize() && !updating_ && !Terminated ; i++) {
			ImageCache &cache = image_list_.GetAt(i);
			load_index_ = i;
			if (!cache.GetRequest()) {
				Synchronize(DeleteImage);
				i--;
			}else if (!cache.GetEnabled()) {
				Synchronize(LoadImage);
				//LoadImage();
				//image_list_.GetAt(load_index_).LoadImage();
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
void __fastcall ImageCacheList::DeleteImage()
{
	image_list_.Delete(load_index_);
}
//---------------------------------------------------------------------------
void __fastcall ImageCacheList::LoadImage()
{
	image_list_.GetAt(load_index_).LoadImage();
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
