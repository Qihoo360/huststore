#include "server_utils.h"
static const char * PID_FILE = "hustdb.pid";

static void manual()
{
    printf("\n");
    printf("    usage:\n");
    printf("        ./hustdb [option]\n");
    printf("            [option]\n");
    printf("                -help: show manual\n");
    printf("                -v: show version\n");
    printf("                -d: run in debug mode\n");
    printf("                -q: quit\n");
    printf("\n");
    printf("    sample:\n");
    printf("        ./hustdb -help\n");
    printf("        ./hustdb\n");
    printf("        ./hustdb -v\n");
    printf("        ./hustdb -d\n");
    printf("        ./hustdb -q\n");
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
        std::string help("-help");
        std::string version("-v");
        std::string debug("-d");
        std::string quit("-q");
        if (option == help)
        {
            manual();
            return false;
        }
        else if (option == version)
        {
            printf("huststore 1.9\n");
            return false;
        }
        else if (option == debug)
        {
            daemon_mode = false;
        }
        else if (option == quit)
        {
            daemon_mode = false;
            std::string pid_file = get_abs_path(PID_FILE);
            send_quit_signal(pid_file.c_str());
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

    if (!run_server(pid_file))
    {
        printf("start_sync_server error\n");
        return -1;
    }

    return 0;
}
