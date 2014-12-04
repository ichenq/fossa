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
static void ev_handler(struct ns_connection *nc, int ev, void *p) {
  struct ns_dns_server_request *dr = (struct ns_dns_server_request *) p;
  in_addr_t cesanta_ip = inet_addr("54.194.65.250");
  in_addr_t addr = * (in_addr_t *) nc->user_data;

  switch (ev) {
    case NS_DNS_QUESTION:
      if (dr->question->rtype != NS_DNS_A_RECORD) {
        break;
      }

      if (strcmp(dr->name, "www.cesanta.com") == 0) {
        ns_dns_reply(nc, NS_DNS_CNAME_RECORD, 3600, dr->name, "cesanta.com",
                     strlen("cesanta.com"));

        ns_dns_reply(nc, NS_DNS_A_RECORD, 3600, "cesanta.com", &cesanta_ip, 4);
        break;
      } else if (strcmp(dr->name, "cesanta.com") == 0) {
        addr = cesanta_ip;
      }

      ns_dns_reply(nc, NS_DNS_A_RECORD, 3600, dr->name, &addr, 4);
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
  set_protocol_dns_server(nc);
  nc->user_data = &addr;

  while (s_exit_flag == 0) {
    ns_mgr_poll(&mgr, 1000);
  }
  ns_mgr_free(&mgr);

  return 0;
}
