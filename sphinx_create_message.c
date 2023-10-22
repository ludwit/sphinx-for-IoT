#include "sphinx.h"

/* fixed nonce because every shared secret is used only once for encryption */
const unsigned char nonce[crypto_stream_NONCEBYTES] = { 0xff, 0xcb, 0x7c, 0x4f, 0xcc, 0x0e, 0xf9, 0x29, 0xde, 0xaa, 0x42, 0xd2, 0xa2, 0x3e, 0x5f, 0xa3, 0xbd, 0x6d, 0xd8, 0x76, 0xf8, 0x7c, 0x84, 0x3f };

void calculate_shared_secrets(unsigned char *sphinx_message, unsigned char shared_secrets[][KEY_SIZE], network_node *path_nodes[], uint8_t path_len)
{
    /* secret ecc key of the sender */
    unsigned char secret_key[KEY_SIZE];

    /* public keys for computing the shared secrets at each hop */
    unsigned char public_keys[2*SPHINX_MAX_PATH][KEY_SIZE]; // could also only save the last two ones

    /* blinding factors for each hop */
    unsigned char blinding_factors[2*SPHINX_MAX_PATH][KEY_SIZE];

    /* used to store intermediate results */
    unsigned char buff_shared_secret[KEY_SIZE];

    /* generates an ephemeral asymmetric key pair for the sender
     * public key for first hop is the generic public key of the sender */
    crypto_box_keypair(public_keys[0], secret_key);

    /* put public key for first hop in message */
    memcpy(sphinx_message, public_keys[0], KEY_SIZE);

    /* prepare root for calculation of public keys, shared secrets and blinding factors */

    /* calculates raw shared secret with first hop */
    crypto_scalarmult(buff_shared_secret, secret_key, path_nodes[0]->public_key);

    /* hash shared secret */
    hash_shared_secret(shared_secrets[0], buff_shared_secret);

    /* calculates blinding factor at first hop */
    hash_blinding_factor(blinding_factors[0], public_keys[0], shared_secrets[0]);

    /* iteratively calculates all remaining public keys, shared secrets and blinding factors */
    for (uint8_t i=1; i<path_len; i++) {

        /* blinds the public key for node i-1 to get public key for node i */
        crypto_scalarmult(public_keys[i], blinding_factors[i-1], public_keys[i-1]);

        /* calculates the generic shared secret with node i */
        crypto_scalarmult(buff_shared_secret, secret_key, path_nodes[i]->public_key);

        /* iteratively applies all past blinding to shared secret with node i */
        for (uint8_t j=0; j<i; j++) {
            crypto_scalarmult(shared_secrets[i], blinding_factors[j], buff_shared_secret);
            memcpy(buff_shared_secret, &shared_secrets[i], KEY_SIZE);
        }

        /* hash shared secret */
        hash_shared_secret(shared_secrets[i], buff_shared_secret);

        /* calculates blinding factor */
        hash_blinding_factor(blinding_factors[i], public_keys[i], shared_secrets[i]);
    }
}

void calculate_filler_string(unsigned char *filler_string, unsigned char shared_secrets[][KEY_SIZE], uint8_t path_len)
{
    uint8_t string_size = 0;

    for (uint8_t i=0; i<path_len; i++) {
        /* move filler string for NODE_ROUTING_SIZE = NODE_FILLER_PADDING_SIZE bytes to the left */
        memmove(&filler_string[MAX_FILLER_STRING_SIZE - string_size - NODE_ROUTING_SIZE], &filler_string[MAX_FILLER_STRING_SIZE - string_size], string_size);
        /* set the rightmost NODE_ROUTE_SIZE bytes to zero */
        memset(&filler_string[MAX_FILLER_STRING_SIZE - NODE_ROUTING_SIZE], 0, NODE_ROUTING_SIZE);
        /* increase size variable */
        string_size += NODE_ROUTING_SIZE;
        /* calculate pseudo random byte stream with shared secret */
        crypto_stream(prg_stream, MAX_FILLER_STRING_SIZE + NODE_FILLER_PADDING_SIZE, nonce, shared_secrets[i]);
        /* xor filler string with random byte stream */
        xor_backwards_inplace(filler_string, MAX_FILLER_STRING_SIZE, prg_stream, MAX_FILLER_STRING_SIZE + NODE_FILLER_PADDING_SIZE, string_size);
    }

    /* cut off last node padding and move filler string in place for encapsulation of routing and mac */
    memmove(&filler_string[MAX_FILLER_STRING_SIZE - ((path_len - 1) * NODE_ROUTING_SIZE)], &filler_string[MAX_FILLER_STRING_SIZE - ((path_len) * NODE_ROUTING_SIZE)], (path_len - 1) * NODE_ROUTING_SIZE);
}

void encapsulate_routing_and_mac(unsigned char *routing_and_mac, unsigned char shared_secrets[][KEY_SIZE], network_node *path_nodes[], uint8_t path_len, unsigned char *id)
{
    /* padding to keep header size invariant regardless of actual path length */
    uint8_t header_padding_size = (SPHINX_MAX_PATH - path_len) * NODE_ROUTING_SIZE;

    /* prepare root routing information for iteration */
    memcpy(&routing_and_mac[MAC_SIZE], &path_nodes[path_len-1]->addr, ADDR_SIZE);
    memcpy(&routing_and_mac[MAC_SIZE + ADDR_SIZE], id, ID_SIZE);
    random_bytes(&routing_and_mac[MAC_SIZE + ADDR_SIZE + ID_SIZE], header_padding_size);

    for (int8_t i=path_len-1; i>=0; i--) {

        /* calculate pseudo random byte stream with shared secret */
        crypto_stream(prg_stream, ENC_ROUTING_SIZE, nonce, shared_secrets[i]);

        /* xor routing information for node i with prg stream */
        xor_backwards_inplace(&routing_and_mac[MAC_SIZE], ENC_ROUTING_SIZE, prg_stream, ENC_ROUTING_SIZE, ENC_ROUTING_SIZE);

        /* calculate mac of encrypted routing information */
        crypto_onetimeauth(routing_and_mac, &routing_and_mac[MAC_SIZE], ENC_ROUTING_SIZE, shared_secrets[i]);

        /* end early if last iteration */
        if (i>0) {

            /* cut off one filler string in routing_and_mac to make space for next hop address and mac */
            memmove(&routing_and_mac[ADDR_SIZE + MAC_SIZE], routing_and_mac, MAC_SIZE + ENC_ROUTING_SIZE - NODE_FILLER_PADDING_SIZE);

            /* put address of node i in place for next iteration */
            memcpy(&routing_and_mac[MAC_SIZE], &path_nodes[i]->addr, ADDR_SIZE);
        }
    }
}

void encrypt_surb_and_payload(unsigned char *surb_and_payload, unsigned char shared_secrets[][KEY_SIZE], uint8_t path_len)
{
    for (int8_t i=path_len-1; i>=0; i--) {

        crypto_stream(prg_stream, PRG_STREAM_SIZE, nonce, shared_secrets[i]);

        xor_backwards_inplace(surb_and_payload, MAC_SIZE + SURB_SIZE + PAYLOAD_SIZE, prg_stream, PRG_STREAM_SIZE, MAC_SIZE + SURB_SIZE + PAYLOAD_SIZE);
    }
}

void build_sphinx_surb(unsigned char *sphinx_surb, unsigned char shared_secrets[][KEY_SIZE], unsigned char *id, network_node *path_nodes[], uint8_t path_len_reply)
{
    /* save address of first hop to surb */
    memcpy(sphinx_surb, &path_nodes[0]->addr, ADDR_SIZE);

    /* precalculates the accumulated padding added at each hop */
    calculate_filler_string(&sphinx_surb[ADDR_SIZE + MAC_SIZE], shared_secrets, path_len_reply);

    /* calculates the nested encrypted routing information */
    encapsulate_routing_and_mac(&sphinx_surb[ADDR_SIZE], shared_secrets, path_nodes, path_len_reply, id);
}

void build_sphinx_header(unsigned char *sphinx_header, unsigned char shared_secrets[][KEY_SIZE], network_node *path_nodes[], uint8_t path_len_dest)
{
    /* use 0s as ID for dest */
    unsigned char id_dest[ID_SIZE];
    memset(&id_dest, 0x00, ID_SIZE);

    /* precalculates the accumulated padding added at each hop */
    calculate_filler_string(&sphinx_header[KEY_SIZE + MAC_SIZE], shared_secrets, path_len_dest);

    /* calculates the nested encrypted routing information */
    encapsulate_routing_and_mac(&sphinx_header[KEY_SIZE], shared_secrets, path_nodes, path_len_dest, &id_dest[0]);
}


ipv6_addr_t *sphinx_create_message(unsigned char *sphinx_message, unsigned char *id, ipv6_addr_t *dest_addr, char *data, size_t data_len)
{
    /* network path for sphinx message to destination and reply */
    network_node* path_nodes[2*SPHINX_MAX_PATH];

    /* shared secrets with nodes in path */
    unsigned char shared_secrets[2*SPHINX_MAX_PATH][KEY_SIZE];

    /* choose random number for path length to dest */
    uint8_t path_len_dest = random_uint32_range(3, SPHINX_MAX_PATH+1);

    /* choose random number for path length of reply */
    uint8_t path_len_reply = random_uint32_range(3, SPHINX_MAX_PATH+1);
    
    /* builds a random path to the destination and back */
    if ((build_mix_path(path_nodes, path_len_dest, &local_addr, dest_addr) < 0) ||
        (build_mix_path(&path_nodes[path_len_dest], path_len_reply, dest_addr, &local_addr)) < 0) {
        puts("sphinx: error: could not build mix path");
        return NULL;
    }

    /* precalculates the shared secrets with all nodes in path */
    calculate_shared_secrets(sphinx_message, shared_secrets, path_nodes, path_len_dest+path_len_reply);

    build_sphinx_header(sphinx_message, shared_secrets, path_nodes, path_len_dest);

    build_sphinx_surb(&sphinx_message[HEADER_SIZE + MAC_SIZE], &shared_secrets[path_len_dest], id, &path_nodes[path_len_dest], path_len_reply);

    /* put payload in place and add padding if necessary */
    memcpy(&sphinx_message[HEADER_SIZE + MAC_SIZE + SURB_SIZE], data, data_len);
    memset(&sphinx_message[HEADER_SIZE + MAC_SIZE + SURB_SIZE + data_len], 0, PAYLOAD_SIZE - data_len);

    /* calculate mac of surb and payload for integrity checking at dest */
    crypto_onetimeauth(&sphinx_message[HEADER_SIZE], &sphinx_message[HEADER_SIZE + MAC_SIZE], SURB_SIZE + PAYLOAD_SIZE, shared_secrets[path_len_dest-1]);

    /* encrypt surb payload and mac of both multiple times */
    encrypt_surb_and_payload(&sphinx_message[HEADER_SIZE], shared_secrets, path_len_dest);

    return &path_nodes[0]->addr;
}