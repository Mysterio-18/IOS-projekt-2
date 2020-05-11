//
// Created by dominik on 4/22/19.
//



#ifndef IOS_MAIN_H
#define IOS_MAIN_H

#define S1_NAME "/xbedna69_sem1_writing"
#define S2_NAME "/xbedna69_sem2_check_capacity_left"
#define S3_NAME "/xbedna69_sem3_hack_wait_for_captain"
#define S4_NAME "/xbedna69_sem4_serf_wait_for_captain"
#define S5_NAME "/xbedna69_sem5_stop_captain"
#define S6_NAME "/xbedna69_boat_away"
#define S7_NAME "/xbedna69_captain_chosen"


int parse_args(int argc, char* argv[],long int args_ints[6],unsigned args_unsigned[6]);
int init_file();
int init_semaphores();
int init_shared_memory();
int init_processes(unsigned args_unsigned[6]);
int main_hacker_proc(unsigned int P, unsigned int H, unsigned W, unsigned R);
int main_serf_proc(unsigned int P, unsigned int S, unsigned W, unsigned R);
int hackers_proc(int pi, unsigned W, unsigned R);
int serves_proc(int pi, unsigned W, unsigned R);
int board_hacker_captain(int pi, unsigned R);
int board_serf_captain(int pi, unsigned R);
void clean();

#endif //IOS_MAIN_H
