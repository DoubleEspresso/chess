#pragma once
#ifndef HAVOC_CLOCK_MANAGER_H
#define HAVOC_CLOCK_MANAGER_H

#include <memory>
#include <map>
#include <iostream>
#include <thread>

#include "singleton.h"


namespace haVoc {

	class Clockmanager final : public Singleton<Clockmanager>
	{
	private:

	public:

	public:
		Clockmanager(token);
		~Clockmanager();
	};

}

#endif

