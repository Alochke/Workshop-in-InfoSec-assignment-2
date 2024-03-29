#include "hw2secws.h"

MODULE_SIGNATURE

/* All nf_hook_ops will be pointed by the hooks pointer, so it'll function as an array (we'll use hooks[i] to refer to hook i.) */
static struct nf_hook_ops *hooks;
unsigned int accepted = 0; /* An enumerator for the accepted number of packets. */
unsigned int dropped = 0;  /* An enumerator for the dropped number of packets. */


/* 
    The dropped packet handling procedure.
    This function also increments the dropped packets counter.

    Returns: NF_DROP which is defined in netfilter.h and makes the handler drop the packet.
*/
static unsigned int nf_forward_fn(void* priv, struct sk_buff *skb, const struct nf_hook_ops *state)
{
    printk(BLOCK_MESSAGE);
    dropped++;
    return NF_DROP;
}

/* 
    The allowed packet handling procedure.
    This function also increments the accepted packet counter.

    Returns: NF_ACCEPT which is defined in netfilter.h and makes the handler accept the packet.
*/
static unsigned int nf_local_fn(void* priv, struct sk_buff *skb, const struct nf_hook_ops *state)
{
    printk(ALLOW_MESSAGE);
    accepted++;
    return NF_ACCEPT;
}

/*
    Deletes hooks with indecies less than max from existance and frees their kernel allocated memory.
    
    Parameters:
    - max (size_t): Hooks with indecies in [0,max] will be unregisterred.
*/
static void destroy_hooks(size_t max)
{
    size_t i;   /* for loop's index */
    for (i = 0; i < max; i++)
    {
        nf_unregister_net_hook(&init_net, &hooks[i]);
    }
    kfree(hooks);
}

/*
    The module initialization function.
    
    Returns: -ENOMEM in case it was not able to allocate memory for the nf_hook_ops structs, 0 on success and the value returned by the function that failed in any case a function called by LKM _init failed.
*/
static int __init LKM_init(void)
{
    size_t i;   /* for loop's index */
    int err;    /* we'll use that variable at the end of the function definition to save the return value of nf_register_net_hook, in case it'll error and we'll return its return value. */
    
    ERR_CHECK((hooks = kmalloc(sizeof(struct nf_hook_ops) * HOOKS_NUM, GFP_KERNEL)) == NULL, printk(KERN_ERR "nf_register_net_hook failed.") , -ENOMEM)

    for(i = 0; i < HOOKS_NUM; i++)
    {
        hooks[i].pf = PF_INET;                      /* IPv4 */
        hooks[i].priority = NF_IP_PRI_FIRST;		/* max hook priority */
        switch (i)                                  /* Netfilter hook point and hook function */
        {
        case FORWARD:
            hooks[i].hook = (nf_hookfn*) nf_forward_fn;
            hooks[i].hooknum = NF_IP_FORWARD;
            break;
        default:
            hooks[i].hook = (nf_hookfn*) nf_local_fn;
            switch (i)
            {
            case IN:
                hooks[i].hooknum = NF_IP_LOCAL_IN;
                break;
            default:
                hooks[i].hooknum = NF_IP_LOCAL_OUT; /* The only hook point left possible */
            }
        }
        ERR_CHECK((err = nf_register_net_hook(&init_net, &hooks[i])) < 0, destroy_hooks(i); printk(KERN_ERR "nf_register_net_hook failed.");, err);
    }
    ERR_CHECK((err = sysfs_init()), destroy_hooks(HOOKS_NUM);, err)
    return SUCCESS;
}

/* module removal method. */
static void __exit LKM_exit(void)
{
    sysfs_exit();
    destroy_hooks(HOOKS_NUM);
}

module_init(LKM_init);
module_exit(LKM_exit);