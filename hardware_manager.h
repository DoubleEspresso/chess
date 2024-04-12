#pragma once
#ifndef HAVOC_HARDWARE_MANAGER_H
#define HAVOC_HARDWARE_MANAGER_H

#include <memory>
#include <map>
#include <iostream>
#include <thread>

#include "singleton.h"


namespace haVoc {

	class Hardwaremanager final : public Singleton<Hardwaremanager>
	{
	private:

	public:

	public:
		Hardwaremanager(token);
		~Hardwaremanager();
	};

}

#endif

