
#include "../eval.h"

namespace Evaluation {

	namespace {
		inline bool isWhiteFence() {
            //fence = wPawns & soutOne(bPawns);
            //if (popCount(fence) < 3)
            //    return false;
            //fence |= wPawns & soutOne(fence);
            //fence |= wPawns & soutOne(fence);
            //fence |= wPawns & soutOne(fence);
            //fence |= wPawns & soutOne(fence);
            //fence |= bPawnAttacks(bPawns);
            //flood = RANK1BB | allBitsBelow[BitScanForward(fence)];
            //U64 above = allBitsAbove[BitScanReverse(fence)];
            //while (true) {
            //    U64 temp = flood;
            //    flood |= eastOne(flood) | westOne(flood); /* Set all 8 neighbors */
            //    flood |= soutOne(flood) | nortOne(flood);
            //    flood &= ~fence;
            //    if (flood == temp)
            //        break; /* Fill has stopped, blockage? */
            //    if (flood & above) /* break through? */
            //        return false; /* yes, no blockage */
            //    if (flood & bPawns) {
            //        bPawns &= ~flood;  /* "capture" undefended black pawns */
            //        fence = wPawns & soutOne(bPawns);
            //        if (popCount(fence) < 3)
            //            return false;
            //        fence |= wPawns & soutOne(fence);
            //        fence |= wPawns & soutOne(fence);
            //        fence |= wPawns & soutOne(fence);
            //        fence |= wPawns & soutOne(fence);
            //        fence |= bPawnAttacks(bPawns);
            //    }
            //}
            return true;
		}
	}

	int Evaluation::eval_blockade_eg(const Color& c, const position& p) {

		int score = 0;

		return score;
	}

}
