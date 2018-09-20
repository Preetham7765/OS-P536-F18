  #include <stdio.h>
  #include <string.h>
  #include <stdlib.h>
  #include <stdbool.h>
  #include <pthread.h>

  #define THREADS 5


  typedef struct threadData {
    int id;
    char *pattern;
    char *filename;
    long int offset;
    size_t start;
    char* begin;

  }threadData;

  bool *b;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t lock;

  void* grepFile(void *args) {

    threadData* data = (threadData*) args;
    char* buffer = (char *)malloc(data->offset + 1);
    FILE* fptr = fopen(data->filename, "r");

    if(fptr) {
      
      char* line = NULL;
      long int curCount=0;
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
      // printf("%s",buffer);
      // printf("%s", buffer);
      if(buffer != NULL)
        printf("%s",buffer);
      pthread_cond_broadcast(&cond);
      }

      b[data->id] = true;
      pthread_mutex_unlock(&lock);

      free(buffer);
      pthread_exit(NULL);

  }

  
  void* pipedgrepFile(void *args) {

    threadData* data = (threadData*) args;
    char*buffer = (char *)malloc(data->offset +1);
      
    char* line = NULL;
    long int curCount=0;
    size_t len = 0;
    size_t curReadBytes=0;
    size_t readBytes = 0;
      
      char *token = strtok(data->begin,"\n");
      while(token != NULL && curReadBytes < data->offset){
      
      char* match = NULL;
      char* lineSearch  = token;

      while((match = strstr(lineSearch, data->pattern)) != NULL){
          
        if(match == line || (match != NULL && !isalnum(match[-1]))){
          match += strlen(data->pattern);
          if(!isalnum(*match)){
            // char *final_output;
            // printf("%s\n", token);
            sprintf(buffer, "%s%s\n", buffer, token);
            // strcat(buffer, final_output);
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
    
    pthread_mutex_lock(&lock);
      if(data->id ==0){
        printf("thread id %d , %s",data->id, buffer);
        pthread_cond_broadcast(&cond);
      }
      else {
        while(!b[(data->id)-1]){
          pthread_cond_wait(&cond, &lock);
        } 
      
      printf("Thread id %d buffer %s", data->id, buffer);
      pthread_cond_broadcast(&cond);
      }

      b[data->id] = true;
      pthread_mutex_unlock(&lock);
  
    
      pthread_exit(NULL);

  }



  void piped_main(char *pattern, int no_of_threads) {

    char *pipe_buffer = (char *)calloc(2048 * 2048, sizeof(char));
    FILE *fptr;
    pthread_t threads[no_of_threads];
    threadData *data[no_of_threads];
    char ch;
    size_t fileSize=0;
  
    fptr = fopen("/dev/stdin", "r");

    if(fptr < 0){
      return ;
    }

    // int findline=0;
    char c;
    fread(pipe_buffer,1, 2048 * 2048, fptr);

    
    fileSize = strlen(pipe_buffer);
    // printf("FileSize%ld\n", fileSize);
    int closestLine = 0;
    int i=0;
    int readBytes = 0;
    int threads_running = 0;

  

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
    
    for(int i=0; i<no_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
  }

  void setupThreadData(threadData **data, char *filename, char *pattern) {

    *data = (threadData *)malloc(sizeof(threadData));
    (*data)->filename = (char*)malloc(strlen(filename)+1);
    (*data)->pattern = (char *)malloc(strlen(pattern)+1);
  }

  int parseArguments(int argc, char* argv[], char **filename, char **word, int *no_of_threads) {


    // ./paragrep word
    // ./paragrep word [filename]
    // ./paragrep word [filename] [-t]
    // ./paragrep word -t 100
    // ./paragrep word [-t] [filename]
    // ./paragrep word filename -t

    if(argc == 1){
      printf("Too few arguments");
      return 0;
    }
    else if(argc == 2){
      *word = argv[1];
    }
    else if(argc == 3){
      for(int arg=0; arg < argc; arg++){
        if(strstr(argv[arg], "-t")){
          printf("Invalid arguments passed\n");
          return 0;
        }
      }
      *word = argv[1];
      *filename = argv[2];
    }
    else if(argc == 5){
      for(int arg=1; arg < argc; arg++){
        if(!strcmp(argv[arg],"-t")){
          if(arg == argc -1){
            printf("Invalid arguments passed\n");
            return 0;
          }
          else {
            *no_of_threads = atoi(argv[arg + 1]);
            arg++;
          }
        }
        else if(!strcmp(*word," ")){
          sprintf(*word, "%s",argv[arg]); 
        }
        else{
          sprintf(*filename, "%s",argv[arg]); 
        }
      }
    }
    else {
      printf("Invalid number of arguments\n");
      return 0;
    }

    return 1;
  }


  int arg_input(char* filename, char *pattern, int no_of_threads) {

    FILE *fptr;
    pthread_t threads[no_of_threads];
    threadData *data[no_of_threads];
    char *buffer;
    char ch;
    size_t fileSize=0;

    fptr = fopen(filename, "r");
    if(fptr < 0){
      goto cleanup;
    }

    fseek(fptr,0,SEEK_END);
    fileSize = ftell(fptr);
    fseek(fptr,0,SEEK_SET);
    int closestLine = 0;
    int i=0;
    int temp =0;

    for(i=0; i< no_of_threads-1; i++){
      // set fptr to appropriate value
      
      setupThreadData(&data[i], filename, pattern);

      data[i]->id = i;
      sprintf(data[i]->pattern, "%s", pattern);
      strcpy(data[i]->filename,filename);
      
      data[i]->start = i * fileSize/no_of_threads + temp;
      // printf("start %d\n", data[i]->start);
      fseek(fptr, fileSize/no_of_threads, SEEK_CUR);
      closestLine = 0;
      data[i]->offset = (fileSize/no_of_threads);
      // printf("offset %d\n", data[i]->offset);
      while((ch = fgetc(fptr))!= EOF && ch != '\n'){
        closestLine++;
      }

      data[i]->offset +=  closestLine;
      temp+= closestLine + 1;
      pthread_create(&threads[i], NULL, &grepFile, data[i]);
      
      // printf("close %d\n",closestLine);
    }
    setupThreadData(&data[i], filename, pattern);
    data[i]->id = i;
    sprintf(data[i]->pattern, "%s", pattern);
    strcpy(data[i]->filename,filename);
    
    data[i]->start = (i * fileSize/no_of_threads) + temp;
    fseek(fptr, fileSize/no_of_threads, SEEK_CUR);
    data[i]->offset = fileSize - data[i]->start;
    pthread_create(&threads[i], NULL, &grepFile, data[i]);

    for(int i=0; i<no_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    cleanup:fclose(fptr);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);
    for(i=0;i<no_of_threads; i++){
      free(data[i]->pattern);
      free(data[i]->filename);
      free(data[i]);
    }
    
    return 0;
  }

  int main(int argc, char* argv[]){

    char *filename = (char*)malloc(256);
    char *pattern = (char*)malloc(256);
    strcpy(filename," ");
    strcpy(pattern," ");
    long int lineCount=0;
    int no_of_threads= THREADS;

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    if(! parseArguments(argc, argv, &filename, &pattern, &no_of_threads)){
        return 1;
    }
    // printf("filename %s, pattern %s, thread %d\n", filename, pattern, no_of_threads);
    b = (bool *)malloc(no_of_threads);
    
    if(!strcmp(filename," ")){
        piped_main(pattern, 1);
    }
    else {
      arg_input(filename, pattern, no_of_threads);
    }
    

    // free(filename);
    // free(pattern);
    free(b);

    return 0;
  }

