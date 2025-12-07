#pragma once

#include <memory>
#include <vector>

#include <spdlog/common.h>  // spdlog::source_loc, SPDLOG_FUNCTION
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace bolero {

class Logger {
   public:
    static std::shared_ptr<spdlog::logger> GetLogger() {
        static std::shared_ptr<spdlog::logger> logger = [] {
            // 콘솔 sink
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

            // 파일 sink
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("bolero.log", true);

            std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

            auto multi_logger = std::make_shared<spdlog::logger>("bolero_logger", sinks.begin(), sinks.end());

            // 패턴에 파일/라인/함수 포함:
            // %s = 파일명, %# = 라인, %! = 함수명
            // 예: [2025-01-21 12:34:56.789] [info] [module.cpp:42 MyClass::Foo] 메시지
            multi_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%# %!] %v");

            multi_logger->set_level(spdlog::level::trace);
            multi_logger->flush_on(spdlog::level::info);

            spdlog::register_logger(multi_logger);
            return multi_logger;
        }();

        return logger;
    }

    static void SetLogLevel(spdlog::level::level_enum level) { GetLogger()->set_level(level); }
};

}  // namespace bolero

// ---- 매크로 정의 (파일/라인/함수 위치 포함) ----

#define BOLERO_LOG_INTERNAL(level, ...)                                                                \
    ::bolero::Logger::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level, \
                                       __VA_ARGS__)

#define BOLERO_LOG_TRACE(...) BOLERO_LOG_INTERNAL(spdlog::level::trace, __VA_ARGS__)
#define BOLERO_LOG_DEBUG(...) BOLERO_LOG_INTERNAL(spdlog::level::debug, __VA_ARGS__)
#define BOLERO_LOG_INFO(...) BOLERO_LOG_INTERNAL(spdlog::level::info, __VA_ARGS__)
#define BOLERO_LOG_WARN(...) BOLERO_LOG_INTERNAL(spdlog::level::warn, __VA_ARGS__)
#define BOLERO_LOG_ERROR(...) BOLERO_LOG_INTERNAL(spdlog::level::err, __VA_ARGS__)
#define BOLERO_LOG_CRITICAL(...) BOLERO_LOG_INTERNAL(spdlog::level::critical, __VA_ARGS__)
