#include "game_manager.h"
#include "build.h"
#include "debug_manager.h";

namespace haVoc {

	Gamemanager::Gamemanager(token)
	{
		init();
	}

	Gamemanager::~Gamemanager()
	{

	}

	/// <summary>
	/// Initializes all managers and loads engine
	/// settings for either debug or release builds
	/// </summary>
	void Gamemanager::init()
	{
		auto fn = Build::is_debug() ? "engine_debug_config.xml" : "engine_config.xml";
		m_xml = std::make_shared<XML>(fn);

		// 1. initialize debug logging (only for debug builds)
		m_valid = init_debugging();

		// 2. 
	}

	/// <summary>
	/// Initializes the logging system for this build
	/// </summary>
	/// <returns></returns>
	bool Gamemanager::init_debugging()
	{
		// no logging in release builds
		if (!Build::is_debug())
			return true;

		// set the folder we write to for logging
		std::string folder = "";
		if (!m_xml->try_get("logging", "folder", folder))
			return false;

		// is logging enabled for this build?
		bool enabled = false;
		if (!m_xml->try_get("logging", "enabled", enabled))
			return false;

		if (!enabled)
			return true;

		// debug build with logging enabled - create our logging instance
		auto& dm = Debugmanager::instance();
		dm.init(folder);

		return true;
	}
}