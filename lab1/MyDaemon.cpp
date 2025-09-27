#include "MyDaemon.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <cstring>
#include <csignal>

// ---------------- Constructor & Singleton ----------------

MyDaemon& MyDaemon::instance() {
    static MyDaemon inst;
    return inst;
}

MyDaemon::MyDaemon()
    : config_path(),
      cfg(),
      interval(20),
      pidfile(PIDFILE),
      logfile(LOGFILE),
      sigpipe{-1, -1},
      terminate_requested(false)
{}

// ---------------- PID file handling ----------------

void MyDaemon::handle_existing_pidfile() {
    std::ifstream ifs(pidfile);
    if (!ifs.is_open()) return;
    pid_t oldpid = 0;
    ifs >> oldpid;
    ifs.close();
    if (oldpid <= 0) { unlink(pidfile.c_str()); return; }

    std::string procpath = "/proc/" + std::to_string(oldpid);
    struct stat st;
    if (stat(procpath.c_str(), &st) == 0) {
        std::cerr << "Found existing pid " << oldpid << ", sending SIGTERM...\n";
        if (kill(oldpid, SIGTERM) == 0) {
            for (int i = 0; i < 5; ++i) {
                if (stat(procpath.c_str(), &st) != 0) break;
                sleep(1);
            }
        }
    } else {
        unlink(pidfile.c_str());
    }
}

bool MyDaemon::write_pidfile() {
    int fd = open(pidfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        safe_syslog(LOG_ERR, "Cannot open pidfile %s: %s", pidfile.c_str(), strerror(errno));
        return false;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
        safe_syslog(LOG_WARNING, "Could not lock pidfile %s", pidfile.c_str());

    char buf[64]; 
    int len = snprintf(buf, sizeof(buf), "%d\n", getpid());
    if (write(fd, buf, len) != len)
        safe_syslog(LOG_ERR, "Failed to write pidfile %s", pidfile.c_str());

    close(fd);
    return true;
}

// ---------------- Daemonization ----------------

bool MyDaemon::daemonize() {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return false; }
    if (pid > 0) _exit(0);

    if (setsid() < 0) { perror("setsid"); return false; }

    pid = fork();
    if (pid < 0) { perror("fork2"); return false; }
    if (pid > 0) _exit(0);

    if (chdir("/") != 0) safe_syslog(LOG_WARNING, "chdir failed: %s", strerror(errno));
    umask(0);

    long maxfd = sysconf(_SC_OPEN_MAX); 
    if (maxfd == -1) maxfd = 1024;
    for (long fd = 0; fd < maxfd; ++fd) close((int)fd);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);

    if (pipe(sigpipe) != 0) return false;
    int flags0 = fcntl(sigpipe[0], F_GETFD);
    int flags1 = fcntl(sigpipe[1], F_GETFD);
    if (flags0 >= 0) fcntl(sigpipe[0], F_SETFD, flags0 | FD_CLOEXEC);
    if (flags1 >= 0) fcntl(sigpipe[1], F_SETFD, flags1 | FD_CLOEXEC);

    return true;
}

// ---------------- Signal handling ----------------

void MyDaemon::sig_handler(int sig) {
    char c = (sig == SIGHUP) ? 'H' : (sig == SIGTERM) ? 'T' : '?';
    if (sigpipe[1] != -1) { ssize_t r = write(sigpipe[1], &c, 1); (void)r; }
}

void MyDaemon::sig_handler_static(int sig) {
    MyDaemon::instance().sig_handler(sig);
}

void MyDaemon::setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = sig_handler_static;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    struct sigaction sa2;
    sa2.sa_handler = SIG_IGN;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGPIPE, &sa2, nullptr);
}

// ---------------- Main loop ----------------

void MyDaemon::main_loop() {
    while (!terminate_requested) {
        struct pollfd pfd;
        pfd.fd = sigpipe[0];
        pfd.events = POLLIN;
        int rv = poll(&pfd, 1, interval*1000);

        if (rv < 0) { 
            if (errno == EINTR) continue;
            safe_syslog(LOG_ERR, "poll failed: %s", strerror(errno));
            continue;
        } else if (rv == 0) {
            do_scan_cycle();
        } else if (pfd.revents & POLLIN) {
            char buf[64];
            ssize_t n = read(sigpipe[0], buf, sizeof(buf));
            if (n <= 0) continue;
            for (ssize_t i = 0; i < n; ++i) {
                char c = buf[i];
                if (c == 'H') { safe_syslog(LOG_INFO, "Received SIGHUP -> reload config"); cfg.load_from(config_path); }
                else if (c == 'T') { safe_syslog(LOG_INFO, "Received SIGTERM -> terminating"); terminate_requested = true; }
            }
        }
    }
}

// ---------------- Scan cycle ----------------

void MyDaemon::do_scan_cycle() {
    auto entries = cfg.get_entries();
    if (entries.empty()) {
        safe_syslog(LOG_INFO, "No folders configured; skipping scan");
        return;
    }

    std::ofstream ofs;
    { 
        std::lock_guard<std::mutex> lock(file_mutex);
        ofs.open(logfile, std::ios::app);
        if (!ofs.is_open()) {
            safe_syslog(LOG_ERR, "Cannot open logfile %s", logfile.c_str());
            return;
        }

        for (const auto& e : entries) {
            try {
                for (const auto& dirent : fs::directory_iterator(e.folder)) {
                    if (!dirent.is_regular_file()) continue;
                    std::string fileext = dirent.path().extension().string(); 
                    if (!fileext.empty() && fileext.front() == '.') fileext.erase(0,1);
                    if (fileext != e.ext) continue;

                    std::uintmax_t sz = 0; 
                    try { sz = fs::file_size(dirent.path()); } catch(...) { sz=0; }

                    ofs << timestamp_now() << " | " 
                        << e.folder.string() << " | " 
                        << dirent.path().filename().string() << " | " 
                        << sz << " bytes\n";
                }
            } catch(...) {}
        }

        ofs.close();
    }

    safe_syslog(LOG_INFO, "Completed scan cycle (wrote to %s)", logfile.c_str());
}

// ---------------- Cleanup ----------------

void MyDaemon::cleanup() {
    unlink(pidfile.c_str());
    safe_syslog(LOG_INFO, "Daemon exiting");
    closelog();
}

// ---------------- Run ----------------

int MyDaemon::run(const std::string& cfgpath_in) {
    config_path = fs::absolute(cfgpath_in);
    handle_existing_pidfile();

    if (pipe(sigpipe) != 0) {
        perror("pipe");
        return 1;
    }

    int flags0 = fcntl(sigpipe[0], F_GETFD);
    int flags1 = fcntl(sigpipe[1], F_GETFD);
    if (flags0 >= 0) fcntl(sigpipe[0], F_SETFD, flags0 | FD_CLOEXEC);
    if (flags1 >= 0) fcntl(sigpipe[1], F_SETFD, flags1 | FD_CLOEXEC);

    if (!daemonize()) {
        std::cerr << "Failed to daemonize\n";
        return 1;
    }

    if (!write_pidfile()) {
        safe_syslog(LOG_ERR, "Failed to write pidfile");
    }

    openlog(DAEMON_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
    safe_syslog(LOG_INFO, "Daemon started (pid=%d)", getpid());

    setup_signal_handlers();

    if (!cfg.load_from(config_path)) {
        safe_syslog(LOG_ERR, "Initial config load failed from %s", config_path.c_str());
    } else {
        safe_syslog(LOG_INFO, "Config loaded from %s", config_path.c_str());
    }

    main_loop();
    cleanup();
    return 0;
}
