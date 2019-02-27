
#include <signal.h>
#include <ucontext.h>

#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/tasking/dart_tasking_priv.h>


static void handler(int signum, siginfo_t *si, void *ptr)
{
   ucontext_t *uc = (ucontext_t *)ptr;

   DART_LOG_ERROR("Thread %d caught signal %d: address %p, ctx %p",
                  dart__tasking__thread_num(), signum, si->si_addr, uc);
   dart_abort(signum);

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
