#ifndef _GEN_SCORES_H
#define _GEN_SCORES_H

#include <vector>
#include <functional>
#include <fstream>
#include <algorithm>

#include "../../search.h"
#include "../../uci.h"
#include "../sgd/sgd.h"


// Adaptive random search
class GEN {

	private:
		std::vector<TrainingElement> _trainData;
		int _searchDepth = 4;

		void LoadFenPositions() {
			std::ifstream trainFile("input.txt");
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

	public:

		GEN() { }

		bool MakeTrainData() {
			// 0. Load and shuffle training data
			LoadFenPositions();
			std::cout << "Loaded " << _trainData.size() << " training positions for eval tuning.." << std::endl;

			for (auto& obs : _trainData) {

				// Setup position
				position p;
				std::istringstream fen(obs.fen);
				p.setup(fen);

				limits lims;
				lims.depth = _searchDepth;
				obs.target = Search::start(p, lims, true);
			}

			std::ofstream outfile;
			outfile.open("train_.txt", std::ios_base::app);
			for (const auto& obs : _trainData) {
				outfile << obs.fen << ";" << obs.target << "\n";
			}
			
			return true;
		};
};

#endif