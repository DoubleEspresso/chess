#pragma once
#ifndef HAVOC_PARAMS_MANAGER_H
#define HAVOC_PARAMS_MANAGER_H

#include <memory>
#include <map>
#include <iostream>
#include <thread>

#include "singleton.h"


namespace haVoc {

	class Paramsmanager final : public Singleton<Paramsmanager>
	{
	private:

	public:

	public:
		Paramsmanager(token);
		~Paramsmanager();
	};

}

#endif
