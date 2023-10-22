#include "sphinx.h"

void print_hex_memory(void *mem, uint16_t mem_size)
{
    unsigned char *p = (unsigned char *) mem;

    for (uint16_t i=0; i<mem_size -1; i++) {
        if (! (i % 16) && i) {
            printf("\n");
        }
        printf("0x%02x, ", p[i]);
    }
    printf("0x%02x\n\n", p[mem_size-1]);
}

void print_id(unsigned char *id)
{
    printf("sphinx: ");
    for (uint8_t i=0; i<ID_SIZE; i++) {
        printf("%02x", id[i]);
    }
    printf(": ");
}

int8_t get_local_ipv6_addr(ipv6_addr_t *result)
{
    netif_t *netif;
    ipv6_addr_t addrs[1];

    netif = netif_iter(NULL);

    if ((netif_get_ipv6(netif, addrs, ARRAY_SIZE(addrs))) < 0) {
        return -1;
    }

    *result = addrs[0];
    return 1;
}

int8_t udp_send(ipv6_addr_t *dest_addr, unsigned char *message, size_t message_size)
{
    ssize_t res = EADDRINUSE;
    /* set up remote endpoint */
    sock_udp_ep_t remote = { .family = AF_INET6 };
    remote.port = SPHINX_PORT;
    memcpy(remote.addr.ipv6, dest_addr, sizeof(ipv6_addr_t));

    /* send message */
    if ((res = sock_udp_send(NULL, message, message_size, &remote)) < 0) {
        printf("sphinx: sock_udp_send: error %d\n", res);
        return -1;
    }

    return 1;
}

void hash_blinding_factor(unsigned char *dest, unsigned char *public_key, unsigned char *sharde_secret)
{
    unsigned char hash_input[2 * KEY_SIZE];
    unsigned char hash[crypto_hash_BYTES];

    memcpy(&hash_input[0], public_key, KEY_SIZE);
    memcpy(&hash_input[KEY_SIZE], sharde_secret, KEY_SIZE);
    crypto_hash(hash, hash_input, sizeof(hash_input));
    memcpy(dest, &hash, KEY_SIZE);
}

void hash_shared_secret(unsigned char *dest, unsigned char *raw_sharde_secret)
{
    unsigned char hash[crypto_hash_BYTES];
    crypto_hash(hash, raw_sharde_secret, KEY_SIZE);
    memcpy(dest, &hash, KEY_SIZE);
}

void xor_backwards_inplace(unsigned char *dest, size_t dest_size, unsigned char *arg, size_t arg_size, uint16_t num_bytes)
{
    for (int i=1; i<=num_bytes; i++) {
        dest[dest_size - i] = dest[dest_size -  i] ^ arg[arg_size - i];
    }
}

void delete_sent_message(int i)
{   
    if (i < sent_msg_count - 1) {
        memmove(&sent_msg_table[i], &sent_msg_table[i+1], (sent_msg_count - i - 1) * sizeof(event_send));
    }
    sent_msg_count--;
}