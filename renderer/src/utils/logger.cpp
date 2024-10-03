#include <renderer/rdrpch.h>

#include "renderer/logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <vector>
#include <array>

namespace rdr
{
	void Logger::Init(std::string_view name)
	{
		std::vector<spdlog::sink_ptr> sinks;

		sinks.emplace_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
		sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(std::string(name) + ".log", true));

		sinks[0]->set_pattern("%^[%T] %n: %v%$");
		sinks[0]->set_level(spdlog::level::info);

		sinks[1]->set_pattern("[%T] [%l] %n: %v");

		std::array<std::pair<std::string_view, std::shared_ptr<spdlog::logger>*>, 2> loggers =
		{
			std::make_pair("renderer", &mLogger),
			std::make_pair(name, &mExternalLogger)
		};

		for (auto& [name, logger] : loggers)
		{
			auto& lg = *logger;
			lg = std::make_shared<spdlog::logger>(std::string(name), sinks.begin(), sinks.end());
			spdlog::register_logger(lg);
			lg->set_level(spdlog::level::trace);
			lg->flush_on(spdlog::level::trace);
		}

		RDR_LOG_TRACE("Logger Online");
	}

	void Logger::Shutdown()
	{

	}
}
