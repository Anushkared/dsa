/*
Car Parking Management System (Single-file C project)
Features:
 - 2D array for parking layout (rows x cols)
 - Queue for waiting cars (FIFO)
 - Time-stamped entry/exit and fee calculation
 - Console frontend (menu-driven)
 - Simple backend persistence for parking records (records.txt)
 - Save/Load current parking state (state.dat)

Team Members:
1. Kavya        - 22BI25AR402-T
2. K. Anushka    - 22BI25AR405-T
3. Aisha. S      - 22BI25AR407-T
4. Hafeeza Muskan- 22BI25AR411-T

Compile: gcc Car_Parking_Management_System.c -o parking
Run: ./parking

Notes:
 - This is a single-file implementation to make compilation and testing easy.
 - You may split into multiple files (header + sources) if you prefer.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAX_ROWS 4
#define MAX_COLS 5
#define MAX_SLOTS (MAX_ROWS*MAX_COLS)
#define MAX_REG_LEN 32
#define QUEUE_CAP 50
#define RECORDS_FILENAME "records.txt"
#define STATE_FILENAME "state.dat"
#define RATE_PER_HOUR 20.0    // change as needed

/* Slot representation */
typedef struct {
    int occupied;           // 0 = free, 1 = occupied
    char reg[MAX_REG_LEN];  // vehicle registration / id
    time_t entry_time;      // timestamp when car entered
} Slot;

/* Parking lot as 2D array wrapper */
typedef struct {
    int rows;
    int cols;
    Slot slots[MAX_ROWS][MAX_COLS];
} ParkingLot;

/* Simple queue for waiting vehicles (store registration numbers) */
typedef struct {
    char items[QUEUE_CAP][MAX_REG_LEN];
    int front;
    int rear;
    int size;
} Queue;

/* Initialize queue */
void initQueue(Queue *q){
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}
int isQueueEmpty(Queue *q){ return q->size == 0; }
int isQueueFull(Queue *q){ return q->size == QUEUE_CAP; }

void enqueue(Queue *q, const char *reg){
    if(isQueueFull(q)){
        printf("Waiting queue is full. Cannot enqueue %s\n", reg);
        return;
    }
    q->rear = (q->rear + 1) % QUEUE_CAP;
    strncpy(q->items[q->rear], reg, MAX_REG_LEN-1);
    q->items[q->rear][MAX_REG_LEN-1] = '\0';
    q->size++;
}

void dequeue(Queue *q, char *out){
    if(isQueueEmpty(q)){
        out[0] = '\0';
        return;
    }
    strncpy(out, q->items[q->front], MAX_REG_LEN-1);
    out[MAX_REG_LEN-1] = '\0';
    q->front = (q->front + 1) % QUEUE_CAP;
    q->size--;
}

/* Initialize parking lot */
void initParkingLot(ParkingLot *p, int rows, int cols){
    p->rows = rows;
    p->cols = cols;
    for(int i=0;i<rows;i++){
        for(int j=0;j<cols;j++){
            p->slots[i][j].occupied = 0;
            p->slots[i][j].reg[0] = '\0';
            p->slots[i][j].entry_time = 0;
        }
    }
}

/* Find nearest available slot (row-major) -1 */
int findNearestFreeSlot(ParkingLot *p, int *out_r, int *out_c){
    for(int i=0;i<p->rows;i++){
        for(int j=0;j<p->cols;j++){
            if(!p->slots[i][j].occupied){
                *out_r = i; *out_c = j;
                return 1;
            }
        }
    }
    return 0; // none free
}

/* Visualize parking slots */
void displayParkingLot(ParkingLot *p){
    printf("\nParking Layout (%dx%d) - 'Free' or Reg#: \n", p->rows, p->cols);
    printf("+----+----+----+----+----+\n");
    for(int i=0;i<p->rows;i++){
        for(int j=0;j<p->cols;j++){
            if(p->slots[i][j].occupied){
                printf("| %-3s", p->slots[i][j].reg);
            } else {
                printf("| %-3s", "---");
            }
        }
        printf("|\n");
        printf("+----+----+----+----+----+\n");
    }
}

/* Display slots with indices for easier reference */
void displayParkingLotWithIndices(ParkingLot *p){
    printf("\nParking Slots with indices (row,col) and status:\n");
    for(int i=0;i<p->rows;i++){
        for(int j=0;j<p->cols;j++){
            printf("[%d,%d] ", i, j);
            if(p->slots[i][j].occupied){
                char timestr[64];
                struct tm *tminfo = localtime(&p->slots[i][j].entry_time);
                strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M", tminfo);
                printf("OCC(%s @%s)  ", p->slots[i][j].reg, timestr);
            } else printf("FREE       ");
        }
        printf("\n");
    }
}

/* Save a record to textual log file */
void appendRecord(const char *reg, time_t entry, time_t exit_time, double fee){
    FILE *f = fopen(RECORDS_FILENAME, "a");
    if(!f) return;
    char entry_s[64], exit_s[64];
    struct tm *tm;
    tm = localtime(&entry);
    strftime(entry_s, sizeof(entry_s), "%Y-%m-%d %H:%M:%S", tm);
    tm = localtime(&exit_time);
    strftime(exit_s, sizeof(exit_s), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(f, "%s, %s, %s, %.2f\n", reg, entry_s, exit_s, fee);
    fclose(f);
}

/* Save current state (parking lot + queue) to binary file */
void saveState(ParkingLot *p, Queue *q){
    FILE *f = fopen(STATE_FILENAME, "wb");
    if(!f){ printf("Warning: could not save state to %s\n", STATE_FILENAME); return; }
    fwrite(&p->rows, sizeof(int), 1, f);
    fwrite(&p->cols, sizeof(int), 1, f);
    fwrite(p->slots, sizeof(Slot), MAX_ROWS*MAX_COLS, f);
    fwrite(&q->front, sizeof(int), 1, f);
    fwrite(&q->rear, sizeof(int), 1, f);
    fwrite(&q->size, sizeof(int), 1, f);
    fwrite(q->items, sizeof(q->items), 1, f);
    fclose(f);
}

/* Load state if exists */
int loadState(ParkingLot *p, Queue *q){
    FILE *f = fopen(STATE_FILENAME, "rb");
    if(!f) return 0;
    int rows, cols;
    fread(&rows, sizeof(int), 1, f);
    fread(&cols, sizeof(int), 1, f);
    p->rows = rows; p->cols = cols;
    fread(p->slots, sizeof(Slot), MAX_ROWS*MAX_COLS, f);
    fread(&q->front, sizeof(int), 1, f);
    fread(&q->rear, sizeof(int), 1, f);
    fread(&q->size, sizeof(int), 1, f);
    fread(q->items, sizeof(q->items), 1, f);
    fclose(f);
    return 1;
}

/* Check if reg already parked */
int isCarParked(ParkingLot *p, const char *reg, int *r_out, int *c_out){
    for(int i=0;i<p->rows;i++){
        for(int j=0;j<p->cols;j++){
            if(p->slots[i][j].occupied && strcmp(p->slots[i][j].reg, reg) == 0){
                if(r_out) *r_out = i;
                if(c_out) *c_out = j;
                return 1;
            }
        }
    }
    return 0;
}

/* Car entry: assign slot or enqueue */
void carEntry(ParkingLot *p, Queue *q){
    char reg[MAX_REG_LEN];
    printf("Enter vehicle registration number: ");
    scanf("%31s", reg);

    int rr, cc;
    if(isCarParked(p, reg, NULL, NULL)){
        printf("This vehicle is already parked in the lot.\n");
        return;
    }

    if(findNearestFreeSlot(p, &rr, &cc)){
        p->slots[rr][cc].occupied = 1;
        strncpy(p->slots[rr][cc].reg, reg, MAX_REG_LEN-1);
        p->slots[rr][cc].reg[MAX_REG_LEN-1] = '\0';
        p->slots[rr][cc].entry_time = time(NULL);
        printf("Allocated slot: [%d,%d] to %s\n", rr, cc, reg);
    } else {
        printf("Parking is full. Adding vehicle to waiting queue.\n");
        enqueue(q, reg);
    }
}

/* When slot freed, if queue not empty assign first in queue */
void assignFromQueueIfAny(ParkingLot *p, Queue *q){
    if(isQueueEmpty(q)) return;
    int rr, cc;
    if(findNearestFreeSlot(p, &rr, &cc)){
        char reg[MAX_REG_LEN];
        dequeue(q, reg);
        p->slots[rr][cc].occupied = 1;
        strncpy(p->slots[rr][cc].reg, reg, MAX_REG_LEN-1);
        p->slots[rr][cc].entry_time = time(NULL);
        printf("Assigned queued vehicle %s to slot [%d,%d]\n", reg, rr, cc);
    }
}

/* Car exit: search by reg, compute fee, free slot */
void carExit(ParkingLot *p, Queue *q){
    char reg[MAX_REG_LEN];
    printf("Enter vehicle registration number to exit: ");
    scanf("%31s", reg);

    int rr=-1, cc=-1;
    if(!isCarParked(p, reg, &rr, &cc)){
        printf("Vehicle not found in parking slots.\n");
        return;
    }
    time_t exit_time = time(NULL);
    time_t entry_time = p->slots[rr][cc].entry_time;
    double seconds = difftime(exit_time, entry_time);
    double hours = seconds / 3600.0;
    double billed_hours = ceil(hours);
    if(billed_hours < 1) billed_hours = 1; // minimum 1 hour
    double fee = billed_hours * RATE_PER_HOUR;

    char entry_s[64], exit_s[64];
    struct tm *tm;
    tm = localtime(&entry_time);
    strftime(entry_s, sizeof(entry_s), "%Y-%m-%d %H:%M:%S", tm);
    tm = localtime(&exit_time);
    strftime(exit_s, sizeof(exit_s), "%Y-%m-%d %H:%M:%S", tm);

    printf("Vehicle %s leaving slot [%d,%d]\n", reg, rr, cc);
    printf("Entry: %s\nExit : %s\nDuration(seconds): %.0f\nHours(billed): %.0f\nFee: %.2f\n",
           entry_s, exit_s, seconds, billed_hours, fee);

    appendRecord(reg, entry_time, exit_time, fee);
    // free slot
    p->slots[rr][cc].occupied = 0;
    p->slots[rr][cc].reg[0] = '\0';
    p->slots[rr][cc].entry_time = 0;

    // assign from queue
    assignFromQueueIfAny(p, q);
}

/* Display waiting queue */
void displayQueue(Queue *q){
    if(isQueueEmpty(q)){
        printf("Waiting queue is empty.\n");
        return;
    }
    printf("Waiting queue (front -> rear):\n");
    int idx = q->front;
    for(int i=0;i<q->size;i++){
        printf("%d. %s\n", i+1, q->items[idx]);
        idx = (idx + 1) % QUEUE_CAP;
    }
}

/* Show records file tail-like (last N entries) */
void showRecentRecords(int n){
    FILE *f = fopen(RECORDS_FILENAME, "r");
    if(!f){ printf("No records yet.\n"); return; }
    // Read all lines into memory (ok for small projects)
    char buffer[256];
    char *lines[1000];
    int count = 0;
    while(fgets(buffer, sizeof(buffer), f) && count < 1000){
        lines[count] = malloc(strlen(buffer)+1);
        strcpy(lines[count], buffer);
        count++;
    }
    fclose(f);
    int start = count - n; if(start < 0) start = 0;
    printf("Last %d records:\n", n);
    for(int i=start;i<count;i++){
        printf("%s", lines[i]);
        free(lines[i]);
    }
}

/* Simple search for a vehicle in parking and queue */
void searchVehicle(ParkingLot *p, Queue *q){
    char reg[MAX_REG_LEN];
    printf("Enter registration to search: ");
    scanf("%31s", reg);
    int rr, cc;
    if(isCarParked(p, reg, &rr, &cc)){
        printf("Found in slot [%d,%d] - entry time: %ld\n", rr, cc, (long)p->slots[rr][cc].entry_time);
        return;
    }
    // search queue
    int idx = q->front;
    for(int i=0;i<q->size;i++){
        if(strcmp(q->items[idx], reg) == 0){
            printf("Found in waiting queue position %d\n", i+1);
            return;
        }
        idx = (idx + 1) % QUEUE_CAP;
    }
    printf("Vehicle not found in parking or queue.\n");
}

/* Main menu */
void menuLoop(ParkingLot *p, Queue *q){
    int choice;
    while(1){
        printf("\n==== Car Parking Management System ===="
               "\n1. Car Entry\n2. Car Exit\n3. Display Parking Layout\n4. Display Parking Layout (detailed)\n5. Display Waiting Queue\n6. Show Recent Records\n7. Search Vehicle\n8. Save State\n9. Load State\n0. Exit\nChoose: ");
        if(scanf("%d", &choice) != 1){
            // clear invalid input
            while(getchar()!='\n');
            printf("Invalid choice. Try again.\n");
            continue;
        }
        switch(choice){
            case 1: carEntry(p, q); break;
            case 2: carExit(p, q); break;
            case 3: displayParkingLot(p); break;
            case 4: displayParkingLotWithIndices(p); break;
            case 5: displayQueue(q); break;
            case 6: showRecentRecords(10); break;
            case 7: searchVehicle(p, q); break;
            case 8: saveState(p, q); printf("State saved.\n"); break;
            case 9: if(loadState(p, q)) printf("State loaded.\n"); else printf("No saved state found.\n"); break;
            case 0: saveState(p, q); printf("Exiting. State saved. Bye!\n"); return;
            default: printf("Invalid option.\n");
        }
    }
}

int main(){
    ParkingLot lot;
    Queue q;
    initQueue(&q);
    initParkingLot(&lot, MAX_ROWS, MAX_COLS);

    // try load state on startup
    if(loadState(&lot, &q)){
        printf("Previous state loaded from %s\n", STATE_FILENAME);
    }

    printf("Welcome to Car Parking Management System\n");
    printf("Rate per hour: %.2f\n", RATE_PER_HOUR);
    menuLoop(&lot, &q);
    return 0;
}
