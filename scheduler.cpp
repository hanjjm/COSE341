
#include <stdio.h>

#include <stdlib.h>

#include <time.h>

#include <unistd.h>

#include <string.h>

//#include <windows.h>

#define DELAY 1

#define MinProcess 1

#define MaxProcess 10

#define MaxArrivaltime 20

#define MaxBursttime 10

#define MaxIOtime 6

#define IOfrequency 3

#define Maxperiod 200

 

int Context_Switch = 0;

 

int TIME; //�ð��� ����� ��Ÿ�������� ����

 

int gantt[1000];//��Ʈ ��Ʈ

 

int IO_time[1000]; //����� �ð�

 

 

 

typedef struct{

 

    int pid;

 

    int arrival_time;

 

    int waiting_time;

 

    int burst_time;

 

    int execution_time;

 

    int remain_time;

 

    int finish_time;

 

    int IO_start_time;

 

    int IO_execution_time;

 

    int IO_start_point;//�̰Ŵ� io�����Ҷ� �������� time 

 

    int IO_finish_time;//�̰Ŵ� io�۾� ������ ���������� �ִµ� �ű⼭ io�۾� ��ٸ��� �ִ� �ð� ///�� �ΰ��� io�۾��� �߻����� �� waiting time����� ���ϱ� ���ؼ� ����!!!!! 

 

    int IO_remain_time;

 

    int aging; 

 

    int period;

 

    int priority;//for realtime

 

}PCB;

 

 

 

typedef struct{

 

    int process;

 

    int IO_count;

 

    int context_switch;

 

    int sum_of_waiting_time;

 

    float ave_of_waiting_time;

 

    int sum_of_burst_time;

 

    float ave_of_burst_time;

 

    int sum_of_turnaround_time;

 

    float ave_of_turnaround_time;

 

}RESULT;

 

 

 

typedef struct{

 

    int front, rear, size, count;

 

    PCB *buffer;

 

}Queue;

 

 

 

PCB Running_Queue;//CPU���� ����Ǵ�  

 

RESULT result[10];

 

 

 

int process_count(void){

 

    int process_num;

 

    while(1){

 

        printf("\n");

 

        printf("�� ���� pocess�� �����Ͻðڽ��ϱ�? (1~10): ");

 

        scanf ("%d", &process_num);

 

            if(process_num < MinProcess || process_num > MaxProcess)

 

                printf("���ڷ� �ٽ� �Է����ֽʽÿ�. (1~10)\n");

 

            else break;

 

        }

 

    return process_num;

 

}

 

 

 

void init_Queue(Queue *queue, int p_num){

 

    p_num += 1;   //n+1 size�� queue�� n���� process�� �� �� �ִ�.

 

    queue->buffer = (PCB*)malloc(p_num*sizeof(PCB));

 

    memset(queue->buffer, 0, p_num * sizeof(PCB));

 

    queue->size = p_num;

 

    queue->front = 0;

 

    queue->rear = 0;

 

    queue->count = 0;

 

}

 

 

 

PCB init_PCB(){

 

    PCB pcb;

 

    pcb.pid = 0;

 

    pcb.arrival_time = 0;

 

    pcb.burst_time = 0;

 

    pcb.IO_start_time = 0;

 

    pcb.IO_execution_time = 0;

 

    pcb.IO_finish_time = 0;

 

    pcb.IO_start_point = 0;

 

    pcb.aging = 0;

 

    pcb.IO_remain_time = 0;

 

    pcb.execution_time = 0;

 

    pcb.priority = 0;

 

    pcb.remain_time = 0;

 

    pcb.waiting_time = 0;

 

    pcb.finish_time = 0;

 

    pcb.period = 0;

 

    return pcb;

 

} 

 

 

 

PCB get_Queue_head(Queue *queue){//Queue�� head�� ��ȯ�ϴ� �Լ�

 

    return queue->buffer[(queue->front+1)%queue->size];

 

} 

 

 

 

void dequeue(Queue *queue){

 

    if(queue->count == 0){

 

        printf("Queue ����ֽ��ϴ�.\n");

 

        return;

 

    } 

 

    queue->count -= 1;

 

    queue->buffer[queue->front] = init_PCB();//queue�� buffer�� �ִ� dequeue�Ѱ� �ʱ�ȭ���ְ� 

 

    queue->front = (queue->front+1)%queue->size;//queue�� front�� ��ĭ �ø�  

 

}

 

 

 

void enqueue(Queue *queue, PCB process){

 

    if((queue->rear+1)%queue->size == queue->front){//circular���� queue�� ��á�� �� ����

 

        printf("Queue �����ֽ��ϴ�.\n");

 

        exit(-1);//�������� ���� 

 

    }

 

    queue->count += 1;

 

    queue->rear = (queue->rear+1)%queue->size;//rear ��ĭ����������

 

    queue->buffer[queue->rear] = process;//process enqueue

 

}
 

int Queue_full_check(Queue *queue){//process num�� queue�� �ִ� process����

 

    if(queue->front == (queue->rear+1)%queue->size) return 1;//circular queue���� full ���� 

 

    else return 0;

 

}
 
/*
PCB dispatcher(Queue *Ready_Queue, PCB Running_Queue){

 

    if(Running_Queue.remain_time != 0) enqueue(Ready_Queue, Running_Queue);// running queue�� �۾��� �ȳ����� ��, ready ť�� ������

 

    Running_Queue = get_Queue_head(Ready_Queue);

 

    Context_Switch = Context_Switch + 1;

 

    return Running_Queue;

 

} 

 */

//���⼭ interrupt�� �߻����� �� running queue�� �ִ� ����

 

//ready queue�� �������� �־��ְ� running queue������ ready queue�� head�� �ִ�

 

//���� ��������, ��ĭ�� ������ �̷��ְ� context ++, running queue ��ȯ

 

//��, ����ť->����ť, ����ť���۾��̾ȳ������� ����ť->����ť, ������ �˰��� ���� ���� �ʾ���! 

 

 

 

 

 

void Copy_Queue(Queue *queue, Queue *copy_Queue){

 

    copy_Queue->front = queue->front;

 

    copy_Queue->rear = queue->rear;

 

    copy_Queue->count = queue->count;

 

    copy_Queue->size = queue->size;

 

    

    int i = 0;

    for(i=0; i<queue->size;i++)    copy_Queue->buffer[i] = queue->buffer[i];

 

}

 

 

 

void *sort_by_arrive(Queue *queue){// �� ���� �� 

 

    PCB temp;

     

    int i = 0, j = 0;

    

    for(i=(queue->front + 1)%queue->size; (i % queue->size)!=((queue->rear) % queue->size); i++){

 

        //�ݺ����� ����, i = front+1���� �����Ͽ� ������������ ��, ó������ ������

 

        i = i % queue->size;

 

        for(j=(i+1)% queue->size; (j % queue->size)!=((queue->rear+1) % queue->size); j++){

 

            //i �������� �� ���� 

 

            j = j % queue->size;

 

            if(queue->buffer[i].arrival_time>queue->buffer[j].arrival_time

 

                || (queue->buffer[i].arrival_time == queue->buffer[j].arrival_time

 

                    && queue->buffer[i].pid > queue->buffer[j].pid)){

 

                        temp = queue->buffer[i];

 

                        queue->buffer[i] = queue->buffer[j];

 

                        queue->buffer[j] = temp;

 

            }

 

        }

 

    }

 

    return queue;

 

}

 

//selection sort�� arrival time ������ ����!

 

 

 

void *sort_by_remain(Queue *queue){//�����ð������� ����  

 

    PCB temp;

    int i = 0;

    for(i=(queue->front + 1)%queue->size; (i % queue->size)!=((queue->rear) % queue->size); i++){

 

        i = i % queue->size;

 

    int j = 0;

        

    for(j=(i+1)% queue->size; (j % queue->size)!=((queue->rear+1) % queue->size); j++){

 

            j = j % queue->size;

 

            if(queue->buffer[i].remain_time>queue->buffer[j].remain_time

 

                || (queue->buffer[i].remain_time == queue->buffer[j].remain_time

 

                    && queue->buffer[i].pid > queue->buffer[j].pid)){

 

                        temp = queue->buffer[i];

 

                        queue->buffer[i] = queue->buffer[j];

 

                        queue->buffer[j] = temp;

 

            }

 

        }

 

    }

 

    return queue;

 

}

 

 

 

void *sort_by_priority(Queue *queue){//priority������ ����  

 

    PCB temp;

    int i = 0;

    for(i=(queue->front + 1)%queue->size; (i % queue->size)!=((queue->rear) % queue->size); i++){

 

        i = i % queue->size;

        

    int j = 0;

        for(j=(i+1)% queue->size; (j % queue->size)!=((queue->rear+1) % queue->size); j++){

 

            j = j % queue->size;

 

            if(queue->buffer[i].priority>queue->buffer[j].priority

 

                || (queue->buffer[i].priority == queue->buffer[j].priority

 

                    && queue->buffer[i].pid > queue->buffer[j].pid)){

 

                        temp = queue->buffer[i];

 

                        queue->buffer[i] = queue->buffer[j];

 

                        queue->buffer[j] = temp;

 

            }

 

        }

 

    }

 

    return queue;

 

}

 

void *sort_by_lottery(Queue *queue){//lottery algorithm�� ���� �Լ�, 1~100���� random�� �� �޾Ƽ� �׸�ŭ mix~~ 

    int mix;

    srand(time(NULL));

    PCB temp;

    mix = rand() % 100 ;

    int k;

    for(k = 0; k < mix; k++){

    int i = 0;

    for(i=(queue->front + 1)%queue->size; (i % queue->size)!=((queue->rear) % queue->size); i++){

        i = i % queue->size;

    int j = 0;

        for(j=(i+1)% queue->size; (j % queue->size)!=((queue->rear+1) % queue->size); j++){

            j = j % queue->size;

            temp = queue->buffer[i];

            queue->buffer[i] = queue->buffer[j];

            queue->buffer[j] = temp;

        }

    }

}

    return queue;

}

 

int *random_priority(int *priority_arr, int process_num){

 

    int mix1, mix2, temp;

 

    srand(time(NULL));

    int i = 0;

    for(i = 0; i < process_num; i++){

 

        priority_arr[i] = i+1;

 

 }

 

    for(i = 0; i < 100; i++){//������ �����Ͽ� �켱������ 100���� �����ϰ� �ٲ� 

 

        mix1 = rand() % process_num;

 

        mix2 = rand() % process_num;

 

        temp = priority_arr[mix1];

 

        priority_arr[mix1] = priority_arr[mix2];

 

        priority_arr[mix2] = temp;

 

    }

 

    return priority_arr; 

 

}

 

 

 

void *random_process(Queue *queue){//�̰��� ���� ó���� process���� �ʱ�ȭ���ش�. 

 

    int priority_arr_temp[100];

 

    int *priority_arr = random_priority(priority_arr_temp, queue->size);

 

    srand(time(NULL));

 

    

    int i = 0;

    for(i = (queue->front+1); i < queue->size; i++){

 

        queue->buffer[i].pid = i; // 1,2,3... �̷������� ����

 

        queue->buffer[i].arrival_time = rand() % MaxArrivaltime; // ���� �ð�0~19 �������� ����        

 

        queue->buffer[i].burst_time = rand() % MaxBursttime + 1; //burst time 1~10 �������� ����

 

        queue->buffer[i].waiting_time = 0; //���ð� 0���� �ʱ�ȭ

 

        queue->buffer[i].finish_time = 0;

 

        queue->buffer[i].execution_time = 0; //����ð� 0���� �ʱ�ȭ

 

        queue->buffer[i].remain_time = queue->buffer[i].burst_time; // ���� �ð��� burst_time���� �ʱ�ȭ

 

        queue->buffer[i].priority = priority_arr[i]; // �켱������ 1~���μ��� ������������ �ʱ�ȭ

 

        queue->buffer[i].period = ((rand() % 4 +1) * Maxperiod) /4;   //25, 50, 75, 100

 

        queue->buffer[i].IO_execution_time = 0;

 

        queue->buffer[i].IO_finish_time = 0;

 

        queue->buffer[i].IO_start_point = 0;

 

        queue->buffer[i].aging = 0;

 

        if(rand() % IOfrequency == 0 && queue->buffer[i].remain_time>1){  //IO_FREQ�ÿ� �ѹ� �� IO���� �Ҵ�

 

            queue->buffer[i].IO_remain_time = rand() % MaxIOtime +1;  //1 ~ 5���� ���� remain time ���Ƿ� �������� 

 

            queue->buffer[i].IO_start_time = rand() % queue->buffer[i].IO_remain_time; //IO���۽ð��� ������ �ð����� �����ð����� �۰� 

 

            if (queue->buffer[i].IO_start_time == 0 || queue->buffer[i].IO_start_time >= queue->buffer[i].burst_time){//io strat time�� cpu burst���� ũ�ų�, io start time�� 0�̸�, io start time�� 1�� ���������!~ 

 

                queue->buffer[i].IO_start_time = 1;

 

                }

 

            if (queue->buffer[i].IO_start_time == 1 && queue->buffer[i].burst_time == 1){//���߿� �߰��Ѱǵ� ��¥ ���� ���� ������� cpu burst�� io start time�� 1�ΰ� �� 

 

                queue->buffer[i].IO_start_time == 0;

 

                queue->buffer[i].IO_remain_time == 0;

 

                }

 

            }

 

            queue->count = queue->count+1;

 

            queue->rear = queue->rear+1;//�ϳ� ���鶧���� count, rear �ϳ��� ���� 

 

    }

 

    queue->rear = queue->rear%queue->size;

 

}

 

 

 

void print_Process(Queue *queue){

 

    printf("\nProcess information\n");

 

    printf("| PID |Arrival time|Finish time|Waiting time|Burst time|Executuin time|Remaining time|IO start time|IO execution time|IO remain time|Priority|Period|\n");

    int i = 0;

    for (i = queue->front+1; i < (i % queue->size)!=((queue->rear+1) % queue->size); i++){

 

        printf("|  %d  |    %d       |     %d      |    %d     |     %d     |     %d      |      %d       |       %d        |         %d       |     %d      |    %d    |   %d  |\n", 

 

            queue->buffer[i].pid, queue->buffer[i].arrival_time, queue->buffer[i].finish_time, queue->buffer[i].waiting_time, queue->buffer[i].burst_time, queue->buffer[i].execution_time,

 

            queue->buffer[i].remain_time, queue->buffer[i].IO_start_time, queue->buffer[i].IO_execution_time, queue->buffer[i].IO_remain_time, queue->buffer[i].priority, queue->buffer[i].period);

 

    }

 

    printf("\n");

 

}//process ó���� ��� ���

 

 

 

void print_running_Queue(PCB running_Queue){ // running ť ���

 

 

 

    printf("\nProcess\n| PID |Arrival time|Finish time|Waiting time|Burst time|Executuin time|Remaining time|IO start time|IO execution time|IO remain time|Priority|Period|\n\n");

 

        printf("|  %d  |    %d       |     %d      |    %d     |     %d     |     %d      |      %d       |       %d        |         %d       |     %d      |    %d    |   %d  |\n", 

 

            running_Queue.pid, running_Queue.arrival_time, running_Queue.finish_time, running_Queue.waiting_time, running_Queue.burst_time, running_Queue.execution_time, 

 

            running_Queue.remain_time, running_Queue.IO_start_time, running_Queue.IO_execution_time, running_Queue.IO_remain_time, running_Queue.priority, running_Queue.period);

 

        

 

}

 

 

 

void print_Queue(Queue *queue){ // ���ϴ� ť ���

 

    PCB temp;

 

    printf("\nProcess\n");

 

    printf("| PID |Arrival time|Finish time|Waiting time|Burst time|Executuin time|Remaining time|IO start time|IO execution time|IO remain time|Priority|Period|\n");

    int i = 0;

    for (i=(queue->front + 1); (i % queue->size)!=((queue->rear+1) % queue->size); i++){

 

        i = i % queue->size;

 

        temp = queue->buffer[i];

 

        printf("|  %d  |    %d       |     %d      |    %d     |     %d     |     %d      |      %d       |       %d        |         %d       |     %d      |    %d    |   %d  |\n",  

 

        temp.pid, temp.arrival_time, temp.finish_time, temp.waiting_time, temp.burst_time, temp.execution_time, temp.remain_time, temp.IO_start_time, temp.IO_execution_time, temp.IO_remain_time, temp.priority, temp.period);

 

    }

 

    printf("\n");

 

}

 

 

 

void init_Gantt(){

int i = 0;

    for(i=0; i<1000; i++)

 

        gantt[i] = 0;

 

}

 

 

 

void print_Gantt(int time){

 

    printf("\n----------Gantt Chart----------\n");

 

    if(gantt[0] == 0) printf("|");

    int i = 0;

    for(i = 0; i < time; i++){

    if(gantt[i] == 0){

    printf("\033[0;37m");//���� graphic������ ���� ���. 

    }

    if(gantt[i] == 1){

    printf("\033[0;31m");

    }

    if(gantt[i] == 2){

    printf("\033[0;32m");

    }

    if(gantt[i] == 3){

    printf("\033[0;33m");

    }

    if(gantt[i] == 4){

    printf("\033[0;34m");

    }

    if(gantt[i] == 5){

    printf("\033[0;35m");

    }

    if(gantt[i] == 6){

    printf("\033[0;36m");

    }

    if(gantt[i] == 7){

    printf("\033[0;38m");

    }

    if(gantt[i] == 8){

    printf("\033[0;39m");

    }

    if(gantt[i] == 9){

    printf("\033[0;31m");

    }

    if(gantt[i] == 10){

    printf("\033[0;30m");

    }

        if(gantt[i] != gantt[i-1]){

 

            if(gantt[i] >= 10) printf("|%d|", gantt[i]);//process�� max�� ��� ������ 

 

            else printf("|%d", gantt[i]);//������ �ƴϸ� �ٳ����������ϱ� �������� | x 

 

        }

 

        else{

 

            if(gantt[i] >= 10) printf("%d|", gantt[i]);

 

            else printf("%d", gantt[i]);

 

        }

 printf("\033[0m");

    }

    printf("\033[0m");

    printf("|\n");

 

} 

 

void init_result(){

int i = 0;

    for(i= 0; i<15; i++){

 

        result[i].sum_of_waiting_time = 0;

 

        result[i].sum_of_burst_time = 0;

 

        result[i].sum_of_turnaround_time = 0;

 

    }

 

}

 

 

 

void print_algorithm(int a){

 

    if(a == 0)

 

        printf("|         FCFS         |");

 

    else if(a == 1)

 

        printf("|   Nonpreemptive SJF  |");

 

    else if(a == 2)

 

        printf("|Nonpreemptive Priority|");

 

    else if(a == 3)

 

        printf("|      Round Robin     |");

 

    else if(a == 4)

 

        printf("|    Preemptive SJF    |");

 

    else if(a == 5)

 

        printf("|  Preemptive Priority |");

 

    else if(a == 6)

 

        printf("|  Pre_aging Priority  |");

 

    else if(a == 7)

 

        printf("|   Multilevel Queue   |");

 

    else if(a == 8)

 

        printf("|     Rate Monotonic   |");

 

    else if(a == 9)

 

        printf("|   Earliest Deadline  |");

 

    else if(a == 10)

    printf("|        Lottery       |");

}

 

 

 

void print_all_result(){

 

    printf("\n*****Summary of all Results*****\n");

 

    printf("----------------------------------------------------------------------------\n");

 

    printf("|      Algorithm       |  Process Num  |  Context_Switch  |\n");

 

    printf("----------------------------------------------------------------------------\n");

int i = 0;

        for(i=0; i<11; i++){    

 

            print_algorithm(i);

 

            printf("        %d         |         %d         | \n",result[i].process, result[i].context_switch);

 

        }

 

    printf("----------------------------------------------------------------------------\n");

 

    printf("|      Algorithm       | Sum of Waiting Time  | Average Waiting Time |\n");

 

    printf("----------------------------------------------------------------------------\n");

 

        for(i=0; i<11; i++){    

 

            print_algorithm(i);

 

            printf("         %d          |        %.3f        |\n",result[i].sum_of_waiting_time, result[i].ave_of_waiting_time);

 

        }

 

    printf("----------------------------------------------------------------------------\n");

 

    printf("      Algorithm       |Sum of Turnaround Time|Average Turnaround Time|\n");

 

    printf("----------------------------------------------------------------------------\n");

 

        for(i=0; i<11; i++){    

 

            print_algorithm(i);

 

            printf("         %d          |        %.3f        |\n",result[i].sum_of_turnaround_time, result[i].ave_of_turnaround_time);

 

        }

 

}

 

 

 

 

 

void FCFS(Queue *queue, int num){

 

    if(num==1){    

 

    printf("\n-----FCFS Algorithm-----\n");

 

}

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count = 0;

 

    init_Gantt();

 

    Queue FCFS;

 

    init_Queue(&FCFS, queue->size-1);

 

    Copy_Queue(queue, &FCFS);//FCFS����� �ʸ�ȭ�ϰ� �� 

 

        

 

    Queue ready_Queue;

 

    Queue waiting_Queue;

 

    Queue terminated_Queue;

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);//������ ť ����� �ʱ� 

 

    PCB running_Queue;

 

    running_Queue = init_PCB();//running queue����� �ʱ�ȭ(CPU���� ���ư��°� �ϳ��ϱ� PCB�� ����) 

 

//    print_Process(&running_Queue);

 

    sort_by_arrive(&FCFS);

 

    while(Queue_full_check(&terminated_Queue) != 1){


        if(num == 1) sleep(DELAY); 

 


int i = 0;

        for(i=FCFS.front+1; FCFS.buffer[i].pid!=0; i++){ //TIME�� ������ �� ready_Q�� ����

 

            if(FCFS.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&FCFS));

 

                dequeue(&FCFS);//�����ð��� �����ϸ� ����ť�� �־��ش� 

 

            }    

 

        };

 

    //    sort_by_arrive(&ready_Queue);    

 

    

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� ������ ��, �װ��� �����ð��� ������ terminated queu��, ������ ready queue�κ� 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//�̰Ŵ� io�۾� �߻��� �� waiting time ���ϱ� ���ؼ�. io�����ð�-�����ѽð� ���� �� �ð���ŭ �����ϹǷ�!!waiting queue�� �ִ°� waiting time���ԾȵǴϱ� 

 

                //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//�����ϰ��ִ� ���μ����� ������ 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }            

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid != 0){

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch += 1;

 

        }    

 

        if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

 

 

    

 

        print_Gantt(TIME+1);

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

}

 

 

 

        TIME += 1;    

 

 

 

    }

 

    

 

        //������� -�ٽú����ҵ� 

 

        //������ʹ� result�� �����ϴ� ���� 

    int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    result[0].process = queue->size-1;

 

    result[0].context_switch = Context_Switch;

int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[0].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[0].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[0].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[0].ave_of_waiting_time = (float)result[0].sum_of_waiting_time / result[0].process;

 

    result[0].ave_of_burst_time = (float)result[0].sum_of_burst_time / result[0].process;

 

    result[0].ave_of_turnaround_time = (float)result[0].sum_of_turnaround_time / result[0].process;

 

    result[0].IO_count = IO_count;

 

}

 

 

 

 

 

void Nonpreemption_SJF(Queue *queue, int num){//����� ������ ���� ���������� �����ߴٰ� �����ð����� �񱳸� ����!! 

 

if(num==1){    

 

    printf("\n----Nonpreemption SJF----\n");

 

}

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count = 0;

 

    

 

    init_Gantt();

 

    

 

    Queue Non_SJF;

 

    init_Queue(&Non_SJF, queue->size-1);

 

    Copy_Queue(queue, &Non_SJF);

 

    

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

    

 

    

 

    sort_by_arrive(&Non_SJF); // SJF�� �ִ� ���μ������� �����ð� ������ �����Ѵ�

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

        int i = 0;

 

        for(i=Non_SJF.front+1; Non_SJF.buffer[i].pid != 0; i++){ //�����ð��� ���������� TIME�� �����ϸ� READY QUEUE 

 

            if(Non_SJF.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Non_SJF));

 

                dequeue(&Non_SJF);

 

            }

 

        }

 

     if(num == 1) sleep(DELAY);

 

      

 

      

 

    if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� ������ ��, �װ��� �����ð��� ������ terminated queu��, ������ ready queue�κ� 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//�̰Ŵ� io�۾� �߻��� �� waiting time ���ϱ� ���ؼ�. io�����ð�-�����ѽð� ���� �� �ð���ŭ �����ϹǷ�!!waiting queue�� �ִ°� waiting time���ԾȵǴϱ� 

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_remain(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//�����ϰ��ִ� ���μ����� ������ 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }            

 

        

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch+=1;

 

        }

 

        

 

        if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

    }

 

        TIME += 1;    

 

    }

 

 

 

        int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    

 

    result[1].process = queue->size-1;

 

    result[1].context_switch = Context_Switch;

 

int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[1].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[1].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[1].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[1].ave_of_waiting_time = (float)result[1].sum_of_waiting_time / result[1].process;

 

    result[1].ave_of_burst_time = (float)result[1].sum_of_burst_time / result[1].process;

 

    result[1].ave_of_turnaround_time = (float)result[1].sum_of_turnaround_time / result[1].process;

 

    result[1].IO_count = IO_count;

 

}

 

 

 

 

 

void Nonpreemption_Priority(Queue *queue, int num){//����� ������ ���� ���������� �����ߴٰ� �����ð����� �񱳸� ����!! 

 

if(num==1){    

 

    printf("\n----Nonpreemption Priority----\n");

 

}

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count = 0;

 

    

 

    init_Gantt();

 

    

 

    Queue Non_Priority;

 

    init_Queue(&Non_Priority, queue->size-1);

 

    Copy_Queue(queue, &Non_Priority);

 

    

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

    

 

    

 

    sort_by_arrive(&Non_Priority); // SJF�� �ִ� ���μ������� �����ð� ������ �����Ѵ�

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

        int i = 0;

 

        for(i=Non_Priority.front+1; Non_Priority.buffer[i].pid != 0; i++){ //�����ð��� ���������� TIME�� �����ϸ� READY QUEUE 

 

            if(Non_Priority.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Non_Priority));

 

                dequeue(&Non_Priority);

 

            }

 

        }

 

            sort_by_priority(&ready_Queue);    

 

      if(num == 1) sleep(DELAY);

 

      

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� ������ ��, �װ��� �����ð��� ������ terminated queu��, ������ ready queue�κ� 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//�̰Ŵ� io�۾� �߻��� �� waiting time ���ϱ� ���ؼ�. io�����ð�-�����ѽð� ���� �� �ð���ŭ �����ϹǷ�!!waiting queue�� �ִ°� waiting time���ԾȵǴϱ� 

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_priority(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//�����ϰ��ִ� ���μ����� ������ 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }    

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch+=1;

 

        }

 

        

 

        if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

    

 

        int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    

 

    if(num==1){

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    

 

    result[2].process = queue->size-1;

 

    result[2].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[2].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[2].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[2].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[2].ave_of_waiting_time = (float)result[2].sum_of_waiting_time / result[2].process;

 

    result[2].ave_of_burst_time = (float)result[2].sum_of_burst_time / result[2].process;

 

    result[2].ave_of_turnaround_time = (float)result[2].sum_of_turnaround_time / result[2].process;

 

    result[2].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

//�׷��� ����κ��� ������ �𸣰ڴµ� IO�۾��� �ִ°Ŵ� ������ ����..���� ���� ���!! 

 

void Round_Robin(Queue *queue, int num){//����� ������ ���� ���������� �����ߴٰ� �����ð����� �񱳸� ����!! 

 

if(num==1){    

 

    printf("\n----Round Robin----\n");

 

}

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count = 0;

 

    int time_quantum = 3;

 

    

 

    init_Gantt();

 

    

 

    Queue Round_Robin;

 

    init_Queue(&Round_Robin, queue->size-1);

 

    Copy_Queue(queue, &Round_Robin);

 

    

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);

 

    

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

    

 

    

 

    sort_by_arrive(&Round_Robin); // SJF�� �ִ� ���μ������� �����ð� ������ �����Ѵ�

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

        int i = 0;

 

        for(i=Round_Robin.front+1; Round_Robin.buffer[i].pid != 0; i++){ //�����ð��� ���������� TIME�� �����ϸ� READY QUEUE 

 

            if(Round_Robin.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Round_Robin));

 

                dequeue(&Round_Robin);

 

            }

 

        }

 

            

 

    //  if(num == 1) sleep(DELAY);

 

      

 

          if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� ������ ��, �װ��� �����ð��� ������ terminated queu��, ������ ready queue�κ� 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//�̰Ŵ� io�۾� �߻��� �� waiting time ���ϱ� ���ؼ�. io�����ð�-�����ѽð� ���� �� �ð���ŭ �����ϹǷ�!!waiting queue�� �ִ°� waiting time���ԾȵǴϱ� 

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }    

 

        }

 

 

 

    

 

        if(running_Queue.pid !=0){ //running queue�� process�� �����ϸ� �����ð� �ϳ��� ���̰�, ����ð� �ϳ��� �ø� 

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1; 

 

            

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){ //remain time�� ����, io remain time�� ������, ����ť�κ��� 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue,running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                running_Queue = init_PCB();         //run_Q�� �ʱ�ȭ

 

            }

 

            

 

            if(running_Queue.pid != 0 && ((running_Queue.execution_time) % time_quantum) == 0 && running_Queue.IO_start_time == 0){//IO�۾��Ұ��̾����� �׳� �״�� �� �� 

 

                if(running_Queue.execution_time > 0){

 

                    enqueue(&ready_Queue, running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }    

 

            

 

        //�̺κо������. IO start time�� 3�ǹ���� ���� ������ �Ȼ���µ� IO start time�� 1�̳� 2�� ���� IO�۾� ���� �ϴ� excution time�� �����ϰ� �Ǿ

 

        //�ٸ� ���� ���ͼ�, io start time�� 1�϶��� 2�� �� temp ������ ���� �� ���� �ߴ� excetion time��ŭ ���ְ� �װ� 3��ŭ �����ϸ� �Ű� �� 

 

            if(running_Queue.pid != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time < 3){  

 

                if(running_Queue.execution_time > 0){

 

                    int temp1, temp2;

 

                    if(running_Queue.IO_start_time % 3 == 1 && running_Queue.IO_remain_time == 0){

 

                        temp1 = running_Queue.execution_time-1;

 

                        if(temp1%3==0){

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }                    

 

                    if(running_Queue.IO_start_time % 3 == 2 && running_Queue.IO_remain_time == 0){

 

                        temp2 = running_Queue.execution_time-2;

 

                        if(temp2%3==0){

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }

 

                    

 

                }

 

            }

 

            

 

            //�̺κо������2ź.. IO start time�� 3�ǹ���� ���� ������ �Ȼ���µ� IO start time�� 4�̳� 5�� ���� IO�۾� ���� �ϴ� excution time�� �����ϰ� �Ǿ

 

        //�ٸ� ���� ���ͼ�, io start time�� 4�϶��� 5�� �� temp ������ ���� �� ���� �ߴ� excetion time��ŭ ���ְ� �װ� 3��ŭ �����ϸ� �Ű� ��     

 

            if(running_Queue.pid != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time > 3){

 

                if(running_Queue.execution_time == 3){

 

                        enqueue(&ready_Queue, running_Queue);

 

                        running_Queue = init_PCB();

 

                }

 

                

 

                

 

                if(running_Queue.execution_time > 0){

 

                    int temp1, temp2;

 

                    if(running_Queue.IO_start_time % 3 == 1 && running_Queue.IO_remain_time == 0){

 

                        temp1 = running_Queue.execution_time - 4;

 

                        if(temp1%3==0){

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }                    

 

                    if(running_Queue.IO_start_time % 3 == 2 && running_Queue.IO_remain_time == 0){

 

                        temp2 = running_Queue.execution_time - 5;

 

                        if(temp2%3==0){

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }

 

                    

 

                }

 

            }

 

            

 

        if(running_Queue.pid != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time == 3){

 

            int temp = running_Queue.execution_time - 3;

 

            if(temp%3 == 0){

 

                enqueue(&ready_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

                }

 

            }        

 

        }

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch+=1;

 

        }

 

        

 

        if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

    

 

    int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    

 

    if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    

 

    result[3].process = queue->size-1;

 

    result[3].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[3].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[3].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[3].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[3].ave_of_waiting_time = (float)result[3].sum_of_waiting_time / result[3].process;

 

    result[3].ave_of_burst_time = (float)result[3].sum_of_burst_time / result[3].process;

 

    result[3].ave_of_turnaround_time = (float)result[3].sum_of_turnaround_time / result[3].process;

 

    result[3].IO_count = IO_count;

 

}

 

 

 

 

 

void Preemption_SJF(Queue *queue, int num){//����� ������ ���� ���������� �����ߴٰ� �����ð����� �񱳸� ����!! 

 

if(num==1){    

 

    printf("\n----Preemption SJF----\n");

 

}

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count = 0;

 

    

 

    init_Gantt();

 

    

 

    Queue Preemption_SJF;

 

    init_Queue(&Preemption_SJF, queue->size-1);

 

    Copy_Queue(queue, &Preemption_SJF);

 

    

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);

 

    

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

    

 

    

 

    sort_by_arrive(&Preemption_SJF); // SJF�� �ִ� ���μ������� �����ð� ������ �����Ѵ�

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

        int i = 0;

 

        for(i=Preemption_SJF.front+1; Preemption_SJF.buffer[i].pid != 0; i++){ //�����ð��� ���������� TIME�� �����ϸ� READY QUEUE 

 

            if(Preemption_SJF.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Preemption_SJF));

 

                dequeue(&Preemption_SJF);

 

            }

 

        }

 

            

 

        sort_by_remain(&ready_Queue);

 

            

 

      if(num == 1 )sleep(DELAY);

 

      

 

      

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� ������ ��, �װ��� �����ð��� ������ terminated queu��, ������ ready queue�κ� 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//�̰Ŵ� io�۾� �߻��� �� waiting time ���ϱ� ���ؼ�. io�����ð�-�����ѽð� ���� �� �ð���ŭ �����ϹǷ�!!waiting queue�� �ִ°� waiting time���ԾȵǴϱ� 

 

                //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_remain(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//�����ϰ��ִ� ���μ����� ������ 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }            

 

        
     if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //���� running_q�� ���� �ְ�, ready_Q�� �ִٸ�

 

                if(running_Queue.remain_time > get_Queue_head(&ready_Queue).remain_time){ 


                    enqueue(&ready_Queue,running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }
 

 

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch+=1;

 

        }

 

        

 

        if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

    

        int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    

 

    

 

    if(num==1){

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

 

 

    result[4].process = queue->size-1;

 

    result[4].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[4].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[4].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[4].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[4].ave_of_waiting_time = (float)result[4].sum_of_waiting_time / result[4].process;

 

    result[4].ave_of_burst_time = (float)result[4].sum_of_burst_time / result[4].process;

 

    result[4].ave_of_turnaround_time = (float)result[4].sum_of_turnaround_time / result[4].process;

 

    result[4].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

void Preemption_Priority(Queue *queue, int num){//����� ������ ���� ���������� �����ߴٰ� �����ð����� �񱳸� ����!! 

 

if(num==1){    

 

    printf("\n----Preemption Priority----\n");

 

}

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count = 0;

 

    

 

    init_Gantt();

 

    

 

    Queue Preemption_Priority;

 

    init_Queue(&Preemption_Priority, queue->size-1);

 

    Copy_Queue(queue, &Preemption_Priority);

 

    

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

    

 

    

 

    sort_by_arrive(&Preemption_Priority); // SJF�� �ִ� ���μ������� �����ð� ������ �����Ѵ�

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

        int i = 0;

 

        for(i=Preemption_Priority.front+1; Preemption_Priority.buffer[i].pid != 0; i++){ //�����ð��� ���������� TIME�� �����ϸ� READY QUEUE 

 

            if(Preemption_Priority.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Preemption_Priority));

 

                dequeue(&Preemption_Priority);

 

            }

 

        }

 

            sort_by_priority(&ready_Queue);    

 

      if(num == 1) sleep(DELAY);

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� ������ ��, �װ��� �����ð��� ������ terminated queu��, ������ ready queue�κ� 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//�̰Ŵ� io�۾� �߻��� �� waiting time ���ϱ� ���ؼ�. io�����ð�-�����ѽð� ���� �� �ð���ŭ �����ϹǷ�!!waiting queue�� �ִ°� waiting time���ԾȵǴϱ� 

 

                //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

        

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_priority(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//�����ϰ��ִ� ���μ����� ������ 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }            

 

        

 

 

 

        if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //���� running_q�� ���� �ְ�, ready_Q�� �ִٸ�

 

                if(running_Queue.priority > get_Queue_head(&ready_Queue).priority){ 


                    enqueue(&ready_Queue,running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch+=1;

 

        }

 

        

 

    

 

        if(num==1){    

 

        

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

 

    int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    

 

    if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    

 

    result[5].process = queue->size-1;

 

    result[5].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[5].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[5].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[5].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[5].ave_of_waiting_time = (float)result[5].sum_of_waiting_time / result[5].process;

 

    result[5].ave_of_burst_time = (float)result[5].sum_of_burst_time / result[5].process;

 

    result[5].ave_of_turnaround_time = (float)result[5].sum_of_turnaround_time / result[5].process;

 

    result[5].IO_count = IO_count;

 

}

 

 

 

 

 

void Preemption_Priority_aging(Queue *queue, int num){//����� ������ ���� ���������� �����ߴٰ� �����ð����� �񱳸� ����!! 

 

if(num==1){    

 

    printf("\n----Nonpreemption Priority----\n");

 

}

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count = 0;

 

    

 

    init_Gantt();

 

    

 

    Queue Non_Priority;

 

    init_Queue(&Non_Priority, queue->size-1);

 

    Copy_Queue(queue, &Non_Priority);

 

    

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

    

 

    

 

    sort_by_arrive(&Non_Priority); // SJF�� �ִ� ���μ������� �����ð� ������ �����Ѵ�

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

        int i = 0;

 

        for(i=Non_Priority.front+1; Non_Priority.buffer[i].pid != 0; i++){ //�����ð��� ���������� TIME�� �����ϸ� READY QUEUE 

 

            if(Non_Priority.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

            //    Non_Priority.buffer[i].aging = TIME;

 

                enqueue(&ready_Queue, get_Queue_head(&Non_Priority));

 

                dequeue(&Non_Priority);

 

            }

 

        }

 

            sort_by_priority(&ready_Queue);    

 

    if(num == 1) sleep(DELAY);

 

      

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� ������ ��, �װ��� �����ð��� ������ terminated queu��, ������ ready queue�κ� 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//�̰Ŵ� io�۾� �߻��� �� waiting time ���ϱ� ���ؼ�. io�����ð�-�����ѽð� ���� �� �ð���ŭ �����ϹǷ�!!waiting queue�� �ִ°� waiting time���ԾȵǴϱ� 

 

                //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                    waiting_Queue.buffer[waiting_Queue.front+1].aging = TIME;

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

            //        Non_Priority.buffer[i].aging = TIME;

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_priority(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//�����ϰ��ִ� ���μ����� ������ 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }    

 

 

 

 

 

            if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //���� running_q�� ���� �ְ�, ready_Q�� �ִٸ�

 

                if(running_Queue.priority > get_Queue_head(&ready_Queue).priority){ 

 

                running_Queue.aging = TIME;

 

                    enqueue(&ready_Queue,running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }

 

 

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch+=1;

 

        }

 

        

 

        for(i=(ready_Queue.front + 1) % ready_Queue.size; (i % ready_Queue.size)!=((ready_Queue.rear+1) % ready_Queue.size); i++){

 

            if(ready_Queue.buffer[i].burst_time != ready_Queue.buffer[i].remain_time && ready_Queue.buffer[i].aging != TIME && (TIME - ready_Queue.buffer[i].aging) % 8 == 0){

 

                ready_Queue.buffer[i].priority -= 2;

 

            }

 

            

 

            if(ready_Queue.buffer[i].execution_time == 0 && ready_Queue.buffer[i].arrival_time != TIME && ((TIME - ready_Queue.buffer[i].arrival_time)%8 == 0)){

 

                ready_Queue.buffer[i].priority -= 2;

 

            }

 

                

 

        }

 

        

 

        if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

    }

 

        TIME += 1;    

 

    }

 

    

 

        int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    

 

    result[6].process = queue->size-1;

 

    result[6].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[6].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[6].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[6].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[6].ave_of_waiting_time = (float)result[6].sum_of_waiting_time / result[6].process;

 

    result[6].ave_of_burst_time = (float)result[6].sum_of_burst_time / result[6].process;

 

    result[6].ave_of_turnaround_time = (float)result[6].sum_of_turnaround_time / result[6].process;

 

    result[6].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

 

 

 

 

 

 

void Multilevel_Queue(Queue *queue, int num){

 

    if(num==1){    

 

        printf("\n----Multilevel Queue----\n");

 

}

 

    int time_quantum = 3;

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count =0;

 

 

int i = 0;

    for(i=0; i<1000; i++){

 

        gantt[i] = 0;

 

 

 

    }

 

    

 

    Queue RR;    //������ ���μ������� ������ queue�� �޾Ƽ� �����Ѵ�.

 

    Queue FCFS;

 

    init_Queue(&RR, queue->size-1);

 

//    Copy_Queue(queue, &RR);

 

    init_Queue(&FCFS, queue->size-1);

 

    

 

 

 

    

 

    for(i=queue->front+1; i<=queue->rear; i++){

 

        if(queue->buffer[i].pid % 3 != 0) 

 

            enqueue(&RR, queue->buffer[i]);

 

        if(queue->buffer[i].pid % 3 == 0 && queue->buffer[i].pid != 0)

 

            enqueue(&FCFS, queue->buffer[i]);

 

    }

 

 

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    Queue ready_Queue2;

 

 

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&ready_Queue2, queue->size-1);

 

//    init_Queue(&waiting_Queue2,3*queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);

 

 

 

 

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

//    running_Queue2 = init_PCB();

 

 

 

    sort_by_arrive(&RR); 

 

    sort_by_arrive(&FCFS);

 

    if(num==1){

 

    printf("[RR]\n");

 

    print_Queue(&RR);

 

    printf("[FCFS]\n");

 

    print_Queue(&FCFS);

 

}

 

    

 

//waiting queue -> ready queue��

 

//running queue -> readyq2??termi??�� ���� 

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�    

 

    if(num == 1) sleep(DELAY);    

 

        //RR���� arrive_time�� ���� ready_Q�� ���� 

 

        for(i=RR.front+1; RR.buffer[i].pid!=0; i++){ 

 

            if(RR.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&RR));

 

                dequeue(&RR);

 

            }

 

        }

 

        //FCFS���� arrive_time�� ���� ready_Q2�� ���� 

 

        for(i=FCFS.front+1; FCFS.buffer[i].pid!=0; i++){ 

 

            if(FCFS.buffer[i].arrival_time == TIME){    

 

                enqueue(&ready_Queue2, get_Queue_head(&FCFS));

 

                dequeue(&FCFS);

 

            

 

        }

 

        }

 

        

 

        

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;

 

    

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0 && waiting_Queue.buffer[waiting_Queue.front+1].pid % 3 == 0 && waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){

 

                waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;

 

                enqueue(&ready_Queue2, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                dequeue(&waiting_Queue);

 

            }

 

            if((waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time == 0) && waiting_Queue.buffer[waiting_Queue.front+1].pid % 3 != 0 && waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){

 

                waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;

 

                enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                dequeue(&waiting_Queue);

 

            }

 

 

 

        }            

 

                

 

                

 

                                

 

        if(running_Queue.pid !=0){ //running queue�� process�� �����ϸ� �����ð� �ϳ��� ���̰�, ����ð� �ϳ��� �ø� 

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;     

 

                

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//�����ϰ��ִ� ���μ����� ������ 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.pid % 3 == 0 && get_Queue_head(&ready_Queue).pid != 0 && running_Queue.pid != 0){

 

                enqueue(&ready_Queue2, running_Queue);

 

                running_Queue = init_PCB();

 

                running_Queue = get_Queue_head(&ready_Queue);

 

                dequeue(&ready_Queue);

 

            }

 

            

 

            if(running_Queue.pid % 3 == 0 && get_Queue_head(&ready_Queue).pid != 0 && running_Queue.pid != 0 && get_Queue_head(&ready_Queue2).pid == 0){

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.pid % 3 == 0 && get_Queue_head(&ready_Queue).pid == 0 && running_Queue.pid != 0 && get_Queue_head(&ready_Queue2).pid != 0 && running_Queue.remain_time == 0){

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

                running_Queue = get_Queue_head(&ready_Queue2);

 

                dequeue(&ready_Queue2);

 

            }//�������� 

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                running_Queue = init_PCB();         //run_Q�� �ʱ�ȭ

 

            }

 

            

 

            if(running_Queue.pid%3 != 0 && ((running_Queue.execution_time) % time_quantum) == 0 && running_Queue.IO_start_time == 0){//IO�۾��Ұ��̾����� �׳� �״�� �� �� 

 

                if(running_Queue.execution_time > 0){

 

                    if(running_Queue.pid % 3 != 0){    

 

                        if(running_Queue.remain_time == 0){

 

                            enqueue(&terminated_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                        else{

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }

 

            //        if(running_Queue.pid % 3 == 0 && running_Queue.pid != 0){

 

            //            enqueue(&ready_Queue2, running_Queue);

 

            //            running_Queue = init_PCB();

 

            //        }

 

                }

 

            }

 

            

 

 

 

    

 

        //�̺κо������. IO start time�� 3�ǹ���� ���� ������ �Ȼ���µ� IO start time�� 1�̳� 2�� ���� IO�۾� ���� �ϴ� excution time�� �����ϰ� �Ǿ

 

        //�ٸ� ���� ���ͼ�, io start time�� 1�϶��� 2�� �� temp ������ ���� �� ���� �ߴ� excetion time��ŭ ���ְ� �װ� 3��ŭ �����ϸ� �Ű� �� 

 

            if(running_Queue.pid %3 != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time < 3){  

 

                if(running_Queue.execution_time > 0){

 

                    int temp1, temp2;

 

                    if(running_Queue.IO_start_time % 3 == 1 && running_Queue.IO_remain_time == 0){

 

                        temp1 = running_Queue.execution_time-1;

 

                        if(temp1%3==0){

 

                                

 

                                    if(running_Queue.remain_time == 0){

 

                            enqueue(&terminated_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                        else{

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                        

 

                        }

 

                    }                    

 

                    if(running_Queue.IO_start_time % 3 == 2 && running_Queue.IO_remain_time == 0){

 

                        temp2 = running_Queue.execution_time-2;

 

                        if(temp2%3==0){

 

                            

 

                                if(running_Queue.remain_time == 0){

 

                            enqueue(&terminated_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                        else{

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                            

 

                        }

 

                    }

 

                    

 

                }

 

            }

 

            

 

            //�̺κо������2ź.. IO start time�� 3�ǹ���� ���� ������ �Ȼ���µ� IO start time�� 4�̳� 5�� ���� IO�۾� ���� �ϴ� excution time�� �����ϰ� �Ǿ

 

          //�ٸ� ���� ���ͼ�, io start time�� 4�϶��� 5�� �� temp ������ ���� �� ���� �ߴ� excetion time��ŭ ���ְ� �װ� 3��ŭ �����ϸ� �Ű� ��     

 

            if(running_Queue.pid%3 != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time > 3){

 

                if(running_Queue.execution_time == 3){

 

                    

 

                        if(running_Queue.remain_time == 0){

 

                            enqueue(&terminated_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                        else{

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

            

 

                }

 

                

 

                

 

                if(running_Queue.execution_time > 0){

 

                    int temp1, temp2;

 

                    if(running_Queue.IO_start_time % 3 == 1 && running_Queue.IO_remain_time == 0){

 

                        temp1 = running_Queue.execution_time - 4;

 

                        if(temp1%3==0){

 

                            

 

                    if(running_Queue.remain_time == 0){

 

                            enqueue(&terminated_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                        else{

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

        

 

                        }

 

                    }                    

 

                    if(running_Queue.IO_start_time % 3 == 2 && running_Queue.IO_remain_time == 0){

 

                        temp2 = running_Queue.execution_time - 5;

 

                        if(temp2%3==0){

 

                    

 

                        if(running_Queue.remain_time == 0){

 

                            enqueue(&terminated_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                        else{

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                

 

                        }

 

                    }

 

                    

 

                }

 

            }

 

            

 

        if(running_Queue.pid%3 != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time == 3){

 

            int temp = running_Queue.execution_time - 3;

 

            if(temp%3 == 0){

 

            

 

                    if(running_Queue.remain_time == 0){

 

                            enqueue(&terminated_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                        else{

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

        

 

                }     

 

            }

 

                        

 

        }

 

 

 

            if(running_Queue.pid == 0){

 

                if(get_Queue_head(&ready_Queue).pid != 0){

 

                    running_Queue = get_Queue_head(&ready_Queue);

 

                    dequeue(&ready_Queue);

 

                    Context_Switch+=1;

 

                }

 

                

 

                else if(get_Queue_head(&ready_Queue).pid == 0 && get_Queue_head(&ready_Queue2).pid != 0){

 

                    

 

                    running_Queue = get_Queue_head(&ready_Queue2);

 

                    dequeue(&ready_Queue2);

 

                    Context_Switch+=1;

 

                }

 

                            

 

            }

 

        

 

        gantt[TIME+1] = running_Queue.pid;    

 

        if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Ready Queue2----\n");

 

        print_Queue(&ready_Queue2);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

    }

 

        TIME += 1;

 

    }

 

    

int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    result[7].process = queue->size-1;

 

    result[7].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[7].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[7].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[7].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[7].ave_of_waiting_time = (float)result[7].sum_of_waiting_time / result[7].process;

 

    result[7].ave_of_burst_time = (float)result[7].sum_of_burst_time / result[7].process;

 

    result[7].ave_of_turnaround_time = (float)result[7].sum_of_turnaround_time / result[7].process;

 

    result[7].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

void Rate_Monotonic(Queue *queue, int num){

 

    if(num==1){

 

        printf("\n----Rate Monotonic----\n");

 

    }

 

 

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count =0;

 

 

 

    init_Gantt();

 

    Queue Priority;    //������ ���μ������� ������ queue�� �޾Ƽ� �����Ѵ�.

 

    init_Queue(&Priority, queue->size-1);

 

    Copy_Queue(queue, &Priority); 

 

 

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    init_Queue(&ready_Queue, 5 * (queue->size-1));

 

    init_Queue(&waiting_Queue,5 * ( queue->size-1));

 

    init_Queue(&terminated_Queue, 5 *(queue->size-1));

 

 

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

    

int i = 0;

    for(i=Priority.front+1; Priority.buffer[i].pid!=0; i++){

 

        Priority.buffer[i].priority = Priority.buffer[i].period; 

 

        Priority.buffer[i].arrival_time = 0;

 

    }

 

    for(i=Priority.rear+1; i<Priority.size; i++)    Priority.buffer[i].pid = 0;

 

 

 

    sort_by_priority(&Priority);

 

    

 

    if(num==1){

 

    print_Queue(&Priority);

 

}

 

 

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

    if(num == 1) sleep(DELAY);

 

        if(TIME == 201)

 

            break;    

 

        for(i=(Priority.front+1); i != Priority.rear+1; i++){ //TIME�� ������ �� ready_Q�� ����

 

            if((TIME % Priority.buffer[i].period) == 0) {

 

            //    int cnt =0;

int j = 0;

                for(j=ready_Queue.front+1; j<=ready_Queue.rear; j++){

 

                    if(ready_Queue.buffer[j].pid == Priority.buffer[i].pid)

 

                        printf("Time [%d], [%d] deadline miss!!\n",TIME,Priority.buffer[i].pid);

 

                }

 

                Priority.buffer[i].arrival_time = TIME;

 

                enqueue(&ready_Queue, Priority.buffer[i]);

 

            }

 

        }

 

        sort_by_priority(&ready_Queue);

 

      

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� �����ٸ�, ready_q�� ������

 

                if (waiting_Queue.buffer[waiting_Queue.front+1].remain_time ==0){

 

                    waiting_Queue.buffer[waiting_Queue.front+1].finish_time = TIME;

 

                    enqueue(&terminated_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                    dequeue(&waiting_Queue);

 

                }

 

                else{

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                    dequeue(&waiting_Queue);

 

                    }

 

            }

 

        }

 

        sort_by_priority(&ready_Queue);    

 

        

 

        if(running_Queue.pid !=0){ //run_Q����, ready_q�� ����

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){ //no more burst and no more i/o

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue,running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                running_Queue = init_PCB();         //run_Q�� �ʱ�ȭ

 

            }

 

            if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //���� running_q�� ���� �ְ�, ready_Q�� �ִٸ�

 

                if(running_Queue.priority > get_Queue_head(&ready_Queue).priority){ //ready_Q�� �� shortest_job�̶�� ����

 

                    enqueue(&ready_Queue,running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }

 

        }

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch+=1;

 

        }

 

        gantt[TIME] = running_Queue.pid;

 

        

 

                if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

 

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

    }

 

        TIME += 1;    

 

    }

 

    

int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    

 

    if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

 

 

    result[8].process = queue->size-1;

 

    result[8].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[8].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[8].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[8].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[8].ave_of_waiting_time = (float)result[8].sum_of_waiting_time / result[8].process;

 

    result[8].ave_of_burst_time = (float)result[8].sum_of_burst_time / result[8].process;

 

    result[8].ave_of_turnaround_time = (float)result[8].sum_of_turnaround_time / result[8].process;

 

    result[8].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

 

 

 

 

void EDF(Queue *queue, int num){

 

    if(num==1){

 

            printf("\n----Earliest Deadline First----\n");

 

    }

 

    Context_Switch = 0;

 

    TIME = 0;

 

    int IO_count =0;

 

 

 

    init_Gantt();

 

    Queue Priority;    //������ ���μ������� ������ queue�� �޾Ƽ� �����Ѵ�.

 

    init_Queue(&Priority, queue->size-1);

 

    Copy_Queue(queue, &Priority); 

 

 

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

 

    init_Queue(&ready_Queue, 5 * (queue->size-1));

 

    init_Queue(&waiting_Queue,5 * ( queue->size-1));

 

    init_Queue(&terminated_Queue, 5 *(queue->size-1));

 

 

 

    PCB running_Queue;

 

    running_Queue = init_PCB();

 

    

int i = 0;

    for(i=Priority.front+1; Priority.buffer[i].pid!=0; i++){

 

        Priority.buffer[i].priority = Priority.buffer[i].period; 

 

        Priority.buffer[i].arrival_time = 0;

 

    }

 

    for(i=Priority.rear+1; i<Priority.size; i++)    Priority.buffer[i].pid = 0;

 

 

 

    sort_by_priority(&Priority);

 

    if(num==1){

 

    print_Queue(&Priority);

 

}

 

 

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

    if(num == 1) sleep(DELAY);

 

    for(i=(ready_Queue.front+1)%ready_Queue.size; i <= ready_Queue.rear; i++)

 

                ready_Queue.buffer[i].priority -= 1;

 

                running_Queue.priority -= 1;

 

            for(i=waiting_Queue.front+1; i <= waiting_Queue.rear; i++)

 

                waiting_Queue.buffer[i].priority -= 1;

 

    

 

    

 

        if(TIME == 201)

 

            break;    

 

        for(i=(Priority.front+1); i != Priority.rear+1; i++){ //TIME�� ������ �� ready_Q�� ����

 

            if((TIME % Priority.buffer[i].period) == 0) {

int j = 0;

                for(j=ready_Queue.front+1; j<=ready_Queue.rear; j++){

 

                    if(ready_Queue.buffer[j].pid == Priority.buffer[i].pid)

 

                        printf("Time [%d], [%d] deadline miss!!\n",TIME,Priority.buffer[i].pid);

 

                }

 

                Priority.buffer[i].arrival_time = TIME;

 

                enqueue(&ready_Queue, Priority.buffer[i]);

 

            }

 

        }

 

        sort_by_priority(&ready_Queue);

 

      

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� �����ٸ�, ready_q�� ������

 

                if (waiting_Queue.buffer[waiting_Queue.front+1].remain_time ==0){

 

                    waiting_Queue.buffer[waiting_Queue.front+1].finish_time = TIME;

 

                    enqueue(&terminated_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                    dequeue(&waiting_Queue);

 

                }

 

                else{

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                    dequeue(&waiting_Queue);

 

                    }

 

            }

 

        }

 

        sort_by_priority(&ready_Queue);    

 

        

 

        if(running_Queue.pid !=0){ //run_Q����, ready_q�� ����

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){ //no more burst and no more i/o

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue,running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                running_Queue = init_PCB();         //run_Q�� �ʱ�ȭ

 

            }

 

            if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //���� running_q�� ���� �ְ�, ready_Q�� �ִٸ�

 

                if(running_Queue.priority > get_Queue_head(&ready_Queue).priority){ //ready_Q�� �� shortest_job�̶�� ����

 

                    enqueue(&ready_Queue,running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }

 

        }

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

 

            running_Queue = get_Queue_head(&ready_Queue);

 

            dequeue(&ready_Queue);

 

            Context_Switch+=1;

 

        }

 

        gantt[TIME] = running_Queue.pid;

 

        

 

                if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

 

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

    }

 

        TIME += 1;    

 

    }

 

    

int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    

 

    if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    result[9].process = queue->size-1;

 

    result[9].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[9].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[9].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[9].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[9].ave_of_waiting_time = (float)result[9].sum_of_waiting_time / result[9].process;

 

    result[9].ave_of_burst_time = (float)result[9].sum_of_burst_time / result[9].process;

 

    result[9].ave_of_turnaround_time = (float)result[9].sum_of_turnaround_time / result[9].process;

 

    result[9].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

 

void Lottery(Queue *queue, int num){//����� ������ ���� ���������� �����ߴٰ� �����ð����� �񱳸� ����!! 

if(num==1){    

    printf("\n----Lottery----\n");

}

 

    Context_Switch = 0;

    TIME = 0;

    int IO_count = 0;

    int time_quantum = 3;  

 

    init_Gantt();

    

    Queue Round_Robin;

    init_Queue(&Round_Robin, queue->size-1);

    Copy_Queue(queue, &Round_Robin);   

 

    Queue ready_Queue, waiting_Queue, terminated_Queue;

    init_Queue(&ready_Queue, queue->size-1);

    init_Queue(&waiting_Queue, queue->size-1);

    init_Queue(&terminated_Queue, queue->size-1);

    

    PCB running_Queue;

    running_Queue = init_PCB();

    

    sort_by_arrive(&Round_Robin);

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q�� ������ ���� time�� 1�� �÷����� �ݺ�        

 

        int i = 0;

        for(i=Round_Robin.front+1; Round_Robin.buffer[i].pid != 0; i++){ //�����ð��� ���������� TIME�� �����ϸ� READY QUEUE 

            if(Round_Robin.buffer[i].arrival_time != TIME) 

                break;

            else{

                enqueue(&ready_Queue, get_Queue_head(&Round_Robin));

                dequeue(&Round_Robin);

            }

        }

          

    //  if(num == 1) sleep(DELAY);    

 

          if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q�� ���μ����� �����Ѵٸ� io_time�� 1���δ�.

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

        

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O ������� ������ ��, �װ��� �����ð��� ������ terminated queu��, ������ ready queue�κ� 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//�̰Ŵ� io�۾� �߻��� �� waiting time ���ϱ� ���ؼ�. io�����ð�-�����ѽð� ���� �� �ð���ŭ �����ϹǷ�!!waiting queue�� �ִ°� waiting time���ԾȵǴϱ� 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

                    dequeue(&waiting_Queue);

            }    

        }

    

 

        if(running_Queue.pid !=0){ //running queue�� process�� �����ϸ� �����ð� �ϳ��� ���̰�, ����ð� �ϳ��� �ø� 

            running_Queue.remain_time -= 1;

            running_Queue.execution_time += 1;       

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){ //remain time�� ����, io remain time�� ������, ����ť�κ��� 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue,running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //���� IO������ �����ؾ� �Ѵٸ�

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q�� �̵���Ų��.

 

                running_Queue = init_PCB();         //run_Q�� �ʱ�ȭ

 

            }

 

            

 

            if(running_Queue.pid != 0 && ((running_Queue.execution_time) % time_quantum) == 0 && running_Queue.IO_start_time == 0){//IO�۾��Ұ��̾����� �׳� �״�� �� �� 

 

                if(running_Queue.execution_time > 0){

 

                    enqueue(&ready_Queue, running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }    

 

            

 

        //�̺κо������. IO start time�� 3�ǹ���� ���� ������ �Ȼ���µ� IO start time�� 1�̳� 2�� ���� IO�۾� ���� �ϴ� excution time�� �����ϰ� �Ǿ

 

        //�ٸ� ���� ���ͼ�, io start time�� 1�϶��� 2�� �� temp ������ ���� �� ���� �ߴ� excetion time��ŭ ���ְ� �װ� 3��ŭ �����ϸ� �Ű� �� 

 

            if(running_Queue.pid != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time < 3){  

 

                if(running_Queue.execution_time > 0){

 

                    int temp1, temp2;

 

                    if(running_Queue.IO_start_time % 3 == 1 && running_Queue.IO_remain_time == 0){

 

                        temp1 = running_Queue.execution_time-1;

 

                        if(temp1%3==0){

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }                    

 

                    if(running_Queue.IO_start_time % 3 == 2 && running_Queue.IO_remain_time == 0){

 

                        temp2 = running_Queue.execution_time-2;

 

                        if(temp2%3==0){

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }

 

                    

 

                }

 

            }

 

            

 

            //�̺κо������2ź.. IO start time�� 3�ǹ���� ���� ������ �Ȼ���µ� IO start time�� 4�̳� 5�� ���� IO�۾� ���� �ϴ� excution time�� �����ϰ� �Ǿ

 

        //�ٸ� ���� ���ͼ�, io start time�� 4�϶��� 5�� �� temp ������ ���� �� ���� �ߴ� excetion time��ŭ ���ְ� �װ� 3��ŭ �����ϸ� �Ű� ��     

 

            if(running_Queue.pid != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time > 3){

 

                if(running_Queue.execution_time == 3){

 

                        enqueue(&ready_Queue, running_Queue);

 

                        running_Queue = init_PCB();

 

                }

 

                

 

                

 

                if(running_Queue.execution_time > 0){

 

                    int temp1, temp2;

 

                    if(running_Queue.IO_start_time % 3 == 1 && running_Queue.IO_remain_time == 0){

 

                        temp1 = running_Queue.execution_time - 4;

 

                        if(temp1%3==0){

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }                    

 

                    if(running_Queue.IO_start_time % 3 == 2 && running_Queue.IO_remain_time == 0){

 

                        temp2 = running_Queue.execution_time - 5;

 

                        if(temp2%3==0){

 

                            enqueue(&ready_Queue, running_Queue);

 

                            running_Queue = init_PCB();

 

                        }

 

                    }

 

                    

 

                }

 

            }

 

            

 

        if(running_Queue.pid != 0 && running_Queue.IO_start_time != 0 && running_Queue.IO_start_time == 3){

 

            int temp = running_Queue.execution_time - 3;

 

            if(temp%3 == 0){

 

                enqueue(&ready_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

                }

 

            }        

 

        }

 

 

 

        if(running_Queue.pid == 0 && get_Queue_head(&ready_Queue).pid!=0){ //run_Q empty, is ready_Q : ready->run

        if(num == 1) printf("\n��÷ START\n");

    sort_by_lottery(&ready_Queue);

            running_Queue = get_Queue_head(&ready_Queue);

            dequeue(&ready_Queue);

    if(num == 1) printf("��÷�� process�� PID : %d\n", running_Queue.pid);

            Context_Switch+=1;

 

        }

 

        

 

        if(num==1){    

 

        printf("\n----Ready Queue----\n");

 

        print_Queue(&ready_Queue);

 

        printf("\n----Running Queue----\n");

 

        print_running_Queue(running_Queue);

 

        printf("\n----Waiting Queue-----\n");

 

        print_Queue(&waiting_Queue);

 

        printf("\n----Terminated Queue----\n");

 

        print_Queue(&terminated_Queue);

 

        gantt[TIME] = running_Queue.pid;

 

        print_Gantt(TIME+1);

 

        printf("\n�ءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءءء�\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

    

 

    int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO�۾��� ������ �����ϰ� �̰͸� ����ϸ� ��@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io�۾��� �߻��ϸ� io�۾� �����ð����� io�۾� ������ �ð��� �� �͸�ŭ �� ������ ���� ����������,�����������Ѥ� 

 

    }

 

    

 

    if(num==1){    

 

    print_Queue(&terminated_Queue);

 

    print_Gantt(TIME-1);

 

}

 

    

 

    result[10].process = queue->size-1;

 

    result[10].context_switch = Context_Switch;

 

    int q = 0;

    for(q=(terminated_Queue.front+1); q<=terminated_Queue.rear; q++){

 

        result[10].sum_of_waiting_time += terminated_Queue.buffer[q].waiting_time;

 

        result[10].sum_of_burst_time += terminated_Queue.buffer[q].burst_time;

 

        result[10].sum_of_turnaround_time += terminated_Queue.buffer[q].waiting_time + terminated_Queue.buffer[q].burst_time + terminated_Queue.buffer[q].IO_finish_time - terminated_Queue.buffer[q].IO_start_point;

 

    }                                                                            //�̰� �����Ѱ� turnaround ���� �� ready queue�� �ִ� �ð� + runningqueue�� �������� �ð� + waiting queue�� �־��� �ð�!!!!!!!!!!!!!!! 

 

    result[10].ave_of_waiting_time = (float)result[10].sum_of_waiting_time / result[10].process;

 

    result[10].ave_of_burst_time = (float)result[10].sum_of_burst_time / result[10].process;

 

    result[10].ave_of_turnaround_time = (float)result[10].sum_of_turnaround_time / result[10].process;

 

    result[10].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

 

 

 

 

 

int main(){    

 

    

 

    while(1){

 

        int process_num;

 

        int num, num2;

 

        Queue ready_Queue;

 

        printf("\n[MENU]\n");

 

        printf("[1. ���μ��� ����]\n");

 

        printf("[2. Scheduling �˰���]\n");

 

        printf("[3. ��ü �˰��� ��]\n");

 

        printf("[4. ����]\n");

 

        printf("�� ���� �����Ͻðڽ��ϱ�?  :  ");

 

        scanf("%d",&num);

 

        

 

        switch(num){

 

            case(1):

 

                init_result();

 

                process_num = process_count();

 

                init_Queue(&ready_Queue, process_num);

 

                random_process(&ready_Queue);

 

                printf("[������ ť ���]\n");

 

                print_Queue(&ready_Queue);

 

                break;

 

            case(2):

 

                printf("\n[1. FCFS ����]\n");

 

                printf("[2. Nonpreemptive SJF ����]]\n");

 

                printf("[3. Nonpreemptive_Priority ����]\n");

 

                printf("[4. Round Robin ����]\n");

 

                printf("[5. Preemptive SJF ����]\n");

 

                printf("[6. Preemptive Priority����]\n");

 

                printf("[7. Preemptive Priority(aging)����]\n");

 

                printf("[8. Multilevel Queue ����]\n");

 

                printf("[9. Rate Monotonic ����]\n");

 

                printf("[10. Earliest Deadline First����]\n");

                

                printf("[11. Lottery����]\n");

 

                printf("�� ���� �����Ͻðڽ��ϱ�?  :  ");

 

                scanf("%d",&num2);

 

                switch(num2){

 

                    case(1):

 

                        FCFS(&ready_Queue, 1);

 

                        break;

 

                    case(2):

 

                        Nonpreemption_SJF(&ready_Queue, 1);

 

                        break;

 

                    case(3):

 

                        Nonpreemption_Priority(&ready_Queue, 1);

 

                        break;

 

                    case(4):

 

                        Round_Robin(&ready_Queue, 1);

 

                        break;

 

                    case(5):

 

                        Preemption_SJF(&ready_Queue, 1);

 

                        break;

 

                    case(6):

 

                        Preemption_Priority(&ready_Queue, 1);

 

                        break;

 

                    case(7):

 

                        Preemption_Priority_aging(&ready_Queue, 1);

 

                        break;

 

                    case(8):

 

                        Multilevel_Queue(&ready_Queue, 1);

 

                        break;

 

                    case(9):

 

                        Rate_Monotonic(&ready_Queue, 1);

 

                        break;

 

                    case(10):

 

                        EDF(&ready_Queue, 1);

 

                        break;

                        

                    case(11):

                        

                        Lottery(&ready_Queue, 1);

 

                }

 

                break;

 

            case(3):

 

                init_result();

                

                FCFS(&ready_Queue, 0);

 

                Nonpreemption_SJF(&ready_Queue, 0);

 

                Nonpreemption_Priority(&ready_Queue, 0);

 

                Round_Robin(&ready_Queue, 0);

 

                Preemption_SJF(&ready_Queue, 0);

 

                Preemption_Priority(&ready_Queue, 0);

 

                Preemption_Priority_aging(&ready_Queue, 0);

 

                Multilevel_Queue(&ready_Queue, 0);

 

                Rate_Monotonic(&ready_Queue, 0);

 

                EDF(&ready_Queue, 0);

                

                Lottery(&ready_Queue, 0);

 

                print_all_result();//�ϋ� �����ؤ��ŕ��� ��.   

 

                break;

 

            case(4):

 

                return 0;

 

            default:

 

                printf("�߸��� �����Դϴ�. �ٽ� �Է����ּ���!\n");

 

                break;

 

        }

 

    }

 

}

 

