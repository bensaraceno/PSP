#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED

/*
	;This is a comment line indicated by ";"	<- For comments
	[key_name]						<- For key
	variable = value int				<- For number value
	variable = "value string"				<- For string value
		or variable = value string			<- For string value (spaces truncated)
*/

#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_TYPE_STRING		0						// String variables
#define CONFIG_TYPE_INTEGER		1						// Integer variables
#define CONFIG_TYPE_FLOAT		2						// Float variables

#define CONFIG_CAR_KEYBEGIN		'['						// Key caracter for begin
#define CONFIG_CAR_KEYEND		']'						// Key caracter for end
#define CONFIG_CAR_COMMENT		';'						// Key caracter for comments
#define CONFIG_CAR_STRING		'"'						// Key caracter for string


// Function declarations.
int configCreate (char *);								// Create a new configuration file
int configLoad (char *);								// Load an existing configuration file
void configClose (void);								// Close the current opened configuration file
int configRead (char *, char *, void *, int*, int*);	// Read a variable into the current opened configuration file
int configWrite (char *, char *, void *, int, int);		// Write a variable into the current opened configuration file

#endif
