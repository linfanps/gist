#ifndef _AE_H
#define _AE_H

#define AE_OK 0
#define AE_ERR -1
#define AE_CLIENT_SHUTDOWN -2

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

struct ae_event_loop;
typedef int ae_file_proc(struct ae_event_loop *ev_loop, int fd, void *data, int mask);

typedef struct ae_file_event {
    int mask;
    ae_file_proc *read_process;
    ae_file_proc *write_process;
    void *data;
} ae_file_event;

typedef struct ae_fired_event {
    int mask;
    int fd;
} ae_fired_event;

typedef struct ae_event_loop {
    int maxfd;
    int setsize;
    ae_file_event *events;
    ae_fired_event *fired;
    void *data;
} ae_event_loop;

ae_event_loop *ae_create_event_loop(int setsize);
void ae_delete_event_loop(ae_event_loop *ev_loop);
int ae_create_file_event(ae_event_loop *ev_loop, int fd, int mask, ae_file_proc *proc, void *data);
void ae_delete_file_event(ae_event_loop *ev_loop, int fd, int mask);
int ae_process_events(ae_event_loop *ev_loop);
void ae_main(ae_event_loop *ev_loop);
#endif
