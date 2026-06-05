#include "shell.h"


int main(void)
{
	setup_signals();
	run_shell();
	return 0;
}
void run_shell(void)
{
	char *line;
	char **tokens;

	while (1)
	{
		printf("%s+------------+------------+------------+------------+------------+------------+%s\n", COLOR_BLUE, COLOR_RESET);
		printf("%stron_cole%s%s>> %s", COLOR_CYAN, COLOR_RESET, COLOR_GREEN, COLOR_RESET );
		fflush(stdout);

		line = read_line();

		if (line == NULL)
		{
			printf("\n");
			break;
		}

		if (strlen(line) == 0) 
		{
			free(line);
			continue;
		}

		tokens = tokenize(line);

		if (tokens == NULL || tokens[0] == NULL)
		{
			free(line);
			free(tokens);
			continue;
		}

		int n_pipes = count_pipes(tokens);

		if (is_builtin(tokens[0]))
		{
			run_builtin(tokens);
		}

		else if ( n_pipes > 0)
		{
			execute_piped(tokens, n_pipes);
		}

		else
		{
			execute(tokens);
		}
		printf("%s+------------+------------+------------+------------+------------+------------+%s\n", COLOR_BLUE, COLOR_RESET);

		free(line);
		free(tokens);
	}
}

char *read_line(void)
{
	char *buffer = malloc(MAX_INPUT);
	if(!buffer)
	{
		perror("memory couldn't be allocated");
		exit(1);
	}
	if (fgets(buffer, MAX_INPUT, stdin) == NULL)
	{
		free(buffer);
		return NULL;
	}
	buffer[strcspn(buffer, "\n")] = '\0';
	return buffer;
}

char **tokenize(char *line)
{
	char **tokens = malloc(MAX_ARGS * sizeof(char *));
	if (!tokens)
	{
		perror("memory allocation for tokens");
		exit(1);
	}

	int i = 0;
	char *token = strtok(line, " \t\r\n");

	while (token != NULL && i < MAX_ARGS -1)
	{
		tokens[i++] = token;
		token = strtok(NULL, " \t\r\n");
	}

	tokens[i] = NULL;
	return tokens;
}

int count_pipes(char **tokens)
{

	int count = 0;
	for (int i = 0; tokens[i] != NULL; i++)
	{
		if (strcmp(tokens[i], "|") == 0)
			count++;
	}
	return count;
}


int find_token(char **tokens, const char *sym)
{
	for (int i = 0; tokens[i] != NULL; i++)
	{
		if (strcmp(tokens[i], sym) == 0)
			return i;
	}
	return -1;
}

void execute(char **args)
{
	char *clean[MAX_ARGS];
	char *infile = NULL;
	char *outfile = NULL;
	int append =0;
	int j = 0;

	for (int i = 0; args[i] != NULL; i++)
	{
		if (strcmp(args[i], "<") == 0)
		{
			if(args[i+1] != NULL)
			{
				infile = args[i+1];
				i++;
			}
		}


		else if (strcmp(args[i], ">") == 0)
		{
			if (args[i + 1] != NULL)
			{
				outfile = args[i + 1];
				append = 0;
				i++;
			}

		}
		else if (strcmp(args[i], ">>") == 0)
		{
			if (args[i + 1] != NULL)
			{
				outfile = args[i + 1]; 
				append = 1; 
				i++; 
			}
		} 

		else
		{
			clean[j++] = args[i];
		}
	}
	clean[j] = NULL;

	pid_t pid = fork();

	if (pid < 0)
	{
		perror("couldn't fork");
		return;
	}


	if (pid == 0)
	{
		// child process

		// input redirection
		if (infile)
		{
			int fid = open(infile, O_RDONLY);
			if (fid < 0)
			{
				perror(infile);
				exit(1);	
			}
			dup2(fid, STDIN_FILENO);
			close(fid);
		}

		if (outfile)
		{
			int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
			int fid = open(outfile, flags, 0644);
			if (fid < 0)
			{
				perror(outfile);
				exit(1);
			}
			dup2(fid, STDOUT_FILENO);
			close(fid);
		}

		execvp(clean[0], clean);

		fprintf(stderr,  "tron_cole: %s: command was not found\n", clean[0]);
		exit(127);
	}
	else
	{
		int status;
		waitpid(pid, &status, 0);
	}
}

static const char *BUILTINS[] = 
{
	"cd", "exit", "pwd", "help", NULL
};

int is_builtin(char *cmd)
{
	for (int i = 0; BUILTINS[i] != NULL; i++)
	{
		if (strcmp(cmd, BUILTINS[i]) == 0)
		{
			return 1;
		}
	}

	return 0;
}

void run_builtin(char **args)
{
	if (strcmp(args[0], "exit") == 0)
	{		
		int code = args[1] ? atoi(args[1]) : 0;
		exit(code);
	}

	else if (strcmp(args[0], "cd") == 0)
	{
		const char *dir = args[1] ? args[1] : getenv("HOME");
		if (chdir(dir) != 0)
		{
			perror("cd error");
		}
	}

	else if (strcmp(args[0], "pwd") == 0)
	{
		char buffer[1024];
		if (getcwd(buffer, sizeof(buffer)) != NULL)
		{
			printf("%s\n", buffer);
		}
		else
		{
			perror("pwd error");
		}
	}

	else if (strcmp(args[0], "help") == 0) 
	{
		printf("tron_cole -- built-in commands: \n");
		printf("  cd [dir] -> change director\n");
		printf("  pwd      -> print working directory\n");
		printf("  exit [n] -> exit with code n\n");
		printf("  help     -> show this message\n");
	}
}

void execute_piped(char **tokens, int n_pipes)
{
	int n_cmds = n_pipes +1;
	int pipes[MAX_PIPES][2];
	pid_t pids[MAX_PIPES + 1];

	char **cmds[MAX_PIPES + 1];
	int cmd_idx = 0;
	int arg_start = 0;

	for (int i = 0; tokens[i] != NULL; i++) {
		if (strcmp(tokens[i], "|") == 0 || tokens[i+1] == NULL) {
			int end = (strcmp(tokens[i], "|") == 0) ? i : i + 1;
			int len = end - arg_start;
			cmds[cmd_idx] = malloc((len + 1) * sizeof(char *));
			memcpy(cmds[cmd_idx], &tokens[arg_start], len * sizeof(char *));
			cmds[cmd_idx][len] = NULL;
			arg_start = i + 1;
			cmd_idx++;
		}
	}


	for (int i = 0; i < n_pipes; i++) {
		if (pipe(pipes[i]) < 0) {
			perror("pipe");
			return;
		}
	}

	for (int i = 0; i < n_cmds; i++)
	{
		pids[i] = fork();
		if (pids[i] < 0) 
		{
			perror("fork");
			return;
		}

		if (pids[i] == 0)
		{
			if (i > 0)
				dup2(pipes[i-1][0], STDIN_FILENO);

			if (i < n_pipes)
				dup2(pipes[i][1], STDOUT_FILENO);

			for (int k = 0; k < n_pipes; k++) 
			{
				close(pipes[k][0]);
				close(pipes[k][1]);
			}

			execvp(cmds[i][0], cmds[i]);
			fprintf(stderr, "tron_cole: %s: not found\n", cmds[i][0]);
			exit(127);
		}
	}

	for (int i = 0; i < n_pipes; i++)
	{
		close(pipes[i][0]);
		close(pipes[i][1]);
	}
	for (int i = 0; i < n_cmds; i++) 
	{
		int status;
		waitpid(pids[i], &status, 0);
	}
	for (int i = 0; i < n_cmds; i++)
		free(cmds[i]);
}

static void sigint_handler(int sig)
{
	(void)sig;  
	printf("\nmyshell> ");
	fflush(stdout);
}

void setup_signals(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));


	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sa, NULL);


	sa.sa_handler = SIG_IGN;
	sigaction(SIGTSTP, &sa, NULL);


	sa.sa_handler = SIG_DFL;
	sa.sa_flags = SA_RESTART ;
#ifdef SA_NOCLDWAIT
	sa.sa_flags |= SA_NOCLDWAIT;
#endif
	sigaction(SIGCHLD, &sa, NULL);
}
