#include <stdio.h>
#include <libdash.h>
#include <string>
#include <algorithm>
#include <iterator>
#include <functional>
#include <inttypes.h>
#include <stdint.h>

#include "dart_tools.h"
#include "ayu_events.h"
#include "Ayudame_rt.h"
#include "Ayudame.h"

static unsigned long long myglobalid;

static std::set<std::string> set;
static std::set<std::string>::iterator it;

/**
 * Function to manipulate the most significant 2 bytes of an usigned 64 bit integer
 * when using thee default mask = 0xFFFF.
 * In modern 64 bit cpu architectures, memory adresses only use the first 48 bits;
 * the remaining 16 bits are left free. Since Ayudame needs unique unsigned 64 bit integers
 * to identify tasks, we use the remaining 16 bits to encode the unit id additionally to the
 * 48 bit memory adresses. Therefore the returned unsigned 64 bit integers are unique.
 */
static uint64_t manipulateUnsignedInteger (uint64_t input, unsigned long long value) {
  unsigned long long mask = 0xFFFF; //mask the first most significant 2 bytes
  input = (input & ~(mask << 48)) | (value << 48);
  return input;
}

static void
callback_on_task_create(uint64_t task, dart_task_prio_t prio, const char *name, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  std::string s_name = name;
  std::hash<std::string> hash_function;
  uint64_t hash_name = hash_function(s_name);
  if (set.count(name) == 0) {
    set.insert(name);
    ayu_event_registerfunction(hash_name, name);
  }
  ayu_event_addtask(task, hash_name, (uint64_t) prio, (uint64_t) 0); //scope_id set to zero.
 
  
}

static void
callback_on_task_begin(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  ayu_event_preruntask(task, thread);
  ayu_event_runtask(task);
  
}

static void
callback_on_task_end(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  ayu_event_postruntask(task);
  ayu_event_removetask(task); 
}

static void
callback_on_task_cancel(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  ayu_event_removetask(task);      
}

static void
callback_on_task_yield_leave(uint64_t task, uint64_t thread,  void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
}

static void
callback_on_task_yield_resume(uint64_t task, uint64_t thread,  void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
}

static void
callback_on_task_finalize(void *userdata) {
  ayu_event_finish();
}

static void
callback_on_task_add_to_queue(uint64_t task, uint64_t thread, void *userdata)
{
  task = manipulateUnsignedInteger(task, myglobalid);
  ayu_event_addtasktoqueue(task, thread);
}

static void
callback_on_local_dep(
  uint64_t task1,
  uint64_t task2,
  uint64_t memaddr,
  int32_t  unitid,
  int      edge_type,
  void    *userdata)
{
  task1 = manipulateUnsignedInteger(task1, (unsigned long long) unitid);
  task2 = manipulateUnsignedInteger(task2, (unsigned long long) unitid);
  ayu_event_adddependency(task2, task1, memaddr, memaddr);
}

static void
callback_on_remote_dep(
  uint64_t to_task,
  uint64_t from_task,
  int      to_dep_type,
  int      from_dep_type,
  uint64_t memaddr,
  int32_t  to_unitid,
  int32_t  from_unitid,
  int      edge_type,
  void    *userdata)
{
  to_task = manipulateUnsignedInteger(to_task, (unsigned long long) to_unitid);
  from_task = manipulateUnsignedInteger(from_task, (unsigned long long) from_unitid);

  //This sleep of 1s ensures that remote dependencies are deferred
  //in order to prevent congestion inside Temanejo, when the respective
  //task is not yet created.
  sleep(1);
  ayu_event_adddependency(to_task, from_task, memaddr, memaddr);
}


static void
call_register_functions () {
  //Task state changes
  dart_tool_register_task_create (&callback_on_task_create, NULL);
  dart_tool_register_task_add_to_queue (&callback_on_task_add_to_queue, NULL);
  dart_tool_register_task_begin (&callback_on_task_begin, NULL);
  dart_tool_register_task_end (&callback_on_task_end, NULL);
  dart_tool_register_task_cancel (&callback_on_task_cancel, NULL);
  dart_tool_register_task_yield_leave (&callback_on_task_yield_leave, NULL);
  dart_tool_register_task_yield_resume (&callback_on_task_yield_resume, NULL);
  //DART unit finializes
  dart_tool_register_task_finalize (&callback_on_task_finalize, NULL);
  //Dependencies
  dart_tool_register_local_dep (&callback_on_local_dep, NULL);
  dart_tool_register_remote_dep(&callback_on_remote_dep, NULL);

}

extern "C" int init_ext_tool(int num_threads, int num_units, int32_t myguid) {
  //set global id for the whole instance
  myglobalid = (unsigned long long) myguid;
  int pid = getpid();
  //If A port <=1024 is called a system port
  //admin right may be needed to open a socket
  if (pid <= 1024) {
    pid = pid + 1024;    
  }
  std::string pid_string = std::to_string(pid);
  //Create an array, where the PID of the current unit is saved
  int sendarray[1];
  sendarray[0] = pid;
  int *rbuf;
  if (myguid == 0) {
   rbuf = (int *)malloc(num_units*sizeof(int));  
  }
  //Create the dart_team_unit_t struct, then override team value to 0 (root).
  //We need this in the dart_gather function to send everything to unit 0. 
  dart_team_unit_t rootTeam;
  dart_team_myid(DART_TEAM_ALL, &rootTeam);
  rootTeam.id = 0;
  dart_ret_t gather = dart_gather(sendarray,rbuf, 1, DART_TYPE_INT, rootTeam, DART_TEAM_ALL);
  //set the port
  int rv = setenv("AYU_PORT", pid_string.c_str(), 1);
  char *envvar = getenv("AYU_PORT");
  if (envvar !=NULL)
    if (myguid == 0) {
      std::string port_list = "";
      //iterate over the integers in the buffer
      for (int i=0; i < num_units; i++) {
      auto s = std::to_string(rbuf[i]);
      port_list.append(s);
      port_list.append(",");
    }
   //To remove the last comma after last iteration
   if (!port_list.empty()) {
     port_list.pop_back();
   }
   std::cout << "Ayudame port list: " << port_list << std::endl;     
  }
  //initialize ayudame
  ayu_event_preinit(AYU_RT_DART);
  ayu_event_init(num_threads); // or AYU_UNKNOWN_NTHREADS
  call_register_functions();
  //Successful initialization
  return 0;
}
