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

