#include <cstring>

#include "threads.h"
#include "board.h"
#include "search.h"
#include "uci.h"
#include "material.h"
#include "opts.h"
#include "hashtable.h"
#include "pawns.h"

ThreadPool Threads;
U16 BestMoves[2];
U16 PonderMoves[2];
ThreadTimer * timer_thread;

namespace
{
  extern "C"
  {
    long start_routine(ThreadBase *th) { th->idle_loop(); return 0; }
  }
}

template<typename T> T* new_thread() {
  T* th = new T();
  thread_create(th->handle, start_routine, th);
  return th;
}

void ThreadBase::notify()
{
  mutex.lock();
  sleep_condition.signal();
  mutex.unlock();
}

void ThreadBase::wait_for(volatile const bool& condition)
{
  mutex.lock();
  while (!condition) sleep_condition.wait(mutex);
  mutex.unlock();
}

Thread::Thread()
{
  searching = false;
  nb_splits = 0;
  currSplitBlock = NULL;
  currBoard = NULL;
  idx = Threads.size(); // Starts from 0
}

bool Thread::found_cutoff() const
{
  for (SplitBlock* sb = currSplitBlock; sb; sb = sb->parentSplitBlock)
    if (sb->cut_occurred) return true;

  return false;
}

bool Thread::available_to(const Thread* master) const
{
  if (searching) return false;
  const int sz = nb_splits;
  return !sz || SplitBlocks[sz - 1].slaves_mask.test(master->idx);
}

bool Thread::split(Board& b, U16* bestmove, Node* node, int alpha, int beta, int* besteval, int depth, MoveSelect* ms, bool cutNode)
{
  //if (nb_splits > SPLITS_PER_THREAD) 
  //{
  //	printf(" .. requested split with splitCount(%d), ignored\n", nb_splits);
  //	return false;
  //}
  SplitBlock& sb = SplitBlocks[nb_splits];
  sb.masterThread = this;
  sb.parentSplitBlock = currSplitBlock;
  sb.slaves_mask = 0, sb.slaves_mask.set(idx);
  sb.depth = depth;
  sb.besteval = *besteval;
  sb.bestmove = *bestmove;
  sb.alpha = alpha;
  sb.beta = beta;
  sb.cutNode = cutNode;
  sb.ms = ms;
  sb.b = &b;
  sb.cut_occurred = false;
  sb.nodes_searched = 0;
  sb.node = node;

  Threads.mutex.lock();
  sb.split_mutex.lock();

  sb.allSlavesSearching = true; // Must be set under lock protection
  ++nb_splits;
  currSplitBlock = &sb;
  currBoard = NULL;
  //std::cout << " dbg bestmove before search: " << SanSquares[get_from(*bestmove)] << SanSquares[get_to(*bestmove)] << " eval: " << *besteval << std::endl;
  Thread* slave;
  while ((slave = Threads.available_slave(this)) != NULL)
    {
      sb.slaves_mask.set(slave->idx);
      slave->currSplitBlock = &sb;
      slave->searching = true;
      slave->notify();
    }

  sb.split_mutex.unlock();
  Threads.mutex.unlock();

  // master sent to worker::idle_loop to start a search
  Thread::idle_loop();

  Threads.mutex.lock();
  sb.split_mutex.lock();

  searching = false; // master returns from search, is no longer searching (?)
  --nb_splits;
  currSplitBlock = sb.parentSplitBlock;
  currBoard = &b;
  *bestmove = sb.bestmove;
  *besteval = sb.besteval;
  //std::cout << " dbg bestmove after search: " << SanSquares[get_from(*bestmove)] << SanSquares[get_to(*bestmove)] << " eval: " << *besteval << std::endl;
  b.inc_nodes_searched(sb.nodes_searched);

  sb.split_mutex.unlock();
  Threads.mutex.unlock();
  return true;
}

void ThreadMaster::idle_loop()
{
  //printf("..MAIN thread-%lu enters idle loop\n", this->idx);
  while (true)
    {
      mutex.lock();
      thinking = false;

      while (!thinking)
	{
	  Threads.sleep_condition.signal();
	  sleep_condition.wait(mutex);
	}

      mutex.unlock();
      searching = true;
      Board board(ROOT_BOARD, this);
      Search::run(board, ROOT_DEPTH);
      //UCI::PrintBest(ROOT_BOARD);
      //std::cout << "bestmove " << SanSquares[get_from(BestMoves[0])] + SanSquares[get_to(BestMoves[0])] << " ponder " << SanSquares[get_from(BestMoves[1])] + SanSquares[get_to(BestMoves[1])] << std::endl;
      std::cout << "bestmove " << UCI::move_to_string(BestMoves[0]) << " ponder " << UCI::move_to_string(BestMoves[1]) << std::endl;
      if (options["pondering"])
	{
	  U16 bm = BestMoves[0];
	  U16 pm = BestMoves[1];
	  if (bm != MOVE_NONE && pm != MOVE_NONE)
	    {
	      UCI_SIGNALS.stop = false;
	      hashTable.clear();
	      material.clear();
	      pawnTable.clear();
	      BoardData pd;
	      board.do_move(pd, bm);
	      board.do_move(pd, pm);
	      Search::run(board, ROOT_DEPTH, true);
	    }
	}
      searching = false;
      //timerthread->idle_loop();
      //printf("..master ends search...returning to slarp\n");
      //UCI::PrintBest(ROOT_BOARD);
    }
}

ThreadTimer::ThreadTimer()
{
  searching = adding_time = false;
}

void ThreadTimer::init()
{
  timer_thread = new_thread<ThreadTimer>();
}

void ThreadTimer::idle_loop()
{
  //printf("..[Threads] timer thread enters idle state\n");
  while (true)
    {
      tmutex.lock();
      searching = false;

      while (!searching)
	{
	  Threads.sleep_condition.signal();
	  sleep_condition.wait(tmutex);
	}
      tmutex.unlock();
      searching = true;
      if (searching) check_time();
    }
}

void ThreadTimer::check_time()
{
#ifdef __linux
  lClock timer;
  struct timespec delay;
  delay.tv_sec = 0.050; // ms
  delay.tv_nsec = 50 * 1000000;
#elif OSX
  lClock timer;
  struct timespec delay;
  delay.tv_sec = 0.050; // ms
  delay.tv_nsec = 50 * 1000000;
#else
  int delay = 100;//50; // ms
  Clock timer;
#endif
  timer.start();
  //bool sudden_death = search_limits->movestogo > 0;
  bool fixed_time = search_limits->movetime > 0;

  // time alloc based on limits..
  double time_limit = estimate_max_time();
  if (fixed_time)
    {
      do
	{
	  elapsed += timer.elapsed_ms();
	  sleep_condition.wait_for(mutex, delay);
	} while (!UCI_SIGNALS.stop && searching && elapsed <= search_limits->movetime);
    }
  else if (time_limit > -1) // dynamic time estimate during a real game
    {
      //while (true)
      //{
      do
	{
	  // track timing stats for search, we do this for limits.infinite/limits.ponder too
	  elapsed += timer.elapsed_ms();
	  sleep_condition.wait_for(mutex, delay);
	} while (!UCI_SIGNALS.stop && searching && elapsed <= time_limit);
      // we are either not searching, or elapsed > time_limit, and we should check if more time is needed
      //if (elapsed >= time_limit)
      //{
      //	//moretime_mutex.lock();
      //	UCI_SIGNALS.timesUp = true; // signal the searching thread to check if more time is needed...

      //	adding_time = true;
      //	while (adding_time) moretime_condition.wait(moretime_mutex); // wait here for a response from the searching thread
      //	adding_time = false;

      //	if (!UCI_SIGNALS.timesUp) // search thread indicated we need more time, allocate a little more and continue searching
      //	{
      //		double old_limit = time_limit;
      //		time_limit += estimate_max_time() / 1.5;
      //		UCI_SIGNALS.timesUp = false; // reset
      //		//moretime_mutex.unlock();
      //	}
      //	else
      //	{
      //		UCI_SIGNALS.timesUp = false; // reset
      //		//moretime_mutex.unlock();
      //		break; // break from the main while-loop -- which will flag uci-signals stop to the main searching thread, and quit the search.
      //	}
      //}
      //}
    }
  else
    {
      do
	{
	  // track timing stats for search, we do this for limits.infinite/limits.ponder too
	  elapsed += timer.elapsed_ms();
	  //thread_sleep(delay);
	  sleep_condition.wait_for(mutex, delay);
	} while (!UCI_SIGNALS.stop && searching);
    }
  // signal searching threads to stop
  UCI_SIGNALS.stop = true;
  searching = false;
}

double ThreadTimer::estimate_max_time()
{
  // catch exception here when "search" is called from UCI.cmd
  double time_per_move_ms = 0;
  if (search_limits->infinite || search_limits->ponder) return -1;
  else
    {
      bool sudden_death = search_limits->movestogo == 0; // no moves until next time control
      bool exact_time = search_limits->movetime != 0; // searching for an exact number of ms?
      //bool exact_depth = search_limits->depth != 0;
      double remainder_ms = (ROOT_BOARD.whos_move() == WHITE ? search_limits->wtime + search_limits->winc : search_limits->btime + search_limits->binc);
      MaterialEntry * me = material.get(ROOT_BOARD);
      GamePhase gp = me->game_phase;
      double moves_to_go = 45.0 - (gp == MIDDLE_GAME ? 22.5 : 30.0);
      //int moves_made = search_limits->movetime;
      if (sudden_death && !exact_time)
	{
	  // simple start to time estimate...
	  // idea avg. moves per game ~ 45
	  // time per move ~ time / mpg * weight
	  // weight based on game phase, less time in opening/ending, more time in middle game
	  // give about 1/6 time to opening/ending and 2/3 time to middle game.
	  time_per_move_ms = 1.5 * remainder_ms / moves_to_go * (gp == MIDDLE_GAME ? 0.6 : 0.17);
	  //dbg
	  //std::cout << " time-per-move from sudden_death && !exact_time " << time_per_move_ms << " ms " << std::endl;
	}
      else if (exact_time) return (double)search_limits->movetime;
      else if (!sudden_death)
	{
	  time_per_move_ms = remainder_ms / search_limits->movestogo * (gp == MIDDLE_GAME ? 0.6 : 0.17);
	  //std::cout << " time-per-move from !sudden_death " << time_per_move_ms << " ms " << std::endl;
	}
    }
  return 1.5 * time_per_move_ms;
}

void ThreadPool::init()
{
  push_back(new_thread<ThreadMaster>());
  int nb_worker_threads = 1;// opts["WorkerThreads"];
  if (nb_worker_threads > 0)
    {
      if (nb_worker_threads > MAX_THREADS)
	{
	  printf("..[Threads] Too many threads set, using max value %d instead\n", MAX_THREADS);
	  nb_worker_threads = MAX_THREADS;
	}
      //printf("..[Threads] initializing %d slave threads\n", nb_worker_threads);

      for (int i = 1; i < nb_worker_threads; ++i)
	{
	  Threads.push_back(new_thread<Thread>());
	}
    }
  else
    {
      printf("..[Threads] threads option not set in settings\n");
      return;
    }
}

void ThreadPool::exit()
{
  for (const_iterator it = begin(); it != end(); ++it)
    {
      if (*it) (*it)->searching = false;
      delete (*it);
    }
}

Thread* ThreadPool::available_slave(const Thread * master) const
{
  for (const_iterator it = begin(); it != end(); ++it)
    if ((*it)->available_to(master))
      return *it;
  return NULL;
}

void ThreadPool::wait_for_search_finished()
{
  ThreadMaster* th = main();
  th->mutex.lock();
  while (th->thinking) sleep_condition.wait(th->mutex);
  th->mutex.unlock();
}

void ThreadPool::start_thinking(const Board &b)
{
  wait_for_search_finished();
  main()->thinking = true;
  main()->notify();
}

void Thread::idle_loop()
{
  //if (this->idx == 0) printf("..[Threads] MASTER thread-%d enters idle state\n", this->idx);
  SplitBlock * this_sb = nb_splits ? currSplitBlock : NULL;
  //assert(!this_sp || (this_sp->masterThread == this && searching));

  while (true)
    {
      while (searching)
	{
	  Threads.mutex.lock();
	  //assert(currSplitBlock);
	  SplitBlock * sb = currSplitBlock;
	  Threads.mutex.unlock();

	  //if (this->idx == 0) printf("..[Threads] MASTER thread-%d begins search\n", this->idx);
	  Board board(*sb->b, this);

	  Node node;
	  std::memcpy(&node, sb->node, sizeof(Node));
	  //node.alpha = sb->alpha;
	  //node.ply = 1; // ???
	  node.type = SPLIT;
	  //node.ms = sb->ms;
	  node.sb = sb;

	  sb->split_mutex.lock();

	  //assert(currBord == NULL);
	  currBoard = &board;

	  Search::from_thread(board, sb->alpha, sb->beta, sb->depth, node);

	  //assert(searching);

	  searching = false;
	  currBoard = NULL;
	  currSplitBlock->slaves_mask.reset(idx);
	  currSplitBlock->allSlavesSearching = false;
	  //sb->nodes_searched += board.get_nodes_searched();

	  if (this != sb->masterThread
	      &&  sb->slaves_mask.none())
	    {
	      //assert(!sb->masterThread->is_searching);
	      //printf("..[Threads] worker thread-%d notifies master, all threads are finished with search!\n", this->idx);
	      sb->masterThread->notify();
	    }
	  sb->split_mutex.unlock();

	  if (Threads.size() > 2)
	    {
	      for (size_t i = 0; i < Threads.size(); ++i)
		{
		  const int sz = Threads[i]->nb_splits;
		  sb = sz ? &Threads[i]->SplitBlocks[sz - 1] : NULL;
		  if (sb && sb->allSlavesSearching && available_to(Threads[i]))
		    {
		      Threads.mutex.lock();
		      sb->split_mutex.lock();
		      if (sb->allSlavesSearching && available_to(Threads[i]))
			{
			  sb->slaves_mask.set(idx);
			  currSplitBlock = sb;
			  searching = true;
			}
		      sb->split_mutex.unlock();
		      Threads.mutex.unlock();
		      break;
		    }

		}
	    }
	}
      mutex.lock();
      if (this_sb && this_sb->slaves_mask.none())
	{
	  //assert(!searching);
	  mutex.unlock();
	  break;
	}


      if (!this->searching) sleep_condition.wait(mutex);
      mutex.unlock();
    }
}

