#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  // perform calc
  u32 remain_time;
  u32 end_time; 
  u32 start_exec_time;
  u32 wait;
  u32 response;

  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process); // initialize linked list
// this struct acts as the head of the linked list

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
    // add initialization values!
    (*process_data)[i].remain_time = 0;
    (*process_data)[i].end_time = 0;
    (*process_data)[i].start_exec_time = 0;
    (*process_data)[i].wait = 0;
    (*process_data)[i].response = 0;
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size; // NUM of processes!
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list); // inititalize queue/linked list

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  printf("Initialization values of additional fields:\n");
    for (u32 i = 0; i < size; ++i)
    {
        printf("Process %u:\n", i + 1);
        printf("  PID: %u\n", data[i].pid);
        printf("  Arrival Time: %u\n", data[i].arrival_time);
        printf("  Burst Time: %u\n", data[i].burst_time);
        printf("  Remain Time: %u\n", data[i].remain);
        printf("  End Time: %u\n", data[i].end_time);
        printf("  Start Execution Time: %u\n", data[i].start_exec_time);
        printf("  Wait: %u\n", data[i].wait);
        printf("  Response: %u\n", data[i].response);
    }

  /* Your code here */
  //simulate RR + update the fields in process struct
  if(quantum_length < 0)
    return EINVAL;

  // 1: insert process into list, SORTED by arrival times
  for(u32 i = 0; i < size; i++){
    struct process* new_process = &data[i];
    struct process* curr_process;

    // iterate through list list to find currect pos on arrival time!
    TAILQ_FOREACH(curr_process, &list, pointers){ // forward traversal to go top to bottom of list
      if(curr_process->arrival_time > new_process->arrival_time) { // if new process arrival time EARLIER, insert before curr process
        TAILQ_INSERT_BEFORE(curr_process, new_process, pointers);
        break;
      }
    }

    // if new process has LATEST arrival time, insert at tail
    if(!curr_process) // currn process = NULL or 0 if itts NOT inserted!
      TAILQ_INSERT_TAIL(&list, new_process, pointers);
  }

  // 2: CURRENT TIME = start the timer
  u32 current_time = 0;
  struct process *current_process = NULL;

  // 3: ROUND ROBIN SCHEDULING!
  while(!TAILQ_EMPTY(&list)){
    // get 1st process in list
    current_process = TAILQ_FIRST(&list);
    // remove IT from list
    TAILQ_REMOVE(&list, current_process, pointers);

    // CALCs
    u32 remaintime = current_process->remain_time;
    if(current_process->start_exec_time == 0)
      current_process->start_exec_time = current_time;

    if(remaintime > 0){
      // CASE 1: remain time is > than quantum
      if(remaintime > quantum_length){
        current_process->remain_time = remaintime - quantum_length;
        current_time += quantum_length;
        current_process->end_time = current_time;
      }
      // CASE 2: remain time is <= quantum
      else {
        current_time += remaintime;
        current_process->remain_time = 0;
        current_process->end_time = current_time;
      }
    }

    // response and waiting times
    current_process->wait = current_process->end_time - current_process->arrival_time - current_process->burst_time;
    current_process->response = current_process->start_exec_time - current_process->arrival_time;

    // print process ID, wait time, and response time
    printf("%u\t%u\t%u\n", current_process->pid, current_process->wait, current_process->response);

    //TOTALS
    total_waiting_time += current_process->wait;
    total_response_time += current_process->response;

    // completed process moves to end of queue if STILL has some time
    if (current_process->remain_time > 0)
        TAILQ_INSERT_TAIL(&list, current_process, pointers);
  }
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
