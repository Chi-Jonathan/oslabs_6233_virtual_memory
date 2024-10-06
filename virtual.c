
#include "oslabs.h"
#include <stdio.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Empty Page
const struct PTE EMPTY_PAGE = {0, -1, -1, -1, -1};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Helper functions

void print_pte(struct PTE pte) {
    printf("Valid: %d\n Frame Number: %d\n Arrival Timestamp: %d\n Last Access Timestamp: %d\n Reference Count: %d\n",
        pte.is_valid, pte.frame_number, pte.arrival_timestamp, pte.last_access_timestamp, pte.reference_count);
};

// Inserts a new page into the page table
int insert_page_table(struct PTE page_table[TABLEMAX], int frame_number, int current_timestamp, int page_number) {
    page_table[page_number].is_valid = 1;
    page_table[page_number].frame_number = frame_number;
    page_table[page_number].arrival_timestamp = current_timestamp;
    page_table[page_number].last_access_timestamp = current_timestamp;
    page_table[page_number].reference_count = 1;
    return frame_number;
}

// Updates an entry in the page table if it's already there
int update_page_table(struct PTE page_table[TABLEMAX], int current_timestamp, int page_number) {
    page_table[page_number].last_access_timestamp = current_timestamp;
    page_table[page_number].reference_count += 1;
    return page_table[page_number].frame_number;
}

// Checks if there are is already a frame given or a free frame, if so updates/inserts data into table
int has_usable_frame(struct PTE page_table[TABLEMAX], int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {
    // Checks if there is an entry already in the page table for that page number, if there is then just run the update page table function
    if (page_table[page_number].is_valid) {
        return update_page_table(page_table, current_timestamp, page_number);
    }
    // If there is a free frame then give it that frame and insert the entry into the table
    else if (*frame_cnt != 0) {
        *frame_cnt -= 1;
        return insert_page_table(page_table, frame_pool[*frame_cnt], current_timestamp, page_number);
    }
    else {
        return -1;
    }
}

// Checks if the page index is valid if it is insert the new entry and remove the previous page
int swap_frames(int page_index, struct PTE page_table[TABLEMAX], int page_number, int current_timestamp) {
    if (page_index == -1) {
        return -1;
    }
    else {
        int frame_num = insert_page_table(page_table, page_table[page_index].frame_number, current_timestamp, page_number);
        page_table[page_index]= EMPTY_PAGE;
        return frame_num;
    }
}

// Finds the last arrival time
int find_last_access(struct PTE page_table[TABLEMAX], int table_cnt) {
    int latest_arrival = 0;
    for (int i = 0; i < table_cnt; i++) {
        if (page_table[i].is_valid && page_table[i].last_access_timestamp > latest_arrival) {
            latest_arrival = page_table[i].last_access_timestamp;
        }
    }
    return latest_arrival;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FIFO

// Process Page Access FIFO
int process_page_access_fifo(struct PTE page_table[TABLEMAX],int *table_cnt, int page_number, int frame_pool[POOLMAX],int *frame_cnt, int current_timestamp) {\
    // Checks if there is a usable frame
    int usable_frame = has_usable_frame(page_table, page_number, frame_pool, frame_cnt, current_timestamp);
    if (usable_frame != -1) {
        return usable_frame;
    }
    // If the frame count is zero then find a valid frame with the lowest arrival time to reallocate it to the new entry (FIFO)
    else {
        int first_page_index = -1;
        int arrive_t = 100000000;
        for (int i = 0; i < *table_cnt; i++) {
            if (page_table[i].is_valid && arrive_t > page_table[i].arrival_timestamp) {
                first_page_index = i;
                arrive_t = page_table[i].arrival_timestamp;
            }
        }
        return swap_frames(first_page_index, page_table, page_number, current_timestamp);
    }
}



// Count Page Faults FIFO
int count_page_faults_fifo(struct PTE page_table[TABLEMAX],int table_cnt, int refrence_string[REFERENCEMAX],int reference_cnt,int frame_pool[POOLMAX],int frame_cnt) {
    int faults = 0;
    int latest_arrival = find_last_access(page_table, table_cnt);
    // if the reference string is not valid run the fifo page algorithm to update the table then add one for that page fault for each reference
    for (int i = 0; i < reference_cnt; i++) {
        latest_arrival+=1;
        if(!page_table[refrence_string[i]].is_valid) {
            faults += 1;
        }
        process_page_access_fifo(page_table, &table_cnt, refrence_string[i], frame_pool, &frame_cnt, latest_arrival);
    }
    return faults;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRU

// Process Page Access LRU
int process_page_access_lru(struct PTE page_table[TABLEMAX],int *table_cnt, int page_number, int frame_pool[POOLMAX],int *frame_cnt, int current_timestamp) {
    // Check if there is a usable frame
    int usable_frame = has_usable_frame(page_table, page_number, frame_pool, frame_cnt, current_timestamp);
    if (usable_frame != -1) {
        return usable_frame;
    }
    // If the frame count is zero then find a valid frame with the lowest last access timestamp to reallocate it to the new entry (LRU)
    else {
        int lru_page_index = -1;
        int lats = 10000000;
        for (int i = 0; i < *table_cnt; i++) {
            if (page_table[i].is_valid && lats > page_table[i].last_access_timestamp) {
                lru_page_index = i;
                lats = page_table[i].last_access_timestamp;
            }
        }
        return swap_frames(lru_page_index, page_table, page_number, current_timestamp);
    }

}



// Count Page Faults LRU
int count_page_faults_lru(struct PTE page_table[TABLEMAX],int table_cnt, int refrence_string[REFERENCEMAX],int reference_cnt,int frame_pool[POOLMAX],int frame_cnt) {
    int faults = 0;
    int curr_time = find_last_access(page_table, table_cnt);
    // if the reference string is not valid run the lru page algorithm to update the table then add one for that page fault for each reference
    for (int i = 0; i < reference_cnt; i++) {
        curr_time += 1;
        if(!page_table[refrence_string[i]].is_valid) {
            faults += 1;
        }
        process_page_access_lru(page_table, &table_cnt, refrence_string[i], frame_pool, &frame_cnt, curr_time);
    }
    return faults;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LFU

// Process Page Access LFU
int process_page_access_lfu(struct PTE page_table[TABLEMAX],int *table_cnt, int page_number, int frame_pool[POOLMAX],int *frame_cnt, int current_timestamp) {
    // Check if there is a usable frame
    int usable_frame = has_usable_frame(page_table, page_number, frame_pool, frame_cnt, current_timestamp);
    if (usable_frame != -1) {
        return usable_frame;
    }
    // If the frame count is zero then find a valid frame with the greatest reference count to reallocate it to the new entry (LFU)
    else {
        // Get all the possible indices that can be used
        int lfu_page_index[*table_cnt];
        int possible_entries_cnt = 0;
        int ref_cnt = 10000000;
        // Increments the possible entries count for each new possible entry, a possible entry is a frame with the lowest RC
        for (int i = 0; i < *table_cnt; i++) {
            if (page_table[i].is_valid && ref_cnt > page_table[i].reference_count) {
                lfu_page_index[0] = i;
                ref_cnt = page_table[i].reference_count;
                possible_entries_cnt = 1;
            }
            else if(page_table[i].is_valid && ref_cnt == page_table[i].reference_count) {
                lfu_page_index[possible_entries_cnt] = i;
                ref_cnt = page_table[i].reference_count;
                possible_entries_cnt += 1;
            }
        }
        if (possible_entries_cnt == 0) {
            return -1;
        }
        // if the amount of possible entries is only 1 just swap out that frame
        else if (possible_entries_cnt == 1) {
            return swap_frames(lfu_page_index[0], page_table, page_number, current_timestamp);
        }
        // if the amount of possible entries is more than 1 then perform LRU with the possible entries
        else {
            int lru_page_index = -1;
            int lats = 10000000;
            for (int i = 0; i < possible_entries_cnt; i++) {
                if (lats > page_table[lfu_page_index[i]].last_access_timestamp) {
                    lru_page_index = lfu_page_index[i];
                    lats = page_table[lfu_page_index[i]].last_access_timestamp;
                }
            }
            return swap_frames(lru_page_index, page_table, page_number, current_timestamp);
        }
    }
}



// Count Page Faults LFU
int count_page_faults_lfu(struct PTE page_table[TABLEMAX],int table_cnt, int refrence_string[REFERENCEMAX],int reference_cnt,int frame_pool[POOLMAX],int frame_cnt) {
    int faults = 0;
    int curr_time = find_last_access(page_table, table_cnt);
    // if the reference string is not valid run the lfu page algorithm to update the table then add one for that page fault for each reference
    for (int i = 0; i < reference_cnt; i++) {
        curr_time+=1;
        if(!page_table[refrence_string[i]].is_valid) {
            faults += 1;
        }
        process_page_access_lfu(page_table, &table_cnt, refrence_string[i], frame_pool, &frame_cnt, curr_time);
    }
    return faults;
}


