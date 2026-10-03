#include <iostream>
#include "ProcessManagement.hpp"
#include <cstring>
#include "../encryptDecrypt/Cryption.hpp"
#include <sys/mman.h>
#include <atomic>
#include <sys/fcntl.h>
#include <mman.h>
#include <wait.h>
#include<unistd.h>
#include<semaphore.h>


ProcessManagement::ProcessManagement(){
    sem_t*itemSemaphore=sem_open("/item_semaphore",O_CREAT,0666,0);
    sem_t*emptySlotsSemaphore=sem_open("/empty_slots_semaphore",O_CREAT,0666,1000);

    shmFd = shm_open(shm_Name, O_CREAT | O_RDWR, 0666);
    ftruncate(shmFd, sizeof(SharedMemory));
    SharedMem = static_cast<SharedMemory*>(mmap(nullptr, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0));
    SharedMem->front = 0;
    SharedMem->rear = 0;
    SharedMem->size.store(0);
}

bool ProcessManagement::submitToQueue(std::unique_ptr<Task> task){
    sem_wait(emptySlotsSemaphore);
    std::unique_lock<std::mutex>lock(queueLock);

    if(SharedMem->size.load() >= 1000){
        return false;
    }

    strcpy(SharedMem->tasks[SharedMem->rear], task->toString().c_str());
    SharedMem->rear = (SharedMem->rear + 1)%1000;
    SharedMem->size.fetch_add(1);
    lock.unlock();
    sem_post(itemSemaphore);
    
    int pid = fork();
    if(pid<0){
        return false;
    }else if(pid>0){
        std::cout<<"Enter the parent process"<<std::endl;
    }else{
        std::cout<<"Enter the child process"<<std::endl;
        executeTasks();
        std::cout<<"Exiting the child process"<<std::endl;
        exit(0);
    }
    return true;
}

void ProcessManagement::executeTasks(){
    sem_wait(itemSemaphore);
    std::unique_lock<std::mutex>lock(queueLock);
    char taskStr[256];
    strcpy(taskStr, SharedMem->tasks[SharedMem->front]);
    SharedMem->front = (SharedMem->front + 1)%1000;
    SharedMem->size.fetch_sub(1);
    lock.unlock();
    sem_post(emptySlotsSemaphore);
    std::cout<<"Executing child process"<<std::endl;
    executeCryption(taskStr);
}

ProcessManagement::~ProcessManagement(){
    munmap(SharedMem, sizeof(SharedMemory));
    shm_unlink(shm_Name);
}