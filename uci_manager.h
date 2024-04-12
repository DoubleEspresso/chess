#pragma once
#ifndef HAVOC_UCI_MANAGER_H
#define HAVOC_UCI_MANAGER_H

#include <memory>
#include <map>
#include <iostream>
#include <thread>

#include "singleton.h"


namespace haVoc {

	class UCImanager final : public Singleton<UCImanager>
	{
	private:

	public:

	public:
		UCImanager(token);
		~UCImanager();
	};

}

#endif
