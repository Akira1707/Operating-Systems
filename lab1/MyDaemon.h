#pragma once
#include <filesystem>
#include <string>
#include <mutex>
#include "Config.h"
#include "utils.h"
#include <csignal>

namespace fs = std::filesystem;

static constexpr const char* DAEMON_NAME = "mydaemon";
static constexpr const char* PIDFILE = "/tmp/mydaemon.pid";
static constexpr const char* LOGFILE = "/tmp/mydaemon.log";

class MyDaemon {
public:
    static MyDaemon& instance();

    int run(const std::string& cfgpath_in);

private:
    MyDaemon();
    ~MyDaemon() = default;
    MyDaemon(const MyDaemon&) = delete;
    MyDaemon& operator=(const MyDaemon&) = delete;

    // members
    fs::path config_path;
    Config cfg;
    int interval;
    std::string pidfile;
    std::string logfile;
    int sigpipe[2];
    volatile sig_atomic_t terminate_requested;
    std::mutex file_mutex;

    // internal functions
    void handle_existing_pidfile();
    bool write_pidfile();
    bool daemonize();
    void setup_signal_handlers();
    void main_loop();
    void do_scan_cycle();
    void cleanup();
    void sig_handler(int sig);
    static void sig_handler_static(int sig);
};