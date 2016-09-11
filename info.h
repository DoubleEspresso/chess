#pragma once

#ifndef HEDWIG_INFO_H
#define HEDWIG_INFO_H

#include <stdio.h>
#include <time.h>
#include <new>
#include <string>

#include "definitions.h"

namespace Info
{
	namespace BuildInfo
	{
		void greeting();
	}
	namespace Date
	{
		bool today(char tbuff[], size_t sz);
	}
	namespace GpuSupport
	{

	}
}

void Info::BuildInfo::greeting()
{
	printf("\n+------------------------------------------------\n");
	printf("  \t%s chess - %s\n", ENGINE_NAME, VERSION);

#ifdef _WIN32
	char tbuffer[100];
	Info::Date::today(tbuffer, sizeof(tbuffer));
	printf("  \tCompiled - %s\n", tbuffer);
#else
	printf("  \tCompiled - %s\n", BUILD_DATE);
#endif

	if (sizeof(size_t) == 4)
	{
#ifdef DEBUG
		printf("  \tMode     - %s 32-bit (debug)\n", OS);
#else
		printf("  \tMode     - %s 32-bit (release)\n", OS);
#endif
	}
	else
	{

#ifdef DEBUG
		printf("  \tMode     - %s 64-bit (debug)\n", OS);
#else
		printf("  \tMode     - %s 64-bit (release)\n", OS);
#endif
	}
	printf("  \tAuthor - %s\n", AUTHOR);
	printf("+------------------------------------------------\n");

}

bool Info::Date::today(char tbuff[], size_t sz)
{
	try
	{
		time_t now = time(0);
		struct tm * tinfo = new tm();

#ifdef _WIN32		
		localtime_s(tinfo, &now);
#else
		tinfo = localtime(&now);
#endif
		strftime(tbuff, sz, "%m/%d/%Y %X", tinfo);
	}
	catch (const std::exception& e)
	{
		printf("..[Info] Date::today() exception %s\n", e.what());
		return false;
	}
	return true;
}


#endif
