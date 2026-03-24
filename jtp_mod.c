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
int jtp_rcv(struct sk_buff *skb) {
    struct jtp_hdr *jtp_h;
    static unsigned long total_bytes = 0;

    // Проверка: достаточно ли данных для нашего заголовка?
    if (!pskb_may_pull(skb, sizeof(struct jtp_hdr))) {
        kfree_skb(skb);
        return 0;
    }

    jtp_h = (struct jtp_hdr *)skb_transport_header(skb);
    total_bytes += skb->len;

    // Логируем прогресс каждые 100МБ
    if (total_bytes % (100 * 1024 * 1024) == 0) {
        printk(KERN_INFO "JTP: Received chunk %u. Total: %lu MB\n",
               be32_to_cpu(jtp_h->chunk_id), total_bytes / 1024 / 1024);
    }

    consume_skb(skb); // Правильный способ освобождения в новых ядрах
    return 0;
}

static const struct net_protocol jtp_protocol = {
    .handler = jtp_rcv,
    .no_policy = 1,
};

static int __init jtp_init(void) {
    // В ядре 6.8 регистрация протокола осталась прежней
    if (inet_add_protocol(&jtp_protocol, MAX_JTP_PROTO) < 0) {
        pr_err("JTP: Failed to register protocol\n");
        return -1;
    }
    pr_info("JTP: Jerboa Protocol Module Loaded on 6.8.0 (Proto: %d)\n", MAX_JTP_PROTO);
    return 0;
}

static void __exit jtp_exit(void) {
    inet_del_protocol(&jtp_protocol, MAX_JTP_PROTO);
    pr_info("JTP: Jerboa Protocol Module Unloaded\n");
}

module_init(jtp_init);
module_exit(jtp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Krasavcev");
MODULE_DESCRIPTION("Jerboa Transport Protocol Kernel Module");