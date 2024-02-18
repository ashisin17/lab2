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
  // u32 end_time; 
  u32 start_exec_time;
  u32 wait;
  u32 response;
  int first_time;

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

  /* Your code here */
  //simulate RR + update the fields in process struct
  int finished = 0; // flag to indicate if the RR is finished
  int current_time = 0; // curr time in the SIMULAITION
  int quantum_time_left = quantum_length; // remaining time in quantum!
  int max_ARRIVAL = 0; // max arrival time for ALL processes

  struct process* delayed_process; // what happens if time slice expires?
  int is_delayed = 0;

// find the max arrival time amon all the proceses
  for(u32 i = 0; i < size; i++) {
    if(data[i].arrival_time > max_ARRIVAL) {
      max_ARRIVAL = data[i].arrival_time;
    }
  }

  while(!finished){
    for(u32 i = 0; i < size; i++) {
      // only add process to list if its burst time > 0
      if(data[i].arrival_time == current_time && data[i].burst_time > 0){
        struct process* new_process = &data[i];
        TAILQ_INSERT_TAIL(&list, new_process, pointers); // add new process to END of linked list
      }
    }
    //what happens if new process arrives when the current one runnins is FINISHING? -> prioritze the new one
    if(is_delayed) {
      TAILQ_INSERT_TAIL(&list, delayed_process, pointers);
      is_delayed = 0; // no longer delayed
    }
    if(!TAILQ_EMPTY(&list)) {
      // pop off the TOP process
      struct process* curr_process;
      curr_process = TAILQ_FIRST(&list);

      // is this the first time running?
      if(curr_process->first_time != 1) {
        curr_process->first_time = 1;
        curr_process->start_exec_time = current_time;
        curr_process->remain_time = curr_process->burst_time;
      }

      curr_process->remain_time -= 1;
      quantum_time_left -=1;
      current_time +=1;

      // printf("Process PID: %u\n", curr_process->pid);
      // printf("Remaining Burst Time: %u\n", curr_process->remain_time);
      // printf("Quantum Time Left: %d\n", quantum_time_left);
      // printf("Current Time: %d\n", current_time);
      // printf("\n");


      if(curr_process->remain_time<=0){
        total_response_time += curr_process->start_exec_time - curr_process->arrival_time;
        total_waiting_time += current_time - curr_process->arrival_time - curr_process->burst_time;
        TAILQ_REMOVE(&list, curr_process, pointers);
        quantum_time_left = quantum_length;
      }

      // if time slice of process ends, move it to the back of the queue
      if(quantum_time_left <= 0) {
        TAILQ_REMOVE(&list, curr_process, pointers);
        delayed_process = curr_process; // wanna prioritize the new process, so delay THIS cause time slice EXPIRED
        is_delayed = 1;
        quantum_time_left = quantum_length;
      }
    }
    //what happens if process has burst time 0?
    else if(current_time <= max_ARRIVAL){
      quantum_time_left = quantum_length;
      current_time +=1;
    }

    // queue is EMPTY so end the while!
    else
      finished = 1;
    
    // printf("Current Time: %d\n", current_time);
    // printf("Remaining Time in Quantum: %d\n", quantum_time_left);
    // printf("Delayed Flag: %d\n", is_delayed);
    // printf("Finished Flag: %d\n", finished);
    // printf("\n");

  }
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
