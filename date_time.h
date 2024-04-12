#pragma once
#ifndef HAVOC_DATE_TIME_H
#define HAVOC_DATE_TIME_H

#include <string>
#include <time.h>
#include <iostream>

namespace haVoc {

	static class Datetime final
	{
	private:
		static std::string formatted_time(std::string format)
		{
			time_t t;
			time(&t);
			auto lt = localtime(&t);
			char buffer[256];
			strftime(buffer, sizeof(buffer), format.c_str(), lt);
			return std::string(buffer);
		}


	public:
		static std::string local_time_mmddyyy_hhmmss()
		{
			return formatted_time("%m%d%y_%H%M%S");
		}

		static std::string local_time_mmddyyy()
		{
			return formatted_time("%m%d%y");
		}
	};

}

#endif

