#include "sync_server.h"

static const char * SRV_CONF = "sync_server.json";
static const char * PID_FILE = "hustdbsync.pid";
static const char * LOG_CONF = "zlog.conf";
static const char * LOG_CATEGORY = "hustdbsync";

static void manual()
{
    printf("\n");
    printf("    usage:\n");
    printf("        ./hustdbsync [option]\n");
    printf("            [option]\n");
    printf("                -d: run in debug mode\n");
    printf("                -q: quit\n");
    printf("\n");
    printf("    sample:\n");
    printf("        ./hustdbsync\n");
    printf("        ./hustdbsync -d\n");
    printf("        ./hustdbsync -q\n");
    printf("\n");
}

static bool parse_args(int argc, char *argv[], bool& daemon_mode)
{
    if (2 != argc && 1 != argc)
    {
        manual();
        return false;
    }
    daemon_mode = true;
    if (2 == argc)
    {
        std::string option = argv[1];
        std::string debug("-d");
        std::string quit("-q");
        if (option == debug)
        {
            daemon_mode = false;
        }
        else if (option == quit)
        {
            daemon_mode = false;
            std::string pid_file = get_abs_path(PID_FILE);
            if (!send_quit_signal(pid_file.c_str()))
            {
                printf("send_quit_signal error\n");
            }
            return false;
        }
        else
        {
            manual();
            return false;
        }
    }
    return true;
}

int main (int argc, char *argv[])
{
    bool daemon_mode = true;
    if (!parse_args(argc, argv, daemon_mode))
    {
        return 0;
    }

    std::string pid_file = get_abs_path(PID_FILE);
    if (!enable_start_server(pid_file))
    {
        printf("pid_file already exist\n");
        return 0;
    }

    std::string srv_conf = get_abs_path(SRV_CONF);
    std::string log_conf = get_abs_path(LOG_CONF);

    if(daemon_mode && daemon(1, 0) < 0)
    {
        printf("daemon error\n");
        return -1;
    }

    if (!register_quit_signal())
    {
        printf("register_signal error\n");
        return -1;
    }

    if (!run_server(srv_conf, pid_file, log_conf, LOG_CATEGORY))
    {
        printf("start_sync_server error\n");
        return -1;
    }

    return 0;
}
