//
// Created by Александр Красавцев on 24.03.2026.
//
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#include <net/protocol.h>
#include <net/inet_common.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define MAX_JTP_PROTO 254

/* Структура заголовка Jerboa (NASA-inspired) */
struct jtp_hdr {
    __be16  source;
    __be16  dest;
    __be64  data_offset;  /* 64-бит для работы с огромными данными */
    __be32  chunk_id;
    __sum16 check;
};

// Обработчик входящих пакетов
static atomic64_t jtp_total_bytes = ATOMIC64_INIT(0);
static atomic64_t jtp_packets_count = ATOMIC64_INIT(0);

// Обработчик входящих пакетов
int jtp_rcv(struct sk_buff *skb) {
    atomic64_add(skb->len, &jtp_total_bytes);
    atomic64_inc(&jtp_packets_count);

    consume_skb(skb);
    return 0;
}

// Вывод статистики в /proc/jtp_stats
static int jtp_stats_show(struct seq_file *m, void *v) {
    u64 bytes = atomic64_read(&jtp_total_bytes);
    u64 pkts = atomic64_read(&jtp_packets_count);

    seq_printf(m, "--- Jerboa Transport Protocol (JTP) Stats ---\n");
    seq_printf(m, "Total Received: %llu bytes (%llu MB)\n", bytes, bytes >> 20);
    seq_printf(m, "Packets Count:  %llu\n", pkts);
    seq_printf(m, "Status:         Active (NASA-mode: On)\n");
    return 0;
}

static int jtp_stats_open(struct inode *inode, struct file *file) {
    return single_open(file, jtp_stats_show, NULL);
}

static const struct proc_ops jtp_proc_ops = {
    .proc_open = jtp_stats_open,
    .proc_read = seq_read,
    .proc_release = single_release,
};

static int __init jtp_init(void) {
    // В ядре 6.8 регистрация протокола осталась прежней
    if (inet_add_protocol(&jtp_protocol, MAX_JTP_PROTO) < 0) {
        pr_err("JTP: Failed to register protocol\n");
        return -1;
    }
    pr_info("JTP: Jerboa Protocol Module Loaded on 6.8.0 (Proto: %d)\n", MAX_JTP_PROTO);
    proc_create("jtp_stats", 0, NULL, &jtp_proc_ops);
    return 0;
}

static void __exit jtp_exit(void) {
    inet_del_protocol(&jtp_protocol, MAX_JTP_PROTO);
    pr_info("JTP: Jerboa Protocol Module Unloaded\n");
    remove_proc_entry("jtp_stats", NULL);
}

module_init(jtp_init);
module_exit(jtp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Krasavcev");
MODULE_DESCRIPTION("Jerboa Transport Protocol Kernel Module");