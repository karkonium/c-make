#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "pmake.h"


/* Print the list of actions */
void print_actions(Action *act) {

    while(act != NULL) {
        if (act->args == NULL) {
            fprintf(stderr, "ERROR: action with NULL args\n");
            act = act->next_act;
            continue;
        }

        printf("\t");

        int i = 0;
        while(act->args[i] != NULL) {
            printf("%s ", act->args[i]) ;
            i++;
        }

        printf("\n");
        act = act->next_act;
    }    
}

/* Print the list of rules to stdout in makefile format. If the output
   of print_rules is saved to a file, it should be possible to use it to 
   run make correctly.
 */
void print_rules(Rule *rules){
    
	Rule *cur = rules;
    
    while(cur != NULL) {
        if (cur->dependencies || cur->actions) {
            // Print target
            printf("%s : ", cur->target);
            
            // Print dependencies
            Dependency *dep = cur->dependencies;

            while(dep != NULL){
                if (dep->rule->target == NULL) {
                    fprintf(stderr, "ERROR: dependency with NULL rule\n");
                }
                printf("%s ", dep->rule->target);
                dep = dep->next_dep;
            }
            printf("\n");
            
            // Print actions
            print_actions(cur->actions);
        }
        cur = cur->next_rule;
    }
}
	
Rule* find_rule(Rule* head, char* str) {

	if (head == NULL) return NULL;

	while (head != NULL) {
		if (strcmp(head->target, str) == 0) {
			return head;
		}
		head = head->next_rule;		
	}

	return NULL;
}

void initalize_rule(Rule* tr, char* target, Dependency* dependencies, Action* actions, Rule* next_rule){

	tr->target = target;
	tr->dependencies = NULL;
	tr->actions = NULL;
	tr->next_rule = NULL;
}

void add_dependency(Rule* curr_target, Rule* dependency) {

	Dependency* made = malloc(sizeof(Dependency));
	made->rule = dependency;
	made->next_dep = NULL;
	
	Dependency* tr = curr_target->dependencies; 

	if (tr == NULL) {
		curr_target->dependencies = made;
	} else {
		while (tr->next_dep != NULL) {
			tr = tr->next_dep;
		}
		tr->next_dep = made;
	}
}

void add_action(Rule* curr_target, char** args) {

	Action* made = malloc(sizeof(Action));
	made->args = args;
	made->next_act = NULL;
	
	Action* tr = curr_target->actions; 

	if (tr == NULL) {
		curr_target->actions = made;
	} else {
		while (tr->next_act != NULL) {
			tr = tr->next_act;
		}
		tr->next_act = made;
	}
}

/* Create the rules data structure and return it.
   Figure out what to do with each line from the open file fp
     - If a line starts with a tab it is an action line for the current rule
     - If a line starts with a word character it is a target line, and we will
       create a new rule
     - If a line starts with a '#' or '\n' it is a comment or a blank line 
       and should be ignored. 
     - If a line only has space characters ('', '\t', '\n') in it, it should be
       ignored.
 */
Rule *parse_file(FILE *fp) {

	// In the case that the file doesnt have proper structure -- return NULL?
	// access file -- file is already opened
	int first = 1;
	Rule* rule_head = NULL, *rule_prev = NULL, *curr_target = NULL;
	char* str = malloc(sizeof(char) * MAXLINE);

	// make main linked list of targets
	while (fgets(str, MAXLINE, fp) != NULL) {
		// if line doesn't start with tab then we know its not a action line
		// we know this isnt a comment or an action line therefore its a target line
		if (!is_comment_or_empty(str) && strchr(str, ':') != NULL) {
			// after u get a rule header, then u should split that, for each element, 
			// make a Rule struct block and link that shit together
			char** split_string = build_args(str);
			int i = 0;

			while(split_string[i] != NULL) {	
				if (strchr(split_string[i], ':') == NULL) {
					// if rule exists
					Rule* tr = find_rule(rule_head, split_string[i]);
					// if not create it
					if (tr == NULL) {
						tr = malloc(sizeof(Rule));
						initalize_rule(tr, split_string[i], NULL, NULL, NULL);
						
						// now we have target thats initalized and not in rule linkedlist
						if (first) {
							// we need to store the head of the rule linked list
							first = 0;
							rule_head = tr;
						} else {
							//connect the nodes
							rule_prev->next_rule = tr;
						}
						rule_prev = tr;
					}
					// now we have to add the dependencies to the current target's list
										
					if (i == 0) {
						curr_target = tr; // first element is always the target
					} else {
						// rest of them are dependencies
						add_dependency(curr_target, tr);
					}
				}
				i++;
			}
			
			free(split_string);
		} 
		
		// action
		else if (!is_comment_or_empty(str) && str[0] == '\t') {
			char** args = build_args(str);
			// consider the case where first arg is a relative path
			add_action(curr_target, args);
		}
    }
		
	return rule_head;
}

/*
int main (int argc, char* argv[]) {
		FILE* fp = fopen(argv[1], "r");
        printf("--read file\n");
        Rule* rules = parse_file(fp);
        printf("--parsed file\n");
        print_rules(rules);
        return 0;
		
}/*