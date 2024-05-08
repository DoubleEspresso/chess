#include "eval.h"

namespace Evaluation {

	namespace {
		/*Specialized preprocessing methods*/
		/*Specialized postprocessing methods*/
		/*Specialized initialize/utility methods*/
	}


	// Update this initialize method to adjust which parameters are tuned.
	void Evaluation::Initialize(const std::vector<int>& parameters) {

		// Note: it is assumed the layout of parameter vector is *in order* according to 
		// the layout of the eval data struct.

		// Middle game parameters (in order)
		//pi.mg.tempo = parameters[0];
		// Pawn, knight, bishop, rook, queen material (mg)
		_pIfo.mg.material_vals = { 100, parameters[0], parameters[1], parameters[2], parameters[3] };

		//pi.eg.tempo = parameters[6];
		// Pawn, knight, bishop, rook, queen material (eg)
		_pIfo.eg.material_vals = { 115, parameters[4], parameters[5], parameters[6], parameters[7] };
	}

	void Evaluation::Initialize_KpK(const std::vector<int>& p) {

			int idx = 0;
			_pIfo.eg.sq_scaling[0] = p[idx++];
			//for (int j = 0; j < _pIfo.eg.pawnKingDist.size(); ++j)
			//	_pIfo.eg.pawnKingDist[j] = p[idx++];
			for (int j = 0; j < _pIfo.eg.islandPenalty.size(); ++j)
				_pIfo.eg.islandPenalty[j] = p[idx++];
			for (int j = 0; j < _pIfo.eg.doubledPenalty.size(); ++j)
				_pIfo.eg.doubledPenalty[j] = p[idx++];
			//for (int j = 0; j < _pIfo.eg.doubledOpen.size(); ++j)
			//	_pIfo.eg.doubledOpen[j] = p[idx++];
			_pIfo.eg.doubledDefendedBonus = p[idx++];
			_pIfo.eg.isolatedPenalty = p[idx++];
			_pIfo.eg.isolatedKingDistPenalty = p[idx++];
			_pIfo.eg.isolatedControlPenalty = p[idx++];
			_pIfo.eg.backwardPenalty = p[idx++];
			_pIfo.eg.backwardNeighborPenalty = p[idx++];
			_pIfo.eg.backwardControlPenalty = p[idx++];
			_pIfo.eg.kMajorityBonus = p[idx++];
			_pIfo.eg.qMajorityBonus = p[idx++];
			_pIfo.eg.passedPawnBonus = p[idx++];
			_pIfo.eg.passedUnblockedBonus = p[idx++];
			_pIfo.eg.passedPawnControlBonus = p[idx++];
			_pIfo.eg.passedConnectedBonus = p[idx++];
			_pIfo.eg.passedOutsideBonus = p[idx++];
			_pIfo.eg.passedOutsideNearQueenBonus = p[idx++];
			_pIfo.eg.passedDefendedBonus = p[idx++];
			//_pIfo.eg.passedRookBonus = p[idx++];
			//_pIfo.eg.passedRookBehindBonus = p[idx++];
			//_pIfo.eg.passedERookPenalty = p[idx++];
			//_pIfo.eg.passedERookBehindPenalty = p[idx++];
			_pIfo.eg.passedOutsideBonusAH = p[idx++];
			_pIfo.eg.passedOutsideBonusBG = p[idx++];
			_pIfo.eg.passedEKingWithinSqPenalty = p[idx++];
			_pIfo.eg.passedEKingBlockerPenalty = p[idx++];
			for (int j = 0; j < _pIfo.eg.passedKingDistBonus.size(); ++j)
				_pIfo.eg.passedKingDistBonus[j] = p[idx++];
			for (int j = 0; j < _pIfo.eg.passedEKingDistPenalty.size(); ++j)
				_pIfo.eg.passedEKingDistPenalty[j] = p[idx++];
			for (int j = 0; j < _pIfo.eg.passedDistBonus.size(); ++j)
				_pIfo.eg.passedDistBonus[j] = p[idx++];
			//for (int j = 0; j < _pIfo.eg.kingMobility.size(); ++j)
			//	_pIfo.eg.kingMobility[j] = p[idx++];
			_pIfo.eg.centralizedKingBonus = p[idx++];
			_pIfo.eg.kingOppositionBonus = p[idx++];
			_pIfo.eg.chainBaseAttackBonus = p[idx++];
			_pIfo.eg.chainTipAttackBonus = p[idx++];
			_pIfo.eg.kpkGoodKingBonus = p[idx++];
			_pIfo.eg.kpkBadKingPenalty = p[idx++];
			_pIfo.eg.kpkConnectedPassed = p[idx++];
			_pIfo.eg.kpkKingBehindPenalty = p[idx++];
			_pIfo.eg.kpkEdgeColumnPenalty = p[idx++];
			for (int j = 0; j < _pIfo.eg.kpkPassedDistScale.size(); ++j)
				_pIfo.eg.kpkPassedDistScale[j] = p[idx++];
	}
};