#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// to use exit() function
#include <unistd.h>			// to use fork(), getpid(), exec() functions 
#include <sys/wait.h>		// to use wait() function
#include <signal.h>			// to use signal() function
#include <fcntl.h>			// to use close(), open() functions


void parseInput(char* cmdline, char** commands)
{
    /* This function will parse the input string into multiple commands 
       or a single command with commandsuments depending on the delimiter (&&, ##, >, or spaces).*/
    
    int i = 0;
    char* str;
    int flag = 1;
    while( (str = strsep(&cmdline," ")) != NULL ) //using strsep to separate the inputs  by space delimiter
	{
		if(strlen(str) == 0) // handles the null string 
		{
    		continue;
    	}
		commands[i]=str;
		i++;
	}
	commands[i]=NULL;
}

void executeCommand(char** commands)
{

	// This function will fork a new process to execute a command.
    if(strcmp(commands[0],"cd") == 0)
    {
        chdir(commands[1]); // To change the Directory.
    }
    else if(strcmp(commands[0],"exit") == 0)
    {
        printf("Exiting shell...\n");
        exit(1);
    }
    else
    {
        size_t pid;
		int status;
        pid = fork();
        if(pid < 0)
        {
            // Error in forking.
            printf("Fork failed");
        	exit(1);
        }
        else if(pid == 0)
        {
            // Child Process.
			signal(SIGINT, SIG_DFL);
    		signal(SIGTSTP, SIG_DFL);
            if(execvp(commands[0],commands)<0) // in case of wrong command.
            {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
        }
        else
        {
			 waitpid(pid, &status, WUNTRACED); //this version of wait system call works even if the child has stopped
        }
    } 
}

void executeParallelCommands(char** commands)
{
	//printf("&& \n");
	// This function will run multiple commands in parallel.
	pid_t pid,pidt;
	int status = 0;
	int i = 0;
	int prev = i;
	while(commands[i] != NULL)
	{
		while(commands[i] != NULL && strcmp(commands[i],"&&") != 0)
		{
			i++;
		}
		commands[i] = NULL;
		pid = fork();
		if(pid < 0)
		{
			exit(1);
		}
		if(pid == 0)
		{
            // child process.
			if(execvp(commands[prev],&commands[prev]) < 0)
			{
				printf("Shell:Incorrect command\n");
				exit(1);
			}
		}
		i++;
		prev = i;
	}
	 while((pidt=wait(&status))>0);
}


void executeSequentialCommands(char** commands)
{
	//printf("## \n");
	// This function will run multiple commands in a sequence.
	size_t cmdsize = 64;
	char** cmnd;
    cmnd = malloc(cmdsize * sizeof(char*));
	int i = 0;
	while((i == 0 && commands[i] != NULL) || (commands[i-1] != NULL && commands[i] != NULL))
	{
		int j = i, pos = 0;
		while(commands[j]!=NULL && strcmp(commands[j],"##") != 0)
		{
			cmnd[pos] = commands[j];
			pos++; 
			j++;
		}
		cmnd[pos] = NULL;
		executeCommand(&cmnd[0]);
		i = j+1;
	}
}

void executeCommandRedirection(char** commands)
{
	//printf("> \n");
	// This function will run a single command with output redirected to an output file specificed by user.
	pid_t pid = fork(); 
	int status;
    if (pid < 0) {
        printf("Fork failed");
        exit(1);
    }
	// child process.
    else if (pid == 0) 
    {    
		int i = 0;
		while(commands[i]) // this block of code is for redirection of output
		{
			int redirected = 0;
			if(strcmp(commands[i],">")==0){
				redirected = 1;
				close(STDOUT_FILENO); // close the standard output
				open(commands[i+1], O_CREAT | O_WRONLY | O_APPEND , S_IRWXU); // open the requested file
			}
			if(redirected)
			{
				//this block helps in removing the commandsuments ex if ls > filename,then already the file is opened in above block by closing stdout so adjust the commands
				int j = i;
				while(commands[j+2])
				{
					strcpy(commands[j],commands[j+2]);
					j++;
				}
				commands[j] = NULL;
				commands[j+1] = NULL;
			}   
			i++;
		}
		if(execvp(commands[0],commands)<0) // in case of wrong command.
        {
            printf("Shell: Incorrect command\n");
            exit(1);
        }
	}
    //Parent process
    else {
        waitpid(pid, &status, WUNTRACED); //this version of wait system call works even if the child has stopped
    }
}

int main()
{
	// Initial Declarations
	char* cmdline;
    size_t cmdsize = 64;
    cmdline = (char *)malloc(cmdsize * sizeof(char));

	/* Used in order to ignore both Ctrl+C(SIGINT) and
        Ctrl+Z(SIGKILL) in the shell*/ 
    signal(SIGINT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);

    while(1)	// This loop will keep your shell running until user enters exit.
    {
        // Print the prompt in format - currentWorkingDirectory$
        char dir[1000];
        printf("%s$", getcwd(dir,1000));
        
         // Taking Input.
        getline(&cmdline,&cmdsize,stdin); // using getline function to take input.
        int len = strlen(cmdline);
		cmdline[len-1] = '\0'; 	//this is done because getline is terminated by '\n' so we replace it with'\0'.

		if(strcmp(cmdline,"") == 0)	// When user enters just enter button.
		{
			continue; // we do nothing and just continue.
		}

        char** commands;
        commands = malloc(cmdsize * sizeof(char*));

		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		parseInput(cmdline,commands); 

        if(strcmp(commands[0],"exit") == 0)	// When user enters exit command.
		{
			printf("Exiting shell...\n");
			break;
        }
		
        int i = 0;
        int parallel=0,sequential=0,redirect=0;
		while(commands[i] != NULL) //checking for parallel and sequential processes and redirection
		{
			if(strcmp(commands[i],"&&")==0){
				parallel=1;
				break;
			}
			if(strcmp(commands[i],"##")==0){
				sequential=1;
				break;
			}
			if(strcmp(commands[i],">")==0){
				redirect=1;
				break;
			}
			i++;
		}
		
		if(parallel)
			executeParallelCommands(commands);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&).
		else if(sequential)
			executeSequentialCommands(commands);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##).
		else if(redirect)
			executeCommandRedirection(commands);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user.
        else
            executeCommand(commands);				// This function is invoked when user wants to run a single commands.
    }
}


