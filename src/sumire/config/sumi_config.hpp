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

		SumiConfigData configData{};

	private:
		static constexpr char* CONFIG_PATH = SUMIRE_ENGINE_PATH("config/config.json");

		void createDefaultConfig();
		bool checkConfigFileExists() const;
		std::string readConfigFile() const;
		void writeConfigFile(const char* json) const;
		
	};

}