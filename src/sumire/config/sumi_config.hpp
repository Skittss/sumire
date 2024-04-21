#pragma once

#include <sumire/config/sumi_config_data.hpp>
#include <sumire/util/sumire_engine_path.hpp>

#include <string>

namespace sumire {

	class SumiConfig {
	public:
		SumiConfig();
		~SumiConfig();

		void readConfig();
		void writeConfig() const;

		// TODO: It would be a good idea to make this struct const so it does not 
		//        change at runtime for important constants.
		//       Instead write config changes to a separate object and write that one.
		SumiConfigData configData{};

	private:
		static constexpr char* CONFIG_PATH = SUMIRE_ENGINE_PATH("config/config.json");

		void createDefaultConfig();
		bool checkConfigFileExists() const;
		std::string readConfigFile() const;
		void writeConfigFile(const char* json) const;
		
	};

}