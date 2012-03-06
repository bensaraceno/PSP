#include "config.h"
int cfgFile = 0;
/*
  Convert a string to a number (integer or float)
  Parameters :	string	->	String to convert
			value	->	Result of the conversion
  Return :		The type of value
*/
int configStrToNum (char *string, void *value) {
	int x, v, base, basetest;
	for (x=0,v=0,base=1,basetest=0;x<strlen(string);x++) { // Find base
		if (string[x] == '.') { basetest = 1; } else {
			if (!(basetest)) base *= 10;
			v *= 10;
			v += string[x] - '0';
		}
	}
	if (!(basetest)) { // Integer
		*((int *) value) = v;
		return CONFIG_TYPE_INTEGER;
	}
	*((float *) value) = (float) ((float) v / (base / 10)); // Float
	return CONFIG_TYPE_FLOAT;
}

/*
  Find a variable in openend file
  Parameters :	key		->	Name of the key which contains the variable
			var		->	Name of the variable to find
  Return :		0		->	Variable found
			1		->	Key not found
			2		->	Variable not found
*/
int configFindVar (char *key, char *var) {
	int x;
	char string[128], *ptr, car;
	sceIoLseek(cfgFile, 0, PSP_SEEK_SET); // File pointer to begin.
	string[0] = 0; 
	while (strcasecmp(key,string)) { // Find good key.
		car = 0; ptr = string; // Init variables.
		// Find entry key caracter.
		while (car != CONFIG_CAR_KEYBEGIN) {
			x = sceIoRead(cfgFile, &car, 1); // Read one caracter.
			if (!(x)) return 1; // Exit if no key found.
		}
		car = ' ';
		while ((car == ' ') || (car == '\t')) { // Erase space and tab before key name.
			x = sceIoRead(cfgFile, &car, 1); // Read one caracter.
			if (!(x)) return 2; // Exit if no variable found.
		}
		while ((car != CONFIG_CAR_KEYEND) && (car != '\r') && (car != '\n')) { // Find end key caracter.
			*ptr++ = car; // Copy caracter in string.
			x = sceIoRead(cfgFile, &car, 1); // Read one caracter.
			if (!(x)) return 1; // Exit if no key found.
		}
		if ((car != '\r') && (car != '\n')) { // If a line break ("\n"), then goto next line.
			*ptr = 0; // Erase space and tab after name key.
			while ((string[strlen(string) - 1] == ' ') || (string[strlen(string) - 1] == '\t')) string[strlen(string) - 1] = 0;
		} else { string[0] = 0; }
	}

	string[0] = 0; // Find good variable.
	while (strcasecmp(var,string)) {
		car = ' '; ptr = string; // Init variables.
		while ((car == ' ') || (car == '\t')) { // Erase space and tab before variable name.
			x = sceIoRead(cfgFile, &car, 1);// Read one caracter.
			if (!(x)) return 2;// Exit if no variable found.
		}
		if (car == CONFIG_CAR_KEYBEGIN) return 2; // If another key, exit.
		if (car == CONFIG_CAR_COMMENT) { // If comment, jump next line.
			while ((car != '\r') && (car != '\n')) {
				x = sceIoRead(cfgFile, &car, 1); // Read one caracter.
				if (!(x)) return 2; // Exit if no variable found.
			}
		}
		if ((car != '\r') && (car != '\n')) { // If not end string nor comments...
			while ((car != '=') && (car != '\r') && (car != '\n')) {
				*ptr++ = car; // Copy caracter in string.
				x = sceIoRead(cfgFile, &car, 1); // Read one caracter.
				if (!(x)) return 2; // Exit if no variable found.
			}
			if ((car != '\r') && (car != '\n')) { // If a line break ("\n"), then goto next line.
				*ptr = 0;
				// Erase space and tab after name variable
				while ((string[strlen(string) - 1] == ' ') || (string[strlen(string) - 1] == '\t')) string[strlen(string) - 1] = 0;
			} else { string[0] = 0; }
		}
	}

	// Erase space and tab before value variable
	car = ' ';
	while ((car == ' ') || (car == '\t')) {
		x = sceIoRead(cfgFile,&car,1); // Read one caracter
		if (!(x)) return 2; // Exit if no variable found
	}

	// Pour ne pas perdre le dernier caractere lu qui n'est pas un espace
	sceIoLseek(cfgFile, -1, PSP_SEEK_CUR);
	return 0;
}

/*
  Create a configuration file
  Parameters :	filename	->	Name of the file to create
  Return :		0			->	OK
			1			->	Incorrect parameters or a file is already opened
			2			->	Unable to create file
*/
int configCreate(char *filename) {
	// If already opened or no filename, then exit.
	if ((cfgFile) || (!(filename))) return 1;
	cfgFile = sceIoOpen(filename, PSP_O_RDWR | PSP_O_CREAT | PSP_O_TRUNC, 0777); // Open the file for writing.
	if (cfgFile <= 0) { cfgFile = 0; return 2; } // Verify any errors
	return 0;
}

/*
  Load a configuration file
  Parameters :	filename	->	Name of the file to load
  Return :		0			->	OK
			1			->	Incorrect parameters or a file is already opened
			2			->	Unable to load file
*/
int configLoad(char *filename) {
	// If already opened or no filename, then exit.
	if ((cfgFile) || (!(filename))) return 1;
	cfgFile = sceIoOpen(filename, PSP_O_RDWR, 0777); // Open the file for reading.
	if (cfgFile <= 0) { cfgFile = 0; return 2; } // Verify any errors.
	return 0;
}

/*
  Close a configuration file
  Parameters :	Nothing
  Return :		Nothing
*/
void configClose(void) {
	if (cfgFile) sceIoClose(cfgFile); // Close file if opened
	cfgFile = 0;
}

/*
  Read a variable into opened file
  Parameters :	key		->	Name of the key which contains the variable
			var		->	Name of the variable to find
			value	->	Will contain value of the variable
			type	->	Will contain type of the variable
			size	->	Will contain size of the variable
  Return :		0		->	OK
			1		->	File not opened
			2		->	Variable not found
*/
int configRead(char *key, char *var, void *value, int *type, int *size) {
	int x, s, t;
	char string[1024], *ptr, car;
	if (!(cfgFile)) return 1; // If not opened, exit
	if (configFindVar(key, var)) return 2; // Goto key in file
	// Init variables
	string[0] = 0;
	car = 0; ptr = string;
	t = CONFIG_TYPE_INTEGER; s = 0;

	// Read variable (one line max)
	while ((car != '\r') && (car != '\n') && (car != CONFIG_CAR_COMMENT)) {
		x = sceIoRead(cfgFile, &car, 1); // Read one caracter
		if (!(x)) break; // Break if end file
		*ptr++ = car; // Copy caracter in string
		// Find type
		if ((car != '\r') && (car != '\n') && (car != ' ') && (car != '\t') && (car != CONFIG_CAR_COMMENT)) {
			if (car == '.') t = (t == CONFIG_TYPE_INTEGER) ? CONFIG_TYPE_FLOAT : CONFIG_TYPE_STRING; else if ((car < '0') || (car > '9')) t = CONFIG_TYPE_STRING;
		}
	}

	// Terminate string
	if ((car == '\r') || (car == '\n') || (car == CONFIG_CAR_COMMENT)) *(ptr - 1) = 0; else *ptr = 0;

	// Erase space and tab after value variable
	while ((string[strlen(string) - 1] == ' ') || (string[strlen(string) - 1] == '\t')) string[strlen(string) - 1] = 0;

	// Calculate parameters
	if (string[0] == CONFIG_CAR_STRING) {
		ptr = &string[1]; // Eliminate end string limit caracter
		while ((*ptr != CONFIG_CAR_STRING) && (*ptr != 0)) ptr++;
		*ptr = 0;

		s = strlen(string);

		if (value) memcpy(value,&string[1],s);
		if (type) *type = t;
		if (size) *size = s;
	} else {
		switch (t) {
			case CONFIG_TYPE_STRING:
				s = strlen(string) + 1;
				if (value) memcpy(value,string,s);
				break;

			case CONFIG_TYPE_FLOAT:
				s = sizeof(SceFloat);
				if (value) configStrToNum(string,value);
				break;

			case CONFIG_TYPE_INTEGER :
				s = sizeof(u32);
				if (value) configStrToNum(string,value);
				break;
		}
		if (type) *type = t;
		if (size) *size = s;
	}
	return 0;
}

/*
  Write a variable into opened file
  Parameters :	key		->	Name of the key which contains the variable
			var		->	Name of the variable to write
			value	->	Contain value of the variable
			type	->	Contain type of the variable
			size	->	Contain size of the variable
  Return :		0		->	OK
			1		->	File not opened
*/
int configWrite(char *key, char *var, void *value, int type, int size) {
	sceIoWrite(cfgFile, "some text", 10);
	return 0;
}
