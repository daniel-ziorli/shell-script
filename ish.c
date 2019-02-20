#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>

const char *mypath[] = { "./",
"/usr/bin/", "/bin/", NULL
};

extern char **getln();

void cleanup(){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;

    fp = fopen(".pid", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while (getline(&line, &len, fp) != -1) {
        kill(atoi(line), SIGKILL);
    }

    fclose(fp);
    if (line)
        free(line);

    remove(".pid");
}

void sendToFile(char * file, char * str){
    remove(file);
    FILE *fp = fopen(file, "ab+");
    fprintf(fp, "%s\n", str);
    fclose(fp);
}

/*
  Function Declarations for builtin shell commands:
 */
int ishCd(char **args);
int ishExit(char **args);
int ishGCD(char **args);
int ishArgs(char **args);
int ishAcker(char **args);


/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtinStr[] = {
    "cd",
    "exit",
    "gcd",
    "args",
    "acker"
};

int (*builtinFunc[]) (char **) = {
    &ishCd,
    &ishExit,
    &ishGCD,
    &ishArgs,
    &ishAcker
};

int ishNumBuiltins() {
    return sizeof(builtinStr) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

long A(int x, int y){
    if (x==0)
        return (y+1);
    if (y==0)
        return A(x-1,1);
    return A(x-1,A(x,y-1));
}

int ishAcker(char **args){
    char line[256];
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: %s [Number] [Number] \n", args[0]);
        return 1;
    }

    fprintf(stderr, "warning this function may never finish\ndo you want to continue?[y]: ");

    fgets(line,256,stdin);

    if (line[0] == 'y'){
        fprintf(stderr, "%ld\n", A(atoi(args[1]), atoi(args[2])));
    }
    return 1;
}

int ishArgs(char **args){
    int argc = 0;
    int i = 0;
    for(i = 1; args[i] != NULL; i++){
        argc++;
    }
    fprintf(stderr, "argc = %d args =", argc);

    for(i = 1; args[i] != NULL; i++){
        fprintf(stderr, " %s", args[i]);
    }
    fprintf(stderr, "\n");
    return 1;
}

int ishGCD(char **args){
    int i = 0;
    int gcd = -1;
    int number1 = 0;
    int number2 = 0;
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: %s [Number or Hex Value] [Number or Hex Value] \n", args[0]);
        return 1;
    }
    else {
        if (args[1][0] == '0' && (args[1][1] == 'x' || args[1][1] == 'X')){
            number1 = strtol(args[1], NULL, 16);
        }
        else{
            number1 = atoi(args[1]);
        }

        if (args[2][0] == '0' && (args[2][1] == 'x' || args[2][1] == 'X')){
            number2 = strtol(args[2], NULL, 16);
        }
        else{
            number2 = atoi(args[1]);
        }

        for(i = 1; i <= number1 && i <= number2; ++i)
        {
            // Checks if i is factor of both integers
            if(number1 % i == 0 && number2 % i == 0){
                gcd = i;
            }
        }

        if (gcd == -1){
            fprintf(stderr, "Error while calculating\n");
            return 1;

        }
        fprintf(stderr, "%d\n", gcd);

    }
    return 1;
}

int ishCd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "ish: expected argument to \"cd\"\n");
    }
    else {
        if (chdir(args[1]) != 0) {
            perror("ish");
        }
    }
    return 1;
}

int ishExit(char **args)
{
    cleanup();
    return 0;
}

int ishLaunch(char **args, int count, char* fileOut, char* fileIn)
{
    pid_t pid;
    int wpid = -1;
    int amperstamp = 0;
    int status = -1;

    if (args[count - 1][0] == '&' && strlen(args[count - 1]) == 1){
        amperstamp = 1;
        args[count - 1] = NULL;
    }

    pid = fork();

    if (pid == 0) {
         //Redirection commences if input was set to one.
        if (fileIn) freopen(fileIn, "r", stdin);

        //Redirection commences it output was set to one
        if (fileOut) freopen(fileOut, "w+", stdout);


        //Execute the first argument
        wpid = execvp(args[0], args);
        //If Execvp is less than 0 there was an error.
        if (wpid != 0)
        {
            perror("ish");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else if (pid < 0) {
            // Error forking
            perror("ish");
    }
    else {
        // Parent process
        if (!amperstamp){
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
        else{
            FILE *fp = fopen(".pid", "ab+");
            fprintf(fp, "%d\n", pid);
            fclose(fp);
        }
    }

    return 1;
}

int ishExecute(char **args)
{
    int i, x, counter = 0;
    char * fileOut = NULL;
    char * fileIn = NULL;

    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < ishNumBuiltins(); i++) {
        if (strcmp(args[0], builtinStr[i]) == 0) {
            return (*builtinFunc[i])(args);
        }
    }

    for(i = 0; args[i] != NULL; i++){

        x = i + 1;
        //Used for redirecting the file, delete from arguments and save make input 1
        if(args[i][0] == '<')
        {
            if (args[x] == NULL){
                return EXIT_FAILURE;
            }
            fileIn = args[x];
            args[i] = args[x];
            args[x] = NULL;
        }

        //Used for redirecting the file, delete from arguments and save make output 1
        if(args[i][0] == '>')
        {
            if (args[x] == NULL){
                return EXIT_FAILURE;
            }
            fileOut = args[x];
            args[i] = args[x];
            args[x] = NULL;
        }

        //Counter for all the arguments
        counter++;
    }

    return ishLaunch(args, counter, fileOut, fileIn);
}

char * GetUserName(){
    return getpwuid(getuid())->pw_name;
}

void ishLoop(void)
{
    char **args;
    int status;

    char hostName[2560];
    char cwd[2560];

    do {
        getcwd(cwd, 2560);
        gethostname(hostName, 2560);
        if(geteuid() == 0) printf("%s@%s:~%s #", GetUserName(), hostName, cwd);
        if(geteuid() != 0) printf("%s@%s:~%s $", GetUserName(), hostName, cwd);

        args = getln();
        status = ishExecute(args);

    } while (status);
}

int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  ishLoop();

  return EXIT_SUCCESS;
}
