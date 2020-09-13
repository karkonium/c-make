#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "pmake.h"

#define MAX_INT 2147483647

Rule* get_rule(char* target, Rule* rules) {

    // if target is NULL evaluate first target in Rules list
	if (target == NULL) {
		return rules;
	// if not NULL then try to find the target
	} else {
		while (rules != NULL) {
			if (strcmp(rules->target, target) == 0) {
				return rules;
			}
			rules = rules->next_rule;
		}
	}

	// if target is not found
	return NULL;
}

int execute_actions(Rule* rule) {

	Action* tr = rule->actions;

	while (tr != NULL) {
		int pid = fork();

		if (pid < 0) {
			return 1;
		} else if (pid > 0 && wait(NULL) == -1) {
			return 1;
		} else {
			char** args = tr->args;

			// get path which is first arg
			char* path = args[0];
			if (execvp(path, args) == -1) 
				return 1;
		}

		tr = tr->next_act;
	}

	return 0;
}


int comparefiletime(char* patht, char* pathd) {

	// set a time variable to hold the time when target was 
	long int tsec, tnsec, dsec, dnsec;
	struct stat buffert, bufferd;

	// check if target already exists or not in files
	// if files DNE then set sec is 0 and nsec is 0 so its always smaller
	if (stat(patht, &buffert) == 0) {
		tsec = buffert.st_mtim.tv_sec;
		tnsec = buffert.st_mtim.tv_nsec;
	} else {
		tsec = 0;
		tnsec = 0;
	}

	if (stat(pathd, &bufferd) == 0) {
		dsec = bufferd.st_mtim.tv_sec;
		dnsec = bufferd.st_mtim.tv_nsec;
	} else {
		dsec = 0;
		dnsec = 0;
	}	

	// newer file implies larger sec
	return (tsec < dsec) || (tsec == dsec && tnsec < dnsec);
	
}

void run_make(char *target, Rule *rules, int pflag) {

	Rule* target_rule = get_rule(target, rules);
	
	if (target_rule == NULL) {
		// put out some error msg
		perror("Invalid target\n");
		return;
	}
	
	// now we have a rule block matching target
	Dependency* target_dep = target_rule->dependencies;

	// base case: if dependencies is NULL execute actions
	if (target_dep == NULL) {
		if (execute_actions(target_rule) == 1) perror("Error\n");
		return;
	}

	// loop over all dependencies and recursively call them
	int count = 0;
	int pid = MAX_INT;

	while (target_dep != NULL) {
		if (pflag) {
			pid = fork();
		}

		if (pid < 0) {
			perror("Error\n");
			return;
		}

		//if its a child
		if (pid == 0 || pflag == 0) {
			run_make(target_dep->rule->target, rules, pflag);
			if (pid == 0) return;
		}
		
		target_dep = target_dep->next_dep;
		count++;
	}
	
	for (int i = 0; i < count; i++) {
		wait(NULL);
	}

	// now all dependencies have been evaluated
	// once we've looped over dependencies -- loop once more comparing time
	target_dep = target_rule->dependencies;

	while (target_dep != NULL) {
		if (comparefiletime(target_rule->target, target_dep->rule->target)) {
			// if time of one dependency is newer than target then execute its actions
			if (execute_actions(target_rule) == 1) {
				perror("Error");
				return;
			}

			break;
		}
		target_dep = target_dep->next_dep;
	}
	
    return;
}

