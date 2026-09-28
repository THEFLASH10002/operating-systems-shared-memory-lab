#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>

int main()
{
    int ShmID;
    int *ShmPTR;
    pid_t pid;

    /*
     * Create shared memory for two integers:
     * ShmPTR[0] = BankAccount
     * ShmPTR[1] = Turn
     */
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);

    if (ShmID < 0)
    {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory to the parent process.
    ShmPTR = (int *)shmat(ShmID, NULL, 0);

    if (ShmPTR == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // Initialize shared variables.
    ShmPTR[0] = 0; // BankAccount
    ShmPTR[1] = 0; // Turn: Dad goes first

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        exit(1);
    }

    if (pid > 0)
    {
        /*
         * Parent Process - Dear Old Dad
         */
        int account;
        int balance;
        int i;

        srand(time(NULL) ^ getpid());

        for (i = 0; i < 25; i++)
        {
            // Sleep randomly between 0 and 5 seconds.
            sleep(rand() % 6);

            // Copy BankAccount into local variable.
            account = ShmPTR[0];

            // Strict alternation: wait for Dad's turn.
            while (ShmPTR[1] != 0)
                ;

            if (account <= 100)
            {
                // Generate an amount from 0 through 100.
                balance = rand() % 101;

                if (balance % 2 == 0)
                {
                    account += balance;

                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n",
                           balance, account);
                }
                else
                {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            }
            else
            {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n",
                       account);
            }

            // Copy local account back into shared BankAccount.
            ShmPTR[0] = account;

            // Give turn to Student.
            ShmPTR[1] = 1;

            fflush(stdout);
        }

        // Wait for child to finish.
        wait(NULL);

        // Detach and remove shared memory.
        shmdt(ShmPTR);
        shmctl(ShmID, IPC_RMID, NULL);
    }
    else
    {
        /*
         * Child Process - Poor Student
         */
        int account;
        int balance;
        int i;

        srand(time(NULL) ^ getpid());

        for (i = 0; i < 25; i++)
        {
            // Sleep randomly between 0 and 5 seconds.
            sleep(rand() % 6);

            // Copy BankAccount into local variable.
            account = ShmPTR[0];

            // Strict alternation: wait for Student's turn.
            while (ShmPTR[1] != 1)
                ;

            // Generate amount needed from 0 through 50.
            balance = rand() % 51;

            printf("Poor Student needs $%d\n", balance);

            if (balance <= account)
            {
                account -= balance;

                printf("Poor Student: Withdraws $%d / Balance = $%d\n",
                       balance, account);
            }
            else
            {
                printf("Poor Student: Not Enough Cash ($%d)\n", account);
            }

            // Copy local account back into shared BankAccount.
            ShmPTR[0] = account;

            // Give turn back to Dad.
            ShmPTR[1] = 0;

            fflush(stdout);
        }

        // Child only needs to detach.
        shmdt(ShmPTR);
        exit(0);
    }

    return 0;
}