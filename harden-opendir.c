#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

/**
 * Types needed for opendir
 * See `man opendir`.
*/
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define HOME_PATH "/home"

static DIR* (*real_opendir)(const char *name) = NULL;

DIR *opendir(const char *name) {
	// First-time initialization: get a pointer to the real memcpy 
	if (real_opendir == NULL) {
		char *error;
		real_opendir = dlsym(RTLD_NEXT, "opendir");
		
		/* If we couldn't find the real opendir, abort with a message */
		error = dlerror();
		if (error != NULL) {
			fprintf(stderr, "%s\n", error);
			exit(1);
		}
	}

	char *dname;
	dname = (char*)name; // copies value of name pointer into dname
	if (strcmp(name, ".") == 0) { // ls is for cwd then get current dir name
		dname = get_current_dir_name();
	}
	if (strncmp(dname, HOME_PATH, strlen(HOME_PATH)) != 0) {
		errno = EACCES;

		// add code to send err log to syslog
		return NULL;
	}

	/* Call the real opendir with the same arguments. */
	return real_opendir(name);
}
