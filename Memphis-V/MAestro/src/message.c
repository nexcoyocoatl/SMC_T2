/**
 * MAestro
 * @file message.c
 * 
 * @author Angelo Elias Dalzotto (angelo.dalzotto@edu.pucrs.br)
 * GAPH - Hardware Design Support Group (https://corfu.pucrs.br/)
 * PUCRS - Pontifical Catholic University of Rio Grande do Sul (http://pucrs.br/)
 * 
 * @date May 2025
 * 
 * @brief Message protocol
 */

#include <message.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <dmni.h>
#include <mmr.h>
#include <task_control.h>
#include <halt.h>
#include <kernel_pipe.h>
#include <rpc.h>
#include <llm.h>
#include <task_migration.h>

#include <memphis.h>
#include <memphis/services.h>
#include <memphis/monitor.h>
#include <memphis/messaging.h>

list_t _msg_pndg;

/**
 * @brief Forwards a DATA_AV/MESSAGE_REQUEST in case of migration
 * 
 * @param hdshk Pointer to packet to forward
 * @param task Task that migrated
 * 
 * @return int
 *  0 on success
 * -EINVAL: task not migrated
 * -ENOMEM: could not create outbound packet
 */
int _msg_forward_hdshk(msg_hdshk_t *hdshk, uint16_t task);

/**
 * @brief Updates the task location in case of migration
 * 
 * @param tcb Pointer to the TCB to update
 * @param source Source of the incoming message
 * @param task Task of the incoming message
 * @param src_app App of the incoming message
 */
void _msg_update_tl(tcb_t *tcb, uint32_t source, int16_t task, int8_t src_app);

void msg_pndg_init()
{
    list_init(&_msg_pndg);
}

list_entry_t *msg_pndg_push_back(msg_hdshk_t *hdshk)
{
    list_entry_t *entry = list_push_back(&_msg_pndg, hdshk);
	if (entry == NULL) 
		return NULL;

	MMR_DMNI_IRQ_IP |= (1 << DMNI_IP_PENDING);
	return entry;
}

msg_hdshk_t *msg_pndg_pop_front()
{
	msg_hdshk_t *ret = list_pop_front(&_msg_pndg);

	if (list_empty(&_msg_pndg))
		MMR_DMNI_IRQ_IP &= ~(1 << DMNI_IP_PENDING);
    
    return ret;
}

bool msg_pndg_empty()
{
	return list_empty(&_msg_pndg);
}

int msg_recv_data_av(msg_hdshk_t *hdshk)
{
    // printf("A %x->%x\n", hdshk->sender, hdshk->receiver);

    // printf("Source: %x\n", hdshk->source);
    // printf("Flags: %x | Target: %x\n", hdshk->hermes.flags, hdshk->hermes.address);

    int8_t recv_app = (hdshk->receiver >> 8);
    if (recv_app == -1) /* This message was directed to kernel */
        return msg_send_hdshk(MMR_DMNI_INF_ADDRESS, hdshk->source, hdshk->sender, hdshk->receiver, MESSAGE_REQUEST);

    tcb_t *recv_tcb = tcb_find(hdshk->receiver);
    if (recv_tcb == NULL)   /* Task migrated? Forward. */
        return _msg_forward_hdshk(hdshk, hdshk->receiver);

    /* Update task location in case of migration */
    _msg_update_tl(recv_tcb, hdshk->source, hdshk->sender, recv_app);

    /* Insert the packet to TCB */
    list_t *davs = tcb_get_davs(recv_tcb);
    tl_t   *dav  = tl_emplace_back(davs, hdshk->sender, hdshk->source);
    if (dav == NULL) {
        // printf("*************** NO MEMORY TO STORE DAV \n");
        return -ENOMEM;
    }

    MMR_DBG_ADD_DAV = (hdshk->sender << 16) | (hdshk->receiver & 0xFFFF);

    /* If the consumer task is waiting for a DATA_AV, release it */
    sched_t *sched = tcb_get_sched(recv_tcb);
    if (sched_is_waiting_dav(sched)) {
        sched_release_wait(sched);
        return sched_is_idle();
    }

    return 0;
}

int msg_recv_message_request(msg_hdshk_t *hdshk)
{
    // printf("R %x->%x\n", hdshk->sender, hdshk->receiver);

    const int8_t send_app = (hdshk->sender >> 8);
    if (send_app == -1) {
        /* This message was directed to kernel */
        /* WARN: Never request directly to kernel. Always use DATA_AV first. */
        
        /* Search for the kernel-produced message */
		opipe_t *opipe = kpipe_find(hdshk->receiver);
		if (opipe == NULL)
			return -ENODATA;

		/* Send it like a MESSAGE_DELIVERY */
        int ret = msg_send_message_delivery(opipe->buf, opipe->size, MMR_DMNI_INF_ADDRESS, hdshk->source, hdshk->sender, hdshk->receiver);
        MMR_DBG_REM_PIPE = (hdshk->sender << 16) | (hdshk->receiver & 0xFFFF);

		kpipe_remove(opipe);

        if (halt_pndg()) {
            if (halt_try() == 0)
                halt_clear();
        }

        return ret;
    }

    tcb_t *send_tcb = tcb_find(hdshk->sender);
    if (send_tcb == NULL)   /* Task migrated? Forward. */
        return _msg_forward_hdshk(hdshk, hdshk->sender);

    // printf("Sender is task %d\n", send_tcb->id);

    /* Update task location in case of migration */
    _msg_update_tl(send_tcb, hdshk->source, hdshk->receiver, send_app);

    /* Task found. Now search for message. */
	opipe_t *opipe = tcb_get_opipe(send_tcb);
    int receiver_id = hdshk->receiver;
    if (receiver_id == -1) {
        receiver_id = hdshk->source;
        if (hdshk->source & MEMPHIS_FORCE_PORT)
            receiver_id |= MEMPHIS_FORCE_PORT;
        else
            receiver_id |= MEMPHIS_KERNEL_MSG;
    }

    if ((opipe == NULL) || (opipe_get_receiver(opipe) != receiver_id)) {
        /* No message in producer's pipe to the consumer task */
		/* Insert the message request in the producer's TCB */
		// printf("Message not found. Inserting message request.\n");
		list_t *msgreqs = tcb_get_msgreqs(send_tcb);
		tl_emplace_back(msgreqs, hdshk->receiver, hdshk->source);
		MMR_DBG_ADD_REQ = (hdshk->sender << 16) | (hdshk->receiver & 0xFFFF);
        return 0;
    }

    // printf("Pipe is present!\n");

    if (hdshk->source == MMR_DMNI_INF_ADDRESS) {
        /* MESSAGE_REQUEST came from NoC but the receiver migrated to this address */
		/* Writes to the consumer page address */
		tcb_t *recv_tcb = tcb_find(hdshk->receiver);
		if (recv_tcb == NULL) 
            return -EINVAL;

		ipipe_t *ipipe = tcb_get_ipipe(recv_tcb);
		if (ipipe == NULL)
            return -EINVAL;

		size_t buf_size;
		void *src = opipe_get_buf(opipe, &buf_size);

		int result = ipipe_transfer(
			ipipe, 
			tcb_get_offset(recv_tcb), 
			src, 
			buf_size
		);
		if (result < 0)
			return result;

        MMR_DBG_REM_PIPE = (hdshk->sender << 16) | (hdshk->receiver & 0xFFFF);
		opipe_pop(opipe);
		tcb_destroy_opipe(send_tcb);

		/* Release consumer task */
		sched_t *sched = tcb_get_sched(recv_tcb);
		sched_release_wait(sched);

		if (tcb_need_migration(recv_tcb)) {
			tm_migrate(recv_tcb);
			return 1;
		}

        return sched_is_idle();
    }

	/* Send through NoC */
    int ret = msg_send_message_delivery(opipe->buf, opipe->size, MMR_DMNI_INF_ADDRESS, hdshk->source, hdshk->sender, hdshk->receiver);
    if (ret < 0)
        return ret;

    tcb_destroy_opipe(send_tcb);

	/* Release task for execution if it was blocking another send */
	sched_t *sched = tcb_get_sched(send_tcb);
	if (sched_is_waiting_msgreq(sched)) {
		sched_release_wait(sched);
		if (tcb_has_called_exit(send_tcb))
			tcb_terminate(send_tcb);
        return sched_is_idle();
	}

    return 0;
}

int msg_recv_message_delivery(msg_dlv_t *dlv)
{
    // printf("D %x->%x\n", dlv->hdshk.sender, dlv->hdshk.receiver);

    int8_t recv_app = (dlv->hdshk.receiver >> 8);
    if (recv_app == -1) {
        // printf("Kernel message!\n");
		/* This message was directed to kernel */
		size_t align_size = (dlv->size + 3) & ~3;
		void *rcvmsg = malloc(align_size);
		dmni_recv(rcvmsg, align_size);

		/* Process the message like a syscall triggered from another PE */
		int ret = rpc_hermes_dispatcher(rcvmsg, dlv->size);

		free(rcvmsg);

		return ret;
	}

    tcb_t *recv_tcb = tcb_find(dlv->hdshk.receiver);
    if (recv_tcb == NULL) {
        /* @todo Create an exception and abort task? */
        // printf("TASK NOT FOUND\n");
        return -EINVAL;
    } 

    ipipe_t *ipipe = tcb_get_ipipe(recv_tcb);
    if (ipipe == NULL) {
        /* @todo Create an exception and abort task? */
        // printf("IPIPE NOT FOUND\n");
		return -EINVAL;
    }

    /* Update task location in case of migration */
    _msg_update_tl(recv_tcb, dlv->hdshk.source, dlv->hdshk.sender, recv_app);

    int result = ipipe_receive(ipipe, tcb_get_offset(recv_tcb), dlv->size);
    if (result != dlv->size) {
        // printf("Returned %d from ipipe_receive\n", result);
		dmni_drop_payload(dlv->size - result);
    }

    /* @todo Monitor only if message was not redirected from migration */
    int8_t send_app = (dlv->hdshk.sender >> 8);
    if (llm_has_monitor(MON_SEC) && recv_app != 0 && send_app != 0) {
		llm_sec(
            dlv->timestamp, 
            (dlv->size + sizeof(msg_dlv_t))/4, 
            dlv->hdshk.source, 
            MMR_DMNI_INF_ADDRESS, 
            dlv->hdshk.sender, 
            dlv->hdshk.receiver, 
            MMR_DMNI_HERMES_TIMESTAMP
        );
    }

    if (llm_has_monitor(MON_VOL) && recv_app != 0)
    {
        llm_vol(
            dlv->hdshk.source,
            MMR_DMNI_INF_ADDRESS,
            (dlv->size + sizeof(msg_dlv_t))/4
        );
    }

    sched_t *sched = tcb_get_sched(recv_tcb);
    sched_release_wait(sched);

    if (tcb_need_migration(recv_tcb)) {
        tm_migrate(recv_tcb);
        return 1;
    }

    return sched_is_idle();
}

int msg_send_hdshk(uint32_t source, uint32_t target, uint16_t sender, uint16_t receiver, uint8_t service)
{
    // printf("* %x->%x %c\n", receiver, sender, (service == MESSAGE_REQUEST) ? 'R' : 'A');
    msg_hdshk_t *hdshk = malloc(sizeof(msg_hdshk_t));
    if (hdshk == NULL)
        return -ENOMEM;

    hdshk->hermes.flags   = (target >> 24);
    hdshk->hermes.service = service;
    hdshk->hermes.address = target;
    hdshk->source         = source;
    hdshk->sender         = sender;
    hdshk->receiver       = receiver;

    return dmni_send(hdshk, sizeof(msg_hdshk_t), true, NULL, 0, false);
}

int msg_send_message_delivery(void *pld, size_t size, uint32_t source, uint32_t target, uint16_t sender, uint16_t receiver)
{
    // printf("* %x->%x D\n", sender, receiver);
    msg_dlv_t *dlv = malloc(sizeof(msg_dlv_t));
    if (dlv == NULL)
        return -ENOMEM;

    dlv->hdshk.hermes.flags   = (target >> 24);
    dlv->hdshk.hermes.service = MESSAGE_DELIVERY;
    dlv->hdshk.hermes.address = target;
    dlv->hdshk.source         = source;
    dlv->hdshk.sender         = sender;
    dlv->hdshk.receiver       = receiver;
    dlv->size                 = size;

	size_t align_size = (size + 3) & ~3;

    /* Wait for DMNI release before inserting timestamp */
    while((MMR_DMNI_IRQ_STATUS & (1 << DMNI_STATUS_SEND_ACTIVE)));
    dlv->timestamp            = MMR_RTC_MTIME;

	return dmni_send(dlv, sizeof(msg_dlv_t), true, pld, align_size, true);
}

int _msg_forward_hdshk(msg_hdshk_t *hdshk, uint16_t task)
{
    tl_t *mig = tm_find(task);
    if (mig == NULL)
        return -EINVAL;

    // /* Forward the MESSAGE_REQUEST to the migrated processor */
    uint32_t migrated_addr = tl_get_addr(mig);
    return msg_send_hdshk(hdshk->source, migrated_addr, hdshk->sender, hdshk->receiver, hdshk->hermes.service);
}

void _msg_update_tl(tcb_t *tcb, uint32_t source, int16_t task, int8_t src_app)
{
    int8_t task_app = (task >> 8);
    if (task_app != -1 && task_app == src_app) {
        /* Only update if message came from another task of the same app */
        app_t *app = tcb_get_app(tcb);
        app_update(app, task, source);
    }
}
