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

#include "nn_document_converter_setting.h"

#include "ffastini.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
int NNDocumentConverterSetting::LoadFromTextFile(const char *file_name, FString engine_from, FString engine_to)
{
	int success = false;

	Clear();

	engine_from_ = engine_from;
	engine_to_ = engine_to;

	FFastIni ini = FFastIni(file_name);

	FStringList layer_setting;
	ini.ReadSectionValues("Layer", &layer_setting);
	for (int i = 0; i < layer_setting.GetSize() ; i++) {
		FString s = layer_setting.GetAt(i);
		FString condition = s.SplitBy("=");
		layer_from_.Add(condition.SplitBy(","));
		layer_condition_.Add(condition);
		layer_to_.Add(s.SplitBy(","));
		layer_additional_process_.Add(s);
		success = true;
	}

	FStringList property_setting;
	ini.ReadSectionValues("Property", &property_setting);
	for (int i = 0; i < property_setting.GetSize() ; i++) {
		FString s = property_setting.GetAt(i);
		property_from_.Add(s.SplitBy("="));
		property_to_.Add(s.SplitBy(","));
		property_additional_process_.Add(s);
		success = true;
	}

	FStringList value_setting;
	ini.ReadSectionValues("Value", &value_setting);
	for (int i = 0; i < value_setting.GetSize() ; i++) {
		FString s = value_setting.GetAt(i);
		value_from_.Add(s.SplitBy("="));
		value_to_.Add(s.SplitBy(","));
		value_additional_process_.Add(s);
		success = true;
	}

	return success;
}
//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
#pragma package(smart_init)
