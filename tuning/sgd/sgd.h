#ifndef _SGD_H
#define _SGD_H

#include <vector>
#include <functional>
#include <fstream>
#include <algorithm>
#include <numeric>

#include "../../search.h"


/*
Summary: decided to pass on this for now, the integerized gradients
are difficult to handle and possible numerically unstable. The qSearch
with ttable must have a properly tuned parameter adjust size - else most gradients
will vanish and no learning occurs. The required fine-tuning even for simple problems is discouraging.
*/

namespace Funcs {

	inline double Sigmoid(const double& x, const double& K) {
		auto d = 1 + std::pow(10, -K * x);
		return 1.0 / d;
	}
}

namespace LossFuncs {

	// Mean-squared error loss
	inline double MSE(const std::vector<double>& predictions, const std::vector<double>& targets) {
		double err = 0;
		for (int i = 0; i < predictions.size(); ++i) {
			err += (predictions[i] - targets[i]) * (predictions[i] - targets[i]);
		}
		return err / predictions.size();
	}

	// Mean-absolute error loss
	inline double MAE(const std::vector<double>& predictions, const std::vector<double>& targets) {
		double err = 0;
		for (int i = 0; i < predictions.size(); ++i) {
			err += std::abs(predictions[i] - targets[i]);
		}
		return err / predictions.size();
	}

	// Each prediction is the relative score from qSearch for position i.
	inline double QE(const std::vector<double>& predictions, const std::vector<double>& targets, double K) {
		double err = 0;

		for (int i = 0; i < predictions.size(); ++i) {
			auto s = Funcs::Sigmoid(predictions[i], K);
			auto t = Funcs::Sigmoid(targets[i], K);
			auto dP = t - s;
			err += dP * dP;
		}
		return err / predictions.size();
	}
}

namespace Util {


	template<typename T>
	T mean(const std::vector<T>& vec) {
		const size_t sz = vec.size();
		if (sz <= 1) {
			return vec[0];
		}
		return std::accumulate(vec.begin(), vec.end(), 0.0) / sz;
	}

	template<typename T>
	T variance(const std::vector<T>& vec) {
		const size_t sz = vec.size();
		if (sz <= 1) {
			return 0.0;
		}

		// Calculate the mean
		const T mean = std::accumulate(vec.begin(), vec.end(), 0.0) / sz;

		// Now calculate the variance
		auto variance_func = [&mean, &sz](T accumulator, const T& val) {
			return accumulator + ((val - mean) * (val - mean) / (sz - 1));
			};

		return std::accumulate(vec.begin(), vec.end(), 0.0, variance_func);
	}
}


namespace qSearch {

	inline double qSearch(position& p, std::vector<int>& parameters) {
		// Perform a full-window q-search
		auto alpha = ninf;
		auto beta = inf;

		// Initialize the evaluation method with tunable input parameters
		Evaluation::Evaluation e;
		e.isTuning = true;
		e.Initialize_KpK(parameters);
		//ttable.clear();

		// Run qsearch on this position
		//const unsigned stack_size = 64 + 4;
		//node stack[stack_size];
		//return Search::qsearch<non_pv>(p, alpha, beta, 0, stack + 2, e);
		return e.evaluate(p, *SearchThreads[p.id()], -1);
	}
}

struct TrainingElement {

	// Consists of a string fen position and a target value (Win, Loss, Draw)
	float target = -1.0;
	float prediction = -1.0;
	std::string fen = "";
};

namespace Gradients {

	inline double dSigma(double x, double k) {
		if (std::abs(k * x) > 10)
			return 0;

		auto p = std::pow(10, -k * x);
		auto n = 2.30259 * k * p;
		auto d = (p + 1) * (p + 1);
		return n / d;
	}

	inline double dQE(std::vector<TrainingElement> batch, std::vector<double> qPrev, std::vector<int> parameters, double K, double maxScore) {
		// E = 2/N * Sum_{1,N}(Ri - Sigmoid(qi))*d/dwj(-Sigmoid(qi(wj)))
		// E = 2/N * Sum_{1,N}(Ri - Sigmoid(qi)) * (-sig'(qi(wj)) * d/dwj qi(wj))

		// This means:
		//  - for each position, adjust wj by 1 (smallest unit)
		int count = 0;
		double grad = 0;
		for (const auto& obs : batch) {
			// Setup position
			position p;
			std::istringstream fen(obs.fen);
			p.setup(fen);

			//  - recompute qsearch for this position -> qi(wj+h), compute quotient (qi(wj+h) - qi(wj)) / h
			auto qScore = (int)std::round((double)qSearch::qSearch(p, parameters) / maxScore);
			auto dQ = qScore - qPrev[count];

			//  - analytical evaluation of sig'(qi(wj))
			auto dSig = dSigma(qPrev[count], K);
			auto dP = (obs.target - Funcs::Sigmoid(qPrev[count], K));
			grad += -dP * dSig * dQ;
			count++;
		}
		return 2.0 / batch.size() * grad;
	}
}


// Batched minimiziation of 
//  w:= w - lr/n sum_{1,n} grad(Fi({param}))
class SGD {

	private:
		int _batchSize = 35;
		//int _miniBatchSize = 64;
		int _epochs = 700;
		std::vector<TrainingElement> _trainData;
		std::vector<int> _parameters;
		double _learnRate = 10;

		void LoadTrainData() {
			std::ifstream trainFile("train.txt");
			std::string line;
			while (std::getline(trainFile, line))
			{
				std::istringstream ss(line);
				std::string fen, result;
				std::getline(ss, fen, ';');
				std::getline(ss, result, ';');
				TrainingElement d;
				if (result.find("in check") != std::string::npos)
					continue;
				d.target = std::atof(result.c_str());
				d.fen = fen;
				_trainData.push_back(d);
			}
		}

		void InitializeDefaultParams() {
			// These are initialized *in order* according to ParamInfo struct
			// found in eval.h
			_parameters = { 300, 315, 480, 910, 285, 330, 495, 895 };
			//_parameters = { 10, 20, 30, 40, 50, 60, 70, 80 };
		}

		// Setup a derivative-like quotient (delta=1). We do not use a symmetric difference to avoid
		// running qSearch multiple times. 
		std::vector<int> AdjustParameter(int k) {
			std::vector<int> adjusted = _parameters;
			adjusted[k] += 100;
			return adjusted;
		}

	public:

		SGD() { }

		bool Train() {
			// 0. Load and shuffle training data
			LoadTrainData();
			std::cout << "Loaded " << _trainData.size() << " training positions for eval tuning.." << std::endl;

			// 1. Setup batches
			int numBatches = (_trainData.size() + _batchSize - 1) / _batchSize;

			// 1. Initialize parameters to defaults
			InitializeDefaultParams();

			// Empirical for now, should be fit when eval is ready.
			constexpr double K = 1.0/400.0;

			std::random_device rd;
			std::mt19937 g(rd());

			// 2 Strategies - standard SGD or mini-batch.
			for (int k = 0; k < _epochs; ++k) {
				//std::cout << "..Running epoch " << (k+1) << " of " << _epochs << std::endl;


				// Shuffle data
				std::shuffle(_trainData.begin(), _trainData.end(), g);

				for (int j = 0; j < numBatches; ++j) {

					std::vector<double> targets;
					std::vector<double> predictions;

					// 1. collect batch
					auto startIdx = j * _batchSize;
					auto endIdx = std::min((j + 1) * _batchSize, (int)_trainData.size());
					std::vector<TrainingElement> batch(_trainData.begin() + startIdx, _trainData.begin() + endIdx);

					// 2. Compute loss and update parameters on batch
					for (const auto& obs : batch) {
						// Setup position
						position p;
						std::istringstream fen(obs.fen);
						p.setup(fen);

						// Qsearch predict
						auto qScore = qSearch::qSearch(p, _parameters);
						targets.push_back(obs.target);
						predictions.push_back(qScore/*Funcs::Sigmoid(qScore, K)*/);
					}

					// Compute error
					auto loss = LossFuncs::QE(predictions, targets, K);
					
					if ((k+1) % 100 == 0)
						std::cout << "..Epoch " << (k + 1) << " batch idx " << (j + 1) << " loss: " << loss << std::endl;

					// 3. Parameter update (batched gradient)

					for (int pIdx = 0; pIdx < _parameters.size(); ++pIdx) {
						std::vector<double> adjPredictions;
						auto adjusted = AdjustParameter(pIdx);

						for (const auto& obs : batch) {
							// Setup position
							position p;
							std::istringstream fen(obs.fen);
							p.setup(fen);

							// Qsearch predict
							auto qScore = qSearch::qSearch(p, adjusted);
							adjPredictions.push_back(qScore); ///*Funcs::Sigmoid(qScore, K)*/);
						}

						auto lossAdj = LossFuncs::QE(adjPredictions, targets, K);
						auto grad = (lossAdj - loss);
						//std::cout << "\tdP(" << pIdx << ")=" << _learnRate * grad << std::endl;
						//  - wj(new) = wj(old) - lr * dQE()
						_parameters[pIdx] -= _learnRate * grad;
					}
				}

				// 4. adaptive adjust learn rate?
			}

			std::cout << " ..Tuned parameters " << std::endl;
			for (const auto& p : _parameters) {
				std::cout << p << std::endl;
			}

			return true;
		}

};


#endif