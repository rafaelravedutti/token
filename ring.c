/*
 * token - Chat for token-ring topology based networks
 *
 * Copyright (C) 2014  Rafael Ravedutti Lucio Machado
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>

struct msg_type {
  char priority, src, dest, read, bat;
  char data[MAX_MSG_LENGTH];
  struct msg_type *next;
};

struct connection_data {
  int sock;
  struct sockaddr_in recaddr, sndaddr;
};

static struct msg_type *msgbase = NULL;
static unsigned int msgcounter = 0;

double timestamp() {
  struct timeval tp;

  gettimeofday(&tp, NULL);

  return ((double)(tp.tv_sec + tp.tv_usec/1000000.0));
}

int readline(char str[], int max, FILE *stream) {
  int i = 0, c;

  while((c = getc(stream)) != '\n' && c != EOF) {
    if((max--) > 1)
      str[i++] = c;
  }

  str[i++] = '\0';

  return (max <= 0) ? -1 : i;
}

int confighost(struct connection_data *cd) {
  struct hostent *h;

  if((cd->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  if((h = gethostbyname(MACHINE_HOST)) == NULL) {
    perror("gethostbyname");
    return -2;
  }

  cd->recaddr.sin_family = AF_INET;
  cd->recaddr.sin_port = htons(MACHINE_SOURCE_PORT);
  memset(cd->recaddr.sin_zero, 0, sizeof(cd->recaddr.sin_zero));
  memcpy(&cd->recaddr.sin_addr, h->h_addr_list[0], h->h_length);

  if(bind(cd->sock, (struct sockaddr *) &cd->recaddr, sizeof(struct sockaddr_in)) < 0) {
    perror("bind");
    return -3;
  }

  if((h = gethostbyname(MACHINE_DEST_HOST)) == NULL) {
    perror("gethostbyname");
    return -4;
  }

  cd->sndaddr.sin_family = AF_INET;
  cd->sndaddr.sin_port = htons(MACHINE_DEST_PORT);
  memset(cd->sndaddr.sin_zero, 0, sizeof(cd->sndaddr.sin_zero));
  memcpy(&cd->sndaddr.sin_addr, h->h_addr_list[0], h->h_length);
  return 0;
}

void *receive_data(void *args) {
  struct msg_type msg, answer;
  struct connection_data *condata;
  struct msg_type *m;
  double battime;
  char expired;
  socklen_t recvlen;

  condata = (struct connection_data *) args;

  while(1) {
    if(recvfrom(condata->sock, &msg, sizeof msg - sizeof(struct msg_type *), 0, (struct sockaddr *) &condata->recaddr, &recvlen) > 0) {
      if(msg.bat == 1) {
        battime = timestamp();
        expired = 0;

        if(msgbase != NULL && msgbase->priority >= msg.priority) {
          msg.priority = msgbase->priority;
          sendto(condata->sock, &msg, sizeof msg - sizeof(struct msg_type *), 0, (struct sockaddr *) &condata->sndaddr, sizeof(struct sockaddr_in));
          recvfrom(condata->sock, &msg, sizeof msg - sizeof(struct msg_type *), 0, (struct sockaddr *) &condata->recaddr, &recvlen);

          if(msg.bat == 1) {
            while(msgcounter > 0 && msgbase != NULL && msgbase->priority == msg.priority && !expired) {
              do {
                if(timestamp() - battime < BAT_TIME) {
                  expired = 1;
                }

                sendto(condata->sock, msgbase, sizeof(struct msg_type) - sizeof(struct msg_type *), 0, (struct sockaddr *) &condata->sndaddr, sizeof(struct sockaddr_in));
              } while(recvfrom(condata->sock, &answer, sizeof(struct msg_type) - sizeof(struct msg_type *), 0, (struct sockaddr *) &condata->recaddr, &recvlen) && !expired);

              if(answer.src == MACHINE_TAG) {
                if(answer.read == 0) {
                  fprintf(stdout, "> Destino não encontrado!\n");
                }

                --msgcounter;
          
                m = msgbase;
                msgbase = msgbase->next;
                free(m);
              }
            }

            if(msgbase == NULL) {
              msg.priority = 0;
            } else {
              msg.priority = msgbase->priority;
            }
          }
        }

        sendto(condata->sock, &msg, sizeof msg - sizeof(struct msg_type *), 0, (struct sockaddr *) &condata->sndaddr, sizeof(struct sockaddr_in));
      } else {
        if(msg.dest == MACHINE_TAG) {
          msg.read = 1;
          fprintf(stdout, "\r%c: %s\n", msg.src, msg.data);
        }

        if(msg.src != MACHINE_TAG) {
          sendto(condata->sock, &msg, sizeof msg - sizeof(struct msg_type *), 0, (struct sockaddr *) &condata->sndaddr, sizeof(struct sockaddr_in));
        }
      }
    }
  }
}

int main(int argc, const char *argv[]) {
  char inpbuf[MAX_MSG_LENGTH + 8];
  char *saveptr;
  int n;
  struct connection_data condata;
  struct msg_type *prev, *p, *m;
  pthread_t thread_recv;

  if(confighost(&condata) < 0) {
    return -1;
  }

  if(pthread_create(&thread_recv, NULL, receive_data, (void *) &condata) != 0) {
    perror("pthread_create");
    return -2;
  }

#ifdef START_BAT
  struct msg_type batmsg;

  batmsg.src = MACHINE_TAG;
  batmsg.bat = 1;
  batmsg.priority = 0;

  if(sendto(condata.sock, &batmsg, sizeof batmsg - sizeof(struct msg_type *), 0, (struct sockaddr *) &condata.sndaddr, sizeof(struct sockaddr_in)) < 0) {
    perror("sendto(bat)");
  }
#endif

  while(1) {
    if(msgcounter < MAX_MSG_PER_TIME) {
      if((n = readline(inpbuf, sizeof(inpbuf), stdin)) > 0) {
        m = (struct msg_type *) malloc(sizeof(struct msg_type));

        if(m == NULL) {
          perror("malloc");
          return -3;
        }

        m->dest = toupper(*strtok_r(inpbuf, " ", &saveptr));
        m->priority = atoi(strtok_r(NULL, " ", &saveptr));
        m->bat = 0;
        m->read = 0;
        m->src = MACHINE_TAG;
        memcpy(m->data, saveptr, n - (saveptr - inpbuf));

        if(msgbase != NULL) {
          p = msgbase;
          prev = NULL;

          while(p != NULL && p->priority >= m->priority) {
            prev = p;
            p = p->next;
          }

          if(prev != NULL) {
            prev->next = m;
          } else {
            msgbase = m;
          }

          m->next = p;
        } else {
          msgbase = m;
          msgbase->next = NULL;
        }

        ++msgcounter;
      }
    }
  }

  if(pthread_join(thread_recv, NULL) != 0) {
    perror("pthread_join");
    return -1;
  }

  while(msgbase != NULL) {
    m = msgbase;
    msgbase = msgbase->next;
    free(m);
  }

  return 0;
}
