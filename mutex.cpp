#include "Mutex.h"
#include <pthread.h>
#include <cstring>

Mutex::Mutex() {
    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    data = new pthread_mutex_t;
    memcpy(data,(void*)&mut,__SIZEOF_PTHREAD_MUTEX_T);
}

Mutex::Mutex(Mutex &ref) {
}

Mutex::~Mutex() {
    pthread_mutex_destroy( (pthread_mutex_t*) data);
    delete (pthread_mutex_t*) data;
}

void Mutex::lock() {
   pthread_mutex_lock( (pthread_mutex_t*) data );
}

void Mutex::unlock() {
   pthread_mutex_unlock( (pthread_mutex_t*) data );
}

Lock::Lock(Mutex *_mutex) : mutex(_mutex) {
    mutex->lock();
}

Lock::~Lock() {
    mutex->unlock();
};
