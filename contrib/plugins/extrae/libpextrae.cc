#include <libdash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <functional>
#include <algorithm>
#include <iterator>
#include <inttypes.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "dart_tools.h"
#include "extrae_user_events.h"
#include "extrae_types.h"

//Event value is taken modulo
//to have smaller numbers 
#define modulo_value 100000

//Fixed number for the custom Extrae events
static extrae_type_t et = 130000;

int userdata = 42;
unsigned long long myglobalid;

std::shared_mutex mutex_;
std::unordered_map<extrae_value_t, std::string> nameHash;
std::unordered_map<uint64_t, size_t> idHash;

std::hash<std::string> hash_function;

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
/**
 * Insert the task ID and the task name into a unordered_map.
 * The name of the task is hashed to an unsigned integer,
 * which is used as an event ID in Extrae.
 */

static void sendDataToExtrae () {
  std::vector<char*> v_extraeDescription = {"NONE"};
  std::vector<extrae_value_t> v_extraeValue = {0};
  for(auto it1 = nameHash.begin(); it1 != nameHash.end(); ++it1) {
    v_extraeDescription.push_back((char*)it1->second.c_str());
    v_extraeValue.push_back(it1->first % modulo_value);
  }
  unsigned intermediate_task_count = v_extraeValue.size();
  Extrae_define_event_type(&et, "DART-Tasking", &intermediate_task_count, v_extraeValue.data(), v_extraeDescription.data());
}
static void insertTaskIntoMap(uint64_t task, const char* name) {
  std::unique_lock lock(mutex_);
  size_t hash_value = hash_function(name);
  auto searchHash = nameHash.find(hash_value);
  if (searchHash == nameHash.end()) {
    //printf("hash value of task: %s: %zu\n", name, hash_value);
    nameHash[hash_value] = name;
  }
  idHash[task] = hash_value;
}
/**
 * Send the event_type along with the event ID to Extrae
 */
static void send_task_begin_event(uint64_t task) {
  std::shared_lock lock(mutex_);
  auto search = idHash.find(task);
  if (search != idHash.end()) {
    //ensure that the event values do not exceed the previous defined modulo_value
    Extrae_event(et, search->second % modulo_value);
  }
}
/**
 * Send the null-event to Extrae to end the current event.
 */ 
static void send_task_end_event() {
  Extrae_event(et, 0);
}

extern "C" void callback_on_task_create(uint64_t task, dart_task_prio_t prio, const char *name, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_create.: task = %lu, name: %s, unit_id: %llu, userdata: %d\n", task, name, myglobalid,  *(int *)userdata);
  insertTaskIntoMap(task, name);
}

extern "C" void callback_on_task_begin(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_begin.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);
  send_task_begin_event(task);
  
}

extern "C" void callback_on_task_end(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_end.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);
  send_task_end_event();
  
}

extern "C" void callback_on_task_cancel(uint64_t task, uint64_t thread, void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_cancel.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);
  send_task_end_event();
}

extern "C" void callback_on_task_yield_leave(uint64_t task, uint64_t thread,  void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for tas(char *)k_yield_leave.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);    
  send_task_end_event();
}

extern "C" void callback_on_task_yield_resume(uint64_t task, uint64_t thread,  void *userdata) {
  task = manipulateUnsignedInteger(task, myglobalid);
  //printf("Hi, i'm the callback function for task_yield_resume.: task = %lu, thread: %lu, unit_id: %llu, userdata: %d\n", task, thread, myglobalid, *(int*) userdata);   
  send_task_begin_event(task);
}

extern "C" void callback_on_task_finalize(void *userdata) {
  //number_of_units_finished.operator++();
  //printf("Hi, i'm the callback function for task_finalize.: userdata %d, unit_id %llu\n", *(int*) userdata, myglobalid);
  sendDataToExtrae();
}

void call_register_functions () {
  dart_tool_register_task_create (&callback_on_task_create, &userdata);
  dart_tool_register_task_begin (&callback_on_task_begin, &userdata);
  dart_tool_register_task_end (&callback_on_task_end, &userdata);
  dart_tool_register_task_cancel (&callback_on_task_cancel, &userdata);
  dart_tool_register_task_yield_leave (&callback_on_task_yield_leave, &userdata);
  dart_tool_register_task_yield_resume (&callback_on_task_yield_resume, &userdata);
  dart_tool_register_task_finalize (&callback_on_task_finalize, &userdata);

}

extern "C" int init_ext_tool(int num_threads, int num_units, int32_t myguid) {
  //printf("Hi, i'm the init function. Running with %d threads and %d DASH units. Process id is %d, Unit id is %d\n", num_threads, num_units,pid, myguid);
  //set global id for the whole instance
  myglobalid = (unsigned long long) myguid;
  call_register_functions();
  return 0;
}
