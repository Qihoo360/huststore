#include "../lib/cond.h"

Condition::Condition ( )
{
    pthread_cond_init (&_cond_var, NULL);
}

Condition::~ Condition ( )
{
    pthread_cond_destroy (&_cond_var);
}

void Condition::wait ( pthread_mutex_t *mutex )
{
    pthread_cond_wait (&_cond_var, mutex);
}

void Condition::signal ( )
{
    pthread_cond_signal (&_cond_var);
}

void Condition::broadcast ( )
{
    pthread_cond_broadcast (&_cond_var);
}
