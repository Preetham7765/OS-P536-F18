
#include<xinu.h>
#include<stdio.h>


shellcmd xsh_hello(int nargs, char *args[]){
	
	/***
        * Check for number of arguments
        *****/  
	if(nargs < 2 ){
		fprintf(stderr,"%s: too few arguments\n", args[0]);
		fprintf(stderr, "Try %s --help for more information\n", args[0]);
		return 1;
		
	}

	else if(nargs > 2){
		fprintf(stderr, "%s: too many arguments\n", args[0]);
		fprintf(stderr, "Try %s --help for more information\n", args[0]);
		return 1;	
	}

	printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);

	return 0;

}
