/* barangay_relief_array.c
   Recreation using only arrays, functions, loops, and conditionals.
   No string.h — all string operations done manually with loops/arrays.
   Compile: gcc -std=c99 -O2 -o barangay_relief_array barangay_relief_array.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_HH 1024
#define MAX_NAME 100
#define MAX_ZONE 50
#define CSV_FILE "relief_data_array.csv"
#define MAX_ACCOUNTS 15
#define MAX_RESIDENTS 50                    // changeable, depending on the population
#define MAX_LEN 100
#define USER_LEN 24

/* Parallel arrays for households */
unsigned long long hh_id[MAX_HH];
char hh_head[MAX_HH][MAX_NAME];
char hh_zone[MAX_HH][MAX_ZONE];
int hh_members[MAX_HH];
int hh_elderly[MAX_HH];
int hh_infants[MAX_HH];
int hh_disabled[MAX_HH];
int hh_pregnant[MAX_HH];
double hh_vuln[MAX_HH];
int hh_served[MAX_HH];
unsigned long long hh_order[MAX_HH];

int heap_size = 0;
unsigned long long global_counter = 0;
unsigned long long next_id = 1000;

/* ---------- Manual string utilities (no string.h) ---------- */

void accountManager();

/* Returns length of a null-terminated char array */
int my_strlen(const char *s) {
    int i = 0;
    while (s[i] != '\0') i++;
    return i;
}

/* Copies up to n-1 characters from src to dst, always null-terminates */
void my_strncpy(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* Copies src to dst (no length limit, caller must ensure space) */
void my_strcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* Parses an unsigned long long from a decimal string; stops at non-digit */
unsigned long long my_strtoull(const char *s) {
    unsigned long long result = 0;
    int i = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i++;
    }
    return result;
}

/* Parses an int from a decimal string (handles leading minus) */
int my_atoi(const char *s) {
    int result = 0;
    int sign = 1;
    int i = 0;
    if (s[i] == '-') { sign = -1; i++; }
    while (s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i++;
    }
    return sign * result;
}

/*
 * Manual CSV tokeniser replacing strtok.
 * Reads the next comma-delimited token from buf starting at *pos.
 * Writes token into out (max out_size chars including null terminator).
 * Returns 1 if a token was found, 0 if end of string reached.
 */
int csv_next_token(const char *buf, int *pos, char *out, int out_size) {
    if (buf[*pos] == '\0') return 0;
    int j = 0;
    while (buf[*pos] != '\0' && buf[*pos] != ',') {
        if (j < out_size - 1) {
            out[j] = buf[*pos];
            j++;
        }
        (*pos)++;
    }
    out[j] = '\0';
    if (buf[*pos] == ',') (*pos)++;   /* skip delimiter */
    return 1;
}

/* ---------- Utility functions ---------- */

void swap_all(int i, int j) {
    /* Swap IDs */
    unsigned long long tmp_id = hh_id[i];
    hh_id[i] = hh_id[j];
    hh_id[j] = tmp_id;

    /* Swap head names */
    char tmp_head[MAX_NAME];
    my_strncpy(tmp_head,    hh_head[i], MAX_NAME);
    my_strncpy(hh_head[i],  hh_head[j], MAX_NAME);
    my_strncpy(hh_head[j],  tmp_head,   MAX_NAME);

    /* Swap zones */
    char tmp_zone[MAX_ZONE];
    my_strncpy(tmp_zone,    hh_zone[i], MAX_ZONE);
    my_strncpy(hh_zone[i],  hh_zone[j], MAX_ZONE);
    my_strncpy(hh_zone[j],  tmp_zone,   MAX_ZONE);

    int tmp;
    tmp = hh_members[i];  hh_members[i]  = hh_members[j];  hh_members[j]  = tmp;
    tmp = hh_elderly[i];  hh_elderly[i]  = hh_elderly[j];  hh_elderly[j]  = tmp;
    tmp = hh_infants[i];  hh_infants[i]  = hh_infants[j];  hh_infants[j]  = tmp;
    tmp = hh_disabled[i]; hh_disabled[i] = hh_disabled[j]; hh_disabled[j] = tmp;
    tmp = hh_pregnant[i]; hh_pregnant[i] = hh_pregnant[j]; hh_pregnant[j] = tmp;

    double dtmp   = hh_vuln[i];  hh_vuln[i]  = hh_vuln[j];  hh_vuln[j]  = dtmp;
    tmp           = hh_served[i]; hh_served[i] = hh_served[j]; hh_served[j] = tmp;

    unsigned long long otmp = hh_order[i];
    hh_order[i] = hh_order[j];
    hh_order[j] = otmp;
}

double compute_vulnerability_index(int idx) {
    double score = 0.0;
    score += hh_members[idx]  * 0.5;
    score += hh_elderly[idx]  * 2.0;
    score += hh_infants[idx]  * 1.8;
    score += hh_disabled[idx] * 2.5;
    score += hh_pregnant[idx] * 1.5;
    if      (hh_members[idx] >= 8) score += 2.0;
    else if (hh_members[idx] >= 5) score += 1.0;
    if (hh_served[idx]) score -= 1000.0;
    return score;
}

/* Higher priority: larger vulnerability; tie-breaker: smaller order (earlier) */
int higher_priority_idx(int a, int b) {
    if (hh_vuln[a] != hh_vuln[b]) return hh_vuln[a] > hh_vuln[b];
    return hh_order[a] < hh_order[b];
}

/* ---------- Heap operations ---------- */

void heapify_up(int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (higher_priority_idx(idx, parent)) {
            swap_all(idx, parent);
            idx = parent;
        } else break;
    }
}

void heapify_down(int idx) {
    while (1) {
        int left    = idx * 2 + 1;
        int right   = idx * 2 + 2;
        int largest = idx;
        if (left  < heap_size && higher_priority_idx(left,  largest)) largest = left;
        if (right < heap_size && higher_priority_idx(right, largest)) largest = right;
        if (largest != idx) {
            swap_all(idx, largest);
            idx = largest;
        } else break;
    }
}

void rebuild_heap(void) {
    int i;
    for (i = (heap_size / 2) - 1; i >= 0; --i) heapify_down(i);
}

/* ---------- Persistence ---------- */

void save_data(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) { printf("Failed to open file for saving.\n"); return; }

    fprintf(f, "id,head_name,zone,members,elderly,infants,disabled,pregnant,served,order\n");

    int i, k;
    for (i = 0; i < heap_size; ++i) {
        /* Copy and sanitise head name: replace commas with semicolons */
        char head_safe[MAX_NAME];
        my_strncpy(head_safe, hh_head[i], MAX_NAME);
        for (k = 0; head_safe[k] != '\0'; ++k)
            if (head_safe[k] == ',') head_safe[k] = ';';

        /* Copy and sanitise zone */
        char zone_safe[MAX_ZONE];
        my_strncpy(zone_safe, hh_zone[i], MAX_ZONE);
        for (k = 0; zone_safe[k] != '\0'; ++k)
            if (zone_safe[k] == ',') zone_safe[k] = ';';

        fprintf(f, "%llu,%s,%s,%d,%d,%d,%d,%d,%d,%llu\n",
                (unsigned long long)hh_id[i],
                head_safe, zone_safe,
                hh_members[i], hh_elderly[i], hh_infants[i],
                hh_disabled[i], hh_pregnant[i],
                hh_served[i],
                (unsigned long long)hh_order[i]);
    }
    fclose(f);
    printf("Data saved to %s\n", filename);
}

void load_data(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { printf("No saved data found (%s).\n", filename); return; }

    char line[512];
    /* Skip header line */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return; }

    heap_size = 0;
    while (fgets(line, sizeof(line), f)) {
        /* Convert line into zero-terminated buffer without trailing newline */
        int len = 0;
        while (line[len] != '\0') len++;
        if (len > 0 && line[len - 1] == '\n') { line[len - 1] = '\0'; len--; }

        char tok[MAX_NAME];
        int pos = 0;   /* cursor into line[] */

        /* id */
        if (!csv_next_token(line, &pos, tok, MAX_NAME)) continue;
        hh_id[heap_size] = my_strtoull(tok);

        /* head name */
        if (!csv_next_token(line, &pos, tok, MAX_NAME))
            hh_head[heap_size][0] = '\0';
        else
            my_strncpy(hh_head[heap_size], tok, MAX_NAME);

        /* zone */
        if (!csv_next_token(line, &pos, tok, MAX_ZONE))
            hh_zone[heap_size][0] = '\0';
        else
            my_strncpy(hh_zone[heap_size], tok, MAX_ZONE);

        /* members, elderly, infants, disabled, pregnant, served */
        if (csv_next_token(line, &pos, tok, MAX_NAME)) hh_members[heap_size]  = my_atoi(tok); else hh_members[heap_size]  = 0;
        if (csv_next_token(line, &pos, tok, MAX_NAME)) hh_elderly[heap_size]  = my_atoi(tok); else hh_elderly[heap_size]  = 0;
        if (csv_next_token(line, &pos, tok, MAX_NAME)) hh_infants[heap_size]  = my_atoi(tok); else hh_infants[heap_size]  = 0;
        if (csv_next_token(line, &pos, tok, MAX_NAME)) hh_disabled[heap_size] = my_atoi(tok); else hh_disabled[heap_size] = 0;
        if (csv_next_token(line, &pos, tok, MAX_NAME)) hh_pregnant[heap_size] = my_atoi(tok); else hh_pregnant[heap_size] = 0;
        if (csv_next_token(line, &pos, tok, MAX_NAME)) hh_served[heap_size]   = my_atoi(tok); else hh_served[heap_size]   = 0;

        /* order */
        if (csv_next_token(line, &pos, tok, MAX_NAME)) hh_order[heap_size] = my_strtoull(tok); else hh_order[heap_size] = 0;

        /* Trim any stray newline that ended up inside the last zone token */
        int zlen = my_strlen(hh_zone[heap_size]);
        if (zlen > 0 && hh_zone[heap_size][zlen - 1] == '\n')
            hh_zone[heap_size][zlen - 1] = '\0';

        hh_vuln[heap_size] = compute_vulnerability_index(heap_size);

        if (hh_id[heap_size]    > next_id)         next_id         = hh_id[heap_size];
        if (hh_order[heap_size] > global_counter)  global_counter  = hh_order[heap_size];

        heap_size++;
        if (heap_size >= MAX_HH) break;
    }
    rebuild_heap();
    fclose(f);
    printf("Data loaded from %s (%d households)\n", filename, heap_size);
}

/* ---------- Input helpers ---------- */

void read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) { buf[0] = '\0'; return; }
    int len = 0;
    while (buf[len] != '\0') len++;
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);
    }
}

int read_int_prompt(const char *prompt) {
    char line[128];
    while (1) {
        if (prompt) printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) { clearerr(stdin); continue; }

        /* Check that the line contains at least one digit (allow leading minus) */
        int i = 0;
        if (line[i] == '-') i++;
        if (line[i] < '0' || line[i] > '9') {
            printf("Invalid number. Try again.\n");
            continue;
        }
        return my_atoi(line);
    }
}

/* ---------- UI actions ---------- */

void register_household_ui(void) {
    if (heap_size >= MAX_HH) { printf("Maximum households reached.\n"); return; }
    int idx = heap_size;
    hh_id[idx] = ++next_id;
    printf("Enter household head name: ");
    read_line(hh_head[idx], MAX_NAME);
    printf("Enter zone/purok: ");
    read_line(hh_zone[idx], MAX_ZONE);
    hh_members[idx]  = read_int_prompt("Total household members: ");
    hh_elderly[idx]  = read_int_prompt("Number of elderly (60+): ");
    hh_infants[idx]  = read_int_prompt("Number of infants (<=2): ");
    hh_disabled[idx] = read_int_prompt("Number of persons with disability: ");
    hh_pregnant[idx] = read_int_prompt("Number of pregnant members: ");
    hh_served[idx]   = 0;
    hh_order[idx]    = ++global_counter;
    hh_vuln[idx]     = compute_vulnerability_index(idx);
    heap_size++;
    heapify_up(heap_size - 1);
    printf("Registered household ID %llu (Head: %s) with vulnerability %.2f\n",
           (unsigned long long)hh_id[heap_size - 1],
           hh_head[heap_size - 1],
           hh_vuln[heap_size - 1]);
}

void update_all_scores(void) {
    int i;
    for (i = 0; i < heap_size; ++i) hh_vuln[i] = compute_vulnerability_index(i);
    rebuild_heap();
}

unsigned long long heap_pop_top(void) {
    if (heap_size == 0) return 0ULL;
    unsigned long long topid = hh_id[0];
    heap_size--;
    if (heap_size > 0) {
        my_strncpy(hh_head[0], hh_head[heap_size], MAX_NAME);
        my_strncpy(hh_zone[0], hh_zone[heap_size], MAX_ZONE);
        hh_id[0]       = hh_id[heap_size];
        hh_members[0]  = hh_members[heap_size];
        hh_elderly[0]  = hh_elderly[heap_size];
        hh_infants[0]  = hh_infants[heap_size];
        hh_disabled[0] = hh_disabled[heap_size];
        hh_pregnant[0] = hh_pregnant[heap_size];
        hh_vuln[0]     = hh_vuln[heap_size];
        hh_served[0]   = hh_served[heap_size];
        hh_order[0]    = hh_order[heap_size];
        heapify_down(0);
    }
    return topid;
}

void serve_next_ui(void) {
    if (heap_size == 0) { printf("No households to serve.\n"); return; }
    update_all_scores();
    if (hh_served[0]) {
        heap_pop_top();
        printf("Top household already served; removed from queue.\n");
        return;
    }
    printf("Next to serve: ID %llu | Head: %s | Zone: %s | Vulnerability: %.2f\n",
           (unsigned long long)hh_id[0], hh_head[0], hh_zone[0], hh_vuln[0]);
    int confirm = read_int_prompt("Allocate relief pack to this household? (1=yes, 0=no): ");
    if (confirm == 1) {
        hh_served[0] = 1;
        unsigned long long served_id = heap_pop_top();
        printf("Allocated relief to ID %llu\n", served_id);
    } else {
        printf("Cancelled.\n");
    }
}

void peek_next_ui(void) {
    if (heap_size == 0) { printf("No households registered.\n"); return; }
    update_all_scores();
    printf("Next to serve: ID %llu | Head: %s | Zone: %s | Vulnerability: %.2f | Served: %s\n",
           (unsigned long long)hh_id[0], hh_head[0], hh_zone[0], hh_vuln[0],
           hh_served[0] ? "Yes" : "No");
}

void list_ui(void) {
    update_all_scores();
    if (heap_size == 0) { printf("No households registered.\n"); return; }

    /* Non-destructive sort: copy all arrays into temporaries, then selection-find */
    int tmp_size = heap_size;
    unsigned long long tmp_id[MAX_HH];
    char tmp_head[MAX_HH][MAX_NAME];
    char tmp_zone[MAX_HH][MAX_ZONE];
    int tmp_members[MAX_HH], tmp_elderly[MAX_HH], tmp_infants[MAX_HH];
    int tmp_disabled[MAX_HH], tmp_pregnant[MAX_HH], tmp_served[MAX_HH];
    double tmp_vuln[MAX_HH];
    unsigned long long tmp_order[MAX_HH];

    int i, j;
    for (i = 0; i < tmp_size; ++i) {
        tmp_id[i] = hh_id[i];
        my_strncpy(tmp_head[i], hh_head[i], MAX_NAME);
        my_strncpy(tmp_zone[i], hh_zone[i], MAX_ZONE);
        tmp_members[i]  = hh_members[i];
        tmp_elderly[i]  = hh_elderly[i];
        tmp_infants[i]  = hh_infants[i];
        tmp_disabled[i] = hh_disabled[i];
        tmp_pregnant[i] = hh_pregnant[i];
        tmp_vuln[i]     = hh_vuln[i];
        tmp_served[i]   = hh_served[i];
        tmp_order[i]    = hh_order[i];
    }

    int rank = 1;
    while (tmp_size > 0) {
        /* Find index of highest priority */
        int best = 0;
        for (i = 1; i < tmp_size; ++i) {
            if (tmp_vuln[i] > tmp_vuln[best]) best = i;
            else if (tmp_vuln[i] == tmp_vuln[best] && tmp_order[i] < tmp_order[best]) best = i;
        }
        printf("%d) ID:%llu | Head: %s | Zone: %s | Members:%d | Elderly:%d | Infants:%d | Disabled:%d | Pregnant:%d\n",
               rank,
               (unsigned long long)tmp_id[best],
               tmp_head[best], tmp_zone[best],
               tmp_members[best], tmp_elderly[best], tmp_infants[best],
               tmp_disabled[best], tmp_pregnant[best]);
        printf("    Vulnerability: %.2f | Served: %s\n",
               tmp_vuln[best], tmp_served[best] ? "Yes" : "No");

        /* Remove best by shifting remaining entries left */
        for (j = best; j < tmp_size - 1; ++j) {
            tmp_id[j] = tmp_id[j + 1];
            my_strncpy(tmp_head[j], tmp_head[j + 1], MAX_NAME);
            my_strncpy(tmp_zone[j], tmp_zone[j + 1], MAX_ZONE);
            tmp_members[j]  = tmp_members[j + 1];
            tmp_elderly[j]  = tmp_elderly[j + 1];
            tmp_infants[j]  = tmp_infants[j + 1];
            tmp_disabled[j] = tmp_disabled[j + 1];
            tmp_pregnant[j] = tmp_pregnant[j + 1];
            tmp_vuln[j]     = tmp_vuln[j + 1];
            tmp_served[j]   = tmp_served[j + 1];
            tmp_order[j]    = tmp_order[j + 1];
        }
        tmp_size--;
        rank++;
    }
}

int find_index_by_id(unsigned long long id) {
    int i;
    for (i = 0; i < heap_size; ++i)
        if (hh_id[i] == id) return i;
    return -1;
}

void search_update_ui(void) {
    int id_input = read_int_prompt("Enter household ID to search: ");
    unsigned long long id = (unsigned long long)id_input;
    int idx = find_index_by_id(id);
    if (idx < 0) { printf("Household ID %llu not found.\n", id); return; }

    printf("Found: ID %llu | Head: %s | Zone: %s | Members:%d | Elderly:%d | Infants:%d | Disabled:%d | Pregnant:%d | Served:%s\n",
           (unsigned long long)hh_id[idx], hh_head[idx], hh_zone[idx],
           hh_members[idx], hh_elderly[idx], hh_infants[idx],
           hh_disabled[idx], hh_pregnant[idx],
           hh_served[idx] ? "Yes" : "No");

    printf("1) Update counts  2) Toggle served  3) Remove household  0) Cancel\n");
    int choice = read_int_prompt("Choose action: ");

    if (choice == 1) {
        hh_members[idx]  = read_int_prompt("Total household members: ");
        hh_elderly[idx]  = read_int_prompt("Number of elderly (60+): ");
        hh_infants[idx]  = read_int_prompt("Number of infants (<=2): ");
        hh_disabled[idx] = read_int_prompt("Number of persons with disability: ");
        hh_pregnant[idx] = read_int_prompt("Number of pregnant members: ");
        hh_vuln[idx] = compute_vulnerability_index(idx);
        heapify_down(idx);
        heapify_up(idx);
        printf("Updated.\n");
    } else if (choice == 2) {
        hh_served[idx] = !hh_served[idx];
        hh_vuln[idx] = compute_vulnerability_index(idx);
        heapify_down(idx);
        heapify_up(idx);
        printf("Toggled served status. Now served=%d\n", hh_served[idx]);
    } else if (choice == 3) {
        heap_size--;
        if (idx != heap_size) {
            my_strncpy(hh_head[idx], hh_head[heap_size], MAX_NAME);
            my_strncpy(hh_zone[idx], hh_zone[heap_size], MAX_ZONE);
            hh_id[idx]       = hh_id[heap_size];
            hh_members[idx]  = hh_members[heap_size];
            hh_elderly[idx]  = hh_elderly[heap_size];
            hh_infants[idx]  = hh_infants[heap_size];
            hh_disabled[idx] = hh_disabled[heap_size];
            hh_pregnant[idx] = hh_pregnant[heap_size];
            hh_vuln[idx]     = hh_vuln[heap_size];
            hh_served[idx]   = hh_served[heap_size];
            hh_order[idx]    = hh_order[heap_size];
            heapify_down(idx);
            heapify_up(idx);
        }
        printf("Removed household ID %llu\n", id);
    } else {
        printf("Cancelled.\n");
    }
}

/* ---------- Account Manager ---------- */

void accountManager() {
    char usernames[MAX_ACCOUNTS][24] = {"admin"}; // Pre-set admin account
    char passwords[MAX_ACCOUNTS][24] = {"1234"};
    int totalAccounts = 1;
    int loggedIn = 0;

    while (!loggedIn) {
        int mainChoice;
        printf("\n<=====|| WELCOME TO OUR PROGRAM ||=====>\n");
        printf("1. Login\n");
        printf("2. Create an Account\n");
        printf("Choice: ");
        
        if (scanf("%d", &mainChoice) != 1) {
            int c; while((c = getchar()) != '\n' && c != EOF);
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        int c; while((c = getchar()) != '\n' && c != EOF);

        //  LOGIN 
        if (mainChoice == 1) {
            while (1) {
                char u[24], p[24];
                int sub;
                printf("\n<--- LOGIN --->\n");
                printf("1. Enter Credentials\n");
                printf("2. Back\n");
                printf("Choice: ");
                
                if (scanf("%d", &sub) != 1) {
                    int c; while((c = getchar()) != '\n' && c != EOF);
                    continue;
                }
                int c; while((c = getchar()) != '\n' && c != EOF);
                
                if (sub == 2) break; 

                printf("Username: "); 
                scanf("%s", u);
                printf("Password: "); 
                scanf("%s", p);

                bool found = false;

                for (int i = 0; i < totalAccounts; i++) {
                    if (strcmp(usernames[i], u) == 0 && strcmp(passwords[i], p) == 0) {
                        printf("\nWelcome, %s! Access Granted.\n", u);
                        loggedIn = 1;
                        found = true;
                        break;
                    }
                }

                if (found) break; 
                else{
                    printf("Account does not exist or wrong password!\n");
                }
            }
        } 
        
        // CREATE ACCOUNT 
        else if (mainChoice == 2) {
            if (totalAccounts >= MAX_ACCOUNTS) {
                printf("System Full! Cannot add more than %d accounts.\n", MAX_ACCOUNTS);
                continue;
            }

            int sub;
            printf("<---| Create Account |---->\n");
            printf("[1] Proceed\n");
            printf("[2] Back\n");
            printf("Enter Choice: ");
            if (scanf("%d", &sub) != 1) {
                int c; while((c = getchar()) != '\n' && c != EOF);
                continue;
            }
            int c; while((c = getchar()) != '\n' && c != EOF);
            if (sub == 2) continue;

            printf("Enter New Username: ");
            scanf("%s", usernames[totalAccounts]);
            printf("Enter New Password: ");
            scanf("%s", passwords[totalAccounts]);

            totalAccounts++;
            printf("Account successfully created!\n");
        } 
        
        else if (mainChoice == 3) {
            exit(0);
        }
    }
}

/* ---------- Main ---------- */

int main(void) {
    int i;
    for (i = 0; i < MAX_HH; ++i) {
        hh_id[i]       = 0;
        hh_head[i][0]  = '\0';
        hh_zone[i][0]  = '\0';
        hh_members[i]  = 0;
        hh_elderly[i]  = 0;
        hh_infants[i]  = 0;
        hh_disabled[i] = 0;
        hh_pregnant[i] = 0;
        hh_vuln[i]     = 0.0;
        hh_served[i]   = 0;
        hh_order[i]    = 0;
    }

    load_data(CSV_FILE);
    printf("=== Offline Barangay Disaster Relief System (array version) ===\n");
    printf("Register households, prioritize by vulnerability, allocate relief offline.\n");

    accountManager();

    while (1) {
        printf("\nMenu:\n");
        printf("1. Register household\n");
        printf("2. Serve next household (allocate relief)\n");
        printf("3. Peek next household\n");
        printf("4. List households (by priority)\n");
        printf("5. Search / Update / Remove household by ID\n");
        printf("6. Save data\n");
        printf("7. Load data\n");
        printf("8. Open File (list of data)\n");
        printf("9. Exit\n");
        int choice = read_int_prompt("Choose an option: ");

        if      (choice == 1) register_household_ui();
        else if (choice == 2) serve_next_ui();
        else if (choice == 3) peek_next_ui();
        else if (choice == 4) list_ui();
        else if (choice == 5) search_update_ui();
        else if (choice == 6) save_data(CSV_FILE);
        else if (choice == 7) {
            for (i = 0; i < MAX_HH; ++i) {
                hh_id[i]       = 0;
                hh_head[i][0]  = '\0';
                hh_zone[i][0]  = '\0';
                hh_members[i]  = 0;
                hh_elderly[i]  = 0;
                hh_infants[i]  = 0;
                hh_disabled[i] = 0;
                hh_pregnant[i] = 0;
                hh_vuln[i]     = 0.0;
                hh_served[i]   = 0;
                hh_order[i]    = 0;
            }
            heap_size = 0;
            global_counter = 0;
            next_id = 1000;
            load_data(CSV_FILE);
        }
        else if(choice == 8){
            system("start relief_data_array.csv");
        }
        else if (choice == 9) {
            update_all_scores();
            save_data(CSV_FILE);
            printf("Exiting. Stay safe.\n");
            return 0;
        }
        else printf("Invalid option. Try again.\n");
    }
    return 0;
}