#ifndef __HUSTSTORE_BINLOG_SINGLETON_H_
#define __HUSTSTORE_BINLOG_SINGLETON_H_

#include <pthread.h>
#include <cstdlib>

template <typename T>
class singleton_t
{
public:

    static T & instance ( )
    {
        pthread_once ( &_pounce, &singleton_t::init );
        return *_value;
    }
private:
    singleton_t ( );
    ~singleton_t ( );

    static void init ( )
    {
        _value = new T ( );
        ::atexit ( destroy );
    }

    static void destroy ( void )
    {
        if ( _value )
        {
            delete _value;
        }

        _value = NULL;
    }

private:
    static pthread_once_t _pounce;
    static T * _value;
};

template <typename T>
pthread_once_t singleton_t<T>::_pounce = PTHREAD_ONCE_INIT;

template <typename T>
T * singleton_t<T>::_value = NULL;

#endif