// Copyright 2020,2021 Sony Corporation.
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

#include "fstring.h"
#include "file_util.h"

#include "translation.h"
namespace elfe {
//---------------------------------------------------------------------------
void SetupTranslation(TLang *tlang)
{
	//enum language files
	FStringList languages = EnumLanguages();
	FString plugin_path = FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()) + FString("settings\\language\\");

	//Load translation text files
	TStringList *sl = new TStringList();
	for (int it = 0 ; it < languages.GetSize() ; it++){
		sl->LoadFromFile((plugin_path + languages.GetAt(it).SubString(1, 2) + FString(".txt")).PChar());
		if (sl->Count) {
			sl->Delete(0);
			UnicodeString lang = languages.GetAt(it).SubString(1, 2).PChar();
			tlang->AddLang(lang);
			tlang->LangStr[lang]->Text = sl->Text;
		}
	}
	delete sl;
}
//---------------------------------------------------------------------------
FStringList EnumLanguages()
{
	//enum language files
	FString plugin_path = FString(AnsiString(ExtractFilePath(ParamStr(0))).c_str()) + FString("settings\\language\\");

	FStringList result_files = EnumFiles(plugin_path.PChar());
	for (int it = 0 ; it < result_files.GetSize() ; it++){
		FString s = result_files.GetAt(it);
		if (s.SubString(s.Length() - 3, 4).ToLower() != ".txt"){
			result_files.Delete(it);
			it--;
		}
	}
	//Load translation text files
	TStringList *sl = new TStringList();
	for (int it = 0 ; it < result_files.GetSize() ; it++){
		sl->LoadFromFile((plugin_path + result_files.GetAt(it)).PChar());
		if (sl->Count) {
			result_files.GetAt(it) = result_files.GetAt(it).SubString(1, 2) + FString(":") + FString(AnsiString(sl->Strings[0]).c_str());
		}
	}
	delete sl;

	return result_files;
}
//---------------------------------------------------------------------------
UnicodeString GetTranslatedText(TLang *tlang, UnicodeString text)
{
	TStrings *lang_str = tlang->LangStr[tlang->Lang];
	if (lang_str) {
		UnicodeString text2 = text + "=";
		const int text2_length = text2.Length();
		for (int i2 = 0 ; i2 < lang_str->Count ; i2++) {
			if (lang_str->Strings[i2].SubString(1, text2_length) == text2) {
				return lang_str->Strings[i2].SubString(text2_length + 1, lang_str->Strings[i2].Length() - text2_length);
			}
		}
	}
	return text;
}
//---------------------------------------------------------------------------
}
#pragma package(smart_init)
