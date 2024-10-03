#pragma once
#include "renderer/rdrapi.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

#pragma warning(push, 0)
#include <spdlog\spdlog.h>
#include <spdlog\fmt\ostr.h>
#pragma warning(pop)


namespace spdlog
{
	class logger;
}

namespace rdr
{
	class Renderer;
	class Logger
	{
	public:
		static RDRAPI auto GetLogger() { return mExternalLogger; }
		static RDRAPI auto GetInternalLogger() { return mLogger; }
	private:
		static void Init(std::string_view Name);
		static void Shutdown();

		static RDRAPI inline std::shared_ptr<spdlog::logger> mLogger;
		static RDRAPI inline std::shared_ptr<spdlog::logger> mExternalLogger;
		friend class ::rdr::Renderer;
	};
}

template <typename ostream, glm::length_t L, typename T, glm::qualifier Q>
inline ostream& operator<<(ostream& stream, const glm::vec<L, T, Q>& vector)
{
	return stream << glm::to_string(vector);
}

template <typename ostream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline ostream& operator<<(ostream& stream, const glm::mat<C, R, T, Q>& matrix)
{
	return stream << glm::to_string(matrix);
}

template <typename ostream, typename T, glm::qualifier Q>
inline ostream& operator<<(ostream& stream, const glm::qua<T, Q>& quaternion)
{
	return stream << glm::to_string(quaternion);
}

#ifdef RDR_EXPORT
#define RDR_LOGGER (::rdr::Logger::GetInternalLogger())
#else
#define RDR_LOGGER (::rdr::Logger::GetLogger())
#endif

#ifdef _WIN32
#define RDR_DEBUGBREAK (__debugbreak())
#else
#define RDR_DEBUGBREAK
#endif

#define RDR_LOG_TRACE(...)		(RDR_LOGGER->trace(__VA_ARGS__))
#define RDR_LOG_INFO(...)		(RDR_LOGGER->info(__VA_ARGS__))
#define RDR_LOG_WARN(...)		(RDR_LOGGER->warn(__VA_ARGS__))
#define RDR_LOG_ERROR(...)		(RDR_LOGGER->error(__VA_ARGS__))
#define RDR_LOG_CRITICAL(...)	(RDR_LOGGER->critical(__VA_ARGS__))

#define RDR_ASSERT_MSG(check, ...) if (check) {} else { RDR_LOG_ERROR(__VA_ARGS__); }
#define RDR_ASSERT_MSG_BREAK(check, ...) RDR_ASSERT_MSG(check, __VA_ARGS__) if (check) {} else { RDR_DEBUGBREAK; }
#define RDR_ASSERT_NO_MSG(check) if (check) {} else { RDR_LOG_ERROR("Assertion '{0}' failed at line: {1}, function: {2}, file: {3}", #check, __LINE__, __FUNCTION__, __FILE__); }
#define RDR_ASSERT_NO_MSG_BREAK(check) RDR_ASSERT_NO_MSG(check) if (check) {} else { RDR_DEBUGBREAK; }
