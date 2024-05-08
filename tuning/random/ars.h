#ifndef _ARS_H
#define _ARS_H

#include <vector>
#include <functional>
#include <fstream>
#include <algorithm>

#include "../../search.h"
#include "../sgd/sgd.h"


// Adaptive random search
class ARS {

	private:
		int _epochs = 3000;
		int _iStepSize = 50;
		int _populationSize = 200;
		std::vector<TrainingElement> _trainData;
		std::vector<int> _parameters;

		void LoadTrainData() {
			std::ifstream trainFile("train_.txt");
			std::string line;
			while (std::getline(trainFile, line))
			{
				std::istringstream ss(line);
				std::string fen, result;
				std::getline(ss, fen, ';');
				std::getline(ss, result, ';');
				TrainingElement d;
				d.target = std::atof(result.c_str());
				d.fen = fen;
				_trainData.push_back(d);
			}
		}

		void InitializeDefaultParams() {
			// These are initialized *in order* according to ParamInfo struct
			// found in eval.h
			//_parameters = { 300, 315, 480, 910, 285, 330, 495, 895 };
			_parameters = { 20, 21, 40, 80, 25, 26, 60, 85 };
		}


	public:

		ARS() { }

		bool Train() {
			// 0. Load and shuffle training data
			LoadTrainData();
			std::cout << "Loaded " << _trainData.size() << " training positions for eval tuning.." << std::endl;

			// 1. Initialize parameters to defaults
			InitializeDefaultParams();

			// 2. Optionally standardize the scores
			//{
			//	std::vector<double> scores;
			//	for (auto& obs : _trainData) {
			//		scores.push_back(obs.target);
			//	}
			//	auto mean = Util::mean(scores);
			//	auto stdev = Util::variance(scores);
			//	for (auto& obs : _trainData) {
			//		obs.target -= mean;
			//		//obs.target /= stdev;
			//	}
			//}
			// Empirical for now, should be fit when eval is ready.
			constexpr double K = 1.0 / 400.0;

			std::random_device rd;
			std::mt19937 g(rd());
			std::uniform_real_distribution<double> distrib(-1.0, 1.0);
			std::vector<double> stepSizes(_parameters.size(), _iStepSize);

			auto bestObjective = std::numeric_limits<double>::max();
			std::vector<int> bestParameters;

			std::vector<std::vector<int>> population(_populationSize, 
				std::vector<int>(_parameters.size()));


			// Generate a population
			for (auto& individual : population) {
				individual = _parameters; // all default params
			}

			// 2. Random search with adaptive sampling
			for (int k = 0; k < _epochs; ++k) {

				std::vector<double> losses;

				for (auto& individual : population) {

					for (int i = 0; i < individual.size(); ++i) {
						individual[i] += static_cast<int>(stepSizes[i] * distrib(g));
						individual[i] = std::min(1000, std::max(70, individual[i]));
					}


					std::vector<double> targets;
					std::vector<double> predictions;

					// 2. Compute loss and update parameters on batch
					for (const auto& obs : _trainData) {
						// Setup position
						position p;
						std::istringstream fen(obs.fen);
						p.setup(fen);

						// Qsearch predict
						auto qScore = qSearch::qSearch(p, individual);
						targets.push_back(obs.target);
						predictions.push_back(qScore);
					}

					// Compute error
					losses.push_back(LossFuncs::QE(predictions, targets, K));
					losses.push_back(LossFuncs::MSE(predictions, targets));
				}

				// Get best individual
				auto bestIdx = std::min_element(losses.begin(), losses.end()) - losses.begin();

				if ((k + 1) % 100 == 0)
					std::cout << "..Epoch " << (k + 1) << " loss: " << losses[bestIdx] << std::endl;

				// 3. Parameter update (batched gradient)
				auto newBest = false;
				if (losses[bestIdx] < bestObjective) {
					bestObjective = losses[bestIdx];
					bestParameters = population[bestIdx];
					newBest = true;
				}

				// 4. Generate a new population
				for (int i = 0; i < population.size(); ++i) {
					population[i] = (newBest ? population[bestIdx] : bestParameters);
				}

				for (int i = 0; i < _parameters.size(); ++i) {
					stepSizes[i] *= (newBest ? 0.9 : 1.1);
					stepSizes[i] = std::max(2.0, std::min(400.0, stepSizes[i]));
				}
			}

			_parameters = bestParameters;
			std::cout << " ..Tuned parameters " << std::endl;
			for (const auto& p : _parameters)
				std::cout << p << std::endl;

			std::cout << " ..Step sizes " << std::endl;
			for (const auto& p : stepSizes)
				std::cout << p << std::endl;

			return true;
		};
};

#endif