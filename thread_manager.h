#pragma once
#ifndef HAVOC_THREAD_MANAGER_H
#define HAVOC_THREAD_MANAGER_H

#include <memory>
#include <map>
#include <iostream>
#include <thread>

#include "singleton.h"


namespace haVoc {

	class Threadmanager final : public Singleton<Threadmanager>
	{
	private:

	public:

	public:
		Threadmanager(token);
		~Threadmanager();
	};

}

#endif
