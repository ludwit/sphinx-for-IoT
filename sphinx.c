#include "sphinx.h"

/* indicator if sphinx thread is running */
kernel_pid_t sphinx_pid = 0;

char sphinx_server_stack[2*THREAD_STACKSIZE_MAIN + 512];

/* stores random bytes from stream cipher */
unsigned char prg_stream[PRG_STREAM_SIZE];

/* stores created and received sphinx messages */
unsigned char sphinx_message[SPHINX_MESSAGE_SIZE];

/* array to store seen message tags to prevent replay attacks */
unsigned char tag_table[TAG_TABLE_LEN][TAG_SIZE];
uint8_t tag_count = 0;

/* event queue for sphinx thread */
event_queue_t sphinx_queue;

/* ipv6 address of this node */
ipv6_addr_t local_addr;

/* socket to receive sphinx messages */
sock_udp_t sock;

/* stores sent messages */
event_send sent_msg_table[SENT_MSG_TABLE_SIZE];
uint8_t sent_msg_count = 0;

void handle_stop(event_t *event)
{
    (void) event;
    sock_udp_close(&sock);
    thread_zombify();
}

void handle_send(event_t *event)
{
    event_send *sphinx_send = (event_send *) event;

    ipv6_addr_t *first_hop_addr;

    /* used to merge error behaviour */
    bool error = false;

    /* check if this is the first transmit */
    if (sphinx_send->transmit_count == 0) {
        /* check if message can be sent */
        if (sent_msg_count >=  SENT_MSG_TABLE_SIZE) {
            puts("sphinx: error: sending new message, waiting for too many replies");
            return;
        }
        /* else set random id */
        random_bytes(sphinx_send->id, ID_SIZE);
    }

    /* create sphinx message */
    if ((first_hop_addr = sphinx_create_message(sphinx_message, sphinx_send->id, &sphinx_send->dest_addr, sphinx_send->data, sphinx_send->data_len)) == NULL) {
        error = true;
    }

    /* send sphinx message */
    else if (udp_send(first_hop_addr, sphinx_message, SPHINX_MESSAGE_SIZE) < 0) {
        error = true;
    }

    /* adjust the event properties */
    sphinx_send->transmit_count++;
    sphinx_send->timestamp = xtimer_now_usec();

    /* if first transmit add new message to sent message table */
    if (sphinx_send->transmit_count == 1) {
        memcpy(&sent_msg_table[sent_msg_count], sphinx_send, sizeof(event_send));
        sent_msg_count++;
    }

    /* verbose */
    print_id(sphinx_send->id);
    if (error) {
        if (sphinx_send->transmit_count == 1) {
            puts("message not sent");
        } else {
            puts("message not retransmitted");
        }
    } else {
        if (sphinx_send->transmit_count == 1) {
            puts("message sent");
        } else {
            puts("message retransmitted");
        }
    }
}

void handle_socket(sock_udp_t *sock, sock_async_flags_t type, void *node_self)
{
    ssize_t res;

    if (type == SOCK_ASYNC_MSG_RECV) {

        if ((res = sock_udp_recv(sock, sphinx_message, SPHINX_MESSAGE_SIZE, 0, NULL)) < 0) {
            printf("sphinx: sock_udp_recv: error %d\n", res);
            return;
        }
        
        if (res != SPHINX_MESSAGE_SIZE) {
            puts("sphinx: error: received malformed data");
            return;
        }

        if (sphinx_process_message(sphinx_message, (network_node *) node_self, tag_table, &tag_count) < 0) {
            puts("sphinx: error: processing sphinx message");
        }
    }
}

void* sphinx(void *arg)
{
    (void) arg;

    /* used to store time variable */
    uint32_t now;

    event_t *event;

    network_node *node_self;

    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    local.port = SPHINX_PORT;

    if (get_local_ipv6_addr(&local_addr) < 0) {
        puts("error: no local ip address found");
        return NULL;
    }

    /* get node information */
    if ((node_self = get_node(&local_addr)) == NULL) {
        puts("error: no entry in pki with this ipv6 address");
        print_hex_memory(&local_addr, sizeof(ipv6_addr_t));
        return NULL;
    }

    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("error: creating udp sock");
        return NULL;
    }

    event_queue_init(&sphinx_queue);

    /* makes socket create events for asynchronous access */
    sock_udp_event_init(&sock, &sphinx_queue, handle_socket, node_self);

    /* give success prompt */
    puts("sphinx: server running at address");
    ipv6_addr_print(&local_addr);
    puts("");

    while(1) {

        /* wait for event with timeout */
        if ((event = event_wait_timeout(&sphinx_queue, EVENT_TIMEOUT_US))) {
            event->handler(event);
        }
        
        now = xtimer_now_usec();

        /* check status of sent messages */
        for (uint8_t i=0; i<sent_msg_count; i++) {

            /* check if timeout was exceeded */
            if ((now - sent_msg_table[i].timestamp) < MSG_TIMEOUT_US) {
                continue;
            }

            /* check if maximum transmits of message are reached */
            if (sent_msg_table[i].transmit_count >= MAX_TRANSMITS) {
                print_id(sent_msg_table[i].id);
                puts("message discarded");

                /* delete message */
                delete_sent_message(i);
            }

            else {
                /* retransmit message */
                handle_send((event_t *) &sent_msg_table[i]);
            }
        }
    }
}

/* starts the sphinx server thread */
int8_t sphinx_start(void)
{   
    if (sphinx_pid > 0) {
        puts("sphinx: already running");
        return -1;
    }
    if ((sphinx_pid = thread_create(sphinx_server_stack,
                       sizeof(sphinx_server_stack),
                       THREAD_PRIORITY_MAIN - 1,
                       THREAD_CREATE_STACKTEST,
                       sphinx,
                       NULL, "sphinx")) > SCHED_PRIO_LEVELS)
    {
        printf("sphinx: thread_create: error %d\n", sphinx_pid);
        return -1;
    }

    return 1;
}