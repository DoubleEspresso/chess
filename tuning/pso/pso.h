#ifndef _PSO_H
#define _PSO_H

#include <vector>
#include <functional>
#include <fstream>
#include <algorithm>

#include "../../search.h"
#include "../sgd/sgd.h"
#include "../../Submodules/pso-cpp/include/psocpp.h"
#include "../../Submodules/pso-cpp/dep/eigen/Eigen/dense"


struct SearchObjective
{

	SearchObjective()
	{ }


	std::vector<TrainingElement> _trainData;

	void LoadTrainData() const {
		std::ifstream trainFile("train_.txt");
		std::string line;
		auto& td = const_cast<std::vector<TrainingElement>&>(_trainData);
		while (std::getline(trainFile, line))
		{
			std::istringstream ss(line);
			std::string fen, result;
			std::getline(ss, fen, ';');
			std::getline(ss, result, ';');
			TrainingElement d;
			d.target = std::atof(result.c_str());
			d.fen = fen;
			td.push_back(d);
		}
	}

	double GetLoss(std::vector<int>& parameters) const {
		std::vector<double> targets;
		std::vector<double> predictions;

		if (_trainData.size() <= 0)
			LoadTrainData();

		// 2. Compute loss and update parameters on batch
		for (const auto& obs : _trainData) {
			// Setup position
			position p;
			std::istringstream fen(obs.fen);
			p.setup(fen);

			// Qsearch predict
			auto qScore = qSearch::qSearch(p, parameters);
			targets.push_back(obs.target);
			predictions.push_back(qScore);
		}

		return LossFuncs::MSE(predictions, targets);
	}

	template<typename Derived>
	double operator()(const Eigen::MatrixBase<Derived>& xval) const
	{
		assert(xval.size() == 8);
		std::vector<int> parameters;
		for (int i = 0; i < xval.size(); ++i)
			parameters.push_back(xval(i));
		return GetLoss(parameters);
	}
};

// Particle swarm optimization
class PSO {

	private:
		int _populationSize = 400;
		int _epochs = 400;
		std::vector<int> _parameters;

		Eigen::VectorXd InitializeDefaultParams() {
			// These are initialized *in order* according to ParamInfo struct
			// found in eval.h

			//_parameters = { 300, 315, 480, 910, 285, 330, 495, 895 };
			std::vector<double> h = { 20, 21, 40, 80, 25, 26, 60, 85 };
			return Eigen::Map<Eigen::VectorXd, Eigen::Unaligned>(h.data(), h.size());
		}

	public:

		PSO() { }

		bool Train() {

			// Initialize all parameters
			auto guess = InitializeDefaultParams();

			// PSO optimizer
			pso::ParticleSwarmOptimization<double, SearchObjective,
				pso::ExponentialDecrease2<double>> optimizer;

			// Set number of iterations as stop criterion.
			// Set it to 0 or negative for infinite iterations (default is 0).
			optimizer.setMaxIterations(_epochs);

			// Set the minimum change of the x-values (particles) (default is 1e-6).
			// If the change in the current iteration is lower than this value, then
			// the optimizer stops minimizing.
			optimizer.setMinParticleChange(1);

			// Set the number of threads used for evaluation (OpenMP only).
			// Set it to 0 or negative for auto detection (default is 1).
			//optimizer.setThreads(8);

			// Turn verbosity on, so the optimizer prints status updates after each
			// iteration.
			optimizer.setVerbosity(2);

			// Set the bounds in which the optimizer should search.
			// Each column vector defines the (min, max) for each dimension  of the
			// particles.
			Eigen::MatrixXd bounds(2, 8);
			bounds << 70,		70,		70,		70,		70,		70,		70,		70,
					  1000,		1000,	1000,	1000,	1000,	1000,	1000,	1000;

			// start the optimization with a particle count
			auto result = optimizer.minimize(bounds, _populationSize, guess);

			std::cout << "Done! Converged: " << (result.converged ? "true" : "false")
				<< " Iterations: " << result.iterations << std::endl;

			// do something with final function value
			std::cout << "Final fval: " << result.fval << std::endl;

			// do something with final x-value
			std::cout << "Final xval: " << result.xval.transpose() << std::endl;

			return true;
		};
};

#endif