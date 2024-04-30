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
		_pInfo.mg.material_vals = { 100, parameters[0], parameters[1], parameters[2], parameters[3] };

		//pi.eg.tempo = parameters[6];
		// Pawn, knight, bishop, rook, queen material (eg)
		_pInfo.eg.material_vals = { 115, parameters[4], parameters[5], parameters[6], parameters[7] };
	}
};