#include <logging.hpp>

void setupLogging() {
    // Logger setup
    auto stderrSink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    auto fileSink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt", true);
    auto errSink =
        std::make_shared<spdlog::sinks::error_proxy_sink_mt>(stderrSink);
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->set_pattern("%R|%^%7l%$> %v");
    auto logger = std::make_shared<spdlog::logger>(
        "logger", spdlog::sinks_init_list{errSink, fileSink});
    logger->set_formatter(std::move(formatter));
    spdlog::set_default_logger(logger);
    spdlog::flush_on(spdlog::level::trace);
#ifdef _DEBUG
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Running in Debug Mode");
#else
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Running in Release Mode");
#endif
}