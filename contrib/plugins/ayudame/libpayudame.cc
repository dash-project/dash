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

int data_to_send_create = 42;
int data_to_send_begin = 4242;
int data_to_send_end = 424242;
int data_to_send_dep = 64;
int data_to_send_yield = 32;

unsigned long long myglobalid;

std::set<std::string> set;
std::set<std::string>::iterator it;

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

extern "C" void callback_on_task_create(uint64_t task, dart_task_prio_t prio, const char *name, void *userdata) {
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

extern "C" void callback_on_task_begin(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  ayu_event_preruntask(task, thread);
  ayu_event_runtask(task);
  
}

extern "C" void callback_on_task_end(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_end.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);
  ayu_event_postruntask(task);
  ayu_event_removetask(task); 
}

extern "C" void callback_on_task_cancel(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_cancel.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);
  ayu_event_removetask(task);      
}

extern "C" void callback_on_task_yield_leave(uint64_t task, uint64_t thread,  void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_yield_leave.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);    
}

extern "C" void callback_on_task_yield_resume(uint64_t task, uint64_t thread,  void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_yield_resume.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);    
}

extern "C" void callback_on_task_finalize(void *userdata) {
  //printf("Hi, i'm the callback function for task_finalize.: userdata %d\n", *(int*) userdata);
  ayu_event_finish();
}

extern "C" void callback_on_task_add_to_queue(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_add_to_queue.: task: %lu, thread %lu:, unit_id: %llu. userdata %d\n", task, thread, myglobalid, *(int*) userdata);
  ayu_event_addtasktoqueue(task, thread);
}

extern "C" void callback_on_local_dep(uint64_t task1, uint64_t task2, uint64_t memaddr, int32_t unitid, int edge_type, void *userdata) {
  task1 = manipulateUnsignedInteger(task1, (unsigned long long) unitid);
  task2 = manipulateUnsignedInteger(task2, (unsigned long long) unitid);
  ayu_event_adddependency(task2, task1, memaddr, memaddr);
}

extern "C" void callback_on_remote_dep (uint64_t to_task, uint64_t from_task, int to_dep_type, int from_dep_type, uint64_t memaddr, int32_t to_unitid, int32_t from_unitid, int edge_type, void *userdata) {
  //printf("local task = %#018" PRIx64 "\n", to_task);
  //printf("remote task = %#018" PRIx64 "\n", from_task);
  to_task = manipulateUnsignedInteger(to_task, (unsigned long long) to_unitid);
  from_task = manipulateUnsignedInteger(from_task, (unsigned long long) from_unitid);
  //printf("local task AFTER MASK = %#018" PRIx64 "\n", to_task);
  //printf("remote task AFTER MASK = %#018" PRIx64 "\n", from_task);

  //This sleep of 1s ensures that remote dependencies are deferred
  //in order to prevent congestion inside Temanejo, when the respective
  //task is not yet created.
  sleep(1);
  ayu_event_adddependency(to_task, from_task, memaddr, memaddr);
}


void call_register_functions () {
  //Task state changes
  dart_tool_register_task_create (&callback_on_task_create, &data_to_send_create);
  dart_tool_register_task_add_to_queue (&callback_on_task_add_to_queue, &data_to_send_create);
  dart_tool_register_task_begin (&callback_on_task_begin, &data_to_send_begin);
  dart_tool_register_task_end (&callback_on_task_end, &data_to_send_end);
  dart_tool_register_task_cancel (&callback_on_task_cancel, &data_to_send_create);
  dart_tool_register_task_yield_leave (&callback_on_task_yield_leave, &data_to_send_yield);
  dart_tool_register_task_yield_resume (&callback_on_task_yield_resume, &data_to_send_yield);
  //DART unit finializes
  dart_tool_register_task_finalize (&callback_on_task_finalize, &data_to_send_end);
  //Dependencies
  dart_tool_register_local_dep (&callback_on_local_dep, &data_to_send_dep);
  dart_tool_register_remote_dep(&callback_on_remote_dep, &data_to_send_dep);

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
  int sendarray[0];
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
  //printf("The return value of setenv is %d\n", rv);
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
