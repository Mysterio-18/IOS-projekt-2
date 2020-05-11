#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <zconf.h>
#include <wait.h>

#include "main.h"

sem_t *sem1 = NULL;
sem_t *sem2 = NULL;
sem_t *sem3 = NULL;
sem_t *sem4 = NULL;
sem_t *sem5 = NULL;
sem_t *sem6 = NULL;
sem_t *sem7 = NULL;

int action_A_identif;
int *action_A;

int capacity_left_identif;
int *capacity_left;

int NH_identif;
int *NH;

int NS_identif;
int *NS;

int ready_for_board_NH_i;
int *ready_for_board_NH;

int ready_for_board_NS_i;
int *ready_for_board_NS;

FILE *pFile;

int main(int argc, char* argv[]) {
// P H S R W C
    int err = 0;
    long int args_ints[6];
    unsigned args_unsigned[6];

    err = parse_args(argc,argv,args_ints,args_unsigned);
    if(err)
        return 1;

    err = init_file();
    if(err)
        return 1;
    err = init_shared_memory();
    if(err)
        return 1;
    *action_A = 0;
    *NH = 0;
    *NS = 0;
    *ready_for_board_NS = 0;
    *ready_for_board_NH = 0;
    *capacity_left = args_unsigned[5];

    err = init_semaphores();
    if(err)
        return 1;

    err = init_processes(args_unsigned);
    if(err)
        return 1;

    clean();
    return 0;
}

int init_processes(unsigned int args_unsigned[6])
{
    pid_t hacker;
    pid_t serf;
    hacker = fork();
    if(hacker == -1)
    {
        fprintf(stderr,"Neúspěšné vytvoření procesu hacker\n");
        clean();
        return 1;
    }
    else if(hacker == 0)
    {
        int err = main_hacker_proc(args_unsigned[0],args_unsigned[1], args_unsigned[4], args_unsigned[3]);
        if(err)
            exit(1);
        exit(0);
    }
    else
    {
        serf = fork();
        if(serf == -1)
        {

            fprintf(stderr,"Neúspěšné vytvoření procesu hacker\n");
            clean();
            return 1;
        }
        else if(serf == 0)
        {
            int err = main_serf_proc(args_unsigned[0],args_unsigned[2], args_unsigned[4], args_unsigned[3]);
            if(err)
                exit(1);
            exit(0);
        }
        else
        {
            int state,state2;
            waitpid(hacker,&state,0);
            waitpid(serf,&state2,0);
            if(WEXITSTATUS(state) || WEXITSTATUS(state2))
                return 1;
            return 0;
        }
    }


}

int main_hacker_proc(unsigned int P, unsigned int H, unsigned W, unsigned R)
{
    pid_t hacks[P];
    pid_t hack;
    for(unsigned int i = 0;i<P;i++)
    {
        if(H)
        {
            unsigned sleep_time = (unsigned) lrand48() % (H+1);
            sleep_time = sleep_time * 1000;
            usleep(sleep_time);
        }
        hack = fork();
        if(hack < 0)
        {
            fprintf(stderr,"Nelze vytvořit další proces hack\n");
            clean();
            return 1;
        }
        if(hack == 0)
        {
            sem_wait(sem1);
            (*action_A)++;
            fprintf(pFile,"%d    : HACK %d    : starts\n",*action_A,i+1);
            sem_post(sem1);
            hacks[i] = getpid();
            int err = hackers_proc(i+1,W,R);
            if(err)
                exit(1);
            exit(0);
        }
    }

    for(unsigned int i = 0; i<P;i++)
    {
        waitpid(hacks[i],NULL,0);
    }
    return 0;

}

int main_serf_proc(unsigned int P, unsigned int S, unsigned W, unsigned R)
{
    pid_t serfs[P];
    pid_t serf;
    for(unsigned int i = 0;i<P;i++)
    {
        if(S)
        {
            unsigned sleep_time = (unsigned) lrand48() % (S+1);
            sleep_time = sleep_time * 1000;
            usleep(sleep_time);
        }
        serf = fork();
        if(serf < 0)
        {
            fprintf(stderr,"Nelze vytvořit další proces serf\n");
            clean();
            return 1;
        }
        if(serf == 0)
        {
            sem_wait(sem1);

            fprintf(pFile,"%d    : SERF %d    : starts\n",++(*action_A),i+1);
            sem_post(sem1);
            serfs[i] = getpid();
            int err = serves_proc(i+1, W, R);
            if(err)
                exit(1);
            exit(0);
        }
    }

    for(unsigned int i = 0; i<P;i++)
    {
        waitpid(serfs[i],NULL,0);
    }
    return 0;
}

int hackers_proc(int pi, unsigned W, unsigned R)
{
    sem_wait(sem2);
    if((*capacity_left) > 0)
    {
        (*capacity_left)--;
        (*ready_for_board_NH)++;
        sem_wait(sem1);
        fprintf(pFile,"%d    : HACK %d    : waits : %d: %d\n",++(*action_A), pi, ++(*NH), *NS);
        sem_post(sem1);
        sem_post(sem2);
        sem_wait(sem2);
        if((*ready_for_board_NH == 2 && *ready_for_board_NS >= 2) || *ready_for_board_NH == 4)
        {
            if(*ready_for_board_NH == 4)
                *ready_for_board_NH = 0;
            else
            {
                *ready_for_board_NH = *ready_for_board_NH -2;
                *ready_for_board_NS = *ready_for_board_NS -2;
            }
            sem_post(sem2);
            sem_wait(sem6);
            int err = board_hacker_captain(pi,R);
            if(err)
                return 1;
            sem_post(sem6);
        }

        else
        {
            sem_post(sem2);
            sem_wait(sem3);
            sem_wait(sem2);
            sem_wait(sem1);
            fprintf(pFile,"%d    : HACK %d    : member exits: %d: %d\n",++(*action_A), pi, *NH, *NS);
            sem_post(sem1);
            sem_post(sem2);
            sem_post(sem5);
        }

    }
    else
    {
        sem_post(sem2);
        sem_wait(sem2);
        sem_wait(sem1);
        fprintf(pFile,"%d:    HACK %d:    leaves queue: %d: %d\n",++(*action_A), pi, *NH, *NS);
        sem_post(sem1);
        sem_post(sem2);
        unsigned sleep_time = (unsigned) lrand48() % (W+1);
        sleep_time = sleep_time * 1000;
        usleep(sleep_time);
        sem_wait(sem1);
        fprintf(pFile,"%d:    HACK %d:    is back\n",++(*action_A), pi);
        sem_post(sem1);
        hackers_proc(pi,W,R);
    }
    return 0;
}

int serves_proc(int pi, unsigned W, unsigned R)
{
    sem_wait(sem2);
    if((*capacity_left) > 0)
    {
        (*capacity_left)--;
        (*ready_for_board_NS)++;
        sem_wait(sem1);

        fprintf(pFile,"%d    : SERF %d    : waits : %d: %d\n",++(*action_A), pi, *NH, ++(*NS));
        sem_post(sem1);
        sem_post(sem2);
        sem_wait(sem2);
        if((*ready_for_board_NS == 2 && *ready_for_board_NH >= 2) || *ready_for_board_NS == 4)
        {
            if(*ready_for_board_NS == 4)
                *ready_for_board_NS = 0;
            else
            {
                *ready_for_board_NS = *ready_for_board_NS -2;
                *ready_for_board_NH = *ready_for_board_NH -2;
            }

            sem_post(sem2);
            sem_wait(sem6);
            int err = board_serf_captain(pi,R);
            if(err)
                return 1;
            sem_post(sem6);
        }
        else
        {
            sem_post(sem2);
            sem_wait(sem4);
            sem_wait(sem2);
            sem_wait(sem1);
            fprintf(pFile,"%d    : SERF %d    : member exits: %d: %d\n",++(*action_A), pi, *NH, *NS);
            sem_post(sem1);
            sem_post(sem2);
            sem_post(sem5);
        }
    }
    else
    {
        sem_post(sem2);
        sem_wait(sem1);
        fprintf(pFile,"%d:    SERF %d:    leaves queue: %d: %d\n",++(*action_A), pi, *NH, *NS);
        sem_post(sem1);
        unsigned sleep_time = (unsigned) lrand48() % (W+1);
        sleep_time = sleep_time * 1000;
        usleep(sleep_time);
        sem_wait(sem1);
        fprintf(pFile,"%d:    SERF %d:    is back\n",++(*action_A), pi);
        sem_post(sem1);
        serves_proc(pi,W,R);

    }
    return 0;
}

int board_hacker_captain(int pi, unsigned R)
{
    int *only_hackers = malloc(sizeof(int));
    if(only_hackers == NULL)
    {
        fprintf(stderr, "Neúspěšný malloc\n");
        clean();
        return 1;
    }
    sem_wait(sem2);
    if(*NH >=4)
    {
        *NH = *NH - 4;
        *only_hackers = 1;
    }
    else
    {
        *NH = *NH - 2;
        *NS = *NS - 2;
        *only_hackers = 0;
    }
    *capacity_left = *capacity_left +4;
    sem_wait(sem1);
    fprintf(pFile,"%d    : HACK %d    : boards: %d: %d\n",++(*action_A), pi,*NH,*NS);
    sem_post(sem1);
    sem_post(sem2);

    if(R)
    {
        unsigned sleep_time = (unsigned) lrand48() % (R+1);
        sleep_time = sleep_time * 1000;
        usleep(sleep_time);
    }
    if(*only_hackers)
    {
        sem_post(sem3);
        sem_post(sem3);
        sem_post(sem3);
    }
    else
    {
        sem_post(sem3);
        sem_post(sem4);
        sem_post(sem4);
    }
    free(only_hackers);
    sem_wait(sem5);
    sem_wait(sem5);
    sem_wait(sem5);
    sem_wait(sem2);
    sem_wait(sem1);
    fprintf(pFile,"%d    : HACK %d    : captain exits : %d: %d\n",++(*action_A), pi,*NH,*NS);
    sem_post(sem1);
    sem_post(sem2);

    return 0;
}

int board_serf_captain(int pi, unsigned R)
{
    int *only_serves = malloc(sizeof(int));
    if(only_serves == NULL)
    {
        fprintf(stderr,"Neúspěšný malloc\n");
        clean();
        return 1;
    }
    sem_wait(sem2);
    if(*NS >=4)
    {
        *NS = *NS - 4;
        *only_serves = 1;
    }
    else
    {
        *NH = *NH - 2;
        *NS = *NS - 2;
        *only_serves = 0;
    }
    *capacity_left = *capacity_left +4;
    sem_wait(sem1);
    fprintf(pFile,"%d    : SERF %d    : boards: %d: %d\n",++(*action_A), pi,*NH,*NS);
    sem_post(sem1);
    sem_post(sem2);

    if(R)
    {
        unsigned sleep_time = (unsigned) lrand48() % (R+1);
        sleep_time = sleep_time * 1000;
        usleep(sleep_time);
    }
    if(*only_serves)
    {
        sem_post(sem4);
        sem_post(sem4);
        sem_post(sem4);
    }
    else
    {
        sem_post(sem3);
        sem_post(sem3);
        sem_post(sem4);
    }
    free(only_serves);

    sem_wait(sem5);
    sem_wait(sem5);
    sem_wait(sem5);
    sem_wait(sem2);
    sem_wait(sem1);
    fprintf(pFile,"%d    : SERF %d    : captain exits : %d: %d\n",++(*action_A), pi,*NH,*NS);
    sem_post(sem1);
    sem_post(sem2);

    return 0;
}


int init_file()
{
    pFile = fopen("proj2.out","w");
    if(pFile == NULL)
    {
        fprintf(stderr, "Nelze otevřít/vytvořit soubor pro zápis.\n");
        return 1;
    }
    setbuf(pFile,NULL);
    return 0;
}


int init_shared_memory()
{
    action_A_identif = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT | IPC_EXCL | 0666);
    if(action_A_identif < 0)
    {
        clean();
        fprintf(stderr,"Nebyl přidělen validní identifikátor sdílené paměti\n");
        return 1;
    }

    action_A = shmat(action_A_identif,NULL,0);
    if(*action_A == -1)
    {
        clean();
        fprintf(stderr,"Nebyla přidělena sdílená paměť\n");
        return 1;
    }

    capacity_left_identif = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT | IPC_EXCL | 0666);
    if(capacity_left_identif < 0)
    {
        clean();
        fprintf(stderr,"Nebyl přidělen validní identifikátor sdílené paměti\n");
        return 1;
    }

    capacity_left = shmat(capacity_left_identif,NULL,0);
    if(*capacity_left == -1)
    {
        clean();
        fprintf(stderr,"Nebyla přidělena sdílená paměť\n");
        return 1;
    }
    NH_identif = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT | IPC_EXCL | 0666);
    if(NH_identif < 0)
    {
        clean();
        fprintf(stderr,"Nebyl přidělen validní identifikátor sdílené paměti\n");
        return 1;
    }

    NH = shmat(NH_identif,NULL,0);
    if(*NH == -1)
    {
        clean();
        fprintf(stderr,"Nebyla přidělena sdílená paměť\n");
        return 1;
    }

    NS_identif = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT | IPC_EXCL | 0666);
    if(NS_identif < 0)
    {
        clean();
        fprintf(stderr,"Nebyl přidělen validní identifikátor sdílené paměti\n");
        return 1;
    }

    NS = shmat(NS_identif,NULL,0);
    if(*NS == -1)
    {
        clean();
        fprintf(stderr,"Nebyla přidělena sdílená paměť\n");
        return 1;
    }
    ready_for_board_NH_i = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT | IPC_EXCL | 0666);
    if(ready_for_board_NH_i < 0)
    {
        clean();
        fprintf(stderr,"Nebyl přidělen validní identifikátor sdílené paměti\n");
        return 1;
    }

    ready_for_board_NH = shmat(ready_for_board_NH_i,NULL,0);
    if(*ready_for_board_NH == -1)
    {
        clean();
        fprintf(stderr,"Nebyla přidělena sdílená paměť\n");
        return 1;
    }

    ready_for_board_NS_i = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT | IPC_EXCL | 0666);
    if(ready_for_board_NS_i < 0)
    {
        clean();
        fprintf(stderr,"Nebyl přidělen validní identifikátor sdílené paměti\n");
        return 1;
    }

    ready_for_board_NS = shmat(ready_for_board_NS_i,NULL,0);
    if(*ready_for_board_NS == -1)
    {
        clean();
        fprintf(stderr,"Nebyla přidělena sdílená paměť\n");
        return 1;
    }
    return 0;
}

int init_semaphores()
{
    sem1 = sem_open(S1_NAME, O_CREAT | O_EXCL, 0666, 1);
    if(sem1 == SEM_FAILED)
    {
        clean();
        fprintf(stderr,"Neúspěšná inicializace prvního semaforu.\n");
        return 1;
    }
    sem2 = sem_open(S2_NAME, O_CREAT | O_EXCL, 0666, 1);
    if(sem2 == SEM_FAILED)
    {
        clean();
        fprintf(stderr,"Neúspěšná inicializace druhého semaforu.\n");
        return 1;
    }
    sem3 = sem_open(S3_NAME, O_CREAT | O_EXCL, 0666, 0);
    if(sem3 == SEM_FAILED)
    {
        clean();
        fprintf(stderr,"Neúspěšná inicializace třetího semaforu.\n");
        return 1;
    }
    sem4 = sem_open(S4_NAME, O_CREAT | O_EXCL, 0666, 0);
    if(sem4 == SEM_FAILED)
    {
        clean();
        fprintf(stderr,"Neúspěšná inicializace čtvrtého semaforu.\n");
        return 1;
    }
    sem5 = sem_open(S5_NAME, O_CREAT | O_EXCL, 0666, 0);
    if(sem5 == SEM_FAILED)
    {
        clean();
        fprintf(stderr,"Neúspěšná inicializace pátého semaforu.\n");
        return 1;
    }
    sem6 = sem_open(S6_NAME, O_CREAT | O_EXCL, 0666, 1);
    if(sem6 == SEM_FAILED)
    {
        clean();
        fprintf(stderr,"Neúspěšná inicializace pátého semaforu.\n");
        return 1;
    }
    sem7 = sem_open(S7_NAME, O_CREAT | O_EXCL, 0666, 1);
    if(sem7 == SEM_FAILED)
    {
        clean();
        fprintf(stderr,"Neúspěšná inicializace pátého semaforu.\n");
        return 1;
    }
    return 0;
}


int parse_args(int argc, char* argv[],long int args_ints[], unsigned args_unsigned[6])
{
    char *endptr = NULL;
    if(argc != 7)
    {
        fprintf(stderr,"Špatný počet zadaných argumentů - %i, správný počet - 6.\n",argc-1);
        return 1;
    }
    for(int i = 1;i<argc;i++)
    {
        args_ints[i-1] = strtol(argv[i],&endptr,10);
        if(!endptr)
        {
            fprintf(stderr,"Argument číslo %d není integer.\n",i);
            return 1;
        }
    }
    if(args_ints[0] < 2 || (args_ints[0] % 2) != 0)
    {
        fprintf(stderr,"Argument 1 není v povoleném rozsahu.\n");
        return 1;
    }
    for(int i = 1; i<4; i++)
    {
        if(args_ints[i] < 0 || args_ints[i] > 2000)
        {
            fprintf(stderr,"Argument %d není v povoleném rozsahu.\n",i+1);
            return 1;
        }
    }
    if(args_ints[4] < 20 || args_ints[4] > 2000)
    {
        fprintf(stderr,"Argument 5 není v povoleném rozsahu.\n");
        return 1;
    }
    if(args_ints[5] < 5)
    {
        fprintf(stderr,"Argument 6 není v povoleném rozsahu.\n");
        return 1;
    }
    for(int i = 0;i<6;i++)
    {
        args_unsigned[i] = (unsigned)args_ints[i];
    }

    return 0;
}

void clean()
{
    fclose(pFile);
    shmdt(action_A);
    shmctl(action_A_identif,IPC_RMID,NULL);
    shmdt(capacity_left);
    shmctl(capacity_left_identif,IPC_RMID,NULL);
    shmdt(NS);
    shmctl(NS_identif,IPC_RMID,NULL);
    shmdt(NH);
    shmctl(NH_identif,IPC_RMID,NULL);
    sem_close(sem1);
    sem_unlink(S1_NAME);
    sem_close(sem2);
    sem_unlink(S2_NAME);
    sem_close(sem3);
    sem_unlink(S3_NAME);
    sem_close(sem4);
    sem_unlink(S4_NAME);
    sem_close(sem5);
    sem_unlink(S5_NAME);
    sem_close(sem6);
    sem_unlink(S6_NAME);
    sem_close(sem7);
    sem_unlink(S7_NAME);
}


