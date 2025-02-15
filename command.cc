
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#include "command.h"
#include <fcntl.h>
#include <time.h>

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ]);
		}
        printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
}


void Command::execute() {
    if (_numberOfSimpleCommands == 0) {
        prompt();
        return;
    }

    print();

    if (_numberOfSimpleCommands == 1 && 
        strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0) {
        
        printf("Good bye!!\n");
        exit(0);
    }

    if (_numberOfSimpleCommands == 1 &&
        strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {
        
        const char *targetDir;
        if (_simpleCommands[0]->_arguments[1]) {
            targetDir = _simpleCommands[0]->_arguments[1];
        } else {
            targetDir = getenv("HOME");
            if (!targetDir) {
                fprintf(stderr, "cd: HOME environment variable not set\n");
                return;
            }
        }

        if (chdir(targetDir) != 0) {
            perror("cd failed");
        }

        clear();
        prompt();
        return;
    }

    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    pid_t last_pid = 0;
    int fdpipe[2];
    int prev_fd = defaultin; 

    for (int i = 0; i < _numberOfSimpleCommands; i++) {
        SimpleCommand *cmd = _simpleCommands[i];

        if (i < _numberOfSimpleCommands - 1) {
            if (pipe(fdpipe) == -1) {
                perror("pipe failed");
                exit(1);
            }
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {

            if (prev_fd != defaultin) {
                dup2(prev_fd, 0);
                close(prev_fd);
            } else if (_inputFile && i == 0) {
                int in_fd = open(_inputFile, O_RDONLY);
                if (in_fd < 0) {
                    perror("Failed to open input file");
                    exit(1);
                }
                dup2(in_fd, 0);
                close(in_fd);
            }

            if (i < _numberOfSimpleCommands - 1) {
                dup2(fdpipe[1], 1);
                close(fdpipe[1]);
            } else if (_outFile) {
                int out_fd = _append ? open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0666)
                                     : open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (out_fd < 0) {
                    perror("Failed to open output file");
                    exit(1);
                }
                dup2(out_fd, 1);
                close(out_fd);
            }

            if (i < _numberOfSimpleCommands - 1) {
                close(fdpipe[0]);
            }

            execvp(cmd->_arguments[0], cmd->_arguments);
            perror("execvp failed");
            exit(1);
        }

        if (prev_fd != defaultin) {
            close(prev_fd);
        }
        close(fdpipe[1]);
        prev_fd = fdpipe[0];

        last_pid = pid;
    }

    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);

    close(defaultin);
    close(defaultout);
    close(defaulterr);

    if (!_background) {
        waitpid(last_pid, nullptr, 0);
    }

    clear();
    prompt();
}





void handle_sigchld(int sig) {
    int status;
    pid_t pid;

    FILE *log_file = fopen("child_termination.log", "a");
    if (!log_file) {
        perror("Failed to open log file");
        return;
    }

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0';

        if (WIFEXITED(status)) {
            fprintf(log_file, "Child process %d terminated with exit status %d at %s\n",
                    pid, WEXITSTATUS(status), time_str);
        } else if (WIFSIGNALED(status)) {
            fprintf(log_file, "Child process %d was killed by signal %d at %s\n",
                    pid, WTERMSIG(status), time_str);
        }
    }
    fclose(log_file);
}


// Shell implementation

void
Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

int 
main()
{

    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }

    signal(SIGINT, SIG_IGN);

	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

