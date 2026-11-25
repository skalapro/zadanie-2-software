#include <stdio.h>


#define MAX 1000
// tablica z typem wiadomosci od poszczegolnych modulow
char modems[][20] = {
    "Temperature",
    "Pressure",
    "Battery",
    "GPS",
    "Status"
};
// struktura z wszystkimi danymi wiadomosci
typedef struct
{
    int can_id;
    int data_size;
    int period_us;
    int deadline_us;
    int generation_time;
} message;
// struktura z statystykami dla konkretnej wiadomosci
typedef struct
{
    int can_id;
    int missed_deadlines;
    int sent_messages;
    long long total_delay;
    int max_delay;
} stats;

// statystyki dla poszczegolnych modemow
stats message_stats[] =
{
    {0x010, 0, 0, 0, 0},
    {0x020, 0, 0, 0, 0},
    {0x100, 0, 0, 0, 0},
    {0x200, 0, 0, 0, 0},
    {0x300, 0, 0, 0, 0}
};

int queue_size = 0;
int can_busy_until = 0;

message queue[MAX];


void queue_push(message x);
int find_highest_priority(void);
void send_message(int idx, int time);
void queue_remove(int idx);

int main(void)
{
    message modems_messeges[] =
    {
        {0x010, 4, 10000, 50000, 0},   // temperatura
        {0x020, 4, 20000, 10000, 0},   // cisnienie
        {0x100, 2, 100000, 200000, 0},  // bateria
        {0x200, 8, 200000, 500000, 0},  // GPS
        {0x300, 4, 500000, 1000000, 0}   // status
    };
    

    for(int time = 0; time <= 100000000; time++)
    {
        for(int i = 0; i < 5; i++)
        {
            if(time % modems_messeges[i].period_us == 0)
            {
                message temp = modems_messeges[i];

                temp.generation_time = time; // zapis czasu generacji wiadomosci

                queue_push(temp);
            }
        }
        if(queue_size > 0 && time >= can_busy_until)
        {
            int idx = find_highest_priority();
            send_message(idx, time);
            int bits_to_send = 44 + 8 * queue[idx].data_size; //44 - ilosc bitow narzutu protokolu
            bits_to_send += bits_to_send / 10; // 10% stuffingu 
            int transmission_time_us = bits_to_send * 2; // 1 bit na 2us
            queue_remove(idx); 
            can_busy_until = time + transmission_time_us;
        }   
    }
    printf("\n--- RESULTS ---\n");
    for (int i = 0; i < 5; i++)
    {
        printf("--Modem: %s--\n", modems[i]);

        printf("Average delay: %.2f us\n",
            (double)message_stats[i].total_delay / message_stats[i].sent_messages);

        printf("Max delay: %d us\n",
            message_stats[i].max_delay);

        printf("Missed deadlines: %d\n",
            message_stats[i].missed_deadlines);
    }
    return 0;
}
// zapisywanie wiadomosci do kolejki
void queue_push(message x)
{
    queue[queue_size] = x;

    queue_size++;
}
/* funkcja do znajdowania najnizszego id ramki
 nizsze id ramki = wyzszy priorytet*/
int find_highest_priority(void)
{
    int lowest_id = queue[0].can_id;
    int found_index = 0;
    for (int i = 1; i < queue_size; i++)
    {
        if (queue[i].can_id <= lowest_id)
        {
            lowest_id = queue[i].can_id;
            found_index = i; 
        }
    }
    return found_index;
}

void send_message(int idx, int time)
{
    for (int i = 0; i < 5; i++)
    {
        if (message_stats[i].can_id == queue[idx].can_id)
        {
            int delay = time - queue[idx].generation_time;
            message_stats[i].total_delay += delay;
            message_stats[i].sent_messages++;
            if (delay > message_stats[i].max_delay)
                message_stats[i].max_delay = delay;
            if (delay > queue[idx].deadline_us)
                message_stats[i].missed_deadlines++;
        }

    }
}
// usuwanie wiadomosci z kolejki i przesuniecie kolejki
void queue_remove(int idx)
{
    for (int i = idx; i < queue_size - 1; i++)
    {
        queue[i] = queue[i + 1];
    }
    queue_size--;
}