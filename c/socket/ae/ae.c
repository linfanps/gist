/**
 * description : 
 * author : linfan
 * date : 2014-12-20
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "ae.h" 

#include "ae_epoll.c"
ae_event_loop *ae_create_event_loop(int setsize)
{
    ae_event_loop *ev_loop;
    int i;
    if ((ev_loop = (ae_event_loop*)malloc(sizeof(ae_event_loop))) == NULL) {
        return NULL;
    }
    if ((ev_loop->events = malloc(sizeof(ae_file_event) * setsize)) == NULL) {
        free(ev_loop);
        return NULL;
    }
    if ((ev_loop->fired = malloc(sizeof(ae_fired_event) * setsize)) == NULL) {
        free(ev_loop->events);
        free(ev_loop);
        return NULL;
    }
    if ((ev_loop->data = ae_create_api_data(setsize)) == NULL) {
        free(ev_loop->events);
        free(ev_loop->fired);
        free(ev_loop);
        return NULL;
    }
    for (i = 0; i < setsize; i++) {
        ev_loop->events[i].mask = AE_NONE;
    }
    ev_loop->maxfd = -1;
    ev_loop->setsize = setsize;
}
void ae_delete_event_loop(ae_event_loop *ev_loop)
{
    ae_free_api_data(ev_loop);
    free(ev_loop->events);
    free(ev_loop->fired);
    free(ev_loop);
}

int ae_create_file_event(ae_event_loop *ev_loop, int fd, int mask, 
        ae_file_proc *proc, void *data)
{
    if (fd >= ev_loop->setsize) {
        errno = ERANGE;
        return AE_ERR;
    }
    ae_file_event *fe = &(ev_loop->events[fd]);
    if (ae_api_add_event(ev_loop, fd, mask) == -1) {
        return AE_ERR;
    }

    fe->mask |= mask;
    if (mask & AE_READABLE) {
        fe->read_process = proc;
    }
    if (mask & AE_WRITABLE) {
        fe->write_process = proc;
    }
    fe->data = data;
    if (fd > ev_loop->maxfd) {
        ev_loop->maxfd = fd;
    }
    return AE_OK;

}

void ae_delete_file_event(ae_event_loop *ev_loop, int fd, int mask)
{
    if (fd >= ev_loop->setsize) {
        return ;
    }
    ae_file_event *fe = &(ev_loop->events[fd]);
    int j;
    if (fe->mask == AE_NONE) {
        return ;
    }
    fe->mask &= (~mask);
    
    if (fd == ev_loop->maxfd && fe->mask == AE_NONE) { //更新maxfd
        for (j = ev_loop->maxfd - 1; j >= 0; j--) {
            if (ev_loop->events[j].mask != AE_NONE)
                break;
        }
        ev_loop->maxfd = j;
    }
    ae_api_delete_event(ev_loop, fd, mask);
}

int ae_process_events(ae_event_loop *ev_loop)
{
    int j, numevents;
    int fd, mask;
    numevents = ae_api_poll(ev_loop, NULL);

    for (j = 0; j < numevents; j++) {
       fd = ev_loop->fired[j].fd;
       mask = ev_loop->fired[j].mask;
       if (mask & AE_READABLE & ev_loop->events[fd].mask) {
           ev_loop->events[fd].read_process(ev_loop, fd, ev_loop->events[fd].data, mask);
       }
       if (mask & AE_WRITABLE & ev_loop->events[fd].mask) {
           ev_loop->events[fd].write_process(ev_loop, fd, ev_loop->events[fd].data, mask);
       }
    }

}

void ae_main(ae_event_loop *ev_loop) 
{
    while (1) {
        ae_process_events(ev_loop);
    }
}
