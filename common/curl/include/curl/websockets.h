#ifndef CURLINC_WEBSOCKETS_H
#define CURLINC_WEBSOCKETS_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/

/* generic in/out flag bits */
#define CURLWS_TEXT       (1<<0)
#define CURLWS_BINARY     (1<<1)
#define CURLWS_CONT       (1<<2)
#define CURLWS_CLOSE      (1<<3)
#define CURLWS_PING       (1<<4)

/*
 * NAME curl_ws_recv()
 *
 * DESCRIPTION
 *
 * Receives data from the websocket connection. Use after successful
 * curl_easy_perform() with CURLOPT_CONNECT_ONLY option.
 */
CURL_EXTERN CURLcode curl_ws_recv(CURL *curl, void *buffer, size_t buflen,
                                  size_t *recv, unsigned int *recvflags);

/* sendflags for curl_ws_send() */
#define CURLWS_NOCOMPRESS (1<<5)
#define CURLWS_PONG       (1<<6)

/*
 * NAME curl_easy_send()
 *
 * DESCRIPTION
 *
 * Sends data over the websocket connection. Use after successful
 * curl_easy_perform() with CURLOPT_CONNECT_ONLY option.
 */
CURL_EXTERN CURLcode curl_ws_send(CURL *curl, const void *buffer,
                                  size_t buflen, size_t *sent,
                                  unsigned int sendflags);

/* bits for the CURLOPT_WS_OPTIONS bitmask: */
#define CURLWS_RAW_MODE (1<<0)

struct curl_ws_metadata {
  int age;       /* zero */
  int recvflags; /* See the CURLWS_* defines */
};

CURL_EXTERN struct curl_ws_metadata *curl_ws_meta(CURL *curl);

#endif /* CURLINC_WEBSOCKETS_H */
