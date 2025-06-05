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

#pragma hdrstop

#include <math.h>
#include "torch_utility.h"

#if _WIN32
  typedef long long _int_array_type;
#else
  # ifdef __APPLE__
    typedef long long _int_array_type;
  #else
    typedef long _int_array_type;
  #endif
#endif

namespace sdeep_console{

//---------------------------------------------------------------------------
void RemoveTorchLossFunctions(NNStructure &structure, int no_output_layer)
{
	const int output_component_index = structure.GetNNLayerComponents()->GetComponentIndexByType(FString("Output"));

	//Remove link from label data to loss function
	int loss_function_removed = false;
	for (int il = structure.GetNumLayer() - 1 ; il >= 0  ; il--) {
		const NNLayer &layer = structure.GetLayer(il);
		const NNLayerComponent *component = layer.GetComponent();
		int last_identity = false;
		if (layer.GetType() == "Identity"){
			if (structure.EnumLinkIDBySourceLayerID(layer.GetLayerID(), 0).GetSize() == 0){
				last_identity = true;
			}
		}

		//Loss Function or last Identity
		if (layer.GetPropertyIndexByName("IsLossFunction") >= 0 || last_identity) {
			int property_index = -1;
			const NNLayer *source_layer = NULL;
			for (int is = 0 ; is < layer.GetNumSideConnector(0) ; is++){
				const NNLayerSideConnector &side_connector = layer.GetSideConnector(0, is);
				if (structure.GetPinEnable(layer.GetLayerID(), 0, is + 1)) {
					if (layer.GetInputPinConnected(is + 1)){
						//Check dataset input pin to get source dataset layer and delete link to dataset layer
						const int link_id = structure.EnumLinkIDByDestLayerID(layer.GetLayerID(), is + 1).GetAt(0);
						const NNLink *link = structure.GetLinkByID(link_id);
						source_layer = structure.GetLayerByID(link->GetSourceLayerID());
						property_index = source_layer->GetPropertyIndexByName("Dataset");

						//Delete link from dataset to loss layer
						structure.ClearSelection();
						structure.SetCurrentLinkID(link_id);
						structure.DeleteSelected();
					}
				}
			}

			//Delete loss layer
			structure.ClearSelection();
			structure.SetLayerSelected(layer.GetLayerID(), true, false);
			structure.DeleteSelected();
            loss_function_removed = true;
		}
	}

	if (no_output_layer) {
		for (int il = structure.GetNumLayer() - 1 ; il >= 0 ; il--) {
			const NNLayer &layer = structure.GetLayer(il);
			if (layer.GetType() == "Output") {
				//Delete layer with no destination
				structure.ClearSelection();
				structure.SetLayerSelected(layer.GetLayerID(), true, false);
				structure.DeleteSelected();
			}
		}
	}

	structure.RefreshDisplayValue();
	structure.RefreshStatistics();
}
}
#pragma package(smart_init)
