#ifndef _PSO_H
#define _PSO_H

#include <vector>
#include <functional>
#include <fstream>
#include <algorithm>
#include <numeric>

#include "../../search.h"
#include "../sgd/sgd.h"
#include "../../Submodules/pso-cpp/include/psocpp.h"
#include "../../Submodules/pso-cpp/dep/eigen/Eigen/dense"

struct SearchObjective
{

	SearchObjective() { }

	int _batchSize = 64000;
	std::vector<TrainingElement> _trainData;


	void LoadTrainData() const {
		std::ifstream trainFile(R"(C:\Code\chess-testing\tuning\scored-self-match.txt)");
		std::string line;
		auto& td = const_cast<std::vector<TrainingElement>&>(_trainData);
		size_t maxSize = 4000000;
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
			if (td.size() > maxSize)
				break;
		}
	}

	double GetLoss(std::vector<int>& parameters) const {
		std::vector<double> targets;
		std::vector<double> predictions;
		constexpr double K = 1.0 / 200.0;

		if (_trainData.size() <= 0)
			LoadTrainData();

		// 1. collect batch
		auto& td = const_cast<std::vector<TrainingElement>&>(_trainData);
		std::random_device rd;
		std::mt19937 gen(rd());
		std::shuffle(td.begin(), td.end(), gen);
		auto startIdx = 0;
		auto endIdx = std::min( _batchSize, (int)_trainData.size());
		std::vector<TrainingElement> batch(td.begin() + startIdx, td.begin() + endIdx);


		// 2. Compute loss and update parameters on batch
		for (const auto& obs : batch) {
			// Setup position
			position p;
			std::istringstream fen(obs.fen);
			p.setup(fen);

			// Qsearch predict
			auto qScore = qSearch::qSearch(p, parameters);
			targets.push_back(obs.target);
			predictions.push_back(qScore);
		}
		//return LossFuncs::MSE(predictions, targets);
		return LossFuncs::MAE(predictions, targets);
		//return LossFuncs::QE(predictions, targets, K);
	}

	template<typename Derived>
	double operator()(const Eigen::MatrixBase<Derived>& xval) const
	{
		std::vector<int> parameters;
		for (int i = 0; i < xval.size(); ++i)
			parameters.push_back(xval(i));
		return GetLoss(parameters);
	}
};

// Particle swarm optimization
class PSO {

	private:
		int _populationSize = 200;
		int _epochs = 40000;
		std::vector<int> _parameters;

		Eigen::VectorXd InitializeDefaultParams() {
			// These are initialized *in order* according to ParamInfo struct
			// found in eval.h

			//_parameters = { 300, 315, 480, 910, 285, 330, 495, 895 };
			std::vector<double> h = { 20, 21, 40, 80, 25, 26, 60, 85 };
			return Eigen::Map<Eigen::VectorXd, Eigen::Unaligned>(h.data(), h.size());
		}

		std::vector<int> ReadFromKpK_ParamInfo() {

			Evaluation::ParamInfo p;
			const auto& eg = p.eg;
			std::vector<int> kpkParams = { (int)eg.sq_scaling[0] };
			//for (const auto& p : eg.pawnKingDist)
			//	kpkParams.push_back(p);
			for (const auto& p : eg.islandPenalty)
				kpkParams.push_back(p);
			for (const auto& p : eg.doubledPenalty)
				kpkParams.push_back(p);
			//for (const auto& p : eg.doubledOpen)
			//	kpkParams.push_back(p);
			kpkParams.push_back(eg.doubledDefendedBonus);
			kpkParams.push_back(eg.isolatedPenalty);
			kpkParams.push_back(eg.isolatedKingDistPenalty);
			kpkParams.push_back(eg.isolatedControlPenalty);
			kpkParams.push_back(eg.backwardPenalty);
			kpkParams.push_back(eg.backwardNeighborPenalty);
			kpkParams.push_back(eg.backwardControlPenalty);
			kpkParams.push_back(eg.kMajorityBonus);
			kpkParams.push_back(eg.qMajorityBonus);
			kpkParams.push_back(eg.passedPawnBonus);
			kpkParams.push_back(eg.passedUnblockedBonus);
			kpkParams.push_back(eg.passedPawnControlBonus);
			kpkParams.push_back(eg.passedConnectedBonus);
			kpkParams.push_back(eg.passedOutsideBonus);
			kpkParams.push_back(eg.passedOutsideNearQueenBonus);
			kpkParams.push_back(eg.passedDefendedBonus);
			//kpkParams.push_back(eg.passedRookBonus);
			//kpkParams.push_back(eg.passedRookBehindBonus);
			//kpkParams.push_back(eg.passedERookPenalty);
			//kpkParams.push_back(eg.passedERookBehindPenalty);
			kpkParams.push_back(eg.passedOutsideBonusAH);
			kpkParams.push_back(eg.passedOutsideBonusBG);
			kpkParams.push_back(eg.passedEKingWithinSqPenalty);
			kpkParams.push_back(eg.passedEKingBlockerPenalty);
			for (const auto& p : eg.passedKingDistBonus)
				kpkParams.push_back(p);
			for (const auto& p : eg.passedEKingDistPenalty)
				kpkParams.push_back(p);
			for (const auto& p : eg.passedDistBonus)
				kpkParams.push_back(p);
			//for (const auto& p : eg.kingMobility)
			//	kpkParams.push_back(p);
			kpkParams.push_back(eg.centralizedKingBonus);
			kpkParams.push_back(eg.kingOppositionBonus);
			kpkParams.push_back(eg.chainBaseAttackBonus);
			kpkParams.push_back(eg.chainTipAttackBonus);
			kpkParams.push_back(eg.kpkGoodKingBonus);
			kpkParams.push_back(eg.kpkBadKingPenalty);
			kpkParams.push_back(eg.kpkConnectedPassed);
			kpkParams.push_back(eg.kpkKingBehindPenalty);
			kpkParams.push_back(eg.kpkEdgeColumnPenalty);
			for (const auto& p : eg.kpkPassedDistScale)
				kpkParams.push_back(p);

			return kpkParams;
		}

		template<typename T>
		void WriteToKpK_ParamInfo(const std::vector<T>& params, Evaluation::ParamInfo& p) {
			auto& eg = p.eg;
			int idx = 0;
			eg.sq_scaling[0] = p[idx++];
			//for (int j = 0; j < eg.pawnKingDist.size(); ++j)
			//	eg.pawnKingDist[j] = p[idx++];
			//for (int j = 0; j < eg.islandPenalty.size(); ++j)
			//	eg.islandPenalty[j] = p[idx++];
			//for (int j = 0; j < eg.doubledPenalty.size(); ++j)
			//	eg.doubledPenalty[j] = p[idx++];
			//for (int j = 0; j < eg.doubledOpen.size(); ++j)
			//	eg.doubledOpen[j] = p[idx++];
			//eg.doubledDefendedBonus = p[idx++];
			eg.isolatedPenalty = p[idx++];
			eg.isolatedKingDistPenalty = p[idx++];
			eg.isolatedControlPenalty = p[idx++];
			eg.backwardPenalty = p[idx++];
			eg.backwardNeighborPenalty = p[idx++];
			eg.backwardControlPenalty = p[idx++];
			eg.kMajorityBonus = p[idx++];
			eg.qMajorityBonus = p[idx++];
			eg.passedPawnBonus = p[idx++];
			eg.passedUnblockedBonus = p[idx++];
			eg.passedPawnControlBonus = p[idx++];
			eg.passedConnectedBonus = p[idx++];
			eg.passedDefendedBonus = p[idx++];
			//eg.passedRookBonus = p[idx++];
			//eg.passedRookBehindBonus = p[idx++];
			//eg.passedERookPenalty = p[idx++];
			//eg.passedERookBehindPenalty = p[idx++];
			eg.passedOutsideBonusAH = p[idx++];
			eg.passedOutsideBonusBG = p[idx++];
			eg.passedEKingWithinSqPenalty = p[idx++];
			eg.passedEKingBlockerPenalty = p[idx++];
			for (int j = 0; j < eg.passedKingDistBonus.size(); ++j)
				eg.passedKingDistBonus[j] = p[idx++];
			for (int j = 0; j < eg.passedEKingDistPenalty.size(); ++j)
				eg.passedEKingDistPenalty[j] = p[idx++];
			for (int j = 0; j < eg.passedDistBonus.size(); ++j)
				eg.passedDistBonus[j] = p[idx++];
			//for (int j = 0; j < eg.kingMobility.size(); ++j)
			//	eg.kingMobility[j] = p[idx++];
			eg.centralizedKingBonus = p[idx++];
			eg.kingOppositionBonus = p[idx++];
			eg.chainBaseAttackBonus = p[idx++];
			eg.chainTipAttackBonus = p[idx++];
			/*KpK specific*/
			eg.kpkGoodKingBonus = p[idx++];
			eg.kpkBadKingPenalty = p[idx++];
			eg.kpkConnectedPassed = p[idx++];
			eg.kpkKingBehindPenalty = p[idx++];
			eg.kpkEdgeColumnPenalty = p[idx++];
			for (int j = 0; j < eg.kpkPassedDistScale.size(); ++j)
				eg.kpkPassedDistScale[j] = p[idx++];
		}

		Eigen::VectorXd GuessFromTuned_KpK_Params() {

			// Note: these are ordered EXACTLY as laid out in eval.h for kpk/endgame.
			std::vector<double> kpkParams = { };

			return Eigen::Map<Eigen::VectorXd, Eigen::Unaligned>(kpkParams.data(), kpkParams.size());
		}

		Eigen::VectorXd GuessFromInitial_KpK_Params() {

			auto p = ReadFromKpK_ParamInfo();
			std::vector<double> paramD;
			for(auto& param : p)
				paramD.push_back(param);
			return Eigen::Map<Eigen::VectorXd, Eigen::Unaligned>(paramD.data(), paramD.size());

		}

		inline void SetBound(Eigen::MatrixXd& bounds, const int& col, int currVal, int delta = 20) {
			
			delta = std::abs(delta);
			delta = std::max(15, delta);
			bounds(0, col) = currVal - delta;
			bounds(1, col) = currVal + delta;
		}
		inline void SetBound(Eigen::MatrixXd& bounds, const int& col, int currVal, int lo, int hi) {
			bounds(0, col) = currVal - lo;
			bounds(1, col) = currVal + hi;
		}
		inline void SetLowBound(Eigen::MatrixXd& bounds, const int& col, int lo) {
			bounds(0, col) = lo;
		}
		inline void SetHighBound(Eigen::MatrixXd& bounds, const int& col, int hi) {
			bounds(1, col) = hi;
		}

		// Could consider a param struct that defines bounds for each parameter
		// in the eval.h class.
		Eigen::MatrixXd InitializeKpK_Bounds(Eigen::VectorXd& guess) {
			Eigen::MatrixXd bounds(2, guess.size());

			for (int i = 0; i < guess.size(); ++i) {
				SetBound(bounds, i, guess[i], 200); // do not assume anything about these parameter vals
			}

			// All params have 0 as a low bound
			for (int i = 0; i < guess.size(); ++i) {
				SetLowBound(bounds, i, 0);
			}

			// KpK method
			return bounds;
		}

	public:

		PSO() { }

		bool Train() {

			// Initialize all parameters
			auto guess = GuessFromInitial_KpK_Params();

			// PSO optimizer
			pso::ParticleSwarmOptimization<double, SearchObjective,
				pso::ExponentialDecrease3<double>> optimizer;

			// Set number of iterations as stop criterion.
			// Set it to 0 or negative for infinite iterations (default is 0).
			optimizer.setMaxIterations(_epochs);

			// Set the minimum change of the x-values (particles) (default is 1e-6).
			// If the change in the current iteration is lower than this value, then
			// the optimizer stops minimizing.
			optimizer.setMinParticleChange(1e-6);

			// Set the number of threads used for evaluation (OpenMP only).
			// Set it to 0 or negative for auto detection (default is 1).
			//optimizer.setThreads(8);

			optimizer.setMaxVelocity(4.0);
			optimizer.setPhiGlobal(4.0);
			optimizer.setPhiParticles(2.0);

			// Turn verbosity on, so the optimizer prints status updates after each
			// iteration.
			optimizer.setVerbosity(2);

			// Set the bounds in which the optimizer should search.
			// Each column vector defines the (min, max) for each dimension  of the
			// particles.
			Eigen::MatrixXd bounds = InitializeKpK_Bounds(guess);

			// start the optimization with a particle count
			std::cout << " ==== Tuning " << guess.size() << " parameters ====" << std::endl;
			auto result = optimizer.minimize(bounds, _populationSize, guess);

			std::cout << "Done! Converged: " << (result.converged ? "true" : "false")
				<< " Iterations: " << result.iterations << std::endl;

			// do something with final function value
			std::cout << "Final fval: " << result.fval << std::endl;

			// do something with final x-value
			auto paramResult = result.xval.transpose();
			std::cout << "Final xval: " << result.xval.transpose() << std::endl;

			std::cout << "\n === Formatted " << paramResult.size() << " final parameters === " << std::endl;
			for (int i = 0; i < paramResult.size(); ++i) {
				std::cout << paramResult[i] << ", "; 
			}

			return true;
		};
};

#endif