/**
 *  Copyright 2010 by Benjamin J. Land (a.k.a. BenLand100)
 *
 *  This file is part of SJVM the Simple Java Virtual Machine.
 *
 *  SJVM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SJVM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SJVM. If not, see <http://www.gnu.org/licenses/>.
 */

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
