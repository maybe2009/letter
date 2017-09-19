#include <sys/epoll.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_EPOLL_EVENTS 5

struct Epoll 
{
  Epoll(int fd) : __fd(fd) {};
  
  ~Epoll() { close(__fd); };

  int control(int op, int fd, struct epll_event* event)
  {
    return epoll_ctl(__fd, op, fd, event);
  }

  int select(struct epoll_event* events, 
             int maxevents, 
             int timeout) 
  {
    return epoll_wait(__fd, events, maxevents, timeout);
  }

  int __fd;
};

bool is_process_running = true;

void sigint_handler()
{
  printf("Ctrl-C pressed, exit now!");
  is_process_running = false;
}

Epoll make_epoll()
{
  int fd = epoll_create(1);
  if (fd == -1) 
  {
    return NULL;
  }   
  return Epoll(fd);
}

struct EventHandler
{
  virtual void handle(uint32_t event) = 0;
}

struct StdIOEventHandler
{
  virtual void handle(uint32_t event) 
  {
    switch (event)
    {
      case EPOLLIN:
      {
        onRead();
      }

      default:
        printf("events not support");
    }
  }

  void onRead()
  {
    char buf[10];
    ssize_t ret = read(stdin, buf, 10);
    if (ret > 0)
    {
      buf[ret] = 0;
      printf("input is %s", buf);
    }
    else
    {
      printf("error occured during read");
    } 
  }
}

void handle_events(const struct epoll_event* events, uint32_t events_num)
{
  for (uint32_t i = 0; i < events_num; i++)
  {
    const struct epoll_event* ev = events + i;
    if (EventHandler* handler = dynamic_cast<EventHandler*>(ev.data);
    {
      handler->handle(ev.events); 
      
      delete handler;
    }    
  }
}

int main(int argc, const char* argv[])
{
  Epoll epoll_instance = make_epoll();

  StdIOEventHandler* io_handler = new StdIOEventHandler();

  struct epoll_event input_rd;
  input_rd.events = EPOLLIN; 
  input_rd.data = io_handler; 
  
  //exit gracefully
  signal(SIGINT, sigint_handler); 
  
  while (is_process_running)
  {
    struct epoll_event events[MAX_EPOLL_EVENTS]; 

    int ret = epoll_instance.select(epoll_event, MAX_EPOLL_EVENTS, 1000 * 5);
    if (ret == -1)
    {
      printf("select failed");
      continue;
    } 

    handle_events(events, ret); 
  } 
  
  return 0;
}

