// Copyright 2024,2025 Sony Group Corporation.
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

#ifndef torch_utilityH
#define torch_utilityH
//---------------------------------------------------------------------------
#include "nn_structure.h"
#include "sdeep_console_global_settings.h"
#include "nn_document.h"

namespace sdeep_console{
void RemoveTorchLossFunctions(NNStructure &structure, int no_output_layer = false);
}
//---------------------------------------------------------------------------
#endif
