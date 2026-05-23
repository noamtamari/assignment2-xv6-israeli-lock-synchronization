#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define MAX_ISRAELI_LOCKS 15
#define MAX_WAITING_PROCS 16

struct israeli_lock {
    int active;                            // 1 if the lock is active, 0 if it is not
    int held;                              // 1 if the lock is held, 0 if it is not
    int owner_pid;                         // PID of the process holding the lock, valid if held == 1
    int favoritism;                        // percentage (0-100) chance of favoring a process with the same gid as the releaser when releasing the lock
    struct spinlock lock;                  // spinlock protecting this israeli_lock from concurrent access
    struct proc *queue[MAX_WAITING_PROCS]; // queue of processes waiting for the lock, implemented as an array
    int q_size;                            // number of processes currently in the queue
};
// array of israeli locks
static struct israeli_lock israeli_locks[MAX_ISRAELI_LOCKS];

// helper function to check if a lock_id is valid, returns 1 if valid, 0 if not
static int
valid_lock_id(int lock_id)
{
    return lock_id >= 0 && lock_id < MAX_ISRAELI_LOCKS;
}

// helper function to find the index of the first process in the lock's queue with the same gid as the releaser,
// returns index if found, -1 if not found
static int
find_same_gid_index(struct israeli_lock *lk, int gid)
{
    int i;
    for (i = 0; i < lk->q_size; i++) {
        if (lk->queue[i]->gid == gid) {
            return i;
        }
    }
    return -1;
}

// helper function to check if favoritism is valid, returns 1 if valid, 0 if not
static int
valid_favoritism(int favoritism)
{
    return favoritism >= 0 && favoritism <= 100;
}

// initialize the israeli locks
void
israeli_init(void)
{
    int i;
    for (i = 0; i < MAX_ISRAELI_LOCKS; i++) {
        initlock(&israeli_locks[i].lock, "israeli_lock");
        israeli_locks[i].active = 0;
        israeli_locks[i].held = 0;
        israeli_locks[i].owner_pid = 0;
        israeli_locks[i].favoritism = 0;
        israeli_locks[i].q_size = 0;
    }
}

// create a new israeli lock with the given favoritism percentage, returns lock_id if successful, -1 if error
int
israeli_create(int favoritism)
{
    int i;
    if (!valid_favoritism(favoritism)) {
        return -1;
    }
    for (i = 0; i < MAX_ISRAELI_LOCKS; i++) {
        struct israeli_lock *lk = &israeli_locks[i];
        acquire(&lk->lock);
        if (!lk->active) {
            lk->active = 1;
            lk->held = 0;
            lk->owner_pid = 0;
            lk->favoritism = favoritism;
            lk->q_size = 0;
            release(&lk->lock);
            return i;
        }
        release(&lk->lock);
    }
    return -1;
}

// acquire the israeli lock with the given lock_id, returns 0 if successful, -1 if error
int
israeli_acquire(int lock_id)
{
    struct israeli_lock *lk;
    struct proc *p = myproc();
    if (!valid_lock_id(lock_id)) {
        return -1;
    }
    lk = &israeli_locks[lock_id];
    acquire(&lk->lock);
    // Safety check: An inactive lock means it has not been created or has been destroyed.
    if (!lk->active) {
        release(&lk->lock);
        return -1;
    }
    if (!lk->held) {
        lk->held = 1;
        lk->owner_pid = p->pid;
        release(&lk->lock);
        return 0;
    }

    if (lk->q_size >= MAX_WAITING_PROCS) {
        release(&lk->lock);
        return -1;
    }
    // Assign the current process to the end of the waiting queue,
    // then increment the queue size
    lk->queue[lk->q_size++] = p;
    for (;;) {
        sleep(p, &lk->lock); // sleep on the process's own address
        if (!lk->active) {
            release(&lk->lock);
            return -1;
        }
        if (lk->held && lk->owner_pid == p->pid) {
            release(&lk->lock);
            return 0;
        }
    }
}

// release the israeli lock with the given lock_id, returns 0 if successful, -1 if error
// Only the owner can release the lock. If there are waiting processes, the next owner is selected (with favoritism if set).
// Wakes up waiting processes as needed.
int
israeli_release(int lock_id)
{
    struct israeli_lock *lk;
    struct proc *p = myproc();
    int next_index = 0; // index of the next owner in the queue
    int i;
    if (!valid_lock_id(lock_id)) {
        return -1;
    }
    lk = &israeli_locks[lock_id];
    acquire(&lk->lock);
    // only the owner can release the lock, and the lock must be active and held
    if (!lk->active || !lk->held || lk->owner_pid != p->pid) {
        release(&lk->lock);
        return -1;
    }
    // if there are no waiting processes, simply release the lock
    if (lk->q_size == 0) {
        lk->held = 0;
        lk->owner_pid = 0;
        wakeup(lk);
        release(&lk->lock);
        return 0;
    }
    // if there are waiting processes, select the next owner based on the favoritism and group IDs of the waiting processes
    if (lk->favoritism > 0) {
        int same_gid_index = find_same_gid_index(lk, p->gid);
        // if there is a process with the same gid as the releaser,
        // select it as the next owner with probability equal to the favoritism percentage
        if (same_gid_index >= 0) {
            uint r = lcg_rand() % 100;
            if (r < (uint)lk->favoritism) {
                // move the selected process to the front of the queue
                next_index = same_gid_index;
            }
        }
    }
    {
        struct proc *next = lk->queue[next_index];
        // remove the selected process from the queue
        for (i = next_index + 1; i < lk->q_size; i++) {
            lk->queue[i - 1] = lk->queue[i];
        }
        lk->q_size--;
        lk->owner_pid = next->pid;
        lk->held = 1;
        // wake up only the next owner
        wakeup(next);
    }
    release(&lk->lock);
    return 0;
}

// destroy the lock with the given lock_id, returns 0 if successful, -1 if error
int
israeli_destroy(int lock_id)
{
    struct israeli_lock *lk;
    int i;
    if (!valid_lock_id(lock_id)) {
        return -1;
    }
    // only a non-waiting owner can destroy the lock, and the lock must be active
    lk = &israeli_locks[lock_id];
    acquire(&lk->lock);
    // Return an error if the lock is inactive or currently held.
    if (!lk->active || lk->held) {
        release(&lk->lock);
        return -1;
    }
    // Mark the lock as inactive and wake up all waiting processes with an error
    lk->active = 0;
    lk->held = 0;
    lk->owner_pid = 0;
    // Wake up all waiting processes (they sleep on their own address)
    for (i = 0; i < lk->q_size; i++) {
        wakeup(lk->queue[i]);
    }
    lk->q_size = 0;
    release(&lk->lock);
    return 0;
}
