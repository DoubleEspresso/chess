
#include "debug_manager.h"
#include "date_time.h"
#include "version.h"
#include "build.h"
#include "platform_info.h"

namespace haVoc {

	Debugmanager::Debugmanager(token)
	{
		m_inited = false;
	}

	Debugmanager::~Debugmanager()
	{

	}

	void Debugmanager::init(const std::string& folder)
	{
		if (m_inited)
			return;

		char fname[256];
		sprintf(fname, "%s\\haVoc_debug_%s.txt", folder.c_str(), Datetime::local_time_mmddyyy_hhmmss().c_str());
		std::cout << fname << std::endl;
		plog::init(plog::Severity::debug, fname);
		m_inited = true;



		LOG(plog::Severity::info) << "================================================================";
		LOG(plog::Severity::info) << "GIT BRANCH:" << GIT_BRANCH;
		LOG(plog::Severity::info) << "GIT COMMIT:" << GIT_HASH;
		LOG(plog::Severity::info) << "VERSION:" << GIT_VERSION;
		LOG(plog::Severity::info) << "BUILD:" << (Build::is_64bit() ? "64" : "32") << "bit";
		LOG(plog::Severity::info) << Platforminfo::platform_desc();
		LOG(plog::Severity::info) << COPYRIGHT;
		LOG(plog::Severity::info) << "================================================================";
	}
}