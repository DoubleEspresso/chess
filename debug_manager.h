#pragma once
#ifndef HAVOC_BOARD_MANAGER_H
#define HAVOC_BOARD_MANAGER_H


#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

#include "singleton.h"


namespace haVoc {

	class Debugmanager final : public Singleton<Debugmanager> {
	private:
		bool m_inited;

	public:

		void init(const std::string& folder);

		template<typename... Args>
		void Write(std::string msg, Args... args) {
#if DEBUG
			char buffer[256];
			std::sprintf(buffer, msg.c_str(), std::forward<Args>(args)...);
			PLOGD << buffer;
#endif
		}

	public:
		Debugmanager(token);
		~Debugmanager();
	};

}

#endif

