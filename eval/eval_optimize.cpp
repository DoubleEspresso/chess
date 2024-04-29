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

		ParamInfo pi;
		// Middle game parameters (in order)
		pi.mg.tempo = parameters[0];
		// Pawn, knight, bishop, rook, queen material (mg)
		pi.mg.material_vals = { parameters[1], parameters[2], parameters[3], parameters[4], parameters[5] };

		pi.eg.tempo = parameters[6];
		// Pawn, knight, bishop, rook, queen material (eg)
		pi.eg.material_vals = { parameters[7], parameters[8], parameters[9], parameters[10], parameters[11] };
	}
};