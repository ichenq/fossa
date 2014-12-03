/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Try it out with:
 * $ dig -t A www.google.com -4 @localhost -p 5533
 * $ dig -t A www.cesanta.com -4 @localhost -p 5533
 */

#include "../../fossa.h"

#include <stdio.h>

static int s_exit_flag = 0;

/*
 * Example name resolution:
 *
 * Resolves "cesanta.com" with a static IP address,
 * "www.cesanta.com" with an alias to "cesanta.com" and includes
 * the it's A record in the same response.
 *
 * For all other names it will return a dummy IP address passed
 * as it's `data` value.
 */
static void lookup(struct iobuf *io, struct ns_dns_message *msg,
                   struct ns_dns_resource_record *question,
                   enum ns_dns_server_lookup_op op,
                   const char *name, void *data) {
  in_addr_t addr = * (in_addr_t *) data;
  in_addr_t cesanta_ip = inet_addr("54.194.65.250");

  switch (op) {
    case NS_DNS_SERVER_LOOKUP:
      if (question->rtype != NS_DNS_A_RECORD) {
        break;
      }

      if (strcmp(name, "www.cesanta.com") == 0) {
        ns_dns_reply_record(io, msg, question, NS_DNS_CNAME_RECORD, 3600, name,
                            "cesanta.com", strlen("cesanta.com"));

        ns_dns_reply_record(io, msg, question, NS_DNS_A_RECORD, 3600, "cesanta.com",
                            &cesanta_ip, 4);
        break;
      } else if (strcmp(name, "cesanta.com") == 0) {
        addr = cesanta_ip;
      }

      ns_dns_reply_record(io, msg, question, NS_DNS_A_RECORD, 3600, name,
                          &addr, 4);
    case NS_DNS_SERVER_FINALIZE:
      /* you can set error codes here */
      (void) msg;
      break;
  }
}

static void ev_handler(struct ns_connection *nc, int ev, void *ev_data) {
  struct iobuf *io = &nc->send_iobuf;;

  (void) ev_data;

  switch (ev) {
    case NS_RECV:
      ns_dns_create_reply(io, nc->recv_iobuf.buf, nc->recv_iobuf.len, lookup, nc->user_data);
      ns_send(nc, io->buf, io->len);
      iobuf_remove(io, io->len);
      break;
  }
}

int main(int argc, char *argv[]) {
  struct ns_mgr mgr;
  struct ns_connection *nc;
  in_addr_t addr = inet_addr("127.0.0.1");
  char *bind_addr = ":5533";
  char url[256];
  int i;

  ns_mgr_init(&mgr, NULL);

  /* Parse command line arguments */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-D") == 0) {
      mgr.hexdump_file = argv[++i];
    } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
      bind_addr = argv[++i];
    } else {
      addr = inet_addr(argv[i]);
    }
  }

  snprintf(url, sizeof(url), "udp://%s", bind_addr);
  fprintf(stderr, "Listening to '%s'\n", url);
  if ((nc = ns_bind(&mgr, url, ev_handler)) == NULL) {
    fprintf(stderr, "cannot bind to socket\n");
    exit(1);
  }
  nc->user_data = &addr;

  while (s_exit_flag == 0) {
    ns_mgr_poll(&mgr, 1000);
  }
  ns_mgr_free(&mgr);

  return 0;
}
