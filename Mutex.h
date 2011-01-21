/* 
 * File:   Mutex.h
 * Author: Benjamin J. Land
 *
 * Created on December 18, 2008, 8:04 PM
 */

#ifndef _MUTEX_H
#define	_MUTEX_H

class Mutex {
public:
    Mutex();
    ~Mutex();
    void lock();
    void unlock();
private:
    Mutex(Mutex &ref);
    void *data;
};

class Lock {
private:
    Mutex *mutex;
public:
    Lock(Mutex *_mutex);
    ~Lock();
};

#endif	/* _MUTEX_H */

