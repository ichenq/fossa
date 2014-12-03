/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * == DNS server API
 *
 * Disabled by default; enable with `-DNS_ENABLE_DNS_SERVER`.
 */

#ifdef NS_ENABLE_DNS_SERVER

#include "internal.h"

/*
 * Appends a DNS reply to the IO buffer.
 *
 * A DNS request is parsed from `buf` and for each question record
 * the `handler` is invoked to perform a record resolution.
 *
 * If the request is malformed, an reply with the appropriate error will be
 * created.
 *
 * Returns the size of reply appened to the IO buffer, or -1 in case of error.
 *
 * Example of what to do inside the callback:
 *
 * [source,c]
 * -----
 * static void lookup(struct iobuf *io, struct ns_dns_message *msg,
 *                    struct ns_dns_resource_record *question,
 *                    enum ns_dns_server_lookup_op op,
 *                    const char *name, void *data) {
 *     switch (op) {
 *     case NS_DNS_SERVER_LOOKUP:
 *       if (question->rtype != NS_DNS_A_RECORD) {
 *         break;
 *       }
 *
 *       // lookup your names
 *       if (strcmp(name, "example.com") == 0) {
 *         ns_dns_reply_record(io, msg, question, NS_DNS_A_RECORD, 3600, name,
 *                             &example_ip, 4);
 *       }
 *
 *       // you can return multiple records, useful for CNAMEs
 *       if (strcmp(name, "www.cesanta.com") == 0) {
 *         ns_dns_reply_record(io, msg, question, NS_DNS_CNAME_RECORD, 3600, name,
 *                             "cesanta.com", strlen("cesanta.com"));
 *
 *         ns_dns_reply_record(io, msg, question, NS_DNS_A_RECORD, 3600, "cesanta.com",
 *                             &cesanta_ip, 4);
 *         break;
 *       }
 *
 *     case NS_DNS_SERVER_FINALIZE:
 *       // you have control on whether to return error or not.
 *       if (msg->num_answers == 0) {
 *         msg->flags |= 3; // NXDOMAIN
 *       }
 *       break;
 *   }
 * }
 * -----
 */
int ns_dns_create_reply(struct iobuf *io, const char *buf, size_t len,
                        ns_dns_lookup_t handler, void *data) {
  size_t pos = io->len;
  struct ns_dns_message msg;
  char name[512];
  int i;

  if (ns_parse_dns(buf, len, &msg) == -1) {
    /* tentative transaction id, to help client debugging */
    int tid = msg.transaction_id;
    memset(&msg, 0, sizeof(msg));
    msg.transaction_id = tid;
    /* reply + recursion allowed + format error */
    msg.flags = 0x8081;
    ns_dns_insert_header(io, pos, &msg);
    return io->len - pos;
  }

  /* reply + recursion allowed */
  msg.flags |= 0x8080;

  ns_dns_copy_body(io, &msg);

  msg.num_answers = 0;
  for (i = 0; i < msg.num_questions; i++) {
    ns_dns_uncompress_name(&msg, &msg.questions[i].name, name, sizeof(name));
    handler(io, &msg, &msg.questions[i], NS_DNS_SERVER_LOOKUP, name, data);
  }

  handler(io, &msg, NULL, NS_DNS_SERVER_FINALIZE, NULL, data);

  /* prepends header now that we know the number of answers */
  ns_dns_insert_header(io, pos, &msg);

  /*
   * TODO(mkm): figure out how to know if the handlers have some data
   * for other record types and return the right error code.
   */
  return io->len - pos;
}

/*
 * Append a DNS reply record to the IO buffer and to the DNS message.
 *
 * Intended to be used from a `ns_dns_lookup_t` handler called by
 * `ns_dns_create_reply`, see the latter for an example.
 *
 * Returns -1 on error.
 */
int ns_dns_reply_record(struct iobuf *io, struct ns_dns_message *msg,
                        struct ns_dns_resource_record *question, int rtype,
                        int ttl, const char *name, const void *rdata,
                        size_t rdata_len) {
  struct ns_dns_resource_record *ans = &msg->answers[msg->num_answers];
  if (msg->num_answers >= NS_MAX_DNS_ANSWERS) {
    return -1;  /* LCOV_EXCL_LINE */
  }

  *ans = *question;
  ans->kind = NS_DNS_ANSWER;
  ans->rtype = rtype;
  ans->ttl = ttl;

  if (ns_dns_encode_record(io, ans, name, strlen(name), rdata, rdata_len) == -1) {
    return -1;  /* LCOV_EXCL_LINE */
  };

  msg->num_answers++;
  return 0;
}


#endif  /* NS_ENABLE_DNS_SERVER */
