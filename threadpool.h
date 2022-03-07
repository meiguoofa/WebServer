/*
#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include"Locker.h"
#include<memory>
using std::shared_ptr;
template<typename T>
class threadpool {
public:
   threadpool(int thread_number = 8, int max_requests = 1000);
   ~threadpool();
   bool append(shared_ptr<T> request);


private:
   static void* worker(void *arg);
   void run();


private:
   int m_thread_number;
   int m_max_requests;
   pthread_t *m_threads;
   std::list<shared_ptr<T>> m_workqueue;
   Locker m_locker;
   Sem m_sem;
   bool m_stop;
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests): 
    m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL), m_stop(false) {
    if (m_thread_number <= 0 || m_max_requests <= 0) {
        throw std::exception();
    }
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) {
        throw std::exception();
    }
    for (int i = 0 ; i < m_thread_number ; i++) {
        printf("create the %dth thread\n", i + 1);
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i])) {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
bool threadpool<T>::append(shared_ptr<T> request) {
    this->m_locker.lock();
    if (m_workqueue.size() >= m_max_requests) {
        m_locker.unlock();
        return false;
    }
    this->m_workqueue.push_back(request);
    m_locker.unlock();
    m_sem.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker (void *arg) {
    threadpool *pool = static_cast<threadpool*> (arg);
    pool->run();
    return pool;
}


template<typename T>
void threadpool<T>::run() {
    while (!m_stop) {
        m_sem.wait();
        m_locker.lock();
        if (m_workqueue.empty()) {
            m_locker.unlock();
            continue;
        }
        shared_ptr<T> request = m_workqueue.front();
        m_workqueue.pop_front();
        m_locker.unlock();
        if (!request) {
            continue;
        }
        request->process();
    }
}

template<typename T>
threadpool<T>::~threadpool() {
    if (m_threads) {
        delete[] m_threads;
    } 
}


#endif
*/

/*
#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include<memory>
#include"Locker.h"
using std::shared_ptr;

template<typename T>
class threadpool {
public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(shared_ptr<T> request);

private:
    static void* worker(void *arg);
    void run();

private:
    int m_thread_number;
    int m_max_requests;
    pthread_t *m_threads;
    std::list<shared_ptr<T>> m_workqueue;
    Locker m_queuelocker;
    Sem m_queuestat;
    bool m_stop;
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) 
   : m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL) {
       if (max_requests <= 0 || thread_number <= 0) {
           throw std::exception();
       }
       m_threads = new pthread_t(m_thread_number);
       if (!m_threads) {
           throw std::exception();
       }
       for (int i = 0 ; i < thread_number ; i++) {
           printf("create the %dth thread\n", i);
           if (pthread_create(&m_threads[i], NULL, worker, this) != 0) {
               delete [] m_threads;
               throw std::exception();
           }
           if (pthread_detach(m_threads[i])) {
               delete [] m_threads;
               throw std::exception();
           }
       }
}

template<typename T> 
threadpool<T>::~threadpool() {
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(shared_ptr<T> request) {
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
}

template<typename T>
void* threadpool<T>::worker (void *arg) {
    threadpool *pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {
    while (!m_stop) {
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        shared_ptr<T> request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }
        request->process();
    }
}


#endif
*/

#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include"Locker.h"
#include<iostream>
template<typename T>
class threadpool {
public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(T *request);

private:
    static void* worker(void *arg);
    void run();

private:
    int m_thread_number;
    int m_max_requests;
    pthread_t *m_threads;
    std::list<T*> m_workqueue;
    Locker m_queuelocker;
    Sem m_queuestat;
    bool m_stop;
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) 
   : m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL) {
       if (max_requests <= 0 || thread_number <= 0) {
           throw std::exception();
       }
       m_threads = new pthread_t(m_thread_number);
       if (!m_threads) {
           throw std::exception();
       }
       for (int i = 0 ; i < thread_number ; i++) {
           printf("create the %dth thread\n", i);
           if (pthread_create(&m_threads[i], NULL, worker, this) != 0) {
               delete [] m_threads;
               throw std::exception();
           }
           if (pthread_detach(m_threads[i])) {
               delete [] m_threads;
               throw std::exception();
           }
       }
}

template<typename T> 
threadpool<T>::~threadpool() {
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T *request) {
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    //std::cout << "m_workqueue.size() " << m_workqueue.size() << "   " << std::endl;
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker (void *arg) {
    threadpool *pool = static_cast<threadpool*>(arg);
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {
    while (!m_stop) {
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        //std::cout << "////////////////////////////" << std::endl;
        T *request = m_workqueue.front();
        //std::cout << "|||||||||||||||||||||||||||" << std::endl;
        m_workqueue.pop_front();
        //std::cout << "+++++++++++++++++++++++++++++++" << std::endl;
        m_queuelocker.unlock();
        
        if (!request) {
            continue;
        }
        
        request->process();
    }
}


#endif