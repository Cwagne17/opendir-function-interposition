#define _GNU_SOURCE
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define HOME_PATH "/home" // string used to compare the target path of `ls`.

static DIR* (*real_opendir)(const char *name) = NULL;

DIR *opendir(const char *name) {
	// get a pointer to the real opendir 
	if (real_opendir == NULL) {
		/**
		 * From dlsym(3).
		 * 
		 * Finds the next occurrence of the opendir symbol in the
		 * search order after the current object. This allows one to 
		 * provide a wrapper around a function in another shared 
		 * object, so that, for example, the definition of a function
		 * in a preloaded shared object (see LD_PRELOAD in ld.so(8))
		 * can find and invoke the "real" function provided in
		 * another shared object (or for that matter, the "next"
		 * definition of the function in cases where there are
		 * multiple layers of preloading).
		*/
		real_opendir = dlsym(RTLD_NEXT, "opendir"); // obtain address of a symbol in a shared object or executable
		
		/* if real opendir isn't resolved, abort with a msg */
		char *error;
		if ((error = dlerror()) != NULL) {
			fprintf(stderr, "%s\n", error);
			exit(EXIT_FAILURE);
		}
	}

	char tpath[PATH_MAX];
	char *res = realpath(name, tpath); // sets tpath to resolved path
	if (res == NULL) {
        perror("realpath"); // prints descriptive err msg to stderr
        exit(EXIT_FAILURE);
	}

	// stmt is true if tpath does not begin with "/home"
	if (strncmp(tpath, HOME_PATH, strlen(HOME_PATH)) != 0) {
		openlog(
			"libharden", 	// identity when opening log
			LOG_CONS | 		// err to system console if err sending to syslog
			LOG_ODELAY | 	// waits to open conn until syslog() invoked
			LOG_PID, 		// includes PID in msg
			LOG_AUTH 		// labels msg as security/auth
		);
		syslog(
			LOG_AUTH | 		// labels msg as security/auth
		 	LOG_ERR, 		// labels msg as an error
			"UID %d tried using ls with the target %s\n", // formats msg for /var/log/auth.log
			getuid(), tpath // input for formatted msg
		);
		closelog();

		errno = EACCES; // set error no. to Permission Denied
		return NULL; 	// ret NULL to invoke the previous syscall to handle EACCES (exits command)
	}

	/* Call the real opendir with the same arguments. */
	return real_opendir(name);
}
