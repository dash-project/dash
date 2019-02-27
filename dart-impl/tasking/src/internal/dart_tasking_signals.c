
#include <signal.h>
#include <ucontext.h>

#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/tasking/dart_tasking_priv.h>

static const char *
signal_name(int signum)
{
  switch(signum){
    case SIGSEGV: return "Segmentation Fault";
    case SIGBUS:  return "Bus Error";
    default:      return "<unknown>";
  }
}

static void handler(int signum, siginfo_t *si, void *ptr)
{
   ucontext_t *uc = (ucontext_t *)ptr;
   dart_thread_t *thread = dart__tasking__current_thread();
   DART_LOG_ERROR("Thread %d caught signal %d (%s):\n"
                  "\taddress: %p\n"
                  "\tctx: %p\n"
                  "\tcurrent task: %p\n",
                  dart__tasking__thread_num(), signum, signal_name(signum),
                  si->si_addr, uc, thread ? thread->current_task : NULL);

   // re-raise the signal to force shutdown
   raise(signum);
}


void dart__tasking__install_signalhandler()
{
  struct sigaction s;
  s.sa_flags = SA_SIGINFO|SA_RESETHAND;
  s.sa_sigaction = handler;
  sigemptyset(&s.sa_mask);
  sigaction(SIGSEGV, &s, 0);
  sigaction(SIGBUS, &s, 0);
}
