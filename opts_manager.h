#pragma once
#ifndef HAVOC_OPTS_MANAGER_H
#define HAVOC_OPTS_MANAGER_H

#include <memory>
#include <map>
#include <iostream>
#include <thread>

#include "singleton.h"


namespace haVoc {

	class Optionsmanager final : public Singleton<Optionsmanager>
	{
	private:

	public:

	public:
		Optionsmanager(token);
		~Optionsmanager();
	};

}

#endif
