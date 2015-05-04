#include <sys/epoll.h>

typedef struct ae_api_data {
    int epfd;
    struct epoll_event *events;
} ae_api_data;

static void *ae_create_api_data(int setsize)
{
    ae_api_data *data;

    if ((data = (ae_api_data*)malloc(sizeof(ae_api_data))) == NULL) {
        return NULL;
    }
    if ((data->events = malloc(sizeof(struct epoll_event) * setsize)) == NULL) {
        free(data);
        return NULL;
    }
    if ((data->epfd = epoll_create(1024)) == -1) {
        free(data->events);
        free(data);
        return NULL;
    }

    return data;
}
static void ae_free_api_data(ae_event_loop *ev_loop) 
{
    ae_api_data *data = (ae_api_data*)ev_loop->data;
    close(data->epfd);
    free(data->events);
    free(data);

}
static int ae_api_add_event(ae_event_loop *ev_loop, int fd, int mask)
{
    ae_api_data *data = (ae_api_data*)ev_loop->data;
    struct epoll_event ee;
    int op;
    if (ev_loop->events[fd].mask == AE_NONE) {
        op = EPOLL_CTL_ADD;
    } else {
        op = EPOLL_CTL_MOD;
    }
    ee.events = 0;
    mask |= ev_loop->events[fd].mask;
    if (mask & AE_READABLE) {
        ee.events |= EPOLLIN;
    }
    if (mask & AE_WRITABLE) {
        ee.events |= EPOLLOUT;
    }
    ee.data.fd = fd;
    if (epoll_ctl(data->epfd, op, fd, &ee) == -1) {
        return -1;
    }
    return 0;
}

static int ae_api_delete_event(ae_event_loop *ev_loop, int fd, int delmask)
{
    ae_api_data *data = (ae_api_data*)ev_loop->data;
    struct epoll_event ee;
    int mask = (ev_loop->events[fd].mask) & (~delmask);

    ee.events = 0;
    if (mask & AE_READABLE) {
        ee.events |= EPOLLIN;
    }
    if (mask & AE_WRITABLE) {
        ee.events |= EPOLLOUT;
    }
    ee.data.fd = fd;

    if (mask == AE_NONE) {
        epoll_ctl(data->epfd, EPOLL_CTL_DEL, fd, &ee);
    } else {
        epoll_ctl(data->epfd, EPOLL_CTL_MOD, fd, &ee);
    }
    return 0;
}

static int ae_api_poll(ae_event_loop *ev_loop, struct timeval *tvp)
{
    int retval, numevents = 0;
    int timeout;
    ae_api_data *data = (ae_api_data*)ev_loop->data;
    
    if (tvp == NULL) {
        timeout = -1;
    } else {
        timeout = tvp->tv_sec * 1000 + tvp->tv_usec / 1000;
    }

    retval = epoll_wait(data->epfd, data->events, ev_loop->setsize, timeout);
    if (retval > 0) {
        numevents = retval;
        int j, mask = 0;
        struct epoll_event *ee;
        for (j = 0; j < numevents; j++) {
            ee = data->events + j;
            if (ee->events & EPOLLIN) {
                mask |= AE_READABLE;
            } 
            if (ee->events & EPOLLOUT) {
                mask |= AE_WRITABLE;
            }
            
            ev_loop->fired[j].mask = mask;
            ev_loop->fired[j].fd = ee->data.fd;
        }
    }

    return numevents;
}
