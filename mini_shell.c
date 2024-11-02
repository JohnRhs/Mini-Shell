#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <errno.h>
#include <fcntl.h>

void outil_pwd(int argc, char **argv);
void ls(int argc, char **argv);

struct builtin{
    char *nom; 
    void (*fonction)(int, char **);
};

struct command {
    int argc;
    char **argv;
};

int count_args(const char *s) {
    int n = 0;
    char p = ' ';

    while (*s) {
        if (isspace(p) && !isspace(*s)) {
            n++;
        }
        p = *s++;
    }
    return n;
}


struct command *
parse_cmd(const char *buf, int len) {
    struct command *cmd = NULL;
    char prev = ' ';
    int n, beg = 0;

    if ((n = count_args(buf)) == 0) return NULL;

    cmd = malloc(sizeof(*cmd));
    cmd->argc = 0;
    cmd->argv = malloc((n + 1) * sizeof(*cmd->argv));

    for (int i = 0; i < len; i++) {
        if (!isspace(buf[i]) && isspace(prev)) { 
            beg = i;
        } else if (isspace(buf[i]) && !isspace(prev)) {
            cmd->argv[cmd->argc++] = strndup(buf + beg, i - beg);
        }
        prev = buf[i];
    }

    cmd->argv[cmd->argc] = NULL;
    return cmd;
}

#define NB_BUILTINS 4

void builtin_exit(int argc, char **argv) {
    if (argc == 1) {
        _exit(0);
    } else {
        perror("_exit");
        exit(EXIT_FAILURE);
    }
}

void builtin_cd(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "Deja dans le repertoire personnel\n");
        fflush(stderr);
    }
    else{
        if((chdir(argv[1]) < 0)){
            perror("chdir");
            exit(EXIT_FAILURE);
        }
    }
}

const struct builtin builtins[NB_BUILTINS] = {
    {"exit", builtin_exit},
    {"cd", builtin_cd},
    {"pwd", outil_pwd},
    {"ls", ls},
};

int find_builtin(char *cmd){
    for (int i = 0; i < NB_BUILTINS; i++){
        if(strcmp(cmd, builtins[i].nom) == 0){
            return i;
        }
    }
    return -1;   
}

void free_cmd(struct command *cmd){
    for(int i = 0; i < cmd->argc; i++){
        free(cmd->argv[i]);
    }
    free(cmd->argv);
    free(cmd);
}

void exec_cmd(struct command *cmd){
    if(cmd != NULL){
        pid_t p;
        if((p = fork()) < 0){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if(p == 0){
            signal(SIGINT, SIG_DFL);
            if((execvp(cmd->argv[0], cmd->argv + 1)) == -1){
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
        else if(p != 0){
            wait(NULL);
        }
    }
}

void exec_piped_cmds(struct command *cmd1, struct command *cmd2){
    int tube[2];
    pid_t p1, p2;
    if(pipe(tube) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if((p1 = fork()) < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(p1 == 0){
        dup2(tube[1], STDOUT_FILENO);
        close(tube[0]);
        close(tube[1]);
        if((execv(cmd1->argv[0], cmd1->argv + 1)) == -1) {
            exec_cmd(cmd1);
        }    
    }
    wait(NULL);
    if((p2 = fork()) < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(p2 == 0){
        dup2(tube[0], STDIN_FILENO);
        close(tube[1]);
        close(tube[0]);
        if((execv(cmd2->argv[0], cmd2->argv + 1)) == -1) {
            exec_cmd(cmd2);
        }
    }
    close(tube[0]);
    close(tube[1]);
    wait(NULL);
    wait(NULL);
}
        

void outil_pwd(int argc, char **argv) {
    char buf[1024];

    if (getcwd(buf, sizeof(buf)) != NULL) {
        printf("%s\n", buf);
    } else {
        perror("getcwd");
    }
    return;
}

void ls(int argc, char **argv) {
    DIR *repertoire;
    struct dirent *entree;
    if((repertoire=opendir(".")) == NULL){
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    while ((entree = readdir(repertoire)) != NULL) {
        if (entree->d_name[0] != '.') {
            printf("%s    ", entree->d_name);
        }
    }
    printf("\n");
    closedir(repertoire);
}

int main(int argc, char **argv){
    char *username, hostname[50], pwd[50];
    char buf[1024];
    int len;
    char *pipe;
    signal(SIGINT, SIG_IGN);
    if((username = getlogin()) == NULL){
        perror("getlogin");
        exit(EXIT_FAILURE);
    }
    
    if(gethostname(hostname, 50) < 0){
        perror("gethostname");
        exit(EXIT_FAILURE);
    }

    if(getcwd(pwd, 50) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    for(;;){        
        fprintf(stderr,"%s@%s:%s$ ", username, hostname, pwd);
        fflush(stderr);
        if((len = read(STDIN_FILENO, buf, 1024)) < 0){
            perror("read");
            exit(EXIT_FAILURE);
        }
        if(len == 0) break;
        if(len > 0){
            if((pipe = strchr(buf, '|')) != NULL){
                *pipe = '\0';
                struct command *cmd1 = parse_cmd(buf, pipe - buf);
                struct command *cmd2 = parse_cmd(pipe + 2, strlen(pipe + 2));
            
                if (cmd1 != NULL && cmd2 != NULL) {
                    exec_piped_cmds(cmd1, cmd2);
                    if (cmd1 != NULL){
                        free_cmd(cmd1);
                    }
                    if (cmd2 != NULL){
                        free_cmd(cmd2);
                    }
                }
            }
            else{
                struct command *cmd = parse_cmd(buf, len);
                for (int i = 0; i < len; i++) {
                    if (strcmp(&(buf[i]), "\n") == 0) {
                        buf[i] = '\0';
                        break;
                    }
                    if (strcmp(&(buf[i]), "\t") == 0) {
                        buf[i] = '\0';
                        break;
                    } else if(buf[i] == '\"') {
                        memcpy(&buf[i], &buf[i + 1], len - i);
                        i--;
                    }
                }
                if (find_builtin(buf) != -1){
                    builtins[find_builtin(buf)].fonction(argc, argv);
                }
                else if(cmd != NULL){
                    if(cmd->argc > 0){
                        exec_cmd(cmd);
                    }   
                    free_cmd(cmd);
                }
            }
        }
    }
    fprintf(stderr, "Bye\n");
    return 0;
    //printf("%d", find_builtin("exit"));
}