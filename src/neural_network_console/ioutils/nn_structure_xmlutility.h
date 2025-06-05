// Copyright 2016,2017,2018,2019,2020,2021 Sony Corporation.
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
#ifndef nn_structure_xmlutilityH
#define nn_structure_xmlutilityH
//---------------------------------------------------------------------------
#include <fmx.h>
#include "nn_structure.h"
//---------------------------------------------------------------------------
namespace sdeep_console{
//---------------------------------------------------------------------------
int NNStructureToXMLFile(TComponent *AOwner, const NNStructure &structure, UnicodeString file_name);
int XMLFileToNNStructure(TComponent *AOwner, UnicodeString file_name, NNStructure &structure);
int NNStructureToXMLString(TComponent *AOwner, const NNStructure &structure, AnsiString &xml_string);
int XMLStringToNNStructure(TComponent *AOwner, AnsiString xml_string, NNStructure &structure);
//---------------------------------------------------------------------------
};
#endif
