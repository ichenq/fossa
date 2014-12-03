/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_DNS_SERVER_HEADER_DEFINED
#define NS_DNS_SERVER_HEADER_DEFINED

#ifdef NS_ENABLE_DNS_SERVER

#include "dns.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NS_DNS_SERVER_DEFAULT_TTL 3600

enum ns_dns_server_lookup_op {
  NS_DNS_SERVER_LOOKUP,
  NS_DNS_SERVER_FINALIZE
};

typedef void (*ns_dns_lookup_t)(struct iobuf *, struct ns_dns_message *,
                                struct ns_dns_resource_record *, enum ns_dns_server_lookup_op,
                                const char *, void *);

int ns_dns_create_reply(struct iobuf *, const char *, size_t, ns_dns_lookup_t, void *);
int ns_dns_reply_record(struct iobuf *, struct ns_dns_message *, struct ns_dns_resource_record *,
                        int, int, const char *, const void *, size_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NS_ENABLE_DNS_SERVER */
#endif  /* NS_HTTP_HEADER_DEFINED */
