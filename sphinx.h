/* accumulated includes */
#include <stdio.h>
#include <string.h>

#include "net/ipv6/addr.h"
#include "kernel_defines.h"
#include "net/netif.h"
#include "net/ipv6/addr.h"
#include "net/sock/udp.h"
#include "net/sock/async/event.h"
#include "event.h"
#include "shell.h"
#include "thread.h"
#include "random.h"
#include "xtimer.h"

#include "tweetnacl.h"

/* sphinx network metrics */
#define SPHINX_PORT 45678
#define SPHINX_NET_SIZE ARRAY_SIZE(network_pki)
#define SPHINX_MAX_PATH 5

/* sphinx format metrics */
#define KEY_SIZE 32
#define ADDR_SIZE 16
#define MAC_SIZE 16
#define ID_SIZE 16
#define PAYLOAD_SIZE 128
#define NODE_ROUTING_SIZE (ADDR_SIZE + MAC_SIZE)
#define ENC_ROUTING_SIZE (SPHINX_MAX_PATH * NODE_ROUTING_SIZE)
#define NODE_FILLER_PADDING_SIZE NODE_ROUTING_SIZE
#define MAX_FILLER_STRING_SIZE (SPHINX_MAX_PATH * NODE_FILLER_PADDING_SIZE)
#define HEADER_SIZE (KEY_SIZE + MAC_SIZE + ENC_ROUTING_SIZE)
#define SURB_SIZE (ADDR_SIZE + MAC_SIZE + ENC_ROUTING_SIZE)
#define PRG_STREAM_SIZE ( ENC_ROUTING_SIZE + NODE_FILLER_PADDING_SIZE + MAC_SIZE + SURB_SIZE + PAYLOAD_SIZE)
#define SPHINX_MESSAGE_SIZE (HEADER_SIZE + MAC_SIZE + SURB_SIZE + PAYLOAD_SIZE)

/* mix node metrics */
#define TAG_SIZE 4
#define TAG_TABLE_LEN 128
#define SENT_MSG_TABLE_SIZE 10
#define EVENT_TIMEOUT_US (1U * US_PER_SEC)
#define MSG_TIMEOUT_US (120U * US_PER_SEC)
#define MAX_TRANSMITS 3

/* readability */
#define CUT_OFF 16

/* types */
typedef struct {
    clist_node_t list_node;     
    event_handler_t handler; 
    unsigned char id[ID_SIZE];
    uint32_t timestamp;
    uint8_t transmit_count;
    ipv6_addr_t dest_addr;
    char *data;
    size_t data_len;
} event_send;

typedef struct {
    ipv6_addr_t addr;
    unsigned char public_key[KEY_SIZE];
    unsigned char private_key[KEY_SIZE];
} network_node;


/* global variables */

/* indicator if sphinx thread is running */
extern kernel_pid_t sphinx_pid;

/* event queue for sphinx thread */
extern event_queue_t sphinx_queue;

/* ipv6 address of this node */
extern ipv6_addr_t local_addr;

/* stores random bytes from stream cipher */
extern unsigned char prg_stream[PRG_STREAM_SIZE];

/* stores created and received sphinx messages */
extern unsigned char sphinx_message[SPHINX_MESSAGE_SIZE];

/* store state of sent messages */
extern event_send sent_msg_table[SENT_MSG_TABLE_SIZE];
extern uint8_t sent_msg_count;
extern mutex_t sent_msg_mutex;

/* used for encryption */
extern const unsigned char nonce[crypto_stream_NONCEBYTES];


/* global functions */
int8_t sphinx_start(void);
void handle_send(event_t *event);
void handle_stop(event_t *event);
ipv6_addr_t *sphinx_create_message(unsigned char *message, unsigned char *id, ipv6_addr_t *dest_addr, char *data, size_t data_len);
int8_t sphinx_process_message(unsigned char *message, network_node *node_self, unsigned char tag_table[][TAG_SIZE], uint8_t *tag_count);

/* helper functions */
void print_hex_memory (void *mem, uint16_t mem_size);
void print_id(unsigned char *id);
int8_t get_local_ipv6_addr(ipv6_addr_t *result);
int8_t udp_send(ipv6_addr_t *dest_addr, unsigned char *message, size_t message_size);
void hash_blinding_factor(unsigned char *dest, unsigned char *public_key, unsigned char *sharde_secret);
void hash_shared_secret(unsigned char *dest, unsigned char *sharde_secret);
void xor_backwards_inplace(unsigned char *dest, size_t dest_size, unsigned char *arg, size_t arg_size, uint16_t num_bytes);
void print_network(void);
network_node *get_node(ipv6_addr_t *node_addr);
int8_t build_mix_path(network_node *path_nodes[], uint8_t path_len, ipv6_addr_t *start_addr, ipv6_addr_t *dest_addr);
void delete_sent_message(int i);

/* test function */
ipv6_addr_t pki_get_random_addr(void);