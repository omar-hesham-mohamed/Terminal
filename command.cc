
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
#include <fcntl.h>
#include <fstream>
#include <ctime>

#include "command.h"

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
	_append = 0;
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
	_append = 0;
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
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{

    // Don't do anything if there are no simple commands
    if ( _numberOfSimpleCommands == 0 ) {
        prompt();
        return;
    }

	if (_numberOfSimpleCommands == 1 && strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {
        const char *dir = _simpleCommands[0]->_arguments[1];

        if (dir == NULL) {
            dir = getenv("HOME"); // defaults to home
        }

        if (chdir(dir) != 0) {
            perror("cd failed");
        }
		else {
        printf("Changed directory to: %s\n", dir);
    	}

        clear();
        prompt();

        return;
    }

    // Print contents of Command data structure
    print();

    int tmpin = dup(0); //store stdin and stdout
    int tmpout = dup(1);

    int fdin;
    if (_inputFile) {
        fdin = open(_inputFile, O_RDONLY);
    } else {
        fdin = dup(tmpin);
    }

    int pid;
    int fdout;
    for (int i = 0; i < _numberOfSimpleCommands; i++) {
		dup2(fdin, 0); // Set up stdin
        close(fdin); // Close after duplicating to stdin

        if (strcmp(_simpleCommands[i]->_arguments[0], "exit") == 0) {
			printf("Exitting...\n");
			exit(0);

			return;
		}
		
		if (i == _numberOfSimpleCommands - 1) {
            if (_outFile) {
				if (_append){
                	fdout = open(_outFile, O_CREAT | O_WRONLY | O_APPEND, 0666); //create file if needed, open in write only and empty it, append
				}
				else{
					fdout = open(_outFile, O_CREAT | O_WRONLY | O_TRUNC, 0666); //create file if needed, open in write only and empty it, rewrite
				}
            } else if (_errFile) {
                fdout = open(_errFile, O_CREAT | O_WRONLY | O_TRUNC, 0666); //666 -> read and write perms for all users
            } else {
                fdout = dup(tmpout);
            }
        } else {
            int fdpipe[2];
            pipe(fdpipe);
            fdin = fdpipe[0];
            fdout = fdpipe[1];
        }

		dup2(fdout, 1); // Set up stdout
        close(fdout); // Close after duplicating to stdout

        pid = fork();
        if (pid == 0) {
            execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
            perror("execvp"); // execvp should not return
            _exit(1);
        }
    }

    dup2(tmpin, 0); //restore stdin and stdout
    dup2(tmpout, 1);
    close(tmpin);
    close(tmpout);

    if (!_background) {
        waitpid(pid, 0, 0); //wait for child to finish
    }

    // Clear for the next command
    clear();
    
    // Print new prompt
    prompt();
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

void sigchld_handler(int sig)
{
    // Open the log file
    std::ofstream logfile("process_log.txt", std::ios::app);
    if (logfile.is_open()) {
        // Log the termination time
        time_t now = time(0);
        logfile << "Process terminated at: " << ctime(&now);
        logfile.close();
    }
}

int 
main()
{
	signal(SIGINT, SIG_IGN);
	signal(SIGCHLD, sigchld_handler);
	
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

