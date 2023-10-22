#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Transition Transition;

typedef struct Automaton {
    int alphabet;
    int states;
    int initial_state;
    int finals_num;
    int *finals;
    int transitions_num;
    Transition *transition;
} Automaton;

struct Transition {
    int from;
    int to;
    char a;
    Transition *next;
};
int word_exists(const char *subword, const Automaton *automaton);
int exists_for_state(const char *subword, const Automaton *automaton, const int state);
int final_from_state(const Automaton *automaton, const int state);
int resolve_final(const Automaton *automaton, const int state, int *state_visited);
int state_from_start(const Automaton *automaton, const int state);
int resolve_state(const Automaton *automaton, const int state, const int current_state, int *state_visited);

void free_automaton(Automaton automaton) {
    free(automaton.finals);
    Transition *t = automaton.transition;
    while (t) {
        Transition *next = t->next;
        free(t);
        t = next;
    }
}

int read_automaton(FILE *file, Automaton *automaton)
{
    memset(automaton, 0, sizeof(*automaton));
    fscanf(file, "%u", &automaton->alphabet);
    fscanf(file, "%u", &automaton->states);
    fscanf(file, "%u", &automaton->initial_state);
    fscanf(file, "%u", &automaton->finals_num);

    automaton->finals = malloc(sizeof(int) * automaton->finals_num);
    for (int i = 0; i < automaton->finals_num; ++i) {
        fscanf(file, "%u", &automaton->finals[i]);
    }

    automaton->transitions_num = 0;
    Transition *current_transition = NULL;

    for (;;) {
        int s0, s1;
        char a;

        fscanf(file, "%u %c %u", &s0, &a, &s1);

        Transition *new_transition = malloc(sizeof(Transition));
        memset(new_transition, 0, sizeof(Transition));
        new_transition->from = s0;
        new_transition->to = s1;
        new_transition->a = a;

        for (Transition *t = automaton->transition; t; t = t->next) {
            if (t->from != new_transition->from || t->a != new_transition->a)
                continue;

            if (t->to != new_transition->to) {
                printf(
                        "Transition rule %u %c %u makes state machine non-determine\n",
                        new_transition->from,
                        new_transition->a,
                        new_transition->to
                );
                printf(
                        "It collides with rule %u %c %u.\n",
                        t->from,
                        t->a,
                        t->to
                );
                free(new_transition);
                free_automaton(*automaton);
                return -1;
            } else {
                free(new_transition);
                new_transition = NULL;
                return 0;
            }
        }

        if (!new_transition)
            continue;

        if (current_transition)
            current_transition->next = new_transition;
        if (!automaton->transition)
            automaton->transition = new_transition;
        current_transition = new_transition;
        ++automaton->transitions_num;
    }

    return 0;
}

void print_automaton(Automaton *automaton) {
    printf("Alphabet number: %u\n"
            "States number: %u\n"
            "Initial state: %u\n"
            "Final states number: %u\n"
            "Final states: ",
            automaton->alphabet,
            automaton->states,
            automaton->initial_state,
            automaton->finals_num
    );

    for (int i = 0; i < automaton->finals_num; ++i) {
        printf("%d ", automaton->finals[i]);
    }

    printf(
            "\n"
            "Transitions number:    %u\n"
            "Transitions:\n",
            automaton->transitions_num
    );

    for (Transition *current = automaton->transition; current; current = current->next) {
        printf("%u %c %u\n", current->from, current->a, current->to);
    }
}

int word_exists(const char *subword, const Automaton *automaton) {

    if (strlen(subword) == 0) {
        return final_from_state(automaton, automaton->initial_state);
    }

    for (Transition *t = automaton->transition; t; t = t->next) {
        if (t->a != subword[0])
            continue;

        int S0 = t->from;
        if (!exists_for_state(subword, automaton, S0))
            continue;
        if (state_from_start(automaton, S0))
            return 1;
    }
    return 0;
}

int exists_for_state(const char *subword, const Automaton *automaton, const int state) {
    if (subword[0] == 0) {
        return final_from_state(automaton, state);
    }
    for (Transition *t = automaton->transition; t; t = t->next) {
        if (t->from != state || t->a != subword[0])
            continue;
        if (exists_for_state(subword + 1, automaton, t->to))
            return 1;
    }
    return 0;
}

int final_from_state(const Automaton *automaton, const int state) {
    int state_visited[automaton->states];
    memset(state_visited, 0, sizeof(state_visited));

    return resolve_final(automaton, state, state_visited);
}


int resolve_final(const Automaton *automaton, const int state, int *state_visited) {
    state_visited[state] = 1;   // mark current state as visited

    for (int i = 0; i < automaton->finals_num; ++i) {
        if (automaton->finals[i] == state)
            return 1;
    }

    for (Transition *t = automaton->transition; t; t = t->next) {
        if (t->from != state || state_visited[t->to])
            continue;
        if (resolve_final(automaton, t->to, state_visited))
            return 1;
    }

    return 0;
}


int state_from_start(const Automaton *automaton, const int state) {
    int state_visited[automaton->states];
    memset(state_visited, 0, sizeof(state_visited));

    return resolve_state(automaton, state, automaton->initial_state, state_visited);
}


int resolve_state(const Automaton *automaton, const int state, const int current_state, int *state_visited) {
    state_visited[current_state] = 1;
    if (current_state == state)
        return 1;

    for (Transition *t = automaton->transition; t; t = t->next) {
        if (t->from != current_state || state_visited[t->to])
            continue;
        if (resolve_state(automaton, state, t->to, state_visited))
            return 1;
    }

    return 0;
}

int main() {
    const char *file_path = "automaton.txt";
    const char *w0;
    printf("Enter w0: ");
    scanf("%s", w0);

    FILE *file = fopen(file_path, "r");

    if (!file) {
        printf("Cannot open the file\n");
        return -1;
    }

    Automaton automaton;
    read_automaton(file, &automaton);
    fclose(file);

    print_automaton(&automaton);
    printf("\nw = w1w0, where w0 = \"%s\"\n", w0);
    if (word_exists(w0, &automaton))
    {
        printf("Acceptable\n");
    }
    else
    {
        printf("Isn`t acceptable\n");
    }

    free_automaton(automaton);

    return 0;
}