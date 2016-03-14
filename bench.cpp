#include "bench.h"
#include "board.h"
#include "move.h"
#include "material.h"
#include "types.h"

void Benchmark::init(BenchType bt)
{
	if (bt == PAWN_POS)
	{
		pawn_position[0] = "4k3/8/8/8/8/8/PPPPPPPP/4K3 w - - 0 1";
		pawn_position[1] = "4k3/7p/1p4p1/pP3p2/P3p3/3P4/2P1PPPP/4K3 w - - 0 1";
		pawn_position[2] = "4k3/7p/1p4p1/pP3p2/P3p3/3P4/2P1PPPP/4K3 b - - 0 1";
		pawn_position[3] = "4k3/6p1/8/2pp3p/1p1Pp3/pP2P3/P2K1PPP/8 w - - 0 1";
		pawn_position[4] = "4k3/6p1/8/2pp3p/1p1Pp3/pP2P3/P2K1PPP/8 b - d3 0 1";
		pawn_position[5] = "4k3/6p1/8/2pp3P/1p1Pp3/pP6/P2Kpppp/8 b - - 0 1";
		pawn_position[6] = "4k3/6p1/8/2pp3P/1p1Pp3/pP6/P2Kpppp/4NNNN b - - 0 1";
		pawn_position[7] = "8/P1PkPPPP/1PP5/4P3/3P1P2/3K2P1/7P/8 w - - 0 1";
		pawn_position[8] = "8/4pk2/1p1p1p2/P1p1P1pp/1P1P1PPp/P3K3/2P4P/8 w - - 0 1";
		pawn_position[9] = "8/4pk2/1p1p1p2/P1p1P1pp/1P1P1PPp/P3K3/2P4P/8 b - - 0 1";
		bench_pawns();
	}
	else if (bt == MINORS)
	{
		minor_position[0] = "rn1q1rk1/1p2bppp/p2pbn2/4p3/4P3/1NN1BP2/PPPQ2PP/2KR1B1R b - - 4 10";
		minor_position[1] = "rn1q1rk1/1p2bppp/p2pbn2/4p3/4P3/1NN1BP2/PPPQ2PP/2KR1B1R w - - 4 10";
		minor_position[2] = "r2q1rk1/pp1bbppp/8/8/4P3/1B2BP2/PPPQ2PP/2KR3R b - - 0 1";
		minor_position[3] = "r2q1rk1/pp1bbppp/8/8/4P3/1B2BP2/PPPQ2PP/2KR3R w - - 0 1";
		minor_position[4] = "2rq1rk1/pp1bb1pp/8/R7/7R/1B2B3/PPPQ2P1/2K5 w - - 0 1";
		minor_position[5] = "2rq1rk1/pp1bb1pp/8/R7/7R/1B2B3/PPPQ2P1/2K5 b - - 0 1";
		minor_position[6] = "6k1/ppP3pp/8/2BbbB2/8/8/P3p3/2K5 w - - 0 1";
		minor_position[7] = "6k1/ppP3pp/8/2BbbB2/8/8/P3p3/2K5 b - - 0 1";
		minor_position[8] = "6k1/pQP3pp/R7/2BbbB2/7r/8/P3pq2/2K5 w - - 0 1";
		minor_position[9] = "6k1/pQP3pp/R7/2BbbB2/7r/8/P3pq2/2K5 b - - 0 1";
		bench_minors();
	}
	else if (bt == BENCH)
	{
		bench(max_iter);
	}
}

void Benchmark::bench(int d)
{
	Clock timer;
	std::string startPositions[5] =
	{
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
		"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
		"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
		"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6"
	};

	long int results[5][7] = {
		{ 20,  400,  8902,  197281,   4865609, 119060324, 3195901860 },
		{ 48, 2039, 97862, 4085603, 193690690,         0,          0 },
		{ 14,  191,  2812,   43238,    674624,  11030083,  178633661 },
		{ 6,   264,  9467,  422333,  15833292, 706045033,          0 },
		{ 42, 1352, 53392,       0,         0,         0,          0 } };

	int cnt = 0;

	for (int i = 0; i<5; ++i)
	{
		std::istringstream fen(startPositions[i]);
		Board board(fen);

		std::cout << "Position : " << startPositions[i] << std::endl;
		std::cout << "" << std::endl;
		for (int depth = 0; depth < d; depth++)
		{
			timer.start();
			cnt = perft(board, depth + 1);
			timer.stop();
			std::cout << "depth " << (depth + 1) << "\t" << std::right << std::setw(14) << results[i][depth]
				<< "\t" << "perft " << std::setw(14) << cnt << "\t " << std::setw(15) << timer.ms() << " ms " << std::endl;
		}
		std::cout << "" << std::endl;
		std::cout << "" << std::endl;
	}

}

long int Benchmark::perft(Board& b, int d)
{
	if (d == 1)
	{
		MoveGenerator mvs(b);
		return mvs.size();
	}
	BoardData pd;
	int cnt = 0;
	for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
	{
		b.do_move(pd, mvs.move());
		cnt += perft(b, d - 1);
		b.undo_move(mvs.move());
	}
	return cnt;
}

void Benchmark::divide(Board& b, int d)
{
	BoardData pd;
	int total = 0;
	for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
	{
		b.do_move(pd, mvs.move());
		int n = d>1 ? perft(b, d - 1) : 1;
		total += n;
		b.undo_move(mvs.move());
		std::cout << SanSquares[get_from(mvs.move())] << SanSquares[get_to(mvs.move())] << '\t' << n << std::endl;
	}
	std::cout << "---------------------------------" << std::endl;
	std::cout << "total " << '\t' << total << std::endl;
}

void Benchmark::bench_pawns()
{
	Clock c;
	double times[10];
	int counts[10];
	for (int i = 0; i<10; ++i)
	{
		printf("..bench pawns position %d/10\n", i + 1);
		Board b;
		std::istringstream fen_ss(pawn_position[i]);
		b.from_fen(fen_ss);
		int count = 0;
		c.start();
		for (int j = 0; j<max_iter; ++j)
		{
			for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
				count++;
		}
		c.stop();
		times[i] = c.ms();
		counts[i] = count;
	}

	for (int j = 0; j<10; ++j)
		printf("pos(%d)\t%7.0f(ms)\t mvs(%d)\n", j, times[j], counts[j]);
}

void Benchmark::bench_minors()
{
	Clock c;
	double times[10];
	int counts[10];
	for (int i = 0; i<10; ++i)
	{
		printf("..bench minor position %d/10\n", i + 1);
		Board b;
		std::istringstream fen_ss(minor_position[i]);
		b.from_fen(fen_ss);
		int count = 0;
		c.start();
		for (int j = 0; j<max_iter; ++j)
		{
			for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
				count++;
		}
		c.stop();
		times[i] = c.ms();
		counts[i] = count;
	}
	for (int j = 0; j<10; ++j)
		printf("pos(%d)\t%7.0f(ms)\t mvs(%d)\n", j, times[j], counts[j]);
}
