#pragma once


namespace haVoc {

	/// <summary>
	/// Wrapper class around preprocessor definitions for build types
	/// </summary>
	static class Build final {

	public:

		static bool is_debug()
		{
#ifdef DEBUG
			return true;
#endif
			return false;
		}

		static bool is_64bit()
		{
#ifdef _64BIT
			return true;
#endif
			return false;
		}
	};
}