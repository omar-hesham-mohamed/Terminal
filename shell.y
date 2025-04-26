
/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN NEWLINE REDIRECT_IN REDIRECT_OUT REDIRECT_APPEND PIPE AMPERSAND

%union	{
		char   *string_val;
	}

%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include "command.h"
%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: 
	simple_command NEWLINE
	;

simple_command:	
	command_and_args iomodifier_list {
            printf("   Yacc: Execute command\n");
            Command::_currentCommand.execute();
    }
    | command_and_args iomodifier_list AMPERSAND {
            printf("   Yacc: Execute command in background\n");
            Command::_currentCommand._background = 1;
            Command::_currentCommand.execute();
    }
	|
	command_and_args iomodifier_list PIPE simple_command {
        printf("   Yacc: Execute pipe sequence\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
               printf("   Yacc: insert argument \"%s\"\n", $1);

	       Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	;

command_word:
	WORD {
               printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_list:
	iomodifier_opt iomodifier_list
	|
	;

iomodifier_opt:
	REDIRECT_OUT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
	}| REDIRECT_APPEND WORD {
        printf("   Yacc: append output to \"%s\"\n", $2);
        Command::_currentCommand._append = 1;
		Command::_currentCommand._outFile = $2;
    }
    | REDIRECT_IN WORD {
        printf("   Yacc: set input to \"%s\"\n", $2);
        Command::_currentCommand._inputFile = $2;
    }
	| /* can be empty */ 
	;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
