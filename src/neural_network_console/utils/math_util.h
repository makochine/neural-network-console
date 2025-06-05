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

#ifndef math_utilH
#define math_utilH
//---------------------------------------------------------------------------
template <typename _T> class MathUtil{
public:
	static void CovMatrix(const _T *matrix, int dim, int num_data, _T *cov_matrix)
	{
		_T *mean = new _T[dim];
		for (int i = 0; i < dim ; i++) {
			_T sum = (_T)0.0;
			for (int k = 0; k < num_data ; k++) {
				sum += matrix[k * dim + i];
			}
			mean[i] = sum / num_data;
		}
		for (int i = 0; i < dim ; i++) {
			for (int j = 0; j < dim ; j++) {
				_T cov = (_T)0.0;
				for (int k = 0; k < num_data ; k++) {
					cov += (matrix[k * dim + i] - mean[i]) * (matrix[k * dim + j] - mean[j]);
				}
				cov /= num_data;
				cov_matrix[i * dim + j] = cov;
				cov_matrix[j * dim + i] = cov;
			}
		}
		delete[] mean;
	}
	static void Eigen(const _T *cov_matrix, int size, int max_loop, _T eps, _T *eigen)
	{
		// Jacobi
		_T *buf_a = new _T[size * size];
		_T *buf_x = new _T[size * size];
		_T *buf_a2 = new _T[size * size];
		_T *buf_x2 = new _T[size * size];

		int max_i = 0, max_j = 0;
		for (int i = 0; i < size ; i++) {
			for (int j = 0; j < size ; j++) {
				buf_a[i * size + j] = cov_matrix[i * size + j];
				buf_x[i * size + j] = (_T)0.0;
			}
			buf_x[i * size + i] = (_T)1.0;
		}
		// Calculation
		int converged = false;
		int count = 0;
		while (!converged && count < max_loop) {
			// search for maximum
			_T max = (_T)0.0;
			for (int i = 0; i < size ; i++) {
				for (int j = 0; j < size ; j++) {
					if (j < i) {
						_T val = fabs(buf_a[i * size + j]);
						if (val > max) {
							max = val;
							max_i = i;
							max_j = j;
						}
					}
				}
			}
			if (max < eps )            // Converged
				converged = true;
			else{                       // Not Converged
				// prepare
				_T max_ii = buf_a[max_i * size+max_i];
				_T max_jj = buf_a[max_j*size+max_j];
				_T max_ij = buf_a[max_i * size+max_j];
				_T s  = -max_ij;
				_T t  = (_T)0.5 * (max_ii - max_jj);
				_T v = (_T)(fabs(t) / sqrt(s * s + t * t));
				_T sn = (_T)sqrt(0.5 * (1.0 - v));
				if (s * t < 0.0) {
					sn = -sn;
				}
				_T cs = sqrt(1.0 - sn * sn);

				// Calculate Ak
				for (int i = 0; i < size ; i++) {
					if (i == max_i) {
						for (int j = 0; j < size ; j++) {
							if (j == max_i) {
								buf_a2[max_i * size + max_i] = max_ii * cs * cs + max_jj * sn * sn - 2.0 * max_ij * sn * cs;
							}else if (j == max_j) {
								buf_a2[max_i * size + max_j] = 0.0;
							} else {
								buf_a2[max_i * size + j] = buf_a[max_i * size + j] * cs - buf_a[max_j * size + j] * sn;
							}
						}
					}
					else if (i == max_j) {
						for (int j = 0; j < size ; j++) {
							if (j == max_j) {
								buf_a2[max_j * size + max_j] = max_ii * sn * sn + max_jj * cs * cs + 2.0 * max_ij * sn * cs;
							}else if (j == max_i) {
								buf_a2[max_j * size + max_i] = 0.0;
							} else {
								buf_a2[max_j * size + j] = buf_a[max_j * size + j] * cs + buf_a[max_i * size + j] * sn;
							}
						}
					}
					else{
						for (int j = 0; j < size ; j++) {
							if (j == max_i) {
								buf_a2[i * size + max_i] = buf_a[i * size + max_i] * cs - buf_a[i * size + max_j] * sn;
							}else if (j == max_j) {
								buf_a2[i * size + max_j] = buf_a[i * size + max_j] * cs + buf_a[i * size + max_i] * sn;
							} else {
								buf_a2[i * size + j] = buf_a[i * size + j];
							}
						}
					}
				}
				// Calculate Xk
				for (int i = 0; i < size ; i++) {
					for (int j = 0; j < size ; j++) {
						if (j == max_i) {
							buf_x2[i * size + max_i] = buf_x[i * size + max_i] * cs - buf_x[i * size + max_j] * sn;
						}else if (j == max_j) {
							buf_x2[i * size + max_j] = buf_x[i * size + max_j] * cs + buf_x[i * size + max_i] * sn;
						} else {
							buf_x2[i * size + j] = buf_x[i * size + j];
						}
					}
				}
				for (int i = 0 ; i < size ; i++) {
					for (int j = 0; j < size ; j++) {
						buf_a[i * size + j] = buf_a2[i * size + j];
						buf_x[i * size + j] = buf_x2[i * size + j];
					}
				}
				// Next Step
				count++;
			}
		}

		for (int i = 0; i < size ; i++) {
			eigen[i] = buf_a[i * size + i];
		}
		delete[] buf_a;
		delete[] buf_x;
		delete[] buf_a2;
		delete[] buf_x2;
	}
};
//---------------------------------------------------------------------------
#endif
