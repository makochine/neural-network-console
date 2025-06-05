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

#include "wavefile.h"
using namespace elfe;
//---------------------------------------------------------------------------
void TWaveFile::Clear()
{
	if (buf_) {
		delete[] buf_;
		buf_ = NULL;
	}
	ch_ = 0;
	sampling_freq_ = 0;
	length_ = 0;
	bit_width_ = 0;
}
//---------------------------------------------------------------------------
int TWaveFile::MoveToChunk(FILE *F, char* chunk, int max_counter)
{
	char c[5]={0,0,0,0,0};
	FString chunk_s = FString(chunk);
	while (--max_counter){
		fread(&c[3],1,1,F);
		if (chunk_s == c){
			long l;
			fread(&l,sizeof(l),1,F);
			return l;
		}else{
			c[0] = c[1];
			c[1] = c[2];
			c[2] = c[3];
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
int TWaveFile::LoadFromFile(const char *file_name)
{
	Clear();

	FILE *F = fopen(file_name, "rb");
	if (!F){
		return false;
	}

	if (MoveToChunk(F, (char*)"fmt ")){
		short format;
		fread(&format, 2, 1, F);
		if (format == 0x0001) {
			short blockalign;
            long bytepersec;
			fread(&ch_, 2, 1, F);
			fread(&sampling_freq_, 4, 1, F);
			fread(&bytepersec, 4, 1, F);
			fread(&blockalign, 2, 1, F);
			fread(&bit_width_, 2, 1, F);
			if ((bit_width_ == 8 || bit_width_ == 16) && bytepersec == ch_ * sampling_freq_ * bit_width_ / 8) {
				int data_size = MoveToChunk(F, (char*)"data");
				length_ = data_size / (ch_ * (bit_width_ / 8));
				if (data_size > 0 && data_size % length_ == 0) {
					buf_ = new char[data_size];
					if (buf_) {
						fread(buf_, sizeof(char), data_size, F);
					}
				}
			}
		}
	}
	fclose(F);

	return buf_ != NULL;
}
//---------------------------------------------------------------------------
float TWaveFile::GetFloatValue(int pos, int ch) const
{
	if (bit_width_ == 8) {
		return (int)(((unsigned char*)buf_)[pos * ch_ + ch] - 128) * (1.0f / 128);
	}else if (bit_width_ == 16) {
		return ((short*)buf_)[pos * ch_ + ch] * (1.0f / 32768);
	}
	return 0.0f;
}
//---------------------------------------------------------------------------
#pragma package(smart_init)
