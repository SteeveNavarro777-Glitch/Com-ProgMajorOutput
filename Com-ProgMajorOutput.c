#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_HH 1024
#define MAX_NAME 100
#define MAX_ZONE 50
#define CSV_FILE "relief_data_array.txt"
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

// <=====|| FUNCTION PROTOTYPES ||=====>

void accountManager();
int my_strlen(const char *s);
void my_strncpy(char *dst, const char *src, int n);
void my_strcpy(char *dst, const char *src);
unsigned long long my_strtoull(const char *s);
int my_atoi(const char *s);
int next_token(const char *buf, int *pos, char *out, int out_size);
void swap_all(int i, int j);
double compute_vulnerability_index(int idx);
int higher_priority_idx(int a, int b);
void heapify_up(int idx);
void heapify_down(int idx);
void rebuild_heap(void);
void save_data(const char *filename);
void load_data(const char *filename);
void read_line(char *buf, int size);
int read_int_prompt(const char *prompt);
void register_household_ui(void);
void update_all_scores(void);
unsigned long long heap_pop_top(void);
void serve_next_ui(void);
void peek_next_ui(void);
void list_ui(void);
int find_index_by_id(unsigned long long id);
void search_update_ui(void);
void accountManager();
int find_index_by_name(const char *name);
void display_detailed_names(unsigned long long target_id);
void trim_spaces(char *str);

// <=====|| MAIN FUNCTION ||=====> 

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
        printf("6. Show all households' information\n");
        printf("0. Exit\n");
        printf("Choose an option: ");
        int choice;
        scanf("%d", &choice);
        getchar();

        if      (choice == 1){
            register_household_ui();
        } 
        else if (choice == 2){
            serve_next_ui();
            getchar();
        }
        else if (choice == 3){
            peek_next_ui();
        }
        else if (choice == 4){
            list_ui();
        }
        else if (choice == 5){
            search_update_ui();
        }
        else if (choice == 6){
            system("unfit_person.txt");
        }
        else if (choice == 0) {
            update_all_scores();
            save_data(CSV_FILE);
            system("relief_data_array.txt");
            printf("Exiting. Stay safe.\n");
            return 0;
        }
        else{
            printf("Invalid option. Try again.\n");
        }
    }
    return 0;
}


// <=====|| FUNCTION DEFINITIONS ||=====>


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
int next_token(const char *buf, int *pos, char *out, int out_size) {
    if (buf[*pos] == '\0') return 0;
    int j = 0;
    while (buf[*pos] != '\0' && buf[*pos] != '|') {
        if (j < out_size - 1) {
            out[j] = buf[*pos];
            j++;
        }
        (*pos)++;
    }
    out[j] = '\0';
    if (buf[*pos] == '|') (*pos)++;
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

    // Adjusted widths to fit ALL capitalized labels comfortably
    // ID (6) | HEAD_NAME (20) | ZONE (6) | MEMBERS (10) | ELDERLY (10) | INFANTS (10) | DISABLED (10) | PREGNANT (10) | SERVED (8) | ORDER (8)
    fprintf(f, "%-6s |           %-30s        |    %-11s                  | %-10s | %-10s | %-10s | %-10s | %-10s | %-8s | %-8s\n",
            "ID", "HEAD_NAME", "ZONE", "MEMBERS", "ELDERLY", "INFANTS", "DISABLED", "PREGNANT", "SERVED", "ORDER");

    for (int i = 0; i < heap_size; ++i) {
        // These widths MUST match the header widths above exactly to stay aligned
        fprintf(f, "%-6llu | %-20s | %-6s | %-10d | %-10d | %-10d | %-10d | %-10d | %-8d | %-8llu\n",
                hh_id[i],
                hh_head[i], 
                hh_zone[i],
                hh_members[i], 
                hh_elderly[i], 
                hh_infants[i],
                hh_disabled[i], 
                hh_pregnant[i],
                hh_served[i],
                hh_order[i]);
    }
    fclose(f);
    printf("Data saved to %s\n", filename);
}

void load_data(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { printf("No saved data found (%s).\n", filename); return; }

    char line[1024]; // Increased buffer size for the wider "clean" lines
    /* Skip header line */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return; }

    heap_size = 0;
    while (fgets(line, sizeof(line), f)) {
        /* Remove trailing newline */
        int len = my_strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            len--;
        }

        char tok[MAX_NAME];
        int pos = 0;

        /* ID */
        if (!next_token(line, &pos, tok, MAX_NAME)) continue;
        trim_spaces(tok); // CLEAN THE TOKEN
        hh_id[heap_size] = my_strtoull(tok);

        /* Head Name */
        if (!next_token(line, &pos, tok, MAX_NAME)) {
            hh_head[heap_size][0] = '\0';
        } else {
            trim_spaces(tok); // CLEAN THE TOKEN
            my_strncpy(hh_head[heap_size], tok, MAX_NAME);
        }

        /* Zone */
        if (!next_token(line, &pos, tok, MAX_ZONE)) {
            hh_zone[heap_size][0] = '\0';
        } else {
            trim_spaces(tok); // CLEAN THE TOKEN
            my_strncpy(hh_zone[heap_size], tok, MAX_ZONE);
        }

        /* Numeric fields - we trim them before calling my_atoi to be safe */
        if (next_token(line, &pos, tok, MAX_NAME)) { trim_spaces(tok); hh_members[heap_size] = my_atoi(tok); } else hh_members[heap_size] = 0;
        if (next_token(line, &pos, tok, MAX_NAME)) { trim_spaces(tok); hh_elderly[heap_size] = my_atoi(tok); } else hh_elderly[heap_size] = 0;
        if (next_token(line, &pos, tok, MAX_NAME)) { trim_spaces(tok); hh_infants[heap_size] = my_atoi(tok); } else hh_infants[heap_size] = 0;
        if (next_token(line, &pos, tok, MAX_NAME)) { trim_spaces(tok); hh_disabled[heap_size] = my_atoi(tok); } else hh_disabled[heap_size] = 0;
        if (next_token(line, &pos, tok, MAX_NAME)) { trim_spaces(tok); hh_pregnant[heap_size] = my_atoi(tok); } else hh_pregnant[heap_size] = 0;
        if (next_token(line, &pos, tok, MAX_NAME)) { trim_spaces(tok); hh_served[heap_size] = my_atoi(tok); } else hh_served[heap_size] = 0;

        /* Order */
        if (next_token(line, &pos, tok, MAX_NAME)) { 
            trim_spaces(tok); 
            hh_order[heap_size] = my_strtoull(tok); 
        } else {
            hh_order[heap_size] = 0;
        }

        hh_vuln[heap_size] = compute_vulnerability_index(heap_size);

        if (hh_id[heap_size] > next_id) next_id = hh_id[heap_size];
        if (hh_order[heap_size] > global_counter) global_counter = hh_order[heap_size];

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

// <=====|| OPTION #1 FUNCTION ||=====>

void register_household_ui(void) {
    if (heap_size >= MAX_HH) { printf("Maximum households reached.\n"); return; }
    
    int idx = heap_size;
    hh_id[idx] = ++next_id;
    char tempName[MAX_NAME];

    printf("\n--- REGISTERING HOUSEHOLD ID: %llu ---\n", hh_id[idx]);
    printf("Enter household Last name: ");
    read_line(hh_head[idx], MAX_NAME);
    printf("Enter zone/purok: ");
    read_line(hh_zone[idx], MAX_ZONE);
    
    hh_members[idx] = read_int_prompt("Total household members: ");

    // Open unfit_person.txt in Append mode
    FILE *unfitF = fopen("unfit_person.txt", "a");
    if (!unfitF) {
        printf("Error: Could not open unfit_person.txt\n");
        return;
    }
    fprintf(unfitF, "Household ID: %llu \nSurname: %s\n", hh_id[idx], hh_head[idx]);
    fprintf(unfitF, "Zone/Purok: %s\n", hh_zone[idx]);
    fprintf(unfitF, "Household Size = %d\n\n", hh_members[idx]);
    fprintf(unfitF, "Family members: \n");

    // Data collection for vulnerable members
    hh_elderly[idx] = read_int_prompt("Number of elderly (60+): ");
    for (int i = 0; i < hh_elderly[idx]; i++) {
        printf("  -> Name of Elderly #%d: ", i + 1);
        read_line(tempName, MAX_NAME);
        fprintf(unfitF, " Elderly  - %s\n", tempName);
    }

    hh_infants[idx] = read_int_prompt("Number of infants (<=2): ");
    for (int i = 0; i < hh_infants[idx]; i++) {
        printf("  -> Name of Infant #%d: ", i + 1);
        read_line(tempName, MAX_NAME);
        fprintf(unfitF, " Infant   - %s\n", tempName);
    }

    hh_disabled[idx] = read_int_prompt("Number of persons with disability: ");
    for (int i = 0; i < hh_disabled[idx]; i++) {
        printf("  -> Name of PWD #%d: ", i + 1);
        read_line(tempName, MAX_NAME);
        fprintf(unfitF, " PWD      - %s\n", tempName);
    }

    hh_pregnant[idx] = read_int_prompt("Number of pregnant members: ");
    for (int i = 0; i < hh_pregnant[idx]; i++) {
        printf("  -> Name of Pregnant Woman #%d: ", i + 1);
        read_line(tempName, MAX_NAME);
        fprintf(unfitF, " Pregnant - %s\n", tempName);
    }

    int vulnerable_count = hh_elderly[idx] + hh_infants[idx] + hh_disabled[idx] + hh_pregnant[idx];
    int fit_count = hh_members[idx] - vulnerable_count;

    if (fit_count < 0) {
        printf("Warning: Vulnerable count exceeds total members. Setting fit people to 0.\n");
        fit_count = 0;
    }

    if (fit_count > 0) {
        printf("Remaining members (Fit): %d\n", fit_count);
        for (int i = 0; i < fit_count; i++) {
            printf("  -> Name of Fit Member #%d: ", i + 1);
            read_line(tempName, MAX_NAME);
            fprintf(unfitF, " Fit      - %s\n", tempName);
        }
    }

    hh_vuln[idx] = compute_vulnerability_index(idx);

    fprintf(unfitF, "\nVulnerability Rate: %.2lf\n", hh_vuln[idx]);

    fprintf(unfitF, "-----------------------------------\n");
    fclose(unfitF); // Saved to unfit_person.txt

    hh_served[idx] = 0;
    hh_order[idx] = ++global_counter;
    hh_vuln[idx] = compute_vulnerability_index(idx);
    
    heap_size++;
    heapify_up(heap_size - 1);
    
    // NEW: Save to relief_data_array.txt immediately after registration
    save_data(CSV_FILE);
    
    printf("\nHousehold successfully registered!\n");
    printf("Data synced to %s and unfit_person.txt\n", CSV_FILE);
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
    printf("Next to serve: ID %llu | Head: %s | Zone: %s | Vulnerability: %.2f\n", (unsigned long long)hh_id[0], hh_head[0], hh_zone[0], hh_vuln[0]);
    int confirm;
    // = read_int_prompt("Allocate relief pack to this household? (1=yes, 0=no): ");
    printf("Allocate relief pack to this household? (1=yes, 0=no): ");
    scanf("%d", &confirm);

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


// <=====|| OPTION #4 FUNCTION ||=====>

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

    /* Print header */
    printf("\n%-4s | %-8s | %-15s | %-10s | %-8s | %-8s | %-8s | %-9s | %-9s | %-10s\n",
           "RANK", "ID", "HEAD_NAME", "ZONE", "MEMBERS", "ELDERLY", "INFANTS", "DISABLED", "PREGNANT", "SERVED");
    printf("=====|==========|=================|============|==========|==========|==========|===========|===========|============\n");

    int rank = 1;
    while (tmp_size > 0) {
        /* Find index of highest priority */
        int best = 0;
        for (i = 1; i < tmp_size; ++i) {
            if (tmp_vuln[i] > tmp_vuln[best]) best = i;
            else if (tmp_vuln[i] == tmp_vuln[best] && tmp_order[i] < tmp_order[best]) best = i;
        }
        
        printf("%-4d | %-8llu | %-15s | %-10s | %-8d | %-8d | %-8d | %-9d | %-9d | %-10s\n",
               rank,
               (unsigned long long)tmp_id[best],
               tmp_head[best],
               tmp_zone[best],
               tmp_members[best],
               tmp_elderly[best],
               tmp_infants[best],
               tmp_disabled[best],
               tmp_pregnant[best],
               tmp_served[best] ? "Yes" : "No");

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
    printf("\n");
}

int find_index_by_id(unsigned long long id) {
    int i;
    for (i = 0; i < heap_size; ++i)
        if (hh_id[i] == id) return i;
    return -1;
}


//  <=====|| OPTION #5 ||=====>

void search_update_ui(void) {
    if (heap_size == 0) {
        printf("No households registered yet.\n");
        return;
    }

    printf("\n--- Search Household ---\n");
    printf("1. Search by ID\n");
    printf("2. Search by Head Name\n");
    int searchChoice = read_int_prompt("Choose search method: ");

    int idx = -1;
    if (searchChoice == 1) {
        int id_input = read_int_prompt("Enter household ID: ");
        idx = find_index_by_id((unsigned long long)id_input);
    } 
    else if (searchChoice == 2) {
        char name_input[MAX_NAME];
        printf("Enter Head Name: ");
        read_line(name_input, MAX_NAME);
        idx = find_index_by_name(name_input);
    } 
    else {
        printf("Invalid choice.\n");
        return;
    }

    if (idx < 0) {
        printf("Household not found.\n");
        return;
    }

    // Display Block
    printf("\n==========================================\n");
    printf("ID: %llu | Head: %s | Zone: %s\n", hh_id[idx], hh_head[idx], hh_zone[idx]);
    printf("Members: %d (Elderly: %d | Infant: %d | PWD:%d | Pregnant: %d)\n", 
            hh_members[idx], hh_elderly[idx], hh_infants[idx], hh_disabled[idx], hh_pregnant[idx]);
    printf("Vulnerability: %.2f | Served: %s\n", hh_vuln[idx], hh_served[idx] ? "YES" : "NO");
    printf("==========================================\n");

    printf("\nActions: 1) Update counts  2) Toggle served  3) Remove 4) Show All Information  0) Back\n");
    int action = read_int_prompt("Choice: ");

    if (action == 1) {
        // 1. Get New Total first for calculations later
        hh_members[idx] = read_int_prompt("New Total Members: ");

        FILE *unfitF = fopen("unfit_person.txt", "a");
        if (!unfitF) {
            printf("Error: Could not open records file.\n");
            return;
        }
        
        fprintf(unfitF, "UPDATED Household ID: %llu (Head: %s)\n", hh_id[idx], hh_head[idx]);
        char tempName[MAX_NAME];

        // 2. ELDERLY: Number -> Names
        hh_elderly[idx] = read_int_prompt("New Elderly count: ");
        for (int i = 0; i < hh_elderly[idx]; i++) {
            printf("  -> Name of New Elderly #%d: ", i + 1);
            read_line(tempName, MAX_NAME);
            fprintf(unfitF, " - [Elderly] %s\n", tempName);
        }

        // 3. INFANTS: Number -> Names
        hh_infants[idx] = read_int_prompt("New Infants count: ");
        for (int i = 0; i < hh_infants[idx]; i++) {
            printf("  -> Name of New Infant #%d: ", i + 1);
            read_line(tempName, MAX_NAME);
            fprintf(unfitF, " - [Infant] %s\n", tempName);
        }

        // 4. PWD: Number -> Names
        hh_disabled[idx] = read_int_prompt("New PWD count: ");
        for (int i = 0; i < hh_disabled[idx]; i++) {
            printf("  -> Name of New PWD #%d: ", i + 1);
            read_line(tempName, MAX_NAME);
            fprintf(unfitF, " - [PWD] %s\n", tempName);
        }

        // 5. PREGNANT: Number -> Names
        hh_pregnant[idx] = read_int_prompt("New Pregnant count: ");
        for (int i = 0; i < hh_pregnant[idx]; i++) {
            printf("  -> Name of New Pregnant #%d: ", i + 1);
            read_line(tempName, MAX_NAME);
            fprintf(unfitF, " - [Pregnant] %s\n", tempName);
        }

        // 6. FIT PEOPLE: Auto-calculate remaining
        int vulnerable_count = hh_elderly[idx] + hh_infants[idx] + hh_disabled[idx] + hh_pregnant[idx];
        int fit_count = hh_members[idx] - vulnerable_count;
        
        if (fit_count > 0) {
            printf("Remaining members (Fit): %d\n", fit_count);
            for (int i = 0; i < fit_count; i++) {
                printf("  -> Name of New Fit Member #%d: ", i + 1);
                read_line(tempName, MAX_NAME);
                fprintf(unfitF, " - [Fit] %s\n", tempName);
            }
        }

        fprintf(unfitF, "-----------------------------------\n");
        fclose(unfitF);

        // Finalize state in RAM
        hh_vuln[idx] = compute_vulnerability_index(idx);
        rebuild_heap(); 
        printf("\nUpdate successful! All names captured and re-prioritized.\n");
    }
    else if (action == 2) {
        hh_served[idx] = !hh_served[idx]; // Flip the status
        hh_vuln[idx] = compute_vulnerability_index(idx);
        rebuild_heap();
        printf("Status toggled. Now %s.\n", hh_served[idx] ? "Served" : "Unserved");
    } 
    else if (action == 3) {
        unsigned long long removed_id = hh_id[idx];
        // Move the last element to the current spot to "delete"
        heap_size--;
        if (idx != heap_size) {
            swap_all(idx, heap_size);
        }
        rebuild_heap();
        printf("Household ID %llu removed from system.\n", removed_id);
    }
    else if (action == 4) {
        display_detailed_names(hh_id[idx]);
    }
}

/* ---------- Account Manager ---------- */

void accountManager() {
    // Initialized properly (rest will be null/zero)
    char usernames[MAX_ACCOUNTS][24] = {"admin"}; 
    char passwords[MAX_ACCOUNTS][24] = {"1234"};
    int totalAccounts = 1;
    bool loggedIn = false;

    while (!loggedIn) {
        printf("\n<=====|| WELCOME TO OUR PROGRAM ||=====>\n");

        // LOGIN LOOP
        while (1) {
            char u[24], p[24];
            printf("\n<--- LOGIN --->\n"); 

            printf("Username: "); 
            if (fgets(u, sizeof(u), stdin)) {
                // REMOVE NEWLINE: Replace '\n' with null terminator '\0'
                u[strcspn(u, "\n")] = 0;
            }
            
            printf("Password: "); 
            if (fgets(p, sizeof(p), stdin)) {
                p[strcspn(p, "\n")] = 0;
            }

            bool found = false;

            // CORRECT BOUNDS: i < totalAccounts (not <=)
            for (int i = 0; i < totalAccounts; i++) {
                if (strcmp(usernames[i], u) == 0 && strcmp(passwords[i], p) == 0) {
                    printf("\nWelcome, %s! Access Granted.\n", u);
                    loggedIn = true;
                    found = true;
                    break;
                }
            }

            if (found) {
                break; // Exit the inner while(1)
            } else {
                printf("Error: Invalid credentials. Please try again.\n");
                break;
            }
        }
    }
    
    printf("Entering main system...\n");
}

int find_index_by_name(const char *name) {
    for (int i = 0; i < heap_size; i++) {
        // This checks if the search string exists anywhere inside the head name
        if (strstr(hh_head[i], name) != NULL) {
            return i;
        }
    }
    return -1;
}


// <=====|| SUB-OPTION #4 OF OPTION #5 ||=====>

void display_detailed_names(unsigned long long target_id) {
    FILE *f = fopen("unfit_person.txt", "r");
    if (!f) {
        printf("\n[!] No name records found (unfit_person.txt is empty).\n");
        return;
    }

    char line[256];
    char search_str[64];
    long last_pos = -1;
    
    // This will match both "Household ID: 1001" and "UPDATED Household ID: 1001"
    sprintf(search_str, "Household ID: %llu", target_id);

    // FIRST PASS: Find the position of the LAST occurrence in the file
    while (ftell(f), fgets(line, sizeof(line), f)) {
        if (strstr(line, search_str) != NULL) {
            // Store the byte position where this specific block starts
            last_pos = ftell(f) - strlen(line);
        }
    }

    printf("\n------------------------------------------\n");
    printf("   DETAILED MEMBER LIST FOR ID: %llu\n", target_id);
    printf("------------------------------------------\n");

    if (last_pos != -1) {
        // SECOND PASS: Jump directly to that last occurrence and print
        fseek(f, last_pos, SEEK_SET);
        
        // Skip the header line we just jumped to
        fgets(line, sizeof(line), f); 

        while (fgets(line, sizeof(line), f)) {
            // Stop if we hit the separator or the end of the file
            if (strstr(line, "-----------------------------------") != NULL) {
                break; 
            }
            printf("%s", line);
        }
    } else {
        printf("  No specific names recorded for this household.\n");
    }

    printf("------------------------------------------\n");
    fclose(f);
}

void trim_spaces(char *str) {
    if (str == NULL) return;
    
    // Trim trailing spaces/newlines
    int len = my_strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }

    // Trim leading spaces
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t') {
        start++;
    }
    
    if (start > 0) {
        int i;
        for (i = 0; str[i + start] != '\0'; i++) {
            str[i] = str[i + start];
        }
        str[i] = '\0';
    }
}