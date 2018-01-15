//Author Peter Adamson
//command line arguments accepted:
//					-n: the program expects an integer following this argument specifying the number of students
//					-stop: the program expects an integer following this argument specifying how long to run
//					-min: the program expects an integer following this argument specifying the minimum random number to be generated
//					-max: the program expects an integer following this argument specifiying the maximum random number to be generated

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
//#include <unistd.h>

//define the node structure for the queue
typedef struct qNode Node;
struct qNode
{
	int studentNumber;
	int helpTime;
	Node *next;	
};

//global variable declarations
int n;
int stop;
int min;
int max;
int isSleeping; //0 if awake, 1 if sleeping
int studentCount;	//counter to assign student numbers
int studentBeingHelped;
int helpTime;
int count;
Node *waiting[3];	//number of waiting slots available
pthread_mutex_t lock1;

//function declarations
void *student();
void *TA();
int randInt(int min, int max);

int main(int argc, char **argv)
{
	if(argc != 9)	//incorrect number of arguments have been specified
	{
		printf("incorrect number of arguments supplied\n");
		return 1;
	}

	//iterate through the arguments
	int i;
	for(i = 1; i < argc - 1; i = i + 2)
	{
		int j = i + 1;
		if(strcmp(argv[i],"-n") == 0)	//we have the number of students
		{
			n = atoi(argv[j]);
		}
		else if(strcmp(argv[i],"-stop") == 0)	//we have the timeout time
		{
			stop = atoi(argv[j]);
		}
		else if(strcmp(argv[i],"-min") == 0)	//we have the minimum random number to generate
		{
			min = atoi(argv[j]);
		}
		else if(strcmp(argv[i],"-max") == 0)	//we have the maximum random number to generate
		{
			max = atoi(argv[j]);
		}
		else					//we have an invalid argument
		{
			printf("one or more arguments specified incorrectly\n");
			return 1;
		}
	}
	
	//set up our end time condition
	time_t endAt;
	time_t start = time(NULL);
	time_t end = stop;
	endAt = start + end;

	//variable initialization
	count = 0;
	studentCount = 1;
	isSleeping = 1;
	helpTime = 0;
	for(i = 0; i < 3; i++)
	{
		waiting[i] = NULL;
	}

	//declare the teacher assistant and students
	pthread_t taThread;
	pthread_t studentThread[n];

	//create the teacher assistant and students
	pthread_create(&taThread, NULL, TA, NULL);
	for(i = 0; i < n; i++)
	{
		pthread_create(&studentThread[i], NULL, student, NULL);
	}

	//loop the main thread until timeout has been reached
	while(1)
	{
		if(start < endAt)	//we have not reached the timeout
		{
			sleep(1);
			start = time(NULL);
		}
		else			//we have reached the timeout
		{
			pthread_cancel(taThread);	//cancel the TA
			for(i = 0; i < n; i++)
			{
				pthread_cancel(studentThread[i]);	//cancel the students
			}
			break;
		}
	}

	//free up assigned waiting memory
	for(i = 0; i < 3; i++)
	{
		if(waiting[i] != NULL)
		{
			free(waiting[i]);
			waiting[i] = NULL;
		}
	}
}

void *TA()
{
	while(1)	//continuous loop
	{
		while(isSleeping == 1)	//TA is napping
		{
			printf("The TA is napping.\n");
			sleep(1);
		}
		while(isSleeping == 0)	//TA is awake
		{
				pthread_mutex_lock(&lock1);
				printf("student %d is getting help for %d seconds.\n",studentBeingHelped,helpTime);
				sleep(helpTime);
				isSleeping = 0;
				count = count - 1;	
				if(count == 0)	//there are no students waiting
				{
					helpTime = 0;
					isSleeping = 1;
					pthread_mutex_unlock(&lock1);
					break;
				}
				else	//there is at least one student waiting
				{
					Node *student = waiting[0];
					studentBeingHelped = student->studentNumber;
					helpTime = student->helpTime;

					//move students up the waiting list
					int i;
					int j;
					for(i = 0; i < count - 1; i++)
					{
						for(j = i + 1; j > count; j++);
							waiting[i] = waiting[j];
							waiting[j] = NULL;
					}
					isSleeping = 0;
					pthread_mutex_unlock(&lock1);
				}
		}
	}	
}

void *student()
{
	pthread_mutex_lock(&lock1);
	int studentNumber = studentCount;
	pthread_mutex_unlock(&lock1);
	int programmingTime;
	int studentHelpTime;
	pthread_mutex_lock(&lock1);
	studentCount = studentCount + 1;
	pthread_mutex_unlock(&lock1);
	Node *student = NULL;
	student = (Node*)malloc(sizeof(Node));
	student->studentNumber = studentNumber;
	while(1)	//continuous loop
	{
		programmingTime = randInt(min, max);
		studentHelpTime = randInt(min, max);
		student->helpTime = studentHelpTime;
		sleep(programmingTime);
		pthread_mutex_lock(&lock1);
		if(count <= 3)	//the TA is either available, or waiting slots are available
		{
			if(count == 0)	//No students in line, TA is available
			{
				isSleeping = 0;
				count = count + 1;
				studentBeingHelped = studentNumber;
				helpTime = studentHelpTime;
				printf("student %d went straight to the TA\n",studentBeingHelped);
				pthread_mutex_unlock(&lock1);
			}
			else	//students in line, or one with the TA
			{
				printf("student %d is waiting.\n",studentNumber);
				waiting[count - 1] = student;
				count = count + 1;	
				pthread_mutex_unlock(&lock1);
			}
		}
		else	//TA is not available and no waiting slots are available
		{
			pthread_mutex_unlock(&lock1);
			printf("student %d left mad.\n",studentNumber);
		}
	}
}

//generates a random integer between min and max (inclusively)
int randInt(int min, int max)
{
	return rand() % (max + 1 - min) + min;
}
