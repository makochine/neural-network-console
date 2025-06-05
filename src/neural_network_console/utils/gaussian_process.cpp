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

#include <stdlib.h>
#include <math.h>

#include "flist.h"

#include "gaussian_process.h"
//---------------------------------------------------------------------------
namespace elfe{
#define DOUBLEZERO                  1.0e-30
//---------------------------------------------------------------------------
void GaussianProcess::Clear()
{
	x_.Clear();
	y_.Clear();
	beta_inv_ = backup_beta_inv_ = 0.25;
	kernel_matrix_det_ = backup_kernel_matrix_det_ = 1.0;
	ready_ = backup_ready_ = false;
	y_gain_ = 1.0;
	y_offset_ = 0.0;
}
//---------------------------------------------------------------------------
void GaussianProcess::AddData(const FListT<double> &x, double y)
{
	x_.Add(x);
	y_.Add(y);
}
//---------------------------------------------------------------------------
int GaussianProcess::Train()
{
    int result = true;
	assert(x_.GetSize() == y_.GetSize());

	if (x_.GetSize() >= 1) {
		//Calc Kernel (Gram) Matrix
		double *kernel_matrix = new double[x_.GetSize() * x_.GetSize() * 2];
		for (int i = 0 ; i < x_.GetSize() ; i++) {
			for (int i2 = 0 ; i2 < x_.GetSize() ; i2++) {
				kernel_matrix[i * x_.GetSize() * 2 + i2] = Kernel(x_.GetAt(i), x_.GetAt(i2));
			}
			kernel_matrix[i * x_.GetSize() * 2 + i] += beta_inv_;
		}

		//Inverse Matrix
		{
			double **matin = new double *[x_.GetSize()];
			double **matout = new double *[x_.GetSize()];
			for (int i = 0 ; i < x_.GetSize() ; i++) {
				matin[i] = kernel_matrix + i * x_.GetSize() * 2;
				matout[i] = kernel_matrix + i * x_.GetSize() * 2 + x_.GetSize();
			}

			kernel_matrix_det_ = Det(x_.GetSize(), matin, matout);
			result &= kernel_matrix_det_ > 0.0;

			result &= InverseMatrix(x_.GetSize(), matin, matout) > 0.0;

			kernel_matrix_inv_.Clear();
			for (int i = 0 ; i < x_.GetSize() ; i++) {
				FListT<double> &v = kernel_matrix_inv_.Add();
				for (int i2 = 0 ; i2 < x_.GetSize() ; i2++) {
					const double a = kernel_matrix[i * x_.GetSize() * 2 + x_.GetSize() + i2];
					v.Add(a);
				}
			}

			delete[] matin;
			delete[] matout;
		}

		delete[] kernel_matrix;
	}else {
        result = false;
    }

	y_offset_ = -y_.Mean();
	const double y_var = y_.Variance(-y_offset_);
	y_gain_ = y_var > 0.0 ? 1.0 / sqrt(y_var) : 1.0;

	ready_ = result;

	return result;
}
//---------------------------------------------------------------------------
int GaussianProcess::Predict(const FListT<double> &x, double &y_myu, double &y_sigma) const
{
	if (!ready_){
		y_myu = y_offset_;
		y_sigma = y_gain_ > 0.0 ? 1.0 / y_gain_ : 1.0;
		return false;
	}
	assert(x_.GetSize() == y_.GetSize());
	assert(x_.GetSize() == kernel_matrix_inv_.GetSize());

	double *kernel_vector = new double[x_.GetSize()];
	for (int i = 0 ; i < x_.GetSize() ; i++) {
		kernel_vector[i] = Kernel(x_.GetAt(i), x);
	}

	//Calc kK
	double *kK_vector = new double[x_.GetSize()];
	for (int i = 0 ; i < x_.GetSize() ; i++) {
		double x = 0.0;
		const FListT<double> &row = kernel_matrix_inv_.GetAt(i);
		for (int i2 = 0 ; i2 < x_.GetSize() ; i2++) {
			x += kernel_vector[i2] * row.GetAt(i2);
		}
		kK_vector[i] = x;
	}

	//Calc myu
	y_myu = 0.0;
	for (int i = 0 ; i < x_.GetSize() ; i++) {
		y_myu += kK_vector[i] * (y_.GetAt(i) + y_offset_) * y_gain_;
	}
	y_myu = y_myu / y_gain_ - y_offset_;

	//Calc sigma
	y_sigma = 0.0;
	for (int i = 0 ; i < x_.GetSize() ; i++) {
		y_sigma += kK_vector[i] * kernel_vector[i];
	}
	y_sigma = Kernel(x, x) - y_sigma;
	y_sigma /= y_gain_;

	delete[] kK_vector;
	delete[] kernel_vector;

	return true;
}
//---------------------------------------------------------------------------
double GaussianProcess::SearchHyperParameter()
{
	const double log_likelihood_before = EvaluateLogLikelihood();
	const double mse_before = EvaluateTrainingMSE();
	const double normalized_mse_before = mse_before * y_gain_ * y_gain_;

	BackupHyperParameter();
	MutateHyperParameter();
	int result = Train();
	const double log_likelihood_after = EvaluateLogLikelihood();
	const double mse_after = EvaluateTrainingMSE();
	const double normalized_mse_after = mse_after * y_gain_ * y_gain_;
	if (!result ||
		log_likelihood_after < log_likelihood_before ||
		(normalized_mse_after > normalized_mse_before && normalized_mse_after >= 1.0)) {
		RestoreHyperParameter();
		return log_likelihood_before;
	}

	return log_likelihood_after;
}
//---------------------------------------------------------------------------
double GaussianProcess::EvaluateLogLikelihood() const
{
	if (!ready_){
		return 0.0;
	}

	assert(x_.GetSize() == y_.GetSize());
	assert(x_.GetSize() == kernel_matrix_inv_.GetSize());

	//Calc yK
	double *yK_vector = new double[x_.GetSize()];
	for (int i = 0 ; i < x_.GetSize() ; i++) {
		double x = 0.0;
		const FListT<double> &row = kernel_matrix_inv_.GetAt(i);
		for (int i2 = 0 ; i2 < x_.GetSize() ; i2++) {
			const double zero_mean_y = y_.GetAt(i2) + y_offset_;
			const double normalized_y = zero_mean_y * y_gain_;
			x += normalized_y * row.GetAt(i2);
		}
		yK_vector[i] = x;
	}

	//Calc yky
	double result = 0.0;
	for (int i = 0 ; i < x_.GetSize() ; i++) {
		result -= yK_vector[i] * (y_.GetAt(i) + y_offset_) * y_gain_;
	}

	result -= log(kernel_matrix_det_ > DOUBLEZERO ? kernel_matrix_det_ : DOUBLEZERO);
	result -= x_.GetSize() * log(2.0 * 3.1415926);

	delete[] yK_vector;

	return result * 0.5;
}
//---------------------------------------------------------------------------
double GaussianProcess::EvaluateTrainingMSE() const
{
	double mse = 0.0;
	for (int i = 0 ; i < x_.GetSize() ; i++) {
		double myu = 0.0, sigma = 1.0;
		Predict(x_.GetAt(i), myu, sigma);
		const double e = y_.GetAt(i) - myu;
		mse += e * e;
	}
	return mse / x_.GetSize();
}
//---------------------------------------------------------------------------
void GaussianProcess::MutateHyperParameter()
{
	beta_inv_ = exp(log(beta_inv_) + (rand() % 101 - 50) * 0.01);
	/*
	if (beta_inv_ < 0.05) {
		beta_inv_ = 0.05;
	}
	*/
}
//---------------------------------------------------------------------------
void GaussianProcess::BackupHyperParameter()
{
	backup_kernel_matrix_inv_ = kernel_matrix_inv_;
	backup_kernel_matrix_det_ = kernel_matrix_det_;

	backup_beta_inv_ = beta_inv_;

	backup_ready_ = ready_;
}
//---------------------------------------------------------------------------
void GaussianProcess::RestoreHyperParameter()
{
	kernel_matrix_inv_ = backup_kernel_matrix_inv_;
	kernel_matrix_det_ = backup_kernel_matrix_det_;

	beta_inv_ = backup_beta_inv_;

	ready_ = backup_ready_;
}
//---------------------------------------------------------------------------
double GaussianProcess::InverseMatrix(int n, double** a, double** a_inv)
{
	int *ip = new int[n];
	if ( ip == NULL ) {
		return 0.0;
	}
	double det = DecomposeLU(n, a, ip);
	if ( fabs(det) > DOUBLEZERO ) {
		int i, j, k, ii;
		double t;
		for ( k = 0; k != n; ++k ) {
			for ( i = 0; i != n; ++i ) {
				ii = ip[i];  t = (double)(ii == k);
				for ( j = 0; j != i; ++j )
					t -= a[ii][j] * a_inv[j][k];
				a_inv[i][k] = t;
			}
			for ( i = n - 1; i != -1; --i ) {
				t = a_inv[i][k];  ii = ip[i];
				for ( j = i + 1; j != n; ++j )
					t -= a[ii][j] * a_inv[j][k];
				a_inv[i][k] = t / a[ii][i];
			}
		}
	}
	delete[] ip;
	return det;
}
//---------------------------------------------------------------------------
double GaussianProcess::DecomposeLU(int n, double** a, int *ip)
{
	double t, u, det;
	double* weight;
	weight = new double[n];
	det = 0.0;
	for (int k = 0; k != n; ++k ) {
		ip[k] = k;
		u = 0;
		for (int j = 0; j != n; ++j ) {
			t = fabs(a[k][j]);  if ( t > u ) u = t;
		}
		if ( fabs(u) < DOUBLEZERO ) {
			delete[] weight;
			return det;
		}
		weight[k] = 1 / u;
	}
	det = 1.0;
	for (int k = 0; k != n; ++k ) {
		int j = 0;
		u = -1;
		for (int i = k; i != n; ++i ) {
			const int ii = ip[i];
			t = (fabs(a[ii][k]) * weight[ii]);
			if ( t > u ) {  u = t;  j = i;  }
		}
		const int ik = ip[j];
		if ( j != k ) {
			ip[j] = ip[k];  ip[k] = ik;
			det = -det;
		}
		u = a[ik][k];  det *= u;
		if ( fabs(u) < DOUBLEZERO ) {
			delete[] weight;
			return det;
		}
		for (int i = k + 1; i != n; ++i ) {
			const int ii = ip[i];
			t = (a[ii][k] /= u);
			double *aw = &a[ii][k + 1];
			double *awe = aw + (n - (k + 1));
			double *ar = &a[ik][k + 1];
			while ( aw < awe ) {
				*aw++ -= t * *ar++;
			}
		}
	}
	delete[] weight;
	return det;
}
//---------------------------------------------------------------------------
double GaussianProcess::Det(int n, double **a, double **buf)
{
	for (int i = 0 ; i < n ; i++) {
		memcpy(buf[i], a[i], sizeof(double) * n);
	}

	for (int i = 0 ; i < n ; i++) {
		for (int j = 0 ; j < n ; j++) {
			if (i < j) {
				if (fabs(buf[i][i]) < DOUBLEZERO) {
                	return 0.0;
				}
				const double f = buf[j][i] / buf[i][i];
				for (int k = 0 ; k < n ; k++) {
					buf[j][k] -= buf[i][k] * f;
				}
			}
		}
	}

	double result = 1.0;
	for (int i = 0 ; i < n ; i++) {
		result *= buf[i][i];
	}
	return result;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void GaussianProcess_GaussianKernel::MutateHyperParameter()
{
	GaussianProcess::MutateHyperParameter();
	gaussian_kernel_sigma2_inv_ = exp(log(gaussian_kernel_sigma2_inv_) + (rand() % 101 - 50) * 0.01);
}
//---------------------------------------------------------------------------
void GaussianProcess_GaussianKernel::BackupHyperParameter()
{
	GaussianProcess::BackupHyperParameter();
	backup_gaussian_kernel_sigma2_inv_ = gaussian_kernel_sigma2_inv_;
}
//---------------------------------------------------------------------------
void GaussianProcess_GaussianKernel::RestoreHyperParameter()
{
	GaussianProcess::RestoreHyperParameter();
	gaussian_kernel_sigma2_inv_ = backup_gaussian_kernel_sigma2_inv_;
}
//---------------------------------------------------------------------------
double GaussianProcess_GaussianKernel::Kernel(const FListT<double> &x1, const FListT<double> &x2) const
{
	double x = 0.0;
	assert(x1.GetSize() == x2.GetSize());
	for (int i = 0 ; i < x1.GetSize() ; i++) {
		const double d = x1.GetAt(i) - x2.GetAt(i);
		x += d * d;
	}
	double f = -x * gaussian_kernel_sigma2_inv_ * 0.5;
	if (f < -16.0) {
		f = -16.0;
	}
	return exp(f);
}
//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
#pragma package(smart_init)
