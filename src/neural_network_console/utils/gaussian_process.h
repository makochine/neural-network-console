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

#ifndef gaussian_processH
#define gaussian_processH
//---------------------------------------------------------------------------
namespace elfe{
//---------------------------------------------------------------------------
class GaussianProcess
{
public:
	GaussianProcess() :
		beta_inv_(0.25), backup_beta_inv_(0.25),
		kernel_matrix_det_(1.0), backup_kernel_matrix_det_(1.0),
		y_gain_(1.0), y_offset_(0.0),
		ready_(false), backup_ready_(false) {};
	virtual ~GaussianProcess() {};
public:
	//Setup
	void Clear();
	void SetBeta(double beta) {beta_inv_ = 1.0 / beta;};
	double GetBeta() const {return 1.0 / beta_inv_;};
public:
	//Basic Operation
	void AddData(const FListT<double> &x, double y);
	int Train();
	int Predict(const FListT<double> &x, double &y_myu, double &y_sigma) const;
	int GetReady() const {return ready_;};
	double GetYGain() const {return y_gain_;};
	double GetYOffset() const {return y_offset_;};

	double SearchHyperParameter();
	double EvaluateLogLikelihood() const;
	double EvaluateTrainingMSE() const;
protected:
	//HyperParameterSearch
	virtual void MutateHyperParameter();
	virtual void BackupHyperParameter();
	virtual void RestoreHyperParameter();
private:
	//Kernel
	virtual double Kernel(const FListT<double> &x1, const FListT<double> &x2) const = 0;
public:
	//Util
	static double InverseMatrix(int n, double** a, double** a_inv);
	static double DecomposeLU(int n, double** a, int *ip);
	static double Det(int n, double **a, double** buf);
private:
	//Data
	FListP<FListT<double> > x_;
	FListT<double> y_;
private:
    double y_gain_, y_offset_;
	FListP<FListT<double> > kernel_matrix_inv_, backup_kernel_matrix_inv_;
	double kernel_matrix_det_, backup_kernel_matrix_det_;
	int ready_, backup_ready_;
private:
	//Setup
	double beta_inv_, backup_beta_inv_;
};
//---------------------------------------------------------------------------
class GaussianProcess_GaussianKernel : public GaussianProcess
{
public:
	GaussianProcess_GaussianKernel() : gaussian_kernel_sigma2_inv_(1.0), backup_gaussian_kernel_sigma2_inv_(1.0) {};
public:
	void SetGaussianKernelSigma(double sigma) {gaussian_kernel_sigma2_inv_ = 1.0 / (sigma * sigma);};
	double GetGaussianKernelSigma() {return sqrt(1.0 / gaussian_kernel_sigma2_inv_);};
private:
	//HyperParameterSearch
	void MutateHyperParameter() override;
	void BackupHyperParameter() override;
	void RestoreHyperParameter() override;
private:
	//Kernel
	double Kernel(const FListT<double> &x1, const FListT<double> &x2) const override;
private:
	//Setup
	double gaussian_kernel_sigma2_inv_, backup_gaussian_kernel_sigma2_inv_;
};
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
#endif
