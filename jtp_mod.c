//
// Created by Александр Красавцев on 24.03.2026.
//
#include <linux/types.h>

struct jtp_hdr {
    __be16  source;       /* Порт отправителя (как в NASA SDTP) */
    __be16  dest;         /* Порт получателя */
    __be64  data_offset;  /* Смещение в байтах — поддержка файлов > 4ГБ */
    __be32  chunk_id;     /* Номер чанка для контроля пропусков */
    __sum16 check;        /* Контрольная сумма для целостности */
};

/* * NASA-inspired: Мы используем 64-битный offset,
 * чтобы Jerboa мог адресовать до 16 эксабайт данных
 * в рамках одного потока без переполнения счетчика.
 */
#include <linux/module.h>
#include <linux/netprotoprio.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <net/protocol.h>

#define MAX_JTP_PROTO 254 // Наш экспериментальный номер протокола

// Обработчик входящих пакетов JTP
int jtp_rcv(struct sk_buff *skb) {
    struct iphdr *iph = ip_hdr(skb);
    static unsigned long total_bytes = 0;

    total_bytes += skb->len;

    // Каждые 100МБ выводим лог (для теста 1ГБ)
    if (total_bytes % (100 * 1024 * 1024) == 0) {
        printk(KERN_INFO "JTP: Received chunk. Total: %lu MB\n", total_bytes / 1024 / 1024);
    }

    kfree_skb(skb); // Освобождаем буфер
    return 0;
}

// Структура обработчика для стека Linux
static const struct net_protocol jtp_protocol = {
    .handler = jtp_rcv,
    .no_policy = 1,
};

static int __init jtp_init(void) {
    if (inet_add_protocol(&jtp_protocol, MAX_JTP_PROTO) < 0) {
        printk(KERN_ERR "JTP: Failed to register protocol\n");
        return -1;
    }
    printk(KERN_INFO "JTP: Jerboa Protocol Module Loaded (Proto ID: %d)\n", MAX_JTP_PROTO);
    return 0;
}

static void __exit jtp_exit(void) {
    inet_del_protocol(&jtp_protocol, MAX_JTP_PROTO);
    printk(KERN_INFO "JTP: Jerboa Protocol Module Unloaded\n");
}

module_init(jtp_init);
module_exit(jtp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Krasavcev & Gemini");