#include <sys/select.h>
typedef struct ae_api_data {
    fd_set rset;
    fd_set wset;
} ae_api_data;

static void *ae_create_api_data(int setsize)
{
    ae_api_data *data;
    if ((data = (ae_api_data*)malloc(sizeof(ae_api_data))) == NULL) {
        return NULL;
    }
    FD_ZERO(&data->rset);
    FD_ZERO(&data->wset);

    return data;
}

static void ae_free_api_data(ae_event_loop *ev_loop)
{
    ae_api_data *data = (ae_api_data*)ev_loop->data;
    free(data);
}

static int ae_api_add_event(ae_event_loop *ev_loop, int fd, int mask)
{
    ae_api_data *data = (ae_api_data*)ev_loop->data;
    if (mask & AE_READABLE) {
        FD_SET(fd, &data->rset);
    }
    if (mask & AE_WRITABLE) {
        FD_SET(fd, &data->wset);
    }
    return 0;
}

static int ae_api_delete_event(ae_event_loop *ev_loop, int fd, int mask)
{
    ae_api_data *data = (ae_api_data*)ev_loop->data;
    if (mask & AE_READABLE) {
        FD_CLR(fd, &data->rset);
    }
    if (mask & AE_WRITABLE) {
        FD_CLR(fd, &data->wset);
    }
    return 0;
}

static int ae_api_poll(ae_event_loop *ev_loop, struct timeval *tvp)
{
    int retval, numevents = 0, j;
    fd_set rset, wset;

    ae_api_data *data = (ae_api_data*)ev_loop->data;
    memcpy(&rset, &data->rset, sizeof(fd_set));
    memcpy(&wset, &data->wset, sizeof(fd_set));

    retval = select(ev_loop->maxfd+1, &rset, &wset, NULL, tvp);
    if (retval > 0) {
        for (j = 0; j < ev_loop->setsize; j++) {
            if (ev_loop->events[j].mask == AE_NONE)
                continue;
            int mask = 0;
            if (ev_loop->events[j].mask & AE_READABLE & FD_ISSET(j, &rset)) {
                mask |= AE_READABLE;
            }
            if (ev_loop->events[j].mask & AE_WRITABLE & FD_ISSET(j, &wset)) {
                mask |= AE_WRITABLE;
            }
            ev_loop->fired[numevents].fd = j;    
            ev_loop->fired[numevents].mask = mask;    
            numevents++;
        }
    }


    return numevents;
}
