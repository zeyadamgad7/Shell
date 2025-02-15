
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

%token 	NOTOKEN GREAT NEWLINE TWOGREATS LESS PIPE AMPERSAND

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
	command NEWLINE {
		printf("   Yacc: Execute command or pipeline\n");
        Command::_currentCommand.execute();
	} 
	| commands command NEWLINE {
		printf("   Yacc: Execute command or pipeline\n");
        Command::_currentCommand.execute();
	}
	;

command:
	simple_command
	| command PIPE simple_command
	;

simple_command:	
    command_and_args iomodifier_opt background_opt
	| NEWLINE
    | error { yyerrok; }
;


command_and_args:
	command_word arg_list {
		Command::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
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

iomodifier_opt:
    iomodifier_opt GREAT WORD {
        printf("   Yacc: insert output \"%s\"\n", $3);
        Command::_currentCommand._outFile = $3;
    }
    | iomodifier_opt TWOGREATS WORD {
        printf("   Yacc: append output \"%s\"\n", $3);
        Command::_currentCommand._append = true;
        Command::_currentCommand._outFile = $3;
    }
    | iomodifier_opt LESS WORD {
        printf("   Yacc: insert input \"%s\"\n", $3);
        Command::_currentCommand._inputFile = $3;
    }
    | /* can be empty */ 
    ;

background_opt:
	AMPERSAND {
		printf("   Yacc: run in background\n");
		Command::_currentCommand._background = 1;
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
