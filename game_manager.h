#pragma once
#ifndef HAVOC_GAME_MANAGER_H
#define HAVOC_GAME_MANAGER_H

#include <memory>
#include <map>
#include <iostream>
#include <thread>

#include "singleton.h"
#include "xml_parser.h"


namespace haVoc {

	class Gamemanager final : public Singleton<Gamemanager>
	{

	private:
		std::shared_ptr<XML> m_xml;
		bool m_valid = false;

	private:
		void init();
		bool init_debugging();

	public:

	public:
		Gamemanager(token);
		~Gamemanager();
	};

}

#endif
