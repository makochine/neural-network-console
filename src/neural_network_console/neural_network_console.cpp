// Copyright 2018,2019,2020,2021 Sony Corporation.
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

#include <fmx.h>
#ifdef _WIN32
#include <tchar.h>
#endif
#pragma hdrstop
#include <System.StartUpCopy.hpp>
//---------------------------------------------------------------------------
USEFORM("forms\sdeep_console_setup_form.cpp", Fo_Setup);
USEFORM("forms\sdeep_console_replace_form.cpp", Fo_Replace);
USEFORM("forms\sdeep_console_plugin_form.cpp", Fo_Plugin);
USEFORM("forms\sdeep_console_create_dataset_form.cpp", Fo_CreateDataset);
USEFORM("forms\list_frame.cpp", Fr_List); /* TFrame: File Type */
USEFORM("forms\sdeep_console_main_form.cpp", Fo_Main);
USEFORM("forms\sdeep_console_edit_text_form.cpp", Fo_EditText);
USEFORM("forms\sdeep_console_create_unit_form.cpp", Fo_CreateUnit);
//---------------------------------------------------------------------------
extern "C" int FMXmain()
{
	try
	{
		Application->Initialize();
		Application->CreateForm(__classid(TFo_Main), &Fo_Main);
		Application->CreateForm(__classid(TFo_Replace), &Fo_Replace);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
