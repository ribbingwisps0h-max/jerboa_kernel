//
// Created by Александр Красавцев on 24.03.2026.
//
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#include <net/protocol.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define MAX_JTP_PROTO 254

/* Статистика в атомарных переменных для многопоточности */
static atomic64_t jtp_total_bytes = ATOMIC64_INIT(0);
static atomic64_t jtp_packets_count = ATOMIC64_INIT(0);

/* Структура заголовка Jerboa */
struct jtp_hdr {
    __be16  source;
    __be16  dest;
    __be64  data_offset;
    __be32  chunk_id;
    __sum16 check;
};

/* Прототип функции, чтобы компилятор не ругался на missing-prototypes */
int jtp_rcv(struct sk_buff *skb);

/* Обработчик входящих пакетов */
int jtp_rcv(struct sk_buff *skb) {
    if (!pskb_may_pull(skb, sizeof(struct jtp_hdr))) {
        kfree_skb(skb);
        return 0;
    }

    /* Обновляем статистику */
    atomic64_add(skb->len, &jtp_total_bytes);
    atomic64_inc(&jtp_packets_count);

    consume_skb(skb);
    return 0;
}

/* Структура протокола должна быть объявлена ДО jtp_init */
static const struct net_protocol jtp_protocol = {
    .handler = jtp_rcv,
    .no_policy = 1,
};

/* --- Работа с /proc/jtp_stats --- */

static int jtp_stats_show(struct seq_file *m, void *v) {
    u64 bytes = atomic64_read(&jtp_total_bytes);
    u64 pkts = atomic64_read(&jtp_packets_count);

    seq_printf(m, "--- Jerboa Transport Protocol (JTP) Stats ---\n");
    seq_printf(m, "Total Received: %llu bytes (%llu MB)\n", bytes, bytes >> 20);
    seq_printf(m, "Packets Count:  %llu\n", pkts);
    seq_printf(m, "NASA-mode:      Enabled (64-bit offsets)\n");
    return 0;
}

static int jtp_stats_open(struct inode *inode, struct file *file) {
    return single_open(file, jtp_stats_show, NULL);
}

/* В ядре 6.8 используется proc_ops вместо file_operations для /proc */
static const struct proc_ops jtp_proc_ops = {
    .proc_open    = jtp_stats_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* --- Инициализация и выход --- */

static int __init jtp_init(void) {
    // 1. Регистрируем протокол в сетевом стеке
    if (inet_add_protocol(&jtp_protocol, MAX_JTP_PROTO) < 0) {
        pr_err("JTP: Failed to register protocol\n");
        return -1;
    }

    // 2. Создаем файл статистики в /proc
    if (!proc_create("jtp_stats", 0444, NULL, &jtp_proc_ops)) {
        inet_del_protocol(&jtp_protocol, MAX_JTP_PROTO);
        pr_err("JTP: Failed to create /proc/jtp_stats\n");
        return -1;
    }

    pr_info("JTP: Jerboa Protocol Module Loaded (Proto: %d)\n", MAX_JTP_PROTO);
    return 0;
}

static void __exit jtp_exit(void) {
    remove_proc_entry("jtp_stats", NULL);
    inet_del_protocol(&jtp_protocol, MAX_JTP_PROTO);
    pr_info("JTP: Jerboa Protocol Module Unloaded\n");
}

module_init(jtp_init);
module_exit(jtp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Krasavcev");
MODULE_DESCRIPTION("Jerboa Transport Protocol Kernel Module");