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

#ifndef wavefileH
#define wavefileH
//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
class TWaveFile
{
public:
	TWaveFile() : buf_(NULL), ch_(0), sampling_freq_(0), length_(0), bit_width_(0) {}
	virtual ~TWaveFile() {Clear();}
private:
	TWaveFile(const TWaveFile &source);
public:
	void Clear();
	int LoadFromFile(const char *file_name);

	int GetCh() const {return ch_;}
	int GetLength() const {return length_;}
	float GetFloatValue(int pos, int ch) const;
private:
	char *buf_;
	short ch_;
    long sampling_freq_;
	int length_;
	int bit_width_;
private:
	int MoveToChunk(FILE *F, char* chunk, int max_counter = 1024);
};
//---------------------------------------------------------------------------
#endif
