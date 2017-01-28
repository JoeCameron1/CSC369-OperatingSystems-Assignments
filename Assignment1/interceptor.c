#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/current.h>
#include <asm/ptrace.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <asm/unistd.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/syscalls.h>
#include "interceptor.h"


MODULE_DESCRIPTION("My kernel module");
MODULE_AUTHOR("Joseph Cameron");
MODULE_LICENSE("GPL");

//----- System Call Table Stuff ------------------------------------
/* Symbol that allows access to the kernel system call table */
extern void* sys_call_table[];

/* The sys_call_table is read-only => must make it RW before replacing a syscall */
void set_addr_rw(unsigned long addr) {

	unsigned int level;
	pte_t *pte = lookup_address(addr, &level);

	if (pte->pte &~ _PAGE_RW) pte->pte |= _PAGE_RW;

}

/* Restores the sys_call_table as read-only */
void set_addr_ro(unsigned long addr) {

	unsigned int level;
	pte_t *pte = lookup_address(addr, &level);

	pte->pte = pte->pte &~_PAGE_RW;

}
//-------------------------------------------------------------


//----- Data structures and bookkeeping -----------------------
/**
 * This block contains the data structures needed for keeping track of
 * intercepted system calls (including their original calls), pid monitoring
 * synchronization on shared data, etc.
 * It's highly unlikely that you will need any globals other than these.
 */

/* List structure - each intercepted syscall may have a list of monitored pids */
struct pid_list {
	pid_t pid;
	struct list_head list;
};


/* Store info about intercepted/replaced system calls */
typedef struct {

	/* Original system call */
	asmlinkage long (*f)(struct pt_regs);

	/* Status: 1=intercepted, 0=not intercepted */
	int intercepted;

	/* Are any PIDs being monitored for this syscall? */
	int monitored;	
	/* List of monitored PIDs */
	int listcount;
	struct list_head my_list;
}mytable;

/* An entry for each system call in this "metadata" table */
mytable table[NR_syscalls+1];

/* Access to the system call table and your metadata table must be synchronized */
spinlock_t my_table_lock = SPIN_LOCK_UNLOCKED;
spinlock_t sys_call_table_lock = SPIN_LOCK_UNLOCKED;
//-------------------------------------------------------------


//----------LIST OPERATIONS------------------------------------
/**
 * These operations are meant for manipulating the list of pids 
 * Nothing to do here, but please make sure to read over these functions 
 * to understand their purpose, as you will need to use them!
 */

/**
 * Add a pid to a syscall's list of monitored pids. 
 * Returns -ENOMEM if the operation is unsuccessful.
 */
static int add_pid_sysc(pid_t pid, int sysc)
{
	struct pid_list *ple=(struct pid_list*)kmalloc(sizeof(struct pid_list), GFP_KERNEL);

	if (!ple)
		return -ENOMEM;

	INIT_LIST_HEAD(&ple->list);
	ple->pid=pid;

	list_add(&ple->list, &(table[sysc].my_list));
	table[sysc].listcount++;

	return 0;
}

/**
 * Remove a pid from a system call's list of monitored pids.
 * Returns -EINVAL if no such pid was found in the list.
 */
static int del_pid_sysc(pid_t pid, int sysc)
{
	struct list_head *i;
	struct pid_list *ple;

	list_for_each(i, &(table[sysc].my_list)) {

		ple=list_entry(i, struct pid_list, list);
		if(ple->pid == pid) {

			list_del(i);
			kfree(ple);

			table[sysc].listcount--;
			/* If there are no more pids in sysc's list of pids, then
			 * stop the monitoring only if it's not for all pids (monitored=2) */
			if(table[sysc].listcount == 0 && table[sysc].monitored == 1) {
				table[sysc].monitored = 0;
			}

			return 0;
		}
	}

	return -EINVAL;
}

/**
 * Remove a pid from all the lists of monitored pids (for all intercepted syscalls).
 * Returns -1 if this process is not being monitored in any list.
 */
static int del_pid(pid_t pid)
{
	struct list_head *i, *n;
	struct pid_list *ple;
	int ispid = 0, s = 0;

	for(s = 1; s < NR_syscalls; s++) {

		list_for_each_safe(i, n, &(table[s].my_list)) {

			ple=list_entry(i, struct pid_list, list);
			if(ple->pid == pid) {

				list_del(i);
				ispid = 1;
				kfree(ple);

				table[s].listcount--;
				/* If there are no more pids in sysc's list of pids, then
				 * stop the monitoring only if it's not for all pids (monitored=2) */
				if(table[s].listcount == 0 && table[s].monitored == 1) {
					table[s].monitored = 0;
				}
			}
		}
	}

	if (ispid) return 0;
	return -1;
}

/**
 * Clear the list of monitored pids for a specific syscall.
 */
static void destroy_list(int sysc) {

	struct list_head *i, *n;
	struct pid_list *ple;

	list_for_each_safe(i, n, &(table[sysc].my_list)) {

		ple=list_entry(i, struct pid_list, list);
		list_del(i);
		kfree(ple);
	}

	table[sysc].listcount = 0;
	table[sysc].monitored = 0;
}

/**
 * Check if two pids have the same owner - useful for checking if a pid 
 * requested to be monitored is owned by the requesting process.
 * Remember that when requesting to start monitoring for a pid, only the 
 * owner of that pid is allowed to request that.
 */
static int check_pids_same_owner(pid_t pid1, pid_t pid2) {

	struct task_struct *p1 = pid_task(find_vpid(pid1), PIDTYPE_PID);
	struct task_struct *p2 = pid_task(find_vpid(pid2), PIDTYPE_PID);
	if(p1->real_cred->uid != p2->real_cred->uid)
		return -EPERM;
	return 0;
}

/**
 * Check if a pid is already being monitored for a specific syscall.
 * Returns 1 if it already is, or 0 if pid is not in sysc's list.
 */
static int check_pid_monitored(int sysc, pid_t pid) {

	struct list_head *i;
	struct pid_list *ple;

	list_for_each(i, &(table[sysc].my_list)) {

		ple=list_entry(i, struct pid_list, list);
		if(ple->pid == pid) 
			return 1;
		
	}
	return 0;	
}
//----------------------------------------------------------------

//----- Intercepting exit_group ----------------------------------
/**
 * Since a process can exit without its owner specifically requesting
 * to stop monitoring it, we must intercept the exit_group system call
 * so that we can remove the exiting process's pid from *all* syscall lists.
 */  

/** 
 * Stores original exit_group function - after all, we must restore it 
 * when our kernel module exits.
 */
void (*orig_exit_group)(int);

/**
 * Our custom exit_group system call.
 *
 * TODO: When a process exits, make sure to remove that pid from all lists.
 * The exiting process's PID can be retrieved using the current variable (current->pid).
 * Don't forget to call the original exit_group.
 *
 * Note: using printk in this function will potentially result in errors!
 *
 */
void my_exit_group(int status)
{
    
    del_pid(current->pid);
    
    (*orig_exit_group)(status); //Not forgetting the original exit_group call.
    
}
//----------------------------------------------------------------



/** 
 * This is the generic interceptor function.
 * It should just log a message and call the original syscall.
 * 
 * TODO: Implement this function. 
 * - Check first to see if the syscall is being monitored for the current->pid. 
 * - Recall the convention for the "monitored" flag in the mytable struct: 
 *     monitored=0 => not monitored
 *     monitored=1 => some pids are monitored, check the corresponding my_list
 *     monitored=2 => all pids are monitored for this syscall
 * - Use the log_message macro, to log the system call parameters!
 *     Remember that the parameters are passed in the pt_regs registers.
 *     The syscall parameters are found (in order) in the 
 *     ax, bx, cx, dx, si, di, and bp registers (see the pt_regs struct).
 * - Don't forget to call the original system call, so we allow processes to proceed as normal.
 *
 */
asmlinkage long interceptor(struct pt_regs reg) {
    
    //Using synchronization, as we're now accessing data structures.
    spin_lock(&my_table_lock);
    
    if ((table[reg.ax].monitored == 2) || (check_pid_monitored(reg.ax, current->pid) == 1)) {
        log_message(current->pid, reg.ax, reg.bx, reg.cx, reg.dx, reg.si, reg.di, reg.bp);
    }
    
    spin_unlock(&my_table_lock);
    
	return table[reg.ax].f(reg); // The original system call is called here.
    
}

/**
 * My system call - this function is called whenever a user issues a MY_CUSTOM_SYSCALL system call.
 * When that happens, the parameters for this system call indicate one of 4 actions/commands:
 *      - REQUEST_SYSCALL_INTERCEPT to intercept the 'syscall' argument
 *      - REQUEST_SYSCALL_RELEASE to de-intercept the 'syscall' argument
 *      - REQUEST_START_MONITORING to start monitoring for 'pid' whenever it issues 'syscall' 
 *      - REQUEST_STOP_MONITORING to stop monitoring for 'pid'
 *      For the last two, if pid=0, that translates to "all pids".
 * 
 * TODO: Implement this function, to handle all 4 commands correctly.
 *
 * - For each of the commands, check that the arguments are valid (-EINVAL):
 *   a) the syscall must be valid (not negative, not > NR_syscalls, and not MY_CUSTOM_SYSCALL itself)
 *   b) the pid must be valid for the last two commands. It cannot be a negative integer, 
 *      and it must be an existing pid (except for the case when it's 0, indicating that we want 
 *      to start/stop monitoring for "all pids"). 
 *      If a pid belongs to a valid process, then the following expression is non-NULL:
 *           pid_task(find_vpid(pid), PIDTYPE_PID)
 * - Check that the caller has the right permissions (-EPERM)
 *      For the first two commands, we must be root (see the current_uid() macro).
 *      For the last two commands, the following logic applies:
 *        - is the calling process root? if so, all is good, no doubts about permissions.
 *        - if not, then check if the 'pid' requested is owned by the calling process 
 *        - also, if 'pid' is 0 and the calling process is not root, then access is denied 
 *          (monitoring all pids is allowed only for root, obviously).
 *      To determine if two pids have the same owner, use the helper function provided above in this file.
 * - Check for correct context of commands (-EINVAL):
 *     a) Cannot de-intercept a system call that has not been intercepted yet.
 *     b) Cannot stop monitoring for a pid that is not being monitored, or if the 
 *        system call has not been intercepted yet.
 * - Check for -EBUSY conditions:
 *     a) If intercepting a system call that is already intercepted.
 *     b) If monitoring a pid that is already being monitored.
 * - If a pid cannot be added to a monitored list, due to no memory being available,
 *   an -ENOMEM error code should be returned.
 *   
 *   NOTE: The order of the checks may affect the tester, in case of several error conditions
 *   in the same system call, so please be careful!
 *
 * - Make sure to keep track of all the metadata on what is being intercepted and monitored.
 *   Use the helper functions provided above for dealing with list operations.
 *
 * - Whenever altering the sys_call_table, make sure to use the set_addr_rw/set_addr_ro functions
 *   to make the system call table writable, then set it back to read-only. 
 *   For example: set_addr_rw((unsigned long)sys_call_table);
 *   Also, make sure to save the original system call (you'll need it for 'interceptor' to work correctly).
 * 
 * - Make sure to use synchronization to ensure consistency of shared data structures.
 *   Use the calltable_spinlock and pidlist_spinlock to ensure mutual exclusion for accesses 
 *   to the system call table and the lists of monitored pids. Be careful to unlock any spinlocks 
 *   you might be holding, before you exit the function (including error cases!).  
 */
asmlinkage long my_syscall(int cmd, int syscall, int pid) {
    
    //*************************************************************************
    //Error Conditions: Part A from the handout on course website.
    //For each of the commands, check the arguments are valid.
    
    //Immediately test whether the cmd is valid or not.
    if ((cmd != REQUEST_SYSCALL_INTERCEPT) && (cmd != REQUEST_SYSCALL_RELEASE) &&
        (cmd != REQUEST_START_MONITORING) && (cmd != REQUEST_STOP_MONITORING)) {
        return -EINVAL;
    }
    
    //Immediately test whether the syscall is valid or not.
    if ((syscall < 0) || (syscall > NR_syscalls) || (syscall == MY_CUSTOM_SYSCALL)) {
        return -EINVAL;
    }
    
    //Immediately test whether the pid is valid for the REQUEST_START_MONITORING & REQUEST_STOP_MONITORING commands.
    if ((cmd == REQUEST_START_MONITORING) || (cmd == REQUEST_STOP_MONITORING)) {
        if (pid != 0) {
            if ((pid < 0) || (pid_task(find_vpid(pid), PIDTYPE_PID) == NULL)) {
                return -EINVAL;
            }
        }
    }
    
    //*************************************************************************
    //Error Conditions: Part B from the handout on course website.
    //Check for the right permissions: -EPERM
    
    //Immediately test whether the caller has right permissions when issuing REQUEST_SYSCALL_INTERCEPT & REQUEST_SYSCALL_RELEASE commands.
    if ((cmd == REQUEST_SYSCALL_INTERCEPT) || (cmd == REQUEST_SYSCALL_RELEASE)) {
        if (current_uid() != 0) {
            return -EPERM;
        }
    }
    
    //Immediately test whether the caller has right permissions when issuing REQUEST_START_MONITORING & REQUEST_STOP_MONITORING commands.
    if ((cmd == REQUEST_START_MONITORING) || (cmd == REQUEST_STOP_MONITORING)) {
        if ((current_uid() != 0) && ((pid == 0) || (check_pids_same_owner(current->pid, pid) != 0))) {
            return -EPERM;
        }
    }
    
    //*************************************************************************
    //Synchronization begins because we are now accessing syscall tables (data structures).
    
    spin_lock(&my_table_lock);
    
    //*************************************************************************
    //Error Conditions: Parts C, D & E from the handout on course website.
    //Part C: Check for correct context of commands. -EINVAL
    //Part D: Check for -EBUSY conditions.
    //Part E: If a pid cannot be added to a monitored list, due to no memory being available, an -ENOMEM error code should be returned.
    
    //Check whether or not the syscall has been intercepted.
    if (table[syscall].intercepted != 0) {
        
        if (cmd == REQUEST_SYSCALL_INTERCEPT){
            //Test whether we are intercepting a syscall that has already been intercepted.
            spin_unlock(&my_table_lock);
            return -EBUSY;
        }
    } else if ((cmd == REQUEST_SYSCALL_RELEASE) || (cmd == REQUEST_STOP_MONITORING) ||
               (cmd == REQUEST_START_MONITORING)) { //Test whether the other commands are being used before the syscall has been intercepted.
        spin_unlock(&my_table_lock);
        return -EINVAL;
        
    }
    
    //***************************************************************************
    //***************************************************************************
    
    if (cmd == REQUEST_STOP_MONITORING) {
        
        //Test whether REQUEST_STOP_MONITORING is being called when no pids are being monitored.
        if ((pid == 0) && (table[syscall].monitored == 0)) {
            spin_unlock(&my_table_lock);
            return -EINVAL;
        }
        //Test whether REQUEST_STOP_MONITORING is being called on a pid that is not being monitored.
        if ((check_pid_monitored(syscall, pid) == 0) && (table[syscall].monitored != 2)) {
            spin_unlock(&my_table_lock);
            return -EINVAL;
        }
        //Bonus error condition.
        if ((check_pid_monitored(syscall, pid) == 1) && (table[syscall].monitored == 2)) {
            spin_unlock(&my_table_lock);
            return -EINVAL;
        }
        
    }
    
    //*****************************************************************************
    //*****************************************************************************
    
    //Check whether REQUEST_START_MONITORING is being called unnecessarily, when all pids are being monitored.
    if (cmd == REQUEST_START_MONITORING) {
        
        if ((pid == 0) && (table[syscall].monitored == 2) && (table[syscall].listcount == 0)) {
            spin_unlock(&my_table_lock);
            return -EBUSY;
        }
        if ((check_pid_monitored(syscall, pid) == 1) && (table[syscall].monitored == 2)) {
            spin_unlock(&my_table_lock);
            return -EINVAL;
        }
        if ((((check_pid_monitored(syscall, pid) == 1) && (table[syscall].monitored != 2)) ||
             ((check_pid_monitored(syscall, pid) == 0) && (table[syscall].monitored == 2)))) {
            spin_unlock(&my_table_lock);
            return -EBUSY;
        }

    }
    
    //******************************************************************************
    //******************************************************************************
    
    switch (cmd) {
            
        case REQUEST_SYSCALL_INTERCEPT:
            spin_lock(&sys_call_table_lock);
            
            table[syscall].f = sys_call_table[syscall];
            table[syscall].intercepted = 1;
            
            spin_unlock(&my_table_lock);
            
            set_addr_rw((unsigned long) sys_call_table);
            sys_call_table[syscall] = interceptor;
            set_addr_ro((unsigned long) sys_call_table);
            
            spin_unlock(&sys_call_table_lock);
            
            break;
            
        case REQUEST_SYSCALL_RELEASE:
            
            spin_lock(&sys_call_table_lock);
            
            set_addr_rw((unsigned long) sys_call_table);
            
            table[syscall].intercepted = 0;
            sys_call_table[syscall] = table[syscall].f;
            
            spin_unlock(&my_table_lock);
            
            set_addr_ro((unsigned long) sys_call_table);
            
            spin_unlock(&sys_call_table_lock);
            
            break;
            
        case REQUEST_START_MONITORING:
            
            if (pid == 0) {
                destroy_list(syscall);
                table[syscall].monitored = 2;
            } else if (table[syscall].monitored != 2) {
                if (add_pid_sysc(pid, syscall) != 0) {
                    spin_unlock(&my_table_lock);
                    return -ENOMEM; //Error Conditions from Handout: Part E
                }
                table[syscall].monitored = 1;
            } else {
                if (del_pid_sysc(pid, syscall) != 0) {
                    spin_unlock(&my_table_lock);
                    return -EINVAL;
                }
            }
            spin_unlock(&my_table_lock);
            
            break;
            
        case REQUEST_STOP_MONITORING:
            
            if (pid == 0) {
                destroy_list(syscall);
            } else if (table[syscall].monitored == 2) {
                if (add_pid_sysc(pid, syscall) != 0) {
                    spin_unlock(&my_table_lock);
                    return -ENOMEM; //Error Conditions from Handout: Part E
                }
            } else {
                if (del_pid_sysc(pid, syscall) != 0) {
                    spin_unlock(&my_table_lock);
                    return -EINVAL;
                }
            }
            spin_unlock(&my_table_lock);
            
            break;
            
        default:
            
            break;
            
    }

	return 0;
    
}

/**
 *
 */
long (*orig_custom_syscall)(void);


/**
 * Module initialization. 
 *
 * TODO: Make sure to:  
 * - Hijack MY_CUSTOM_SYSCALL and save the original in orig_custom_syscall.
 * - Hijack the exit_group system call (__NR_exit_group) and save the original 
 *   in orig_exit_group.
 * - Make sure to set the system call table to writable when making changes, 
 *   then set it back to read only once done.
 * - Perform any necessary initializations for bookkeeping data structures.
 *   To initialize a list, use 
 *        INIT_LIST_HEAD (&some_list);
 *   where some_list is a "struct list_head". 
 * - Ensure synchronization as needed.
 */
static int init_function(void)
{
    
    int x = 0; //Variable decleration at the beginning prevents compiler warning.
    
    //Syscall table
    
    spin_lock(&sys_call_table_lock);
    
    orig_custom_syscall = sys_call_table[MY_CUSTOM_SYSCALL];
    orig_exit_group = sys_call_table[__NR_exit_group];
    
    set_addr_rw((unsigned long) sys_call_table); //Allow the syscall table to become readable and writable.
    
    sys_call_table[MY_CUSTOM_SYSCALL] = my_syscall;
    sys_call_table[__NR_exit_group] = my_exit_group;
    
    set_addr_ro((unsigned long) sys_call_table); //Set the syscall table to read-only.
    
    spin_unlock(&sys_call_table_lock);
    
    //My Table
    
    spin_lock(&my_table_lock);
    
    while (x < NR_syscalls) {
        table[x].intercepted = 0;
        table[x].monitored = 0;
        table[x].listcount = 0;
        INIT_LIST_HEAD(&(table[x].my_list));
        x++;
    }
    
    spin_unlock(&my_table_lock);

	return 0;
    
}

/**
 * Module exits. 
 *
 * TODO: Make sure to:  
 * - Restore MY_CUSTOM_SYSCALL to the original syscall.
 * - Restore __NR_exit_group to its original syscall.
 * - Make sure to set the system call table to writable when making changes, 
 *   then set it back to read only once done.
 * - Make sure to deintercept all syscalls, and cleanup all pid lists.
 * - Ensure synchronization, if needed.
 */
static void exit_function(void)
{
    
    int y = 0; //Variable decleration at the beginning prevents compiler warning.
    
    //My Table
    
    spin_lock(&my_table_lock);
    
    while (y < NR_syscalls) {
        if (table[y].intercepted == 1) {
            spin_unlock(&my_table_lock);
            my_syscall(REQUEST_SYSCALL_RELEASE, y, y);
            spin_lock(&my_table_lock);
        }
        y++;
    }
    
    spin_unlock(&my_table_lock);
    
    //Syscall table
    
    spin_lock(&sys_call_table_lock);
    
    set_addr_rw((unsigned long) sys_call_table); //Allow the syscall table to become readable and writable.
    
    sys_call_table[MY_CUSTOM_SYSCALL] = orig_custom_syscall;
    sys_call_table[__NR_exit_group] = orig_exit_group;
    
    set_addr_ro((unsigned long) sys_call_table); //Set the syscall table to read-only.
    
    spin_unlock(&sys_call_table_lock);

}

module_init(init_function);
module_exit(exit_function);

