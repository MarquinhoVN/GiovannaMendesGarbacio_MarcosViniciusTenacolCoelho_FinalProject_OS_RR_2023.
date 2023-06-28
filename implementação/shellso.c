#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

#define tam_max 1024
#define max 50
#define clear() printf("\033[H\033[J")

//Tratamento de Sinal do SIGCHLD
void handle_sigchld(int signum) {
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    if (WIFEXITED(status)) {
        printf("\nProcesso filho %d terminou normalmente com código de saída %d\n", pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("\nProcesso filho %d terminou devido ao sinal %d\n", pid, WTERMSIG(status));
    }
  }
}
//Remover \n
void removerN(char *str) {
  size_t newlinePos = strcspn(str, "\n");
  if (str[newlinePos] == '\n') {
      str[newlinePos] = '\0';
  }
}
//Checar se é pra rodar em background
int background(char** parsed){
  for(int i = 0; i<max; i++){
    if(parsed[i] == NULL){
      if(strcmp(parsed[i-1],"&") == 0){
        parsed[i-1] = NULL;
        return 1;
      }
      break;
    }
  }
  return 0;
}
// Checar se CTRL-D Foi pressionado e Checar finalização do programa
int exitC(int a){
  if(a == 1){
    printf("\nCTRL-D foi pressionado.");
    printf("\nShellso Finalizado.");
  };
  if(a == 0){
    printf("\nShellso Finalizado.");
  }
   return 0;
}

//Dividir a String se tiver pipe
int parsePipe(char* str, char** strpiped){
  //Checa se existe pipe
  int i;
  for (i = 0; i < 2; i++) {
    strpiped[i] = strsep(&str, "|");
    if (strpiped[i] == NULL)
        break;
  }
  if (strpiped[1] == NULL)
        return 0; // Se não tiver Pipe
    else {
        return 1;
    }
}

//Separar os argumentos por espaço e adicionar em um ponteiro de vetor de argumentos char
void parseSpace(char* str, char** parsed){
  //Separa os Argumentos
  int i;
  for (i = 0; i < max; i++) {
    parsed[i] = strsep(&str, " ");
    
    if (parsed[i] == NULL)
        break;
    if (strlen(parsed[i]) == 0)
        i--;
  }
}

//Iniciar processamento do INPUT
int inputProcess(char* str, char** parsed, char** parsedPipe){
  char *strpiped[2];
  int piped = 0;
  removerN(str);
  piped = parsePipe(str, strpiped);

  if(piped == 1){
    parseSpace(strpiped[0],parsed);
    parseSpace(strpiped[1],parsedPipe);
  } else{
    parseSpace(str,parsed);
  }
  if(piped ==1){
    return 1;
  }else{
    return 0;
  }
  
} 

//Contar argumentos do input
int argCount(char** parseArgs){
  int args;
  while (parseArgs[args] != NULL) {
    args++;
  }
  return args;
}
//Executar processo sem pipe
int executar_processos(char** parseArgs){
  pid_t pid;
  int back = background(parseArgs);
  int argc = argCount(parseArgs);
  char* inputFile = NULL;
  char* outputFile = NULL;
  if (argc >= 3) {
    if (strcmp(parseArgs[argc - 2], "<=") == 0) {
      inputFile = parseArgs[argc - 1];
      parseArgs[argc - 2] = NULL;
    } else if (strcmp(parseArgs[argc - 2], "=>") == 0) {
      outputFile = parseArgs[argc - 1];
      parseArgs[argc - 2] = NULL;
    }
  }
  signal(SIGCHLD, handle_sigchld);
  pid = fork();
  if (pid == -1) {
    printf("\nFalha no forking");
    return 1;
  } else if (pid == 0) {
    // Redirecionando a entrada
    if (inputFile != NULL) {
      int inputFd = open(inputFile, O_RDONLY);
      if (inputFd < 0) {
        printf("\nErro ao abrir o arquivo de entrada");
        exit(1);
      }
      dup2(inputFd, STDIN_FILENO);
      close(inputFd);
    }

    // Redirecionando a saída
    if (outputFile != NULL) {
      int outputFd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
      if (outputFd < 0) {
        printf("\nErro ao abrir o arquivo de saída");
        exit(1);
      }
      dup2(outputFd, STDOUT_FILENO);
      close(outputFd);
    }
    close(STDIN_FILENO);
    if (execvp(parseArgs[0], parseArgs) < 0) {
        printf("\nNão foi possível executar o comando..");
    }
    exit(0);
  } else {
      if(back == 0){
        wait(NULL);
      } else{
        printf("\nPID Filho 1 [%d]",pid);
      }
      return 1;
  }
}

//executar processo com pipe
void executar_processosPiped(char** parseArgs,char** parseArgsPipe){
  int pipefd[2]; 
  int back = background(parseArgsPipe);
  char* inputFile = NULL;
  char* outputFile = NULL;
  int argc = argCount(parseArgs);
  int argc2 = argCount(parseArgsPipe);
  if (argc >= 3) {
    if (strcmp(parseArgs[argc - 2], "<=") == 0) {
      inputFile = parseArgs[argc - 1];
      parseArgs[argc - 2] = NULL;
    }
  }
  if (argc2 >= 3) {
   if (strcmp(parseArgsPipe[argc2 - 2], "=>") == 0) {
      outputFile = parseArgsPipe[argc2 - 1];
      parseArgsPipe[argc2 - 2] = NULL;
    }
  }
  
  pid_t p1, p2;
  signal(SIGCHLD, handle_sigchld);
  if (pipe(pipefd) < 0) {
      printf("\nPipe não foi inicializado");
      return;
  }
  p1 = fork();
  if (p1 < 0) {
      printf("\nFalha no forking");
      return;
  } 
  if (p1 == 0) {
    
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    if (inputFile != NULL) {
      int inputfd = open(inputFile, O_RDONLY);
      if (inputfd < 0) {
          perror("Erro ao abrir arquivo de entrada");
          exit(1);
      }
      dup2(inputfd, STDIN_FILENO);
      close(inputfd);
    }
    close(STDIN_FILENO);
    if (execvp(parseArgs[0], parseArgs) < 0) {
        printf("\nNão foi possível executar o comando 1..");
        exit(0);
      }
  } else {
      p2 = fork();

      if (p2 < 0) {
          printf("\nFalha no forking");
          return;
      }
      if (p2 == 0) {

        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        if (outputFile != NULL) {
        int outputFd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (outputFd < 0) {
          printf("\nErro ao abrir o arquivo de saída");
          exit(1);
        }
        dup2(outputFd, STDOUT_FILENO);
        close(outputFd);
        }
        close(STDIN_FILENO);
        if (execvp(parseArgsPipe[0], parseArgsPipe) < 0) {
            printf("\nNão foi possível executar o comando 2..");
            exit(0);
        }
      } else {
          if(back == 0){
            wait(NULL);
            wait(NULL);
          } else{
            printf("\nPID Filho 1 [%d]",p1);
            printf("\nPID Filho 2 [%d]",p2);
          }
      }
  }
}

//iniciar execução, definindo se vai iniciar no pipe ou comando unico
int executar(char** parseArgs, char** parseArgsPipe, int piped){
  int i;
  if(strcmp(parseArgs[0],"\n") == 0 || parseArgs[0] == NULL){
    return 1;
  }
  else if(strcmp(parseArgs[0],"fim") == 0){
    return exitC(0);
  } else if(strcmp(parseArgs[0],"cd") == 0){
    if (chdir(parseArgs[1]) == 0) {
        printf("Diretório alterado para: %s\n", parseArgs[1]);
    } else {
        printf("Erro ao alterar o diretório.\n");
    }
    return 1;
  }
  if(piped == 0){
    executar_processos(parseArgs);
  }else if(piped ==1){
    executar_processosPiped(parseArgs, parseArgsPipe);
  }
  return 1;
}

int main(void) {
  clear();
  while(1){
    char *cmd = (char*)malloc(tam_max), *parseArgs[max], *parseArgsPipe[max], cwd[256];
    getcwd(cwd, sizeof(cwd));
    printf("\nDir:(%s) \tDigite seu comando: ",cwd);
    if(fgets(cmd,tam_max,stdin) == NULL){
     return exitC(1); 
    }
    int piped = inputProcess(cmd,parseArgs,parseArgsPipe);
    int a = executar(parseArgs,parseArgsPipe,piped);
    if(a==0){
      exit(0);
    }
    free(cmd);
  }
}