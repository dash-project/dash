
#include <signal.h>
#include <ucontext.h>

#include <execinfo.h>

#include <dash/dart/base/env.h>

#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/tasking/dart_tasking_priv.h>

#define BT_DEPTH 100

static bool enable_stacktrace = false;

static const char *
signal_name(int signum)
{
  switch(signum){
    case SIGSEGV: return "Segmentation Fault";
    case SIGBUS:  return "Bus Error";
    default:      return "<unknown>";
  }
}

static void print_stacktrace()
{
  if (!enable_stacktrace) return;

  DART_LOG_ERROR("Gathering stacktrace...");
  void *buffer[BT_DEPTH];
  int nptrs = backtrace(buffer, BT_DEPTH);
  char **strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    DART_LOG_ERROR("Failed to gather symbols for backtrace!");
  }
  for (int i = 0; i < nptrs; i++) {
    DART_LOG_ERROR("\t %s", strings[i]);
  }
  free(strings);
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

  print_stacktrace();
  // re-raise the signal to force shutdown
  // the default signal handler has already been restored via SA_RESETHAND
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

  enable_stacktrace = dart__base__env__bool(DART_TASK_PRINT_BACKTRACE_ENVSTR,
                                            false);

}
