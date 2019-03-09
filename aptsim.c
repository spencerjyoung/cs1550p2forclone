#include <linux/unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include "sem.h"

void up(struct cs1550_sem *sem) {
	syscall(__NR_cs1550_up, sem);
}
void down(struct cs1550_sem *sem) {
	syscall(__NR_cs1550_down, sem);
}
int tenantArrives(int tenant, time_t start){
	printf("Tenant %d arrives at time %ld", tenant, (time(0) - start));
	printf("\n");
	fflush(stdout);
	return 0;
}
int tenantLeaves(int tenant, time_t start){
	printf("Tenant %d leaves the apartment at time %ld", tenant, (time(0) - start));
	printf("\n");
	fflush(stdout);
	return 0;
}
int agentArrives(int agent, time_t start){
	printf("Agent %d arrives at time %ld", agent, (time(0) - start));
	printf("\n");
	fflush(stdout);
	return 0;
}
int agentLeaves(int agent, time_t start){
	printf("Agent %d leaves the apartment at time %ld", agent, (time(0) - start));
	printf("\n");
	fflush(stdout);
	return 0;
}
int openApt(int agent, time_t start){
	printf("Agent %d opens the apartment for inspection at time %ld", agent, (time(0) - start));
	printf("\n");
	fflush(stdout);	
	return 0;
}
int viewApt(int tenant, time_t start){
	printf("Tenant %d inspects the apartment at time %ld", tenant, (time(0) - start));
	printf("\n");
	fflush(stdout);
	sleep(2); //tenant taking two seconds to view apartment
	return 0;
}

int main(int argc, char * argv[]){ 
	
	//COUNTER VARIABLES
	int * count; //to keep track of number of tenants arrived at apartemnt
	int * count_inspection; //to keep track of number of tenants who have inspected apartment
	//STRUCTS
	struct cs1550_sem * mutex_Tenant; //tenant to not enter apartment if 10 are currently inspecting 
	struct cs1550_sem * mutex_Enter_Tenant; //tenant entering apartment only after agent opens apartment
	struct cs1550_sem * semaphore_Agent; //agent leaves after all tenants have left
	struct cs1550_sem * semaphore_Open_Agent; //agent will not open apartment if another agent has opened
	struct cs1550_sem * mutex_Open_Agent; //agent will open apartment only when a tenant is currently at location
	struct cs1550_sem * count_lock; //lock for the count variable
	//OTHER VARIBLES
    int pid; //process id number 
    int randNum; //for random values
//********STILL NEED TO CHANGE NAMES**** above
	//FOR COMMAND LINE ARGS
	int num_Tenants = 0; //-m: number of tenants
	int num_Agents = 0;  //-k: number of agents
	int prob_Tenants = 0; //-pt: probability of a tenant immediately following another tenant
	int prob_Agents = 0; //-pa: probability of an agent immediately following another agent
	int delay_Tenants = 0; //-dt: delay in seconds when a tenant does not immediately follow another tenant
	int delay_Agents = 0; //-da: delay in seconds when an agent does not immediately follow another agent
	int rand_Tenants = 0; //-st: random seed for the tenant arrival process
	int rand_Agents = 0; //-sa: random seed for the agent arrival process
	int opt;
    int i; //last inputted arguement in the terminal

    /*
	//https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    while ((opt = getopt(argc, argv, "m:k:pt:dt:st:pa:da:sa:")) != -1) {
        switch (opt) {
            case 'm':
                num_Tenants = atoi(optarg);
                break;
            case 'k':
                num_Agents = atoi(optarg);
                break;
            case 'pt':
                prob_Tenants = atoi(optarg);
                break;
            case 'dt':
                delay_Tenants = atoi(optarg);
                break;
           case 'st':
                rand_Tenants = atoi(optarg);
                break;
           case 'pa':
                prob_Agents = atoi(optarg);
                break;
           case 'da':
                delay_Agents = atoi(optarg);
                break;
           case 'sa':
                rand_Agents = atoi(optarg);
                break;
            default:
                fprintf(stderr,\
                    "Usage: %s -m [num_Tenants] -k [num_Agents] -pt [prob_Tenants] -dt [delay_Tenants] -st [rand_Tenants] -pa [prob_Agents] -da [delay_Agents] -sa[rand_Agents]\n",\
                    argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    //end of handling command line arguements 
    */
    for(i = 1; i < argc; i+=2){
		if(strncmp(argv[i], "-m", 2) == 0)
			num_Tenants = atoi(argv[i + 1]);
		if(strncmp(argv[i], "-k", 2) == 0)
			num_Agents = atoi(argv[i + 1]);
		if(strncmp(argv[i], "-pt", 3) == 0)
			prob_Tenants = atoi(argv[i + 1]);
		if(strncmp(argv[i], "-dt", 3) == 0)
			delay_Tenants = atoi(argv[i + 1]);
		if(strncmp(argv[i], "-st", 3) == 0)
			rand_Tenants = atoi(argv[i + 1]);
		if(strncmp(argv[i], "-pa", 3) == 0)
			prob_Agents = atoi(argv[i + 1]);
		if(strncmp(argv[i], "-da", 3) == 0)
			delay_Agents = atoi(argv[i + 1]);
		if(strncmp(argv[i], "-sa", 3) == 0)
			rand_Agents = atoi(argv[i + 1]);		
	}
    
    //begin of printing output to the screen 
    printf("The apartment is now empty\n");
    //https://www.geeksforgeeks.org/use-fflushstdin-c/
    //flushing output buffer and moving buffered data to console
	fflush(stdout);
   
	//number of bytes we need
	void *ptr = mmap(NULL, 6*sizeof(struct cs1550_sem) + sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0); 

	semaphore_Agent = (struct cs1550_sem *) ptr;
	semaphore_Open_Agent = semaphore_Agent + 1;
	mutex_Tenant = semaphore_Open_Agent + 1;
	mutex_Open_Agent = mutex_Tenant + 1;
	mutex_Enter_Tenant = mutex_Open_Agent + 1;
	count_lock = mutex_Enter_Tenant + 1;
	count = (int *)(count_lock + 1);
	count_inspection = (int *)(count + 1);

	semaphore_Agent->value = 0; 
	semaphore_Open_Agent->value = 1;
	mutex_Tenant->value = 10;
	mutex_Open_Agent->value = 0;
	mutex_Enter_Tenant->value = 0;
	count_lock->value = 1;
	*count = 0;
	*count_inspection = 0;

	//https://www.tutorialspoint.com/c_standard_library/c_function_time.htm
	time_t start = time(0);
	//https://www.geeksforgeeks.org/fork-system-call/
	pid = fork();

	//FOR THE AGENT
	if (pid <= 0){
		for(i = 0; i < num_Agents; i++){
			pid = fork();
			//if child process is not the agent process
			if(pid != 0){
				//then you want to keep generating agents based on probability
				randNum = 0; //reseting to zero
				srand(rand_Agents);
				randNum = rand() % 100;
				if(randNum >= prob_Agents)
					sleep(delay_Agents);
			} else if (pid == 0){
				//if child process is the agent process  
				//agent arrival
				agentArrives(i, start);
				//signaling you cant open up
				down(semaphore_Open_Agent); 
				down(mutex_Open_Agent);
				//tells the tenant processes that they can begin to enter
				up(mutex_Enter_Tenant); 
					//checking count to see if no tenants are currently at location
					if(*count == 0){ 
						//if no tenants then tell the agent to leave
						agentLeaves(i, start);
						return 0;
					}
				//opening up the apartment
				openApt(i, start); //Open apartment
					//waking up of the tenants
					if(mutex_Tenant->value < 0){ 
						int j;
						for(j = 0; j < 10; j++)
							up(mutex_Tenant);
					}
				//putting agent sem to sleep
				down(semaphore_Agent);
				agentLeaves(i, start);
				//telling other waiting agents to enter 
				up(semaphore_Open_Agent); 

				return 0;//Termination of child process
			} else {
				printf("error 1");
			}

		}
	}
	//FOR THE TENANTS
	//if the child process is the tenant process
	else if (pid > 0){
		for(i = 0; i < num_Tenants; i++){
			pid = fork();
			//if child process is not the tenant process
			if(pid != 0){
				//then you want to keep generating tenants based on probability
				randNum = 0; //reseting to zero
				srand(rand_Tenants);
				randNum = rand() % 100;
				if(randNum >= prob_Tenants)
					sleep(delay_Tenants);
			} 
			//child is the tenant process
			else if (pid == 0) {
				
				//geting the count
				down(count_lock);
				//incrementing num tenants
				*count = *count + 1;
				//relaseing lock 
				up(count_lock);
				

				//tenant arrival
				tenantArrives(i, start);

				
				//agent can open up
				up(mutex_Open_Agent); 
				//downing tenant mutex
				down(mutex_Tenant);
				//cant see place until agent at location
				down(mutex_Enter_Tenant);
				up(mutex_Enter_Tenant);
				//tenant can view the place
				viewApt(i, start);

				down(count_lock);
				//increating the number of tenants inspecting
				*count_inspection  = *count_inspection + 1; 
				up(count_lock);	

				//tenant leaves
				tenantLeaves(i, start);

				down(count_lock);
				//decrementing number of tenants
				*count = *count - 1;
				up(count_lock);

				//agent leaves if tenants leave or more than 10 inspected
				if(*count_inspection % 10 == 0 || *count == 0){ 
									    
					down(count_lock);
					while(mutex_Enter_Tenant->value > 0) 
						down(mutex_Enter_Tenant);
					up(count_lock);
					up(semaphore_Agent);
				}
				return 0; //Termination of child process
			} else {
				printf("error 2");
			}
		}
	} else {
		printf("error 3");
	}

	int x;
	for(x = 0; x < num_Tenants; x++)
		wait(NULL);
	for(x = 0; x < num_Agents; x++)
		wait(NULL);
	wait(NULL);

}









