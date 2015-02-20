#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

// I won't have time to implement the challenge exercises but
// here are my thoughts on how I would go about implementing them
// and what areas of the current code would need change.
//
// -- Lists of commands would be parsed and placed into an array
// of pointers to struct cmd. That array would have to be dynamically
// re-allocated everytime a new command is encountered using realloc.
// Each command in that list will be executed in it's own child. The
// return status of the child will not interpreted in any special
// way except for the last child. The return status of the last
// command will be the return status of the whole line. If the
// commands were chained using '||' or '&&' then the return of
// status of each child would be significant.
//
// -- A subshell would be parsed and executed in pretty much the same
// way as an execcmd.
//
// -- A background command would be parsed into it's own cmd struct,
// a child would be forked and the parent would exit immediately after
// the fork.

// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Having looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be run (e.g., an execcmd)
  char *file;        // the input/output file
  int mode;          // the mode to open the file with
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

int fork1(void);  // Fork but exits on failure.
struct cmd *parsecmd(char*);
void setup_redirection(struct redircmd*);
void dup2_wrapped(int, int);
char *search_path(char*); // Recursively search through all paths listed
                          // in $PATH to find the given executabe

// Execute cmd.  Never returns. Executes in the child.
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(EXIT_SUCCESS);
  
  errno = 0;
  switch(cmd->type){
  default:
    fprintf(stderr, "unknown commnad type\n");
    exit(EXIT_FAILURE);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(EXIT_SUCCESS);
    execv(search_path(ecmd->argv[0]), ecmd->argv);
    fprintf(stderr, "execv returned with error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    setup_redirection(rcmd);
    runcmd(rcmd->cmd);

  case '|':
    pcmd = (struct pipecmd*)cmd;
    int result = pipe(p);
    if (result == -1) {
      fprintf(stderr, "pipe system call did not complete successfully: %s\n",\
                      strerror(errno));
      exit(EXIT_FAILURE);
    }

    if (fork1() == 0) {  // child1 executes pcmd->left
      close(p[0]);
      dup2_wrapped(p[1], STDOUT_FILENO);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if (fork1() == 0) { // child2 executes pcmd->right
      close(p[1]);
      dup2_wrapped(p[0], STDIN_FILENO);
      close(p[0]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(&r);
    wait(&r);
    exit(EXIT_SUCCESS);
  }    
  exit(EXIT_FAILURE); // should never get here
}

void
setup_redirection(struct redircmd *cmd)
{
  int redirection_fd = 0;
  if (cmd->type == '>') { // output redirection
    redirection_fd = open(cmd->file, cmd->mode, S_IRWXU);
  } else { // input redirection
    redirection_fd = open(cmd->file, cmd->mode);
  }
  if (redirection_fd < 0) {
    fprintf(stderr, "failed to open file for redirection: %s\n",\
                    strerror(errno));
    exit(EXIT_FAILURE);
  }
  dup2_wrapped(redirection_fd, cmd->fd);
}

// wrapper around the dup2 system call used to check for errors
void
dup2_wrapped(int old_fd, int new_fd)
{
  int result = dup2(old_fd, new_fd);
  if (result < 0) {
    fprintf(stderr, "failed to dup file descriptors: %s\n", \
                    strerror(errno));
    exit(EXIT_FAILURE);
  }
}

// List through directories in $PATH to find given executable.
// Returns the given executable if not found in any of the $PATH
// directoties.
char *
search_path(char *exe)
{
  DIR *d;
  struct dirent *dir;

  char *paths = getenv("PATH");
  char *path_dir = strtok(paths, ":");
  while (path_dir != NULL) {
    d = opendir(path_dir);
    if (d == NULL) {
      fprintf(stderr, "cannot open dir: %s", strerror(errno));
    } else {
      while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, exe) == 0) {
          char *final_path = malloc(strlen(path_dir) + strlen(exe) + 2);
          final_path = strcat(final_path, path_dir);
          final_path = strcat(final_path, "/");
          final_path = strcat(final_path, exe);
          return final_path;
        }
      }
    }

    path_dir = strtok(NULL, ":");
  }
  return exe;
}

int
getcmd(char *buf, int nbuf)
{
  
  if (isatty(fileno(stdin)))
    fprintf(stdout, "6.828$ ");
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  int r;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait(&r);
  }
  exit(EXIT_SUCCESS);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->mode = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
