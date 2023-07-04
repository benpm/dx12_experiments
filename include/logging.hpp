#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/fmt.h>

#include <spdlog/details/log_msg.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/formatter.h>
#include <spdlog/pattern_formatter.h>

#include <csignal>
#include <memory>
#include <mutex>
#include <string>

#include <locale>
#include <codecvt>
#include <windows.h>
#include <processthreadsapi.h>
#include <comdef.h>

//Sets the name of the current thread
inline void setCurThreadName(std::string name) {
    // pthread only supports giving threads names with fewer than 16 characters
    // including the null terminator so to make the behavior the same on different
    // platforms the name is truncated to 15 characters
    name.resize(15);
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    HRESULT r;
    r = SetThreadDescription(GetCurrentThread(), converter.from_bytes(name.c_str()).c_str());
}

//Gets the name of the current thread
inline std::string getCurThreadName() {
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    HRESULT r;
    wchar_t name[32];
    r = GetThreadDescription(GetCurrentThread(), (PWSTR*)name);
    if (r != S_OK) {
        _com_error err(r);
        spdlog::warn("Failed to get thread name, error {:x} {}", r, std::string(err.ErrorMessage()));
        return "";
    }
    return converter.to_bytes(name);
}

class tname_formatter_flag : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg &, const std::tm &, spdlog::memory_buf_t &dest) override {
        std::string threadName = fmt::format("{:<15}", getCurThreadName());
        dest.append(threadName.data(), threadName.data() + threadName.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<tname_formatter_flag>();
    }
};

namespace spdlog::sinks {
    template<typename Mutex>
    class error_proxy_sink : public base_sink<Mutex> {
    /* Thanks to @tt4g for this class (https://github.com/gabime/spdlog/issues/1363#issuecomment-567068416) */
    private:
        using BaseSink = base_sink<Mutex>;

        std::shared_ptr<sink> sink_;

    public:
        explicit error_proxy_sink(std::shared_ptr<sink> sink) : sink_(sink){}

        error_proxy_sink(const error_proxy_sink&) = delete;
        error_proxy_sink& operator=(const error_proxy_sink&) = delete;
    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override {
            if (sink_->should_log(msg.level)) {
                sink_->log(msg);
            }
            if (spdlog::level::err == msg.level) {
                std::raise(SIGINT);
            }
        }

        void flush_() override {
            sink_->flush();
        }

        void set_pattern_(const std::string &pattern) override {
            set_formatter_(spdlog::details::make_unique<spdlog::pattern_formatter>(pattern));
        }

        void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override {
            BaseSink::formatter_ = std::move(sink_formatter);

            sink_->set_formatter(BaseSink::formatter_->clone());
        }
    };

    using error_proxy_sink_mt = error_proxy_sink<std::mutex>;
    using error_proxy_sink_st = error_proxy_sink<spdlog::details::null_mutex>;
}

void setupLogging();