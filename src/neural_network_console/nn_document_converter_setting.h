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

#ifndef nn_document_converter_settingH
#define nn_document_converter_settingH
#include "flist.h"
#include "fstring.h"
#include "ffastini.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
using namespace elfe;
//---------------------------------------------------------------------------
class NNDocumentConverterSetting
{
public:
	NNDocumentConverterSetting() {};
	virtual ~NNDocumentConverterSetting() {Clear(); };
	void Clear() {
		engine_from_ = "";
		engine_to_ = "";
		layer_from_.Clear();
		layer_condition_.Clear();
		layer_additional_process_.Clear();
		layer_to_.Clear();
		property_from_.Clear();
		property_to_.Clear();
		value_from_.Clear();
		value_to_.Clear();
	}
public:
	int LoadFromTextFile(const char *file_name, FString engine_from, FString engine_to);
public:
	const FString &GetEngineFrom() const {return engine_from_;};
	const FString &GetEngineTo() const {return engine_to_;};

	const FStringList &GetLayerFrom() const {return layer_from_;};
	const FStringList &GetLayerCondition() const {return layer_condition_;};
	const FStringList &GetLayerAdditionalProcess() const {return layer_additional_process_;};
	const FStringList &GetLayerTo() const {return layer_to_;};

	const FStringList &GetPropertyFrom() const {return property_from_;};
	const FStringList &GetPropertyAdditionalProcess() const {return property_additional_process_;};
	const FStringList &GetPropertyTo() const {return property_to_;};

	const FStringList &GetValueFrom() const {return value_from_;};
	const FStringList &GetValueAdditionalProcess() const {return value_additional_process_;};
	const FStringList &GetValueTo() const {return value_to_;};
private:
	//Engine name
	FString engine_from_;
	FString engine_to_;

	//Layer name convert setting
	FStringList layer_from_;
	FStringList layer_condition_;
	FStringList layer_additional_process_;
	FStringList layer_to_;

	//Property name convert setting
	FStringList property_from_;
	FStringList property_additional_process_;
	FStringList property_to_;

	//Property value convert setting
	FStringList value_from_;
	FStringList value_additional_process_;
	FStringList value_to_;
};
//---------------------------------------------------------------------------
};
#endif
