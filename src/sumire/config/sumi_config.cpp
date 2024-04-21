#include <sumire/config/sumi_config.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>

#define FIELD_PARSE_WARNING_STR(field_name)										\
		"[Sumire::SumiConfig] WARNING: Failed to parse value of config field "	\
		field_name " - using default value."	

#define OBJECT_PARSE_WARNING_STR(obj_name)										\
		"[Sumire::SumiConfig] WARNING: Failed to parse value of config object "	\
		obj_name " - using default value."	

namespace sumire {

	SumiConfig::SumiConfig() {
		readConfig();
	}

	SumiConfig::~SumiConfig() {

	}

	void SumiConfig::readConfig() {

		const std::string config_pth_string{ CONFIG_PATH };

		bool configExists = checkConfigFileExists();
		if (!configExists) {
			createDefaultConfig();
			std::cout << "[Sumire::SumiConfig] No existing config found at" + config_pth_string +
				". A default config was created." << std::endl;
			return;
		}

		std::string json = readConfigFile();

		rapidjson::Document config;
		config.Parse(json.c_str());

		if (!config.IsObject() || config.HasParseError()) {
			throw std::runtime_error("[Sumire::SumiConfig] Failed to parse config. Fix or delete the config at " + config_pth_string);
		}
		
		configData = SumiConfigData{};

		// resolution
		if (config.HasMember("resolution") && config["resolution"].IsObject()) {
			const rapidjson::Value& resolution = config["resolution"];

			// resolution.width
			if (resolution.HasMember("width") && resolution["width"].IsUint()) {
				configData.STARTUP_WIDTH = resolution["width"].GetUint();
			} else {
				std::cout << FIELD_PARSE_WARNING_STR("resolution.width") << std::endl;
			}

			// resolution.height
			if (resolution.HasMember("height") && resolution["height"].IsUint()) {
				configData.STARTUP_HEIGHT = resolution["height"].GetUint();
			}
			else {
				std::cout << FIELD_PARSE_WARNING_STR("resolution.height") << std::endl;
			}

		} else {
			std::cout << OBJECT_PARSE_WARNING_STR("resolution") << std::endl;
		}

		// max_n_lights
		if (config.HasMember("max_n_lights") && config["max_n_lights"].IsUint()) {
			configData.MAX_N_LIGHTS = config["max_n_lights"].GetUint();
		} else {
			std::cout << FIELD_PARSE_WARNING_STR("max_n_lights") << std::endl;
		}

		std::cout << "[Sumire::SumiConfig] Read config from " + config_pth_string << std::endl;
	}

	void SumiConfig::writeConfig() const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();

		writer.Key("resolution");
		writer.StartObject();
		writer.Key("width");
		writer.Uint(configData.STARTUP_WIDTH);
		writer.Key("height");
		writer.Uint(configData.STARTUP_HEIGHT);
		writer.EndObject();

		writer.Key("max_n_lights");
		writer.Uint(configData.MAX_N_LIGHTS);

		writer.Key("graphics_device");
		writer.StartObject();
		writer.Key("idx");
		writer.Uint(configData.GRAPHICS_DEVICE.idx);
		writer.Key("name");
		writer.String(configData.GRAPHICS_DEVICE.name);
		writer.EndObject();

		writer.EndObject();

		writeConfigFile(s.GetString());
	}

	void SumiConfig::createDefaultConfig() {
		configData = SumiConfigData{};
		writeConfig();
	}

	bool SumiConfig::checkConfigFileExists() const {
		std::ifstream f(CONFIG_PATH);
		return f.good();
	}

	std::string SumiConfig::readConfigFile() const {
		std::ifstream file{ CONFIG_PATH, std::ios::ate };

		if (!file.is_open()) {
			const std::string error_pth_out{ CONFIG_PATH };
			throw std::runtime_error("[Sumire::SumiConfig] Could not open config file for read at " + error_pth_out);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::string buffer(fileSize, ' ');
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	void SumiConfig::writeConfigFile(const char* json) const {
		std::ofstream file{ CONFIG_PATH, std::ios::trunc };

		if (!file.is_open()) {
			const std::string error_pth_out{ CONFIG_PATH };
			throw std::runtime_error("[Sumire::SumiConfig] Could not open config file for write at " + error_pth_out);
		}

		file << json;

		file.close();
	}

}