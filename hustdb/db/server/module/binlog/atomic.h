#ifndef __HUSTSTORE_BINLOG_ATOMIC_H_
#define __HUSTSTORE_BINLOG_ATOMIC_H_

template <typename T>
class atomic_integer_t
{
public:
    atomic_integer_t() : _value ( 0 )
    {
    }

    T get()
    {
        return __sync_val_compare_and_swap ( &_value, 0, 0 );
    }

    T fetch_and_add ( T x )
    {
        return __sync_fetch_and_add ( &_value, x );
    }

    T add_and_fetch ( T x )
    {
        return __sync_add_and_fetch ( &_value, x );
    }

    T increment_and_get()
    {
        return add_and_fetch ( 1 );
    }

    T decrement_and_get()
    {
        return add_and_fetch ( -1 );
    }

    void increment()
    {
        increment_and_get();
    }

    void decrement()
    {
        decrement_and_get();
    }

private:
    volatile T _value;
};

typedef atomic_integer_t<int32_t> atomic_int32_t;
typedef atomic_integer_t<int64_t> atomic_int64_t;

#endif