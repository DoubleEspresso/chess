#pragma once
#ifndef HAVOC_BOARD_MANAGER_H
#define HAVOC_BOARD_MANAGER_H

#include <memory>
#include <map>
#include <iostream>
#include <thread>

#include "singleton.h"


namespace haVoc {

	class Boardmanager final : public Singleton<Boardmanager>
	{
	private:

	public:

	public:
		Boardmanager(token);
		~Boardmanager();
	};

}

#endif

