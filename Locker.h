#ifndef LOCKER_H
#define LOCKER_H
#include<exception>
#include<pthread.h>
#include<semaphore.h>
#include<mutex>
class Sem {
public:
   Sem() {
       if (sem_init(&m_sem, 0, 0) != 0) {
           throw std::exception();
       }   
   }

   Sem(int num) {
       if (sem_init(&m_sem, 0, num) != 0) {
           throw std::exception();
       } 
   }

   bool post() {
       return sem_post(&m_sem) == 0;
   }

   bool wait() {
       return sem_wait(&m_sem) == 0; 
   }

   ~Sem() {
       sem_destroy(&m_sem);
   }
private:
   sem_t m_sem;
};


class Locker {
public:
   Locker() {
       if (pthread_mutex_init(&m_mutex, NULL) != 0) {
           throw std::exception();
       }
   }

   ~Locker() {
       pthread_mutex_destroy(&m_mutex);
   }

   bool lock() {
       return pthread_mutex_lock(&m_mutex) == 0;
   }

   bool unlock() {
       return pthread_mutex_unlock(&m_mutex) == 0;
   }
   
private:
   pthread_mutex_t m_mutex;
};

#endif