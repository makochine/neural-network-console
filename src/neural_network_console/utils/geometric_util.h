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

#ifndef geometric_utilH
#define geometric_utilH
//---------------------------------------------------------------------------
typedef struct XandY_{
	double x;
	double y;

	XandY_ operator -(const XandY_ &source){
		XandY_ result = *this;
		result.x -= source.x;
		result.y -= source.y;
		return result;
	};

	XandY_ &operator =(const XandY_ &source){
		this->x = source.x;
		this->y = source.y;
		return *this;
	};

	XandY_ operator +(const XandY_ &source){
		XandY_ result = *this;
		result.x += source.x;
		result.y += source.y;
		return result;
	};

	XandY_ operator *(const double mpx_coef){
		XandY_ result = *this;
		result.x *= mpx_coef;
		result.y *= mpx_coef;
		return result;
	};

	XandY_(const XandY_ &source) :
		x(source.x), y(source.y)
	{
	};
}XandY;

double norm(XandY p);
double dot(XandY p1, XandY p2);
double cross(XandY p1, XandY p2);
double LinePointDistance(XandY line_start, XandY line_end, XandY point);
XandY LineNearestPoint(XandY line_start, XandY line_end, XandY point);

double DistanceToLine(double X, double Y, double X1, double Y1, double X2, double Y2);
//---------------------------------------------------------------------------
#endif
