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

#pragma hdrstop
#pragma argsused

#ifdef _WIN32
#include <tchar.h>
#else
  typedef char _TCHAR;
  #define _tmain main
#endif

#include <stdio.h>

#include "fstring.h"

#include "arrange_layers.h"
#include "complete.h"
#include "create_prototxt.h"
#include "create_result_ini.h"
#include "error_check.h"
#include "export_caffe.h"
#include "export_python_code.h"
#include "get_parameter_name.h"
#include "import_caffe.h"
#include "import_nntxt.h"
#include "mutate.h"
#include "structure_search.h"

#include "version.h"

using namespace elfe;

static int print_version(FStringList &args)
{
  printf("version : %s\n", SDEEPCONSOLE_CLI_VERSION);
  return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	FStringList args;
	for (int i = 0; i < argc ; i ++) {
		args.Add(FString(argv[i]));
	}
	for (int i = args.GetSize() - 2 ; i >= 0 ; i--) {
		if (args.GetAt(i).SubString(1, 1) == "-") {
			args.GetAt(i) = args.GetAt(i) + FString(" ") + args.GetAt(i + 1);
			args.Delete(i + 1);
		}
	}

	if (args.GetSize() < 2) {
		printf("usage : sdeep_console_cli_util "
			   "[arrange_layers, complete, create_prototxt, create_result_ini, "
			   "error_check, export_caffe, export_python_code, get_parameter_name, "
			   "import_caffe, import_nntxt, mutate, structure_search, version]\n");
    print_version(args);
		return 0;
	}

	if (args.GetAt(1) == FString("arrange_layers")) {
		return arrange_layers(args);
	}else if (args.GetAt(1) == FString("complete")) {
		return complete(args);
	}else if (args.GetAt(1) == FString("create_prototxt")) {
		return create_prototxt(args);
	}else if (args.GetAt(1) == FString("create_result_ini")) {
		return create_result_ini(args);
	}else if (args.GetAt(1) == FString("error_check")) {
		return error_check(args);
	}else if (args.GetAt(1) == FString("export_caffe")) {
		return export_caffe(args);
	}else if (args.GetAt(1) == FString("export_python_code")) {
		return export_python_code(args);
	}else if (args.GetAt(1) == FString("get_parameter_name")) {
		return get_parameter_name(args);
	}else if (args.GetAt(1) == FString("import_caffe")) {
		return import_caffe(args);
	}else if (args.GetAt(1) == FString("import_nntxt")) {
		return import_nntxt(args);
	}else if (args.GetAt(1) == FString("mutate")) {
		return mutate(args);
	}else if (args.GetAt(1) == FString("structure_search")) {
		return structure_search(args);
	}else if (args.GetAt(1) == FString("version")) {
		return print_version(args);
	}else {
		printf("error : invalid choice: '%s'\n", args.GetAt(1).PChar());
	}

	return 0;
}
