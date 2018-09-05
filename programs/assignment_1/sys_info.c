#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#define BUFSIZE 20

int main(int argc, char *argv[]){
	
    int pid, fdesc[2];

    /*****
     * Check if exactly two parameters are given.
     * One for the program name and other for command to execute.
    ******/

    if(argc < 2) {
	
	fprintf(stderr, "Error too few arguments");
	return 1;
    }
    else if(argc > 2){
	fprintf(stderr, "Error too many arguments");
        return 1;

    } 	
    /****
     * Create a pipe where fdesc[0] is used to read
     * and fdesc[1] is use to write
     * Used to send command from parent to child
    ***/ 

    if(pipe(fdesc) == -1){
	
	fprintf(stderr, "Creating Pipe Error");
	
	return 1;
    }

    // fork a new process to execute the given command.
    pid = fork();

    // error failed to create new process.
    if(pid <0){
	
	fprintf(stderr, "Error from fork");
	return 1;

    }
    else if(pid ==0){

        // Child process

        char command[BUFSIZE];

        // close the read file descriptor because child wont write 
        // to the pipe
        close(fdesc[1]);

        printf("Child: %d\n", getpid());

        // read the command passed by the parent
        if(read(fdesc[0], command, BUFSIZE) < 0){

	     fprintf(stdout, "Cannot read data");
	     return 1;

        }
        else{
	   
            // execute the command using execl()
	    if(strcmp(command, "/bin/echo") == 0){	
		execl(command, command, "Hello World!", NULL);
	    }else {
		execl(command,command, NULL);
		
	    }
        }

   }
   else {

    //Parent
    printf("Parent: %d\n", getpid());
   
    // close the read file descriptor for parent process since it doesn't read.
    close(fdesc[0]);
   
   // write the command to the pipe 
    if(write(fdesc[1],argv[1], BUFSIZ) < 0){
	fprintf(stderr, "write failed");
	return 1;
   } 
   
   // wait for parent to exit
   wait(NULL);
   return 0;

  }

}
