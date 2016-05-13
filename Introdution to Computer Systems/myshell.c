#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
/*===========================================================================*/
/*WRITING UTILITIES*/
/*===========================================================================*/

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void err() {
  char error_message[30] = "An error has occurred\n";
  write(STDOUT_FILENO, error_message, strlen(error_message));
}

/*===========================================================================*/
/*GLOBAL VARS*/
/*===========================================================================*/

char *array[1024];
char *dw;

/*===========================================================================*/
/*STRUCTS*/
/*===========================================================================*/

typedef struct rtoks rtoks;
typedef struct cmd cmd;
typedef struct cmd_line cmd_line;

struct rtoks{
  char *tok;
  rtoks *next;
};
struct cmd{
  rtoks *t;
  cmd *next;
};
struct cmd_line{
  cmd *c;
  cmd_line *next;
};

/*===========================================================================*/
/*STRUCT UTILITIES*/
/*===========================================================================*/

rtoks *make_rtok(char *s)
{
  
  rtoks *rtok_new = (rtoks*)malloc(sizeof(rtoks));
  rtok_new->tok = strdup(s); 
  rtok_new->next = NULL;
  return rtok_new;
}

cmd *make_cmd(rtoks *rt)
{
  cmd *new_cmd = (cmd*)malloc(sizeof(cmd));
  new_cmd->t = rt;
  new_cmd->next = NULL;
  return new_cmd;
}

cmd_line *make_cmdline(cmd *c)
{
  cmd_line *cmd_new = (cmd_line*)malloc(sizeof(cmd_line));
  cmd_new->c = c;
  cmd_new->next = NULL;
  return cmd_new; 
}

void free_rtoks(rtoks *r)
{
  if(r != NULL){
    free(r->tok);
    free_rtoks(r->next);
    free(r);
    return;
  }else {
    return;
  }
}

void free_cmd(cmd *c)
{
  if(c != NULL){
    free_rtoks(c->t);
    free_cmd(c->next);
    free(c);
    return;
  }else {
    return;
  } 
}

void free_cmdline(cmd_line *cl)
{
  if(cl  != NULL){
    free_cmd(cl->c);
    free_cmdline(cl->next);
    free(cl);
    return;
  }else {
    return;
  }
}

void rtok_show(rtoks *r) 
{
  rtoks *temp = r; 

  while(temp != NULL){
    myPrint(temp->tok);
    if(r->next != NULL){
      write(1, " ", 1);
    }
    temp=temp->next;
  }
  return;
}

void cmd_show(cmd *c) 
{
  cmd *temp = c; 

  while(temp != NULL){
    rtok_show(temp->t);
    if(temp->next != NULL){
      write(1, ">", 1);
    }
    temp=temp->next;
  }
  return;
}

void cmd_lineshow(cmd_line* c) 
{
  cmd_line *temp = c; 

  while(temp!=NULL){
    cmd_show(temp->c);
    if(temp->next != NULL){
      write(1, ";", 1);
    }
    temp=temp->next;
  }
  return;
}

int r_length(rtoks *r)
{
  int n = 0; /*incrementer*/
  rtoks *backup = r; /*to keep track of the ptr to beginning of rtoks*/

  if(r == NULL){
    return 0;
  } else {
    while(backup->next != NULL){
      n++;
      backup = backup->next;
    }
    return n+1;
  }
}

int cmd_length(cmd *c)
{
  int n = 0;
  cmd *b = c;

  while(b != NULL){
    n++;
    b = b->next;
  }
  return n;
}

int cmdline_len(cmd_line *cl)
{
  int n = 0;
  cmd_line *cc = cl;

  while(cc != NULL){
    n++;
    cc = cc->next;
  }
  return n;
} 

/*===========================================================================*/
/*MISCELLANEOUS HELPERS*/
/*===========================================================================*/

void arr_args(rtoks* r, char**argss)
/*for taking a linked list of rtoks and place the strings into an array of ptrs*/
{
  int i = 0;
  int n = r_length(r); /*to know where end of rtoks* is*/
  rtoks *backup = r;

  while(backup != NULL){
    argss[i] = backup->tok;
    i++;
    backup = backup->next;
  }
  argss[n] = NULL; /*set to NULL for execvp*/
  return;
}

void collapsed_str(char* s1, char* s2)
/*this will take a command and take out all the white spaces, used for the exceptions
functions*/ 
{
  int len = strlen(s1);
  int o_count; /*know where we are in original string*/
  int r_count = 0; /*know where we are in the return string*/
 
  memset(s2, '\0', len);
  for(o_count = 0; o_count<len; o_count++) {
    if ((s1[o_count]==' ')||(s1[o_count]=='\t')){ /*if white space, do nothing*/
      continue; 
    }
    else {
      s2[r_count]=s1[o_count];
      r_count++; /*else, set r_count element in s2 to non white space character*/
    }
  }
  s2[r_count]='\0'; /*to set r_count element in s2 to NULL to terminate string*/
  return;
}

int check_space(char* s) 
/*to see if a command is all blank spaces*/
{
  int count = 0;
  int i;
  int len = strlen(s);

  for(i=0; i<len; i++) {
    if((s[i]==' ')||(s[i]=='\t')) /*if space or tab, add to count*/
      count+=1;
  }
  return (len-count-1); /*if 0, all blanks*/
}

/*===========================================================================*/
/*EXCEPTION HELPER FUNCTIONS*/
/*===========================================================================*/
/*This is for the random-ass shit that we needed for stuff to work*/

int exception1(char *s)
/*if the last char in a command is redirection, error*/
{
  int i = 0;
  int len = strlen(s);

  if(s[len-2]=='>'){
    i= 1;
  }
  return i;
}

int exception2(char* s)
/*to catch ">;" ">>"*/
{
  int i = 0;
  char* where;
  where = strstr(s, ">"); /*will return location of ''>'' in s*/

  if (where!=NULL) { 
    if((where[1]=='>')||(where[1]==';')) /*to see what next element is*/
      i = 1;
  }
  return i;
}

int exception3(char* s)
/*to see if input is all ";" */
{
  int i = 0;
  int x = 0;
  int inc = (int)';'; /*int val of ';'*/
  int len = strlen(s)-1;
  int str_val = 0;

  for(;x<len;x++){ /*to find ascii val of the string*/
    str_val+=(int)s[x];
  }
  if ((len*inc)==str_val){ /*see if ascii val = len*val of ';' */
    i=1;
  }
  return i;
}

/*===========================================================================*/
/*PARSING FUNCTIONS*/
/*===========================================================================*/

rtoks *parseRedToken(char *s)
/*to break up a string and make linked list of redtoken by spaces and \t*/
{
  char *saveptr=s;
  char *token;
  token = strtok_r(saveptr, " \t", &saveptr);
  rtoks *rt = make_rtok(token);
  rtoks *f = rt;

  while(token != NULL){
    token = strtok_r(NULL, " \t", &saveptr);
    if(token == NULL){
      return f;
    }
    rt->next = make_rtok(token);
    rt = rt->next;
  }  
  return f;
}

cmd *parseCmd(char *s)
/*to break up string by '>' and then pass that off to parseRedToken*/
{
  char *saveptr=s;
  char *token;
  token = strtok_r(saveptr, ">", &saveptr);
  cmd *cm = make_cmd(parseRedToken(token));
  cmd *f = cm;

  while(token != NULL){
    token = strtok_r(NULL, ">", &saveptr);
    if(token == NULL) {
      return f;
    }
    cm->next = make_cmd(parseRedToken(token));
    cm = cm->next; 
  }
  return f;
}

cmd_line *parseCmdLine(char *s)
/*to break up string by ';' and '\n' and pass that string to parseCmd*/
{
  char *saveptr=s;
  char *token;
  token = strtok_r(saveptr, ";\n", &saveptr);
  cmd_line *cml = make_cmdline(parseCmd(token));
  cmd_line *f = cml;

  while(token != NULL){
    token = strtok_r(NULL, ";\n", &saveptr);
    if(token ==NULL){
      return f;
    }
    if((strcmp(token, " ") == 0) || (strcmp(token, "\t") == 0)){
      continue;
    }else {
      cml->next = make_cmdline(parseCmd(token));
      cml = cml->next;
    }
  }
  return f; 
}


/*===========================================================================*/
/*COMMAND PROCESSING*/
/*===========================================================================*/

void processCmd(cmd *c)
/*to take a single command and then process it based on whether it is exit, cd, etc
if it isnt any of them, pass off to execvp*/
{
  int n = r_length(c->t);
  char **argss = (char**)malloc(sizeof(char*) * (n+1));
  arr_args(c->t, argss);
  char *first = c->t->tok;
  char buf[1024];
  char *sw;
  char *filename;
  int status = 0;
  pid_t cpid;
  int x;
  char *str;
  char *curr;
  char cwd[256];

  if(strcmp(first, "exit") == 0){
    if(((c->t->next) == NULL) && (c->next == NULL)){
      exit(0);
    } else { 
      err();
    }
  }else if (strcmp(first, "cd") == 0){
    if(r_length(c->t) <= 2){  
      if(c->next == NULL){
    if(c->t->next == NULL){
      chdir((const char*) getenv("HOME"));
    } else if(chdir((const char*)c->t->next->tok) == -1) {
      err();
    } else{
      curr=getcwd(cwd, sizeof(cwd));
      chdir(curr);
       setenv("PWD",curr,1);
    }
      } else {
    err();
      }
    }else {
      err();
    }
    
  } else if (strcmp(first, "pwd") == 0){
    if((c->t->next == NULL) && (c->next == NULL)){
      sw = getcwd(buf, 1024);
      if(sw != NULL){
    myPrint(sw);
    write(1, "\n", 1);
      }
      else{ 
    err();
      }
    } else{
      err();
    }
  }else {
    if(cmd_length(c) > 2){
      err();
    }
    if(cmd_length(c) == 1){
      cpid = fork();
      waitpid(-1, &status, 0);
      if(cpid == 0){
    if(execvp(first, argss) == -1) {
      err();
      exit(-1);
    }
    kill(cpid, SIGKILL);
      }
    }
    if(cmd_length(c) == 2){
      str =malloc((strlen(dw)+2)*sizeof(char));
      strcpy(str, dw);
      strcat(str, "/");
      filename = strcat(str, c->next->t->tok);
      if((x = open(filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) != -1){
    cpid = fork();
    waitpid(-1, &status, 0);
    if(cpid==0){
      dup2(x, 1);
      if(execvp(first, argss) == -1) {
        err();
        exit(-1);
      }
      kill(cpid, SIGKILL);
      close(x);
      free(str);
    }
    dup2(1,1);
      } else {
    err();
      }
    }
  }
  free(argss);
}



/*===========================================================================*/
/*MAIN*/
/*===========================================================================*/

int main(int argc, char *argv[]) 
{
  char cmd_buff[1024];
  char *pinput;
  char *fname = argv[1];
  char *mode = "r";
  FILE *f= fopen(fname, mode);
  cmd_line *c, *backup;
  char* extra_buff;
  char* test = malloc(sizeof(char)*514);
  char buf[1024];

  if(argv[2] != NULL){ /*if trying to run batch with 2 filenames*/                                                
    err();
    exit(0);
  }

  if(f == NULL){ /*if read is invalid*/
    err();
    exit(0);
  }
  dw = getcwd(buf, 1024); /*for global var accessible to function calls*/
  while (1) {
    /*INTERACTIVE MODE*/
    if(argv[1] == NULL){ 
      myPrint("myshell> ");
      pinput = fgets(cmd_buff, 100, stdin);
      c = parseCmdLine(cmd_buff);
      backup = c;
      while(backup != NULL){
        processCmd(backup->c);
        backup = backup->next;
      }
    /*BATCH MODE*/ 
    }  else if(argv[1] != NULL){
      memset(cmd_buff,'\0',514);
      pinput = fgets(cmd_buff, 514, f);
      
      if (!pinput) { /*invalid fgets*/
        exit(0);
      }

      if((extra_buff = strstr(cmd_buff,"\n"))!=NULL) {/*check if whole command has been taken in*/
        if (check_space(cmd_buff)==0) /*if all blank spaces, don't print*/
          continue;
        else {
      myPrint(cmd_buff); /*print command*/
      collapsed_str(cmd_buff,test); 
      /*CATCH ERRORS RIGHT AWAY*/
      if(exception1(test)==1){
        err();
      }
      else if(exception2(test)==1) {
        err();
      }
      else if(exception3(test)==1) {
        continue;
      }
      /*PARSE*/
      else {
        c = parseCmdLine(cmd_buff);
        backup = c;
        while(backup != NULL){
          processCmd(backup->c);
          
          backup = backup->next;
        }
        free_cmdline(c);
      }
    }
      }
      /*COMMAND TOO LONG*/
      if(extra_buff==NULL) {
        myPrint(cmd_buff);

        while(fgets(cmd_buff,1024,f)!=NULL) {/*keep getting more chars while you can*/
          if((strlen(cmd_buff)<1023) || cmd_buff[1022]=='\n') { /*if new one is shorter than max, break*/
            myPrint(cmd_buff);
            memset(cmd_buff, '\0', 1024);
            break;
          }
          else 
            myPrint(cmd_buff); /*else keep getting*/
          memset(cmd_buff, '\0', 1024);
        }
    err();
      }
      
    }
  }
  free(test);   
fclose(f);
return 0;
}
