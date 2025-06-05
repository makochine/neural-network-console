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

#include <math.h>
#include "color_util.h"

//---------------------------------------------------------------------------
int FadeColor(int c1, int c2, double level)
{
	int b1 = c1 & 0xff0000;
	int g1 = c1 & 0xff00;
	int r1 = c1 & 0xff;

	int b2 = c2 & 0xff0000;
	int g2 = c2 & 0xff00;
	int r2 = c2 & 0xff;

	return  ((int)(b1 * (1.0 - level) + b2 * level) & 0xff0000) |
		((int)(g1 * (1.0 - level) + g2 * level) & 0xff00) |
		((int)(r1 * (1.0 - level) + r2 * level) & 0xff);
}
//---------------------------------------------------------------------------
int GetCMColor(float f)
{
	int r = 0, g = 0, b = 0;
	if (f < 0.0f) {
		f = 0.0f;
	}else if (f > 1.0f) {
		f = 1.0f;
	}

	if (f < 1.0f / 8){
		//�Â��`��
		float f1 = 0.5 + f * 4;
		b = f1 * 255;
	}else if (f < 3.0f / 8){
		//�`�V�A��
		float f1 = (f - 1.0f / 8) * 4;
		b = 255;
		g = f1 * 255;
	}else if (f < 5.0f / 8){
		//�V�A���`�C�G���[
		float f1 = (f - 3.0f / 8) * 4;
		b = 255 - f1 * 255;
		g = 255;
		r = f1 * 255;
	}else if (f < 7.0f / 8){
		//�C�G���[�`��
		float f1 = (f - 5.0f / 8) * 4;
		r = 255;
		g = 255 - f1 * 255;
	}else{
		//�ԁ`�Z����
		float f1 = (f - 7.0f / 8) * 4;
        r = 255 - f1 * 255;
	}

    if (r < 0){
        r = 0;
    }
    if (r > 255){
        r = 255;
    }

    if (g < 0){
        g = 0;
    }
    if (g > 255){
        g = 255;
    }

    if (b < 0){
        b = 0;
    }
    if (b > 255){
        b = 255;
    }

    return (r << 16) | (g << 8) | b;

}
//---------------------------------------------------------------------------
int GetColor(float f, int base, int cont)
{
    int R = base, G = base, B = base;
    float f2 = fmod(f, 1.0f / 6) * 6;
    if (f <= 1.0/6){
        R = base + cont;
        G = base + f2 * cont;
    }else if (f <= 2.0/6){
        R = base + (1.0f - f2) * cont;
        G = base + cont;
    }else if (f <= 3.0/6){
        G = base + cont;
        B = base + f2 * cont;
    }else if (f <= 4.0/6){
        G = base + (1.0f - f2) * cont;
        B = base + cont;
	}else if (f <= 5.0/6){
        B = base + cont;
        R = base + f2 * cont;
    }else{
        B = base + (1.0f - f2) * cont;
        R = base + cont;
    }
    return B + G * 0x100 + R * 0x10000;
}
//---------------------------------------------------------------------------

#pragma package(smart_init)
