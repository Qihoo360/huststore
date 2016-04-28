#pragma once

#include <unistd.h>
#include <pthread.h>

class Condition
{
public:
    Condition ( );
    ~Condition ( );
    void wait ( pthread_mutex_t *mutex );
    void signal ( );
    void broadcast ( );
private:
    pthread_cond_t _cond_var;
};
