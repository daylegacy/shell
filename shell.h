#include "parser.h"
//#include <sys/types>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#define COMMANDS_SIZE 10
typedef std::vector<char *> cmd;
typedef std::vector<const char *> mod;
int is_mod(const char * mod, std::vector<const char*> modifiers){
        for(int k=0; k<modifiers.size(); k++){
            if(strcmp(mod, modifiers[k])==0){
                return 1;
            }
        }
        return 0;
    }

#ifdef DEBUG
#define printD(...) \
	do{ \
		printf(__VA_ARGS__); \
	}while(0); 
#else
#define printD(...) \
	do{ \
	}while(0); 
#endif

void wait_pids(std::vector<pid_t>& pids, std::vector<mod>& modifiers, int & to_execute, int & last_rv, int i, std::vector<char * >& coms){
    std::vector<int> rvs;
    rvs.reserve(pids.size());
    printD("waiting for %d procs:\n", pids.size());
    for(int i=0;i< pids.size();i++){
        printD("%d \n", pids[i]);
    }
    while(1){// check if all forks exited
        int n_of_finished=0;
        for(int i =0;i<pids.size();i++){
            if(pids[i]==0) {n_of_finished++;continue;}
            int status=-5;
            printD("SHELL: waiting for %d(%s)\n",pids[i], coms[i]);
            pid_t return_pid = waitpid(pids[i], &status, WNOHANG); 
            if (return_pid == -1) {
                printD("ERROR child%d(%s)", pids[i], coms[i]);
            } else if (return_pid == 0) {
                printD("child%d(%s) still running\n",pids[i], coms[i]);
            } else if (return_pid == pids[i]) {
                rvs[i] = WEXITSTATUS(status);
                printD("SHELL: child%d(%s) finished status = %d childs rv:%d\n", pids[i], coms[i], status, WEXITSTATUS(status));
                pids[i] =0;
            }
        }
        if(n_of_finished==pids.size()) break;
        //usleep(30);
    }
    if(pids.size())last_rv = rvs[pids.size()-1];//care
    coms.resize(0);
    pids.resize(0);
    if(i>0 && is_mod("&&", modifiers[i-1])){
        printD("last rv = %d\n", last_rv);
        if(last_rv==0) to_execute=1;
        else to_execute=0;
    }
    if(i>0 && is_mod("||", modifiers[i-1])){
        printD("last rv = %d\n", last_rv);
        if(last_rv==0) to_execute=0;
        else to_execute=1;
    }
}


void create_pipes(std::vector<mod>& modifiers,  int i, int  (&pipe_out)[2], int  (&pipe_in)[2]){
    if(is_mod("|", modifiers.data()[i]) && !(i>0 && is_mod("|", modifiers[i-1]))){ //pipe begins
        pipe(pipe_out);
    }
    else if(is_mod("|", modifiers[i]) && (i>0 && is_mod("|", modifiers[i-1]))){ //pipe continues
        pipe_in[0] = pipe_out[0];pipe_in[1] = pipe_out[1];
        pipe(pipe_out);
    }
    else if((i>0 && is_mod("|", modifiers[i-1])) && !is_mod("|", modifiers[i])){ //pipe ends
        pipe_in[0] = pipe_out[0];
        pipe_in[1] = pipe_out[1];
    }
}
void redirect_stdio(std::vector<mod>& modifiers,  int i, int  (&pipe_out)[2], int  (&pipe_in)[2]){
    if(is_mod("|", modifiers[i]) && !(i>0 && is_mod("|", modifiers[i-1]))){ //pipe begins
        close(pipe_out[0]); dup2(pipe_out[1], STDOUT_FILENO);
    }
    else if(is_mod("|", modifiers[i]) && (i>0 && is_mod("|", modifiers[i-1]))){ //pipe continues
        close(pipe_in[1]);  close(pipe_out[0]);
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
    }
    else if((i>0 && is_mod("|", modifiers[i-1])) && !is_mod("|", modifiers[i])){ //pipe ends
        close(pipe_in[1]);  dup2(pipe_in[0], STDIN_FILENO);
    }
}

void close_descriptors(std::vector<mod>& modifiers,  int i, int  (&pipe_out)[2], int  (&pipe_in)[2], std::vector<int> &fd_to_close){
    if(is_mod("|", modifiers[i]) && !(i>0 && is_mod("|", modifiers[i-1]))){ //pipe begins
        close(pipe_out[1]);
    }
    else if(is_mod("|", modifiers[i]) && (i>0 && is_mod("|", modifiers[i-1]))){ //pipe continues
        close(pipe_out[1]);
        fd_to_close.push_back(pipe_in[0]);
    }
    else if((i>0 && is_mod("|", modifiers[i-1])) && !is_mod("|", modifiers[i])){ //pipe ends
        fd_to_close.push_back(pipe_in[0]);
    }
}

void process_write_to_file(std::vector<cmd> & commands, int i, int & fd, int type){
    if(type ==1){
        printD("%s: detected > \n", commands[i][0]);
        fd = open(commands[i][commands[i].size()-1], O_WRONLY|O_CREAT|O_TRUNC);
        dup2(fd, STDOUT_FILENO);
        commands[i][commands[i].size()-2]=NULL;
    }
    if(type==2){
        printD("%s: detected >> \n", commands[i][0]);
        fd = open(commands[i][commands[i].size()-1], O_WRONLY|O_APPEND|O_CREAT);
        dup2(fd, STDOUT_FILENO);
        commands[i][commands[i].size()-2]=NULL;
    }
}

bool is_in_pipe(std::vector<mod>& modifiers,  int i,std::vector<cmd> & commands){
    return (is_mod("|", modifiers[i])&&commands.size()!=1) || (i>0 && is_mod("|", modifiers[i-1]));
}


struct shell{
    shell(){}
    ~shell(){}
    
    void run(){
        while(1){
            std::vector<cmd> commands;
            std::vector<mod> modifiers;
            //printf("$> ");
            int res = parse_input(commands, modifiers);
            print_commands(commands, modifiers);
            if(commands.size()>0 && 
                !strcmp(commands[0][0], "quit")){
                for(int i = 0; i<commands.size(); i++){
                    for(int k =0;k<commands[i].size();k++){
                        free(commands[i][k]);
                    }
                }
                break;
            }
            if(commands[0].data())execute(commands, modifiers);
            for(int i = 0; i<commands.size(); i++){
                for(int k =0;k<commands[i].size();k++){
                    free(commands[i][k]);
                }
            }
            for(int i = 0; i<modifiers.size(); i++){
                for(int k =0;k<modifiers[i].size();k++){
                }
            }
            if (res==-1) break;
        }
    }

    int execute(std::vector<cmd>& commands, std::vector<mod>& modifiers){
        pid_t pid;
        pid_t cur_pid;
        int pipe_out[2];
        int pipe_in[2];
        int wait_pid=0;
        int last_rv=-100;
        int to_execute=1;
        std::vector<pid_t> pidss;
        std::vector<char *> comss;
        //vector<int> fd_to_close;
        std::vector<int>fd_to_closes;
        auto shell_pid = getpid();
        printD("shell -- %d\n", shell_pid);
        int last = commands.size()-1;
        int i=0;
        for(i=0; i<commands.size();i++){
            cur_pid = getpid();
            int fd=0;
            printD(" i= %d\n", i);

            if(i>0 && !is_mod("|", modifiers[i-1])){//not pipe need to check rvals
                wait_pids(pidss, modifiers, to_execute, last_rv, i, comss);
            }
            if(!to_execute) continue;

            if(is_in_pipe(modifiers, i, commands)) //pipe
                create_pipes(modifiers, i, pipe_out, pipe_in);
            
            switch(pid=fork()) {
            case -1:
                perror("fork"); 
                exit(1); 
            case 0: //fork process
                if(is_in_pipe(modifiers, i, commands))//pipe
                    redirect_stdio(modifiers, i, pipe_out, pipe_in);
                printD("gs = %d\n",commands[i].size() );
                if(commands[i].size()>2)//>/>>
                    if(strcmp(commands[i][commands[i].size()-2], ">")==0)
                        process_write_to_file(commands, i, fd, 1);
                    else if(strcmp(commands[i][commands[i].size()-2], ">>")==0)
                        process_write_to_file(commands, i, fd, 2);

                commands[i].push_back(NULL);
                execvp(commands[i][0], commands[i].data());
                exit(0);
            default: //shell
                pidss.push_back(pid);
                comss.push_back(commands[i][0]);
                printD("SHELL: PID -- %d PID child %d\n", getpid(),pid);
                if(is_in_pipe(modifiers, i, commands))//pipe
                    close_descriptors(modifiers, i, pipe_out, pipe_in, fd_to_closes);

                if(strcmp(commands[i][0], "cd")==0){ //cd
                    chdir(commands[i][1]);
                    to_execute=0;
                }
            }
            if(fd){ //>/>>
                close(fd);
                fd =0;
            }
        }
        for(int i=0;i<fd_to_closes.size();i++){
            close(fd_to_closes[i]);
        }
        printD("absolutely last chechk waiting for %d procs\n", pidss.size());
        wait_pids(pidss, modifiers, to_execute, last_rv, i, comss);
        
        return last_rv;
    }

    int parse_input(std::vector<cmd>& commands, std::vector<mod> & modifiers){
        int cur_command=0;
        while(1){
            int res=0;
            parser parser;
            commands.push_back(std::move(parser.split_to_tokens(stdin, &res, modifiers)));
            if(commands[cur_command].size()==0) {
                commands.pop_back();cur_command--;
                modifiers.pop_back();
            }
            if(res==0)break;
            if(res==-1) return -1;
            cur_command++;
        }
        return cur_command+1;
    }
    void print_commands(std::vector<cmd>& commands, std::vector<mod> & modifiers){
        printD("print commands s=%d: \n", commands.size());
        for(int i=0; i<commands.size(); i++){
            printD("com%d: ", i);
            for(int k=0; k<commands[i].size(); k++){
                printD("(%s) ", commands[i][k]);
            }
            
            printD("mods%d: ", i);
            for(int k=0; k<modifiers[i].size(); k++){
                printD("(%s) ", modifiers[i][k]);
            }
            printD("\n");
        }
    }
    
    void print_command(std::vector<char *> &command){
        printD("name = %s, argc = %d\n", command[0], command.size());
        for(int i=0;i<command.size();i++){
            printD("\targv[%d] = %s\n", i, command[i]);
        }
    }
};


