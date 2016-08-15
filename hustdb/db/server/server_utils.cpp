#include "server_utils.h"

#define SIGVAL_QUIT   0

struct pid_file_t
{
    pid_file_t(const std::string& pid_file);
    ~pid_file_t();
    bool error() const { return err_; }
private:
    bool init();
private:
    bool err_;
    std::string pid_file_;
    FILE * fp_;
};

bool pid_file_t::init()
{
    fp_ = fopen(pid_file_.c_str(), "w");
    if (!fp_)
    {
        return false;
    }
    __pid_t pid = getpid();
    char buf[11] = {0};
    sprintf(buf, "%d", pid);
    fputs(buf, fp_);
    fflush(fp_);
    return true;
}

pid_file_t::pid_file_t(const std::string& pid_file)
: err_(false), pid_file_(pid_file), fp_(NULL)
{
    err_ = !init();
}

pid_file_t::~pid_file_t()
{
    fclose(fp_);
    remove(pid_file_.c_str());
}

std::string get_abs_path(const char * file)
{
    enum { BUF_SIZE = 1024 };
    char path[BUF_SIZE + 1] = {0};
    int rc = readlink("/proc/self/exe", path, BUF_SIZE);
    if (rc < 0 || rc > MAX_PATH - 1)
    {
        return "";
    }

    std::string dir = dirname(path);
    dir.append("/");
    dir.append(file);

    return dir;
}

bool file_exist(const char * file)
{
    int rc = access(file, F_OK);
    return 0 == rc;
}

bool enable_start_server(const std::string& pid_file)
{
    if (!file_exist(pid_file.c_str()))
    {
        return true;
    }

    FILE * fp = fopen(pid_file.c_str(), "r");
    if (!fp)
    {
        return false;
    }
    char pid[11] = {0};
    fgets(pid, 10, fp);
    fclose(fp);

    char dir[32];
    sprintf(dir, "/proc/%s", pid);
    if (file_exist(dir))
    {
        return false;
    }
    remove(pid_file.c_str());
    return true;
}

static void sig_quit_handler(int signo, siginfo_t *info, void *none)
{
    if (!info)
    {
        return;
    }
    if (SIGVAL_QUIT == info->si_value.sival_int)
    {
        libevhtp_exit_server();
    }
}

static __pid_t load_server_pid(const char * pid_file)
{
    FILE * fp = fopen(pid_file, "r");
    if (!fp)
    {
        printf("open pid file error\n");
        return 0;
    }
    char buf[11] = {0};
    fgets(buf, 10, fp);
    fclose(fp);
    return atoi(buf);
}

bool send_quit_signal(const char * pid_file)
{
    __pid_t pid = load_server_pid(pid_file);
    if (0 == pid)
    {
        return false;
    }
    union sigval param;
    param.sival_int = SIGVAL_QUIT;
    if(-1 == sigqueue(pid, SIGTERM, param))
    {
        return false;
    }
    return true;
}

bool register_quit_signal()
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = sig_quit_handler;
    act.sa_flags = SA_SIGINFO;
    if(-1 == sigaction(SIGTERM, &act, NULL))
    {
        return false;
    }
    return true;
}

bool run_server(const std::string& pid_file)
{
    pid_file_t pid(pid_file);
    if (pid.error())
    {
        return false;
    }

    hustdb_t db;
    if (!db.open())
    {
        LOG_ERROR ("[hustdb_network]db open failed");
        return -1;
    }

    server_conf_t cf = db.get_server_conf();

    evhtp::http_basic_auth_t auth(cf.http_security_user.c_str(), cf.http_security_passwd.c_str());

    hustdb_network::ip_allow_t ip_allow_map = {0};
    hustdb_network::get_ip_allow_map ((char *) cf.http_access_allow.c_str(), cf.http_access_allow.size(), &ip_allow_map);

    hustdb_network_ctx_t ctx;
    ctx.base.threads = cf.tcp_worker_count;
    ctx.base.port = cf.tcp_port;
    ctx.base.backlog = cf.tcp_backlog;
    ctx.base.max_body_size = cf.tcp_max_body_size;
    ctx.base.max_keepalive_requests = cf.tcp_max_keepalive_requests;
    ctx.base.auth = auth.c_str();
    ctx.base.disable_100_cont = cf.disable_100_cont;
    ctx.base.enable_defer_accept = cf.tcp_enable_defer_accept;
    ctx.base.enable_nodelay  = cf.tcp_enable_nodelay;
    ctx.base.enable_reuseport = cf.tcp_enable_reuseport;
    ctx.base.recv_timeout.tv_sec = cf.tcp_recv_timeout;
    ctx.base.send_timeout.tv_sec = cf.tcp_send_timeout;
    ctx.ip_allow_map = &ip_allow_map;
    ctx.db = &db;

    if (!hustdb_loop(&ctx))
    {
        LOG_INFO ("[hustdb_network]hustdb_loop error");
    }

    LOG_INFO ("[hustdb_network]hustdb closed");
    return true;
}

