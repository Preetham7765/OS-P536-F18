  #include <stdio.h>
  #include <string.h>
  #include <stdlib.h>
  #include <stdbool.h>
  #include <pthread.h>
  #include <unistd.h>
  #include <getopt.h>
  #include <ctype.h>

  #define THREADS 5
  #define BUFSIZE 2048

  /*
  *
  * structure used to pass data to threads
  * 
  */

  typedef struct threadData {
    int id;
    char *pattern;
    char *filename;
    long int offset;
    size_t start;
    char* begin;

  }threadData;

/*
*
* Global variables
* boolean array to keep track of states of all the threads
*pthread condition variable and mutexes
*/

  bool *b;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t lock;

  /*
  *thread function which looks for pattern in every line
  * starting from start till the offset is reached.
  */

  void* grepFile(void *args) {

    threadData* data = (threadData*) args;
    char* buffer = (char *)calloc(sizeof(char),data->offset + 1);
    FILE* fptr = fopen(data->filename, "r");

    if(fptr) {
      
      char* line = NULL;
      size_t len = 0;
      size_t curReadBytes=0;
      size_t readBytes = 0;

      fseek(fptr, data->start, SEEK_SET);

      while((readBytes = getline(&line,&len,fptr)) != -1 && curReadBytes < data->offset) {
        
        char* match = NULL;
        char* lineSearch  = line;

        while((match = strstr(lineSearch, data->pattern)) != NULL){
          if(match == line || (match != NULL && !isalnum(match[-1]))){

            match += strlen(data->pattern);
            if(!isalnum(*match)){
              strcat(buffer, line);
              break;
            }
            else{
              lineSearch = match;
            }

          }
          else{
            lineSearch = match + strlen(data->pattern);  
          }
          
        }
        curReadBytes+= readBytes;
      }
      if(line != NULL) free(line);
    }

    pthread_mutex_lock(&lock);
      if(data->id ==0){
        printf("%s",buffer);
        pthread_cond_broadcast(&cond);
      }
      else {
        while(!b[(data->id)-1]){
          pthread_cond_wait(&cond, &lock);
        } 
      if(buffer != NULL)
        printf("%s",buffer);
      pthread_cond_broadcast(&cond);
      }

      b[data->id] = true;
      pthread_mutex_unlock(&lock);

      free(buffer);
      pthread_exit(NULL);

  }

  /*
  * This a thread function called when input is given through
  * pipe. Currently ran by only thread.
  * 
  */
  
  void* pipedgrepFile(void *args) {

    threadData* data = (threadData*) args;
    char*buffer = (char *)malloc(data->offset +1);
      
    char* line = NULL;
    size_t curReadBytes=0;
      
      char *token = strtok(data->begin,"\n");
      while(token != NULL && curReadBytes < data->offset){
      
      char* match = NULL;
      char* lineSearch  = token;

      while((match = strstr(lineSearch, data->pattern)) != NULL){
          
        if(match == line || (match != NULL && !isalnum(match[-1]))){
          match += strlen(data->pattern);
          if(!isalnum(*match)){
            sprintf(buffer, "%s%s\n", buffer, token);
            break;
          }
          else{
            lineSearch = match;
          }

        }
        else{
          lineSearch = match + strlen(data->pattern);  
        }

      }
      curReadBytes+= strlen(token);
      token = strtok(NULL, "\n");
      
    }
    
    // pthread_mutex_lock(&lock);
      // if(data->id ==0){
        printf("thread id %d , %s",data->id, buffer);
        // pthread_cond_broadcast(&cond);
      // }
      // else {
        // while(!b[(data->id)-1]){
          // pthread_cond_wait(&cond, &lock);
        // } 
      
      // printf("Thread id %d buffer %s", data->id, buffer);
      // pthread_cond_broadcast(&cond);
      // }

      // b[data->id] = true;
      // pthread_mutex_unlock(&lock);
  
      free(buffer);
      pthread_exit(NULL);

  }

/*
* Driver function to start searching for pattern when
* the input is given through pipe
*/

  void piped_main(char *pattern, int no_of_threads) {
    char *pipe_buffer = (char *)calloc(BUFSIZE * BUFSIZE, sizeof(char));
    FILE *fptr;
    pthread_t threads[no_of_threads];
    threadData *data[no_of_threads];
    size_t fileSize=0;
  
    fptr = fopen("/dev/stdin", "r");
    if(fptr < 0){
      return ;
    }

    fread(pipe_buffer,1, BUFSIZE * BUFSIZE, fptr);
    fileSize = strlen(pipe_buffer);
    int closestLine = 0;
    int i=0;

    for(i=0; i< no_of_threads; i++){
      // set fptr to appropriate value
      data[i] = (threadData *)malloc(sizeof(threadData));
      data[i]->id = i;
      data[i]->pattern = (char *)malloc(strlen(pattern)+1);

      strcpy(data[i]->pattern,pattern);
      data[i]->begin = pipe_buffer + (i * fileSize/no_of_threads + closestLine);
      data[i]->offset = fileSize/no_of_threads;
      char *findline = data[i]->begin + data[i]->offset;
      while(findline[0] != '\n'){
          if(findline[0] != '\0'){
            closestLine++;
            findline++;
            (data[i]->offset)++;
         }
         else{
           break;
         }
        }
      closestLine++;
      (data[i]->offset)++;
      pthread_create(&threads[i], NULL, &pipedgrepFile, data[i]);  
    }
    
    
    for(i=0; i<no_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    free(pipe_buffer);
    for(i=0;i<no_of_threads; i++){
      free(data[i]->pattern);
      free(data[i]->filename);
      free(data[i]);
    }
    
  }
  /*
  * Utitity function that allocated buffer for thread data
  */

  void setupThreadData(threadData **data, char *filename, char *pattern) {

    *data = (threadData *)malloc(sizeof(threadData));
    (*data)->filename = (char*)malloc(strlen(filename)+1);
    (*data)->pattern = (char *)malloc(strlen(pattern)+1);
  }

  /*
  *
  * Utility function to parse command line
  * arguments
  */

  int parseArguments(int argc, char* argv[], char **filename, char **word, int *no_of_threads) {


    // ./paragrep word
    // ./paragrep word [filename]
    // ./paragrep word [filename] [-t]
    // ./paragrep word -t 100
    // ./paragrep word [-t] [filename]
    // ./paragrep word filename -t
    int c ;
    int tflag = 0;
    int i;

    if(argc == 1){
      printf("Usage ./paragrep [-t] <word> <filename>\n");
      return 0;
    }

    if(argc == 2){
      if(!isatty(fileno(stdin))){
        strcpy(*word,argv[1] );
        return 1;
      }
      else{ 
        printf("Usage ./paragrep [-t] <word> <filename>\n");
        return 0;
      }
    }

    while((c = getopt(argc, argv, "t:")) != -1) {

        switch(c){

          case 't':{
            if(!tflag){
              if(optarg != NULL){
                *no_of_threads = atoi(optarg);
              }
            }
            else {
              printf("-t option alerady present\n");
              return 0;
            }
            break;
          }
          case '?':{
            if(optopt == 't'){
              printf("Option -%c requires an argument", optopt);
              return 0;
            }
            else {
              printf("Unknown Option -%c\n", optopt);
              return 0;
            }
          }
          default:{
            printf("Error invalid input\n");
          }

        }

    }
    for(i= optind; i< argc; i++){

      if(!strcmp(*word," ")){
          strcpy(*word, argv[i]); 
        }
        else{
          strcpy(*filename, argv[i]); 
        }
      
    }

    if(!isatty(fileno(stdin)) && strcmp(*filename, " ")) {
        printf("Invalid argument filename should be empty\n");
        return 0;
    }

    if(!strcmp(*word, " ")){
      printf("Invalid argument word should not be empty\n");
      return 0;
    }


    return 1;

  }
/*
* Driver function to handles searches given through commandline
*
*/

  int arg_input(char* filename, char *pattern, int no_of_threads) {
    
    FILE *fptr;
    pthread_t threads[no_of_threads];
    threadData *data[no_of_threads];
    char ch;
    size_t fileSize=0;

    fptr = fopen(filename, "r");
    if(fptr == NULL){
      printf("File not found \n");
      return -1;
    }
    
    fseek(fptr,0,SEEK_END);
    fileSize = ftell(fptr);
    fseek(fptr,0,SEEK_SET);
    int closestLine = 0;
    int i=0;
    int temp =0;
    
    for(i=0; i< no_of_threads-1; i++){
      
      setupThreadData(&data[i], filename, pattern);

      data[i]->id = i;
      strcpy(data[i]->pattern, pattern);
      strcpy(data[i]->filename,filename);
      
      data[i]->start = (i * fileSize/no_of_threads) + temp;
      fseek(fptr, fileSize/no_of_threads, SEEK_CUR);
      closestLine = 0;
      data[i]->offset = (fileSize/no_of_threads);
      while((ch = fgetc(fptr))!= EOF && ch != '\n'){
        closestLine++;
      }

      data[i]->offset +=  closestLine;
      temp+= closestLine;
      pthread_create(&threads[i], NULL, &grepFile, data[i]);
    }
    setupThreadData(&data[i], filename, pattern);
    data[i]->id = i;
    sprintf(data[i]->pattern, "%s", pattern);
    strcpy(data[i]->filename,filename);
    
    data[i]->start = (i * fileSize/no_of_threads) + temp;
    fseek(fptr, fileSize/no_of_threads, SEEK_CUR);
    data[i]->offset = fileSize - data[i]->start;
    pthread_create(&threads[i], NULL, &grepFile, data[i]);

    for(i=0; i<no_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    fclose(fptr);
    for(i=0;i<no_of_threads; i++){
      free(data[i]->pattern);
      free(data[i]->filename);
      free(data[i]);
    }
    
    return 0;
  }

  // Main

  int main(int argc, char* argv[]){

    char *filename = (char*)malloc(256);
    char *pattern = (char*)malloc(256);
    strcpy(filename," ");
    strcpy(pattern," ");
    int no_of_threads= THREADS;

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    if(! parseArguments(argc, argv, &filename, &pattern, &no_of_threads)){
        return 1;
    }

    // printf("Number of threads %d Filename %s word %s\n",no_of_threads,filename, pattern);
    
    b = (bool *)malloc(no_of_threads);
    
    if(!strcmp(filename," ")){
      
        piped_main(pattern, 1);
    }
    else {
      arg_input(filename, pattern, no_of_threads);
    }
    
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);
    free(filename);
    free(pattern);
    free(b);
    

    return 0;
  }

