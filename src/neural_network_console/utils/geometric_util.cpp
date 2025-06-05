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
#include "geometric_util.h"

//---------------------------------------------------------------------------
double norm(XandY p)
{
	return sqrt(p.x * p.x + p.y * p.y);
}
//---------------------------------------------------------------------------
double dot(XandY p1, XandY p2)
{
	return p1.x * p2.x + p1.y * p2.y;
}
//---------------------------------------------------------------------------
double cross(XandY p1, XandY p2)
{
	return p1.x * p2.y + p1.y * p2.x;
}
//---------------------------------------------------------------------------
double LinePointDistance(XandY line_start, XandY line_end, XandY point){
	if (dot(line_end - line_start, point - line_start) < 0.0 ) {
		return norm(point - line_start);
	}else if (dot(line_start - line_end, point - line_end) < 0.0 ) {
		return norm(point - line_end);
	}
	double line_length = norm(line_end - line_start);
	return line_length > 0.0 ?
		fabs(cross(line_end - line_start, point - line_start)) / line_length :
		norm(line_start - point);
}
//---------------------------------------------------------------------------
XandY LineNearestPoint(XandY line_start, XandY line_end, XandY point){
	if (dot(line_end - line_start, point - line_start) <= 0.0 ) {
		return line_start;
	}else if (dot(line_start - line_end, point - line_end) <= 0.0 ) {
		return line_end;
	}

	XandY v = line_end - line_start;
	XandY vp = point - line_start;

	const double num = dot(v, vp);
	const double den1 = norm(v);
	const double den2 = norm(vp);
	const double cos_dist = num / (den1 * den2);
	const XandY result = v * cos_dist * (den2 / den1) + line_start;
	return result;
}
//---------------------------------------------------------------------------
double DistanceToLine(double X, double Y, double X1, double Y1, double X2, double Y2)
{
	/*
	double dtosource =
		(X1 - X) * (X1 - X) +
		(Y1 - Y) * (Y1 - Y);//Source‚Ü‚Å‚Ì‹——£^2
	double dtodest =
		(X2 - X) * (X2 - X) +
		(Y2 - Y) * (Y2 - Y);//Dest‚Ü‚Å‚Ì‹——£^2
	*/
	double K1, K2;
	if (X2 != X1){
		K1 = (Y2 - Y1) / (X2 - X1);
	}else{
		K1 = 100000000000.0;
	}
	if (Y2 != Y1){
		K2 = (X2 - X1) / (Y2 - Y1);
	}else{
		K2 = 100000000000.0;
	}
	double LX = (K1 * X1 + K2 * X - Y1 + Y) / (K1 + K2);
	double LY = K1 * ((K1 * X1 + K2 * X - Y1 + Y) / (K1 + K2) - X1) + Y1;

	double S = 0.5;
	if (fabs(X1 - X2) > 0.0 || fabs(Y1 - Y2) > 0.0){
		if (fabs(X1 - X2) > fabs(Y1 - Y2)){
			S = (LX - X1) / (X2 - X1);
		}else{
			S = (LY - Y1) / (Y2 - Y1);
		}
	}

	if (S < 0.0){
		S = 0.0;
	}
	if (S > 1.0){
		S = 1.0;
	}

	LX = X1 * (1 - S) + X2 * S;
	LY = Y1 * (1 - S) + Y2 * S;

	S = sqrt((LX - X) * (LX - X) + (LY - Y) * (LY - Y));

	return S;
}
//---------------------------------------------------------------------------

#pragma package(smart_init)
