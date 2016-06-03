#include "sync_server.h"
#include <libgen.h>
#include <zlog.h>

#define SIGVAL_QUIT   0

struct log_t
{
    log_t(const std::string& conf, const std::string& category);
    ~log_t();
    bool error() const { return err_; }
    void log(const char * data);
private:
    bool init(const std::string& conf, const std::string& category);
private:
    bool err_;
    zlog_category_t * cat_;
};

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

log_t::log_t(const std::string& conf, const std::string& category)
: err_(false), cat_(NULL)
{
    err_ = !init(conf, category);
}

log_t::~log_t()
{
    zlog_fini();
}

bool log_t::init(const std::string& conf, const std::string& category)
{
    int rc = zlog_init(conf.c_str());
    if (rc)
    {
        return false;
    }
    cat_ = zlog_get_category(category.c_str());
    if (!cat_)
    {
        return false;
    }
    return true;
}

void log_t::log(const char * data)
{
    if (!data)
    {
        return;
    }
    printf("%s", data);
    zlog_debug(cat_, "%s", data);
}

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
    enum { MAX_PATH = 1024 };
    char path[MAX_PATH + 1] = {0};
    int rc = readlink("/proc/self/exe", path, MAX_PATH);
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
        sync_exit_server();
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

static void set_sync_network_ctx(const jos_lib::SyncServerConf& cf,
    sync_network::ip_allow_t& ip_allow_map,
    evhtp::http_basic_auth_t& auth,
    sync_network_ctx_t& ctx)
{
    ctx.base.threads = cf.network.threads;
    ctx.base.port = cf.network.port;
    ctx.base.backlog = cf.network.backlog;
    ctx.base.max_body_size = cf.network.max_body_size;
    ctx.base.max_keepalive_requests = cf.network.max_keepalive_requests;
    ctx.base.auth = auth.c_str();
    ctx.base.disable_100_cont = true;
    ctx.base.enable_defer_accept = cf.network.enable_defer_accept;
    ctx.base.enable_nodelay  = cf.network.enable_nodelay;
    ctx.base.enable_reuseport = cf.network.enable_reuseport;
    ctx.base.recv_timeout.tv_sec = cf.network.recv_timeout;
    ctx.base.send_timeout.tv_sec = cf.network.send_timeout;
    ctx.ip_allow_map = &ip_allow_map;
}

bool run_server(const std::string& srv_conf, const std::string& pid_file, const std::string& log_conf, const std::string& log_category)
{
    pid_file_t pid(pid_file);
    if (pid.error())
    {
        return false;
    }

    log_t log(log_conf, log_category);
    if (log.error())
    {
        return false;
    }

    log.log(srv_conf.c_str());
    log.log(pid_file.c_str());
    log.log(log_conf.c_str());

    jos_lib::SyncServerConf cf;
    if (!jos_lib::Load(srv_conf.c_str(), cf))
    {
        return false;
    }

    std::string json_val;
    jos_lib::Serialize <jos_lib::SyncServerConf, false> (cf, json_val);

    log.log(json_val.c_str());

    if (!init(cf.sync.logs_path.c_str(), cf.sync.ngx_path.c_str(), cf.sync.auth_path.c_str(),
        cf.sync.threads, cf.sync.release_interval, cf.sync.checkdb_interval, cf.sync.checklog_interval))
    {
        log.log("init sync server error");
        return false;
    }

    evhtp::http_basic_auth_t auth(cf.network.user.c_str(), cf.network.passwd.c_str());

    sync_network::ip_allow_t ip_allow_map = {0};
    std::string access_allow = cf.network.access_allow;
    sync_network::get_ip_allow_map ( ( char * ) access_allow.c_str(), access_allow.size(), &ip_allow_map );

    sync_network_ctx_t ctx;
    set_sync_network_ctx(cf, ip_allow_map, auth, ctx);

    log.log("start sync server");

    if (!sync_loop(&ctx))
    {
        log.log("sync_loop error");
    }

    if (!stop_sync())
    {
        log.log("stop_sync error");
    }

    log.log("sync server closed");
    return true;
}

