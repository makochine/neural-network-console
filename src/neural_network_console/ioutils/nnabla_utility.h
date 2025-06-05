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

#ifndef nnabla_utilityH
#define nnabla_utilityH
//---------------------------------------------------------------------------
#include "nn_structure.h"
#include "sdeep_console_global_settings.h"
#include "nn_document.h"

namespace sdeep_console{
FStringList GetNNablaPythonCode(const NNDocument &document);
void NetworkToNNablaProtoTxt(
	const NNStructure &structure, const NNConfig &config,//in
	FStringList &prototxt, FString &indent); //out
int CreateNNablaProtoTxtForTraining(
	const NNDocument &nn_document, const SDeepConsoleGlobalSettings &settings, const char *file_name,//in
	FStringList &prototxt, FStringList &param_csv, FStringList &error_str); //out
void FlattenCurrentStructure(const NNDocument &nn_document, NNStructures &structures, int arrange);
void RemoveLossFunctions(NNStructure &structure, int no_output_layer = false);
int NntxtToDocument(const char *file_name, NNDocument &nn_document, FStringList &error_str);
}
//---------------------------------------------------------------------------
#endif
