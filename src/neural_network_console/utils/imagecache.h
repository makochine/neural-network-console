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

#ifndef imagecacheH
#define imagecacheH

#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include "flist.h"
#include "fstring.h"

using namespace elfe;
//---------------------------------------------------------------------------
class ImageCache
{
public:
	ImageCache() : image_(NULL), enabled_(false), requested_(false){};
	__fastcall virtual ~ImageCache() { Clear();};
	int Clear();
public:
	int LoadImage();
public:
	void SetFileName(const char *file_name) {file_name_ = file_name;};
	const FString &GetFileName() const {return file_name_;};
	void SetDatasetPath(const char *dataset_path) {dataset_path_ = dataset_path;};
	const FString &GetDatasetPath() const {return dataset_path_;};
	void SetRequest(int requested) {requested_ = requested;};
	int GetRequest() const {return requested_;};
	int GetEnabled() const {return enabled_;}
	const TBitmap* GetImage() const {return enabled_ ? image_ : NULL;};
	TBitmap* GetImage_() {return enabled_ ? image_ : NULL;};
	const TPixelFormat GetImagePixelFormat() const {return image_->PixelFormat;};
private:
	FString file_name_;
	FString dataset_path_;
	TBitmap* image_;
	int enabled_;
	int requested_;
};
//---------------------------------------------------------------------------
class ImageCacheList : public TThread
{
public:
	ImageCacheList() : TThread(false), updating_(false), update_count_(0), running_(false), load_index_(0) {};
	virtual __fastcall ~ImageCacheList() {Clear(); };
	int Clear();
	int ClearCache();
public:
	void StartUpdate();
	void RequestImage(const char *dataset_path, const char *file_name);
	void EndUpdate();
	int GetUpdateCount() const {return update_count_;};

	const TBitmap *GetImage(const char *file_name) const;
public:
	void __fastcall Execute();
private:
	void __fastcall LoadImage();
	void __fastcall DeleteImage();
private:
	FListP<ImageCache> image_list_;
	int updating_;
	int running_;
	int load_index_;
	int update_count_;
};
//---------------------------------------------------------------------------
#endif
