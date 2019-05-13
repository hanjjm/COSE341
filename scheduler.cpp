
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

 

int TIME; //시간의 경과를 나타내기위한 변수

 

int gantt[1000];//간트 차트

 

int IO_time[1000]; //입출력 시간

 

 

 

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

 

    int IO_start_point;//이거는 io시작할때 전역변수 time 

 

    int IO_finish_time;//이거는 io작업 끝나는 전역변수에 있는데 거기서 io작업 기다리고 있는 시간 ///이 두개는 io작업이 발생했을 때 waiting time제대로 구하기 위해서 설정!!!!! 

 

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

 

 

 

PCB Running_Queue;//CPU에서 실행되는  

 

RESULT result[10];

 

 

 

int process_count(void){

 

    int process_num;

 

    while(1){

 

        printf("\n");

 

        printf("몇 개의 pocess를 실행하시겠습니까? (1~10): ");

 

        scanf ("%d", &process_num);

 

            if(process_num < MinProcess || process_num > MaxProcess)

 

                printf("숫자룰 다시 입력해주십시오. (1~10)\n");

 

            else break;

 

        }

 

    return process_num;

 

}

 

 

 

void init_Queue(Queue *queue, int p_num){

 

    p_num += 1;   //n+1 size의 queue에 n개의 process가 들어갈 수 있다.

 

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

 

 

 

PCB get_Queue_head(Queue *queue){//Queue의 head를 반환하는 함수

 

    return queue->buffer[(queue->front+1)%queue->size];

 

} 

 

 

 

void dequeue(Queue *queue){

 

    if(queue->count == 0){

 

        printf("Queue 비어있습니다.\n");

 

        return;

 

    } 

 

    queue->count -= 1;

 

    queue->buffer[queue->front] = init_PCB();//queue의 buffer에 있는 dequeue한건 초기화해주고 

 

    queue->front = (queue->front+1)%queue->size;//queue의 front는 한칸 올림  

 

}

 

 

 

void enqueue(Queue *queue, PCB process){

 

    if((queue->rear+1)%queue->size == queue->front){//circular에서 queue과 깍찼을 때 조건

 

        printf("Queue 꽉차있습니다.\n");

 

        exit(-1);//비정상적 종료 

 

    }

 

    queue->count += 1;

 

    queue->rear = (queue->rear+1)%queue->size;//rear 한칸증가시켜줌

 

    queue->buffer[queue->rear] = process;//process enqueue

 

}
 

int Queue_full_check(Queue *queue){//process num은 queue에 있는 process개수

 

    if(queue->front == (queue->rear+1)%queue->size) return 1;//circular queue에서 full 조건 

 

    else return 0;

 

}
 
/*
PCB dispatcher(Queue *Ready_Queue, PCB Running_Queue){

 

    if(Running_Queue.remain_time != 0) enqueue(Ready_Queue, Running_Queue);// running queue에 작업이 안끝났을 때, ready 큐로 보내준

 

    Running_Queue = get_Queue_head(Ready_Queue);

 

    Context_Switch = Context_Switch + 1;

 

    return Running_Queue;

 

} 

 */

//여기서 interrupt가 발생했을 때 running queue에 있던 것을

 

//ready queue의 마지막에 넣어주고 running queue에서는 ready queue의 head에 있던

 

//것을 가져오고, 한칸씩 앞으로 미뤄주고 context ++, running queue 반환

 

//즉, 레디큐->러닝큐, 러닝큐의작업이안끝났으면 러닝큐->레디큐, 하지만 알고리즘에 직접 쓰진 않았음! 

 

 

 

 

 

void Copy_Queue(Queue *queue, Queue *copy_Queue){

 

    copy_Queue->front = queue->front;

 

    copy_Queue->rear = queue->rear;

 

    copy_Queue->count = queue->count;

 

    copy_Queue->size = queue->size;

 

    

    int i = 0;

    for(i=0; i<queue->size;i++)    copy_Queue->buffer[i] = queue->buffer[i];

 

}

 

 

 

void *sort_by_arrive(Queue *queue){// 떄 쓰일 꺼 

 

    PCB temp;

     

    int i = 0, j = 0;

    

    for(i=(queue->front + 1)%queue->size; (i % queue->size)!=((queue->rear) % queue->size); i++){

 

        //반복문의 조건, i = front+1부터 시작하여 같아질떄까지 즉, 처음부터 끝까지

 

        i = i % queue->size;

 

        for(j=(i+1)% queue->size; (j % queue->size)!=((queue->rear+1) % queue->size); j++){

 

            //i 다음부터 또 끝까 

 

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

 

//selection sort로 arrival time 순으로 정렬!

 

 

 

void *sort_by_remain(Queue *queue){//남은시간순으로 정렬  

 

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

 

 

 

void *sort_by_priority(Queue *queue){//priority순으로 정렬  

 

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

 

void *sort_by_lottery(Queue *queue){//lottery algorithm에 쓰인 함수, 1~100까지 random한 수 받아서 그만큼 mix~~ 

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

 

    for(i = 0; i < 100; i++){//난수를 생성하여 우선순위를 100번을 랜덤하게 바꿔 

 

        mix1 = rand() % process_num;

 

        mix2 = rand() % process_num;

 

        temp = priority_arr[mix1];

 

        priority_arr[mix1] = priority_arr[mix2];

 

        priority_arr[mix2] = temp;

 

    }

 

    return priority_arr; 

 

}

 

 

 

void *random_process(Queue *queue){//이것을 통해 처음에 process들을 초기화해준다. 

 

    int priority_arr_temp[100];

 

    int *priority_arr = random_priority(priority_arr_temp, queue->size);

 

    srand(time(NULL));

 

    

    int i = 0;

    for(i = (queue->front+1); i < queue->size; i++){

 

        queue->buffer[i].pid = i; // 1,2,3... 이런식으로 지정

 

        queue->buffer[i].arrival_time = rand() % MaxArrivaltime; // 도착 시간0~19 범위에서 지정        

 

        queue->buffer[i].burst_time = rand() % MaxBursttime + 1; //burst time 1~10 범위에서 지정

 

        queue->buffer[i].waiting_time = 0; //대기시간 0으로 초기화

 

        queue->buffer[i].finish_time = 0;

 

        queue->buffer[i].execution_time = 0; //실행시간 0으로 초기화

 

        queue->buffer[i].remain_time = queue->buffer[i].burst_time; // 남은 시간은 burst_time으로 초기화

 

        queue->buffer[i].priority = priority_arr[i]; // 우선순위는 1~프로세스 갯수범위에서 초기화

 

        queue->buffer[i].period = ((rand() % 4 +1) * Maxperiod) /4;   //25, 50, 75, 100

 

        queue->buffer[i].IO_execution_time = 0;

 

        queue->buffer[i].IO_finish_time = 0;

 

        queue->buffer[i].IO_start_point = 0;

 

        queue->buffer[i].aging = 0;

 

        if(rand() % IOfrequency == 0 && queue->buffer[i].remain_time>1){  //IO_FREQ꼴에 한번 씩 IO연산 할당

 

            queue->buffer[i].IO_remain_time = rand() % MaxIOtime +1;  //1 ~ 5까지 남은 remain time 임의로 설정해줌 

 

            queue->buffer[i].IO_start_time = rand() % queue->buffer[i].IO_remain_time; //IO시작시간은 임의의 시간에서 남은시간보다 작게 

 

            if (queue->buffer[i].IO_start_time == 0 || queue->buffer[i].IO_start_time >= queue->buffer[i].burst_time){//io strat time이 cpu burst보다 크거나, io start time이 0이면, io start time을 1로 만들어주자!~ 

 

                queue->buffer[i].IO_start_time = 1;

 

                }

 

            if (queue->buffer[i].IO_start_time == 1 && queue->buffer[i].burst_time == 1){//나중에 추가한건데 진짜 거의 없는 경우지만 cpu burst랑 io start time이 1인경 웅 

 

                queue->buffer[i].IO_start_time == 0;

 

                queue->buffer[i].IO_remain_time == 0;

 

                }

 

            }

 

            queue->count = queue->count+1;

 

            queue->rear = queue->rear+1;//하나 만들때마다 count, rear 하나씩 증가 

 

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

 

}//process 처음에 목록 출력

 

 

 

void print_running_Queue(PCB running_Queue){ // running 큐 출력

 

 

 

    printf("\nProcess\n| PID |Arrival time|Finish time|Waiting time|Burst time|Executuin time|Remaining time|IO start time|IO execution time|IO remain time|Priority|Period|\n\n");

 

        printf("|  %d  |    %d       |     %d      |    %d     |     %d     |     %d      |      %d       |       %d        |         %d       |     %d      |    %d    |   %d  |\n", 

 

            running_Queue.pid, running_Queue.arrival_time, running_Queue.finish_time, running_Queue.waiting_time, running_Queue.burst_time, running_Queue.execution_time, 

 

            running_Queue.remain_time, running_Queue.IO_start_time, running_Queue.IO_execution_time, running_Queue.IO_remain_time, running_Queue.priority, running_Queue.period);

 

        

 

}

 

 

 

void print_Queue(Queue *queue){ // 원하는 큐 출력

 

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

    printf("\033[0;37m");//좀더 graphic적으로 눈에 띄게. 

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

 

            if(gantt[i] >= 10) printf("|%d|", gantt[i]);//process가 max일 경우 마지막 

 

            else printf("|%d", gantt[i]);//마지막 아니면 바끝나지않으니까 마지막테 | x 

 

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

 

    Copy_Queue(queue, &FCFS);//FCFS만들고 초리화하고 복 

 

        

 

    Queue ready_Queue;

 

    Queue waiting_Queue;

 

    Queue terminated_Queue;

 

    init_Queue(&ready_Queue, queue->size-1);

 

    init_Queue(&waiting_Queue, queue->size-1);

 

    init_Queue(&terminated_Queue, queue->size-1);//가각ㄱ 큐 만들고 초기 

 

    PCB running_Queue;

 

    running_Queue = init_PCB();//running queue만들고 초기화(CPU에서 돌아가는건 하나니까 PCB로 선언) 

 

//    print_Process(&running_Queue);

 

    sort_by_arrive(&FCFS);

 

    while(Queue_full_check(&terminated_Queue) != 1){


        if(num == 1) sleep(DELAY); 

 


int i = 0;

        for(i=FCFS.front+1; FCFS.buffer[i].pid!=0; i++){ //TIME에 도달할 때 ready_Q에 들어간다

 

            if(FCFS.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&FCFS));

 

                dequeue(&FCFS);//도착시간에 도달하면 레디큐로 넣어준다 

 

            }    

 

        };

 

    //    sort_by_arrive(&ready_Queue);    

 

    

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝났을 떄, 그것이 남은시간이 없으면 terminated queu로, 있으면 ready queue로보 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//이거는 io작업 발생할 때 waiting time 구하기 위해서. io끝난시간-시작한시간 빼서 그 시간만큼 빼야하므로!!waiting queue에 있는건 waiting time포함안되니까 

 

                //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//실행하고있던 프로세스가 끝나면 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                

 

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

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

}

 

 

 

        TIME += 1;    

 

 

 

    }

 

    

 

        //여기부터 -다시봐야할듯 

 

        //여기부터는 result에 저장하는 거임 

    int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

    result[0].ave_of_waiting_time = (float)result[0].sum_of_waiting_time / result[0].process;

 

    result[0].ave_of_burst_time = (float)result[0].sum_of_burst_time / result[0].process;

 

    result[0].ave_of_turnaround_time = (float)result[0].sum_of_turnaround_time / result[0].process;

 

    result[0].IO_count = IO_count;

 

}

 

 

 

 

 

void Nonpreemption_SJF(Queue *queue, int num){//여기는 도착한 것을 도착순서로 정렬했다가 남은시간으로 비교를 해함!! 

 

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

 

    

 

    

 

    sort_by_arrive(&Non_SJF); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

        int i = 0;

 

        for(i=Non_SJF.front+1; Non_SJF.buffer[i].pid != 0; i++){ //도착시간이 전역변수인 TIME에 도달하면 READY QUEUE 

 

            if(Non_SJF.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Non_SJF));

 

                dequeue(&Non_SJF);

 

            }

 

        }

 

     if(num == 1) sleep(DELAY);

 

      

 

      

 

    if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝났을 떄, 그것이 남은시간이 없으면 terminated queu로, 있으면 ready queue로보 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//이거는 io작업 발생할 때 waiting time 구하기 위해서. io끝난시간-시작한시간 빼서 그 시간만큼 빼야하므로!!waiting queue에 있는건 waiting time포함안되니까 

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_remain(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//실행하고있던 프로세스가 끝나면 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                

 

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

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

    }

 

        TIME += 1;    

 

    }

 

 

 

        int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

    result[1].ave_of_waiting_time = (float)result[1].sum_of_waiting_time / result[1].process;

 

    result[1].ave_of_burst_time = (float)result[1].sum_of_burst_time / result[1].process;

 

    result[1].ave_of_turnaround_time = (float)result[1].sum_of_turnaround_time / result[1].process;

 

    result[1].IO_count = IO_count;

 

}

 

 

 

 

 

void Nonpreemption_Priority(Queue *queue, int num){//여기는 도착한 것을 도착순서로 정렬했다가 남은시간으로 비교를 해함!! 

 

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

 

    

 

    

 

    sort_by_arrive(&Non_Priority); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

        int i = 0;

 

        for(i=Non_Priority.front+1; Non_Priority.buffer[i].pid != 0; i++){ //도착시간이 전역변수인 TIME에 도달하면 READY QUEUE 

 

            if(Non_Priority.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Non_Priority));

 

                dequeue(&Non_Priority);

 

            }

 

        }

 

            sort_by_priority(&ready_Queue);    

 

      if(num == 1) sleep(DELAY);

 

      

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝났을 떄, 그것이 남은시간이 없으면 terminated queu로, 있으면 ready queue로보 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//이거는 io작업 발생할 때 waiting time 구하기 위해서. io끝난시간-시작한시간 빼서 그 시간만큼 빼야하므로!!waiting queue에 있는건 waiting time포함안되니까 

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_priority(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//실행하고있던 프로세스가 끝나면 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                

 

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

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

    

 

        int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

    result[2].ave_of_waiting_time = (float)result[2].sum_of_waiting_time / result[2].process;

 

    result[2].ave_of_burst_time = (float)result[2].sum_of_burst_time / result[2].process;

 

    result[2].ave_of_turnaround_time = (float)result[2].sum_of_turnaround_time / result[2].process;

 

    result[2].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

//그런데 라운드로빈은 왜인지 모르겠는데 IO작업이 있는거는 오류가 나네..추후 수정 요망!! 

 

void Round_Robin(Queue *queue, int num){//여기는 도착한 것을 도착순서로 정렬했다가 남은시간으로 비교를 해함!! 

 

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

 

    

 

    

 

    sort_by_arrive(&Round_Robin); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

        int i = 0;

 

        for(i=Round_Robin.front+1; Round_Robin.buffer[i].pid != 0; i++){ //도착시간이 전역변수인 TIME에 도달하면 READY QUEUE 

 

            if(Round_Robin.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Round_Robin));

 

                dequeue(&Round_Robin);

 

            }

 

        }

 

            

 

    //  if(num == 1) sleep(DELAY);

 

      

 

          if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝났을 떄, 그것이 남은시간이 없으면 terminated queu로, 있으면 ready queue로보 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//이거는 io작업 발생할 때 waiting time 구하기 위해서. io끝난시간-시작한시간 빼서 그 시간만큼 빼야하므로!!waiting queue에 있는건 waiting time포함안되니까 

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }    

 

        }

 

 

 

    

 

        if(running_Queue.pid !=0){ //running queue에 process가 존재하면 남은시간 하나씩 줄이고, 실행시간 하나씩 늘림 

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1; 

 

            

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){ //remain time이 없고, io remain time도 없으면, 종료큐로보낸 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue,running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                running_Queue = init_PCB();         //run_Q를 초기화

 

            }

 

            

 

            if(running_Queue.pid != 0 && ((running_Queue.execution_time) % time_quantum) == 0 && running_Queue.IO_start_time == 0){//IO작업할것이없으면 그냥 그대로 수 행 

 

                if(running_Queue.execution_time > 0){

 

                    enqueue(&ready_Queue, running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }    

 

            

 

        //이부분어려웠음. IO start time이 3의배수일 때는 문제가 안생겼는데 IO start time이 1이나 2일 때는 IO작업 전에 하던 excution time도 포함하게 되어서

 

        //다른 값이 나와서, io start time이 1일때랑 2일 떄 temp 변수를 만들어서 그 전에 했던 excetion time만큼 빼주고 그게 3만큼 수행하면 옮겨 줌 

 

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

 

            

 

            //이부분어려웠음2탄.. IO start time이 3의배수일 때는 문제가 안생겼는데 IO start time이 4이나 5일 때는 IO작업 전에 하던 excution time도 포함하게 되어서

 

        //다른 값이 나와서, io start time이 4일때랑 5일 떄 temp 변수를 만들어서 그 전에 했던 excetion time만큼 빼주고 그게 3만큼 수행하면 옮겨 줌     

 

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

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

    

 

    int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

    result[3].ave_of_waiting_time = (float)result[3].sum_of_waiting_time / result[3].process;

 

    result[3].ave_of_burst_time = (float)result[3].sum_of_burst_time / result[3].process;

 

    result[3].ave_of_turnaround_time = (float)result[3].sum_of_turnaround_time / result[3].process;

 

    result[3].IO_count = IO_count;

 

}

 

 

 

 

 

void Preemption_SJF(Queue *queue, int num){//여기는 도착한 것을 도착순서로 정렬했다가 남은시간으로 비교를 해함!! 

 

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

 

    

 

    

 

    sort_by_arrive(&Preemption_SJF); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

        int i = 0;

 

        for(i=Preemption_SJF.front+1; Preemption_SJF.buffer[i].pid != 0; i++){ //도착시간이 전역변수인 TIME에 도달하면 READY QUEUE 

 

            if(Preemption_SJF.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Preemption_SJF));

 

                dequeue(&Preemption_SJF);

 

            }

 

        }

 

            

 

        sort_by_remain(&ready_Queue);

 

            

 

      if(num == 1 )sleep(DELAY);

 

      

 

      

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝났을 떄, 그것이 남은시간이 없으면 terminated queu로, 있으면 ready queue로보 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//이거는 io작업 발생할 때 waiting time 구하기 위해서. io끝난시간-시작한시간 빼서 그 시간만큼 빼야하므로!!waiting queue에 있는건 waiting time포함안되니까 

 

                //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_remain(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//실행하고있던 프로세스가 끝나면 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }            

 

        
     if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //만약 running_q가 아직 있고, ready_Q도 있다면

 

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

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

    

        int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

    result[4].ave_of_waiting_time = (float)result[4].sum_of_waiting_time / result[4].process;

 

    result[4].ave_of_burst_time = (float)result[4].sum_of_burst_time / result[4].process;

 

    result[4].ave_of_turnaround_time = (float)result[4].sum_of_turnaround_time / result[4].process;

 

    result[4].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

void Preemption_Priority(Queue *queue, int num){//여기는 도착한 것을 도착순서로 정렬했다가 남은시간으로 비교를 해함!! 

 

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

 

    

 

    

 

    sort_by_arrive(&Preemption_Priority); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

        int i = 0;

 

        for(i=Preemption_Priority.front+1; Preemption_Priority.buffer[i].pid != 0; i++){ //도착시간이 전역변수인 TIME에 도달하면 READY QUEUE 

 

            if(Preemption_Priority.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&Preemption_Priority));

 

                dequeue(&Preemption_Priority);

 

            }

 

        }

 

            sort_by_priority(&ready_Queue);    

 

      if(num == 1) sleep(DELAY);

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝났을 떄, 그것이 남은시간이 없으면 terminated queu로, 있으면 ready queue로보 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//이거는 io작업 발생할 때 waiting time 구하기 위해서. io끝난시간-시작한시간 빼서 그 시간만큼 빼야하므로!!waiting queue에 있는건 waiting time포함안되니까 

 

                //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

        

 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

 

                

 

                    dequeue(&waiting_Queue);

 

            }

 

            

 

            

 

        }

 

        

 

        sort_by_priority(&ready_Queue);

 

 

 

        if(running_Queue.pid != 0){

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//실행하고있던 프로세스가 끝나면 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }            

 

        

 

 

 

        if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //만약 running_q가 아직 있고, ready_Q도 있다면

 

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

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

 

    int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

    result[5].ave_of_waiting_time = (float)result[5].sum_of_waiting_time / result[5].process;

 

    result[5].ave_of_burst_time = (float)result[5].sum_of_burst_time / result[5].process;

 

    result[5].ave_of_turnaround_time = (float)result[5].sum_of_turnaround_time / result[5].process;

 

    result[5].IO_count = IO_count;

 

}

 

 

 

 

 

void Preemption_Priority_aging(Queue *queue, int num){//여기는 도착한 것을 도착순서로 정렬했다가 남은시간으로 비교를 해함!! 

 

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

 

    

 

    

 

    sort_by_arrive(&Non_Priority); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다

 

    

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

        int i = 0;

 

        for(i=Non_Priority.front+1; Non_Priority.buffer[i].pid != 0; i++){ //도착시간이 전역변수인 TIME에 도달하면 READY QUEUE 

 

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

 

      

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

        

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝났을 떄, 그것이 남은시간이 없으면 terminated queu로, 있으면 ready queue로보 

 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//이거는 io작업 발생할 때 waiting time 구하기 위해서. io끝난시간-시작한시간 빼서 그 시간만큼 빼야하므로!!waiting queue에 있는건 waiting time포함안되니까 

 

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

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//실행하고있던 프로세스가 끝나면 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue, running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                

 

                running_Queue = init_PCB();

 

            }

 

        }    

 

 

 

 

 

            if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //만약 running_q가 아직 있고, ready_Q도 있다면

 

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

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

    }

 

        TIME += 1;    

 

    }

 

    

 

        int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

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

 

    

 

    Queue RR;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.

 

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

 

    

 

//waiting queue -> ready queue랑

 

//running queue -> readyq2??termi??로 오ㄹ 

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복    

 

    if(num == 1) sleep(DELAY);    

 

        //RR에서 arrive_time에 따라서 ready_Q로 들어간다 

 

        for(i=RR.front+1; RR.buffer[i].pid!=0; i++){ 

 

            if(RR.buffer[i].arrival_time != TIME) 

 

                break;

 

            else{

 

                enqueue(&ready_Queue, get_Queue_head(&RR));

 

                dequeue(&RR);

 

            }

 

        }

 

        //FCFS에서 arrive_time에 따라서 ready_Q2로 들어간다 

 

        for(i=FCFS.front+1; FCFS.buffer[i].pid!=0; i++){ 

 

            if(FCFS.buffer[i].arrival_time == TIME){    

 

                enqueue(&ready_Queue2, get_Queue_head(&FCFS));

 

                dequeue(&FCFS);

 

            

 

        }

 

        }

 

        

 

        

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

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

 

                

 

                

 

                                

 

        if(running_Queue.pid !=0){ //running queue에 process가 존재하면 남은시간 하나씩 줄이고, 실행시간 하나씩 늘림 

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;     

 

                

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){//실행하고있던 프로세스가 끝나면 

 

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

 

            }//요기기기ㅣ이 

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                running_Queue = init_PCB();         //run_Q를 초기화

 

            }

 

            

 

            if(running_Queue.pid%3 != 0 && ((running_Queue.execution_time) % time_quantum) == 0 && running_Queue.IO_start_time == 0){//IO작업할것이없으면 그냥 그대로 수 행 

 

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

 

            

 

 

 

    

 

        //이부분어려웠음. IO start time이 3의배수일 때는 문제가 안생겼는데 IO start time이 1이나 2일 때는 IO작업 전에 하던 excution time도 포함하게 되어서

 

        //다른 값이 나와서, io start time이 1일때랑 2일 떄 temp 변수를 만들어서 그 전에 했던 excetion time만큼 빼주고 그게 3만큼 수행하면 옮겨 줌 

 

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

 

            

 

            //이부분어려웠음2탄.. IO start time이 3의배수일 때는 문제가 안생겼는데 IO start time이 4이나 5일 때는 IO작업 전에 하던 excution time도 포함하게 되어서

 

          //다른 값이 나와서, io start time이 4일때랑 5일 떄 temp 변수를 만들어서 그 전에 했던 excetion time만큼 빼주고 그게 3만큼 수행하면 옮겨 줌     

 

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

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

    }

 

        TIME += 1;

 

    }

 

    

int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

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

 

    Queue Priority;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.

 

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

 

 

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

    if(num == 1) sleep(DELAY);

 

        if(TIME == 201)

 

            break;    

 

        for(i=(Priority.front+1); i != Priority.rear+1; i++){ //TIME에 도달할 때 ready_Q에 들어간다

 

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

 

      

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다

 

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

 

        

 

        if(running_Queue.pid !=0){ //run_Q존재, ready_q도 존재

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){ //no more burst and no more i/o

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue,running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                running_Queue = init_PCB();         //run_Q를 초기화

 

            }

 

            if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //만약 running_q가 아직 있고, ready_Q도 있다면

 

                if(running_Queue.priority > get_Queue_head(&ready_Queue).priority){ //ready_Q가 더 shortest_job이라면 비운다

 

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

 

 

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

    }

 

        TIME += 1;    

 

    }

 

    

int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

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

 

    Queue Priority;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.

 

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

 

 

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

    if(num == 1) sleep(DELAY);

 

    for(i=(ready_Queue.front+1)%ready_Queue.size; i <= ready_Queue.rear; i++)

 

                ready_Queue.buffer[i].priority -= 1;

 

                running_Queue.priority -= 1;

 

            for(i=waiting_Queue.front+1; i <= waiting_Queue.rear; i++)

 

                waiting_Queue.buffer[i].priority -= 1;

 

    

 

    

 

        if(TIME == 201)

 

            break;    

 

        for(i=(Priority.front+1); i != Priority.rear+1; i++){ //TIME에 도달할 때 ready_Q에 들어간다

 

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

 

      

 

        if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

 

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

 

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다

 

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

 

        

 

        if(running_Queue.pid !=0){ //run_Q존재, ready_q도 존재

 

            running_Queue.remain_time -= 1;

 

            running_Queue.execution_time += 1;

 

            

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){ //no more burst and no more i/o

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue,running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                running_Queue = init_PCB();         //run_Q를 초기화

 

            }

 

            if(running_Queue.pid != 0 && get_Queue_head(&ready_Queue).pid!=0){ //만약 running_q가 아직 있고, ready_Q도 있다면

 

                if(running_Queue.priority > get_Queue_head(&ready_Queue).priority){ //ready_Q가 더 shortest_job이라면 비운다

 

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

 

 

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

    }

 

        TIME += 1;    

 

    }

 

    

int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

    result[9].ave_of_waiting_time = (float)result[9].sum_of_waiting_time / result[9].process;

 

    result[9].ave_of_burst_time = (float)result[9].sum_of_burst_time / result[9].process;

 

    result[9].ave_of_turnaround_time = (float)result[9].sum_of_turnaround_time / result[9].process;

 

    result[9].IO_count = IO_count;

 

}

 

 

 

 

 

 

 

 

void Lottery(Queue *queue, int num){//여기는 도착한 것을 도착순서로 정렬했다가 남은시간으로 비교를 해함!! 

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

 

    while(Queue_full_check(&terminated_Queue) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복        

 

        int i = 0;

        for(i=Round_Robin.front+1; Round_Robin.buffer[i].pid != 0; i++){ //도착시간이 전역변수인 TIME에 도달하면 READY QUEUE 

            if(Round_Robin.buffer[i].arrival_time != TIME) 

                break;

            else{

                enqueue(&ready_Queue, get_Queue_head(&Round_Robin));

                dequeue(&Round_Robin);

            }

        }

          

    //  if(num == 1) sleep(DELAY);    

 

          if(waiting_Queue.buffer[waiting_Queue.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.

            waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time -= 1;

            waiting_Queue.buffer[waiting_Queue.front+1].IO_execution_time += 1;    

        

            if(waiting_Queue.buffer[waiting_Queue.front+1].IO_remain_time ==0){ //I.O 입출력이 끝났을 떄, 그것이 남은시간이 없으면 terminated queu로, 있으면 ready queue로보 

                    waiting_Queue.buffer[waiting_Queue.front+1].IO_finish_time = TIME;//이거는 io작업 발생할 때 waiting time 구하기 위해서. io끝난시간-시작한시간 빼서 그 시간만큼 빼야하므로!!waiting queue에 있는건 waiting time포함안되니까 

                    enqueue(&ready_Queue, waiting_Queue.buffer[waiting_Queue.front+1]);

                    dequeue(&waiting_Queue);

            }    

        }

    

 

        if(running_Queue.pid !=0){ //running queue에 process가 존재하면 남은시간 하나씩 줄이고, 실행시간 하나씩 늘림 

            running_Queue.remain_time -= 1;

            running_Queue.execution_time += 1;       

 

            if(running_Queue.remain_time <= 0 && running_Queue.IO_remain_time == 0){ //remain time이 없고, io remain time도 없으면, 종료큐로보낸 

 

                running_Queue.finish_time = TIME;

 

                enqueue(&terminated_Queue,running_Queue);

 

                running_Queue = init_PCB();

 

            }

 

            

 

            if(running_Queue.IO_remain_time !=0 &&

 

                    (running_Queue.IO_start_time == running_Queue.execution_time)){ //만약 IO연산을 수행해야 한다면

 

                IO_count += 1;

 

                running_Queue.IO_start_point = TIME;

 

                enqueue(&waiting_Queue, running_Queue); //waiting_Q로 이동시킨다.

 

                running_Queue = init_PCB();         //run_Q를 초기화

 

            }

 

            

 

            if(running_Queue.pid != 0 && ((running_Queue.execution_time) % time_quantum) == 0 && running_Queue.IO_start_time == 0){//IO작업할것이없으면 그냥 그대로 수 행 

 

                if(running_Queue.execution_time > 0){

 

                    enqueue(&ready_Queue, running_Queue);

 

                    running_Queue = init_PCB();

 

                }

 

            }    

 

            

 

        //이부분어려웠음. IO start time이 3의배수일 때는 문제가 안생겼는데 IO start time이 1이나 2일 때는 IO작업 전에 하던 excution time도 포함하게 되어서

 

        //다른 값이 나와서, io start time이 1일때랑 2일 떄 temp 변수를 만들어서 그 전에 했던 excetion time만큼 빼주고 그게 3만큼 수행하면 옮겨 줌 

 

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

 

            

 

            //이부분어려웠음2탄.. IO start time이 3의배수일 때는 문제가 안생겼는데 IO start time이 4이나 5일 때는 IO작업 전에 하던 excution time도 포함하게 되어서

 

        //다른 값이 나와서, io start time이 4일때랑 5일 떄 temp 변수를 만들어서 그 전에 했던 excetion time만큼 빼주고 그게 3만큼 수행하면 옮겨 줌     

 

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

        if(num == 1) printf("\n추첨 START\n");

    sort_by_lottery(&ready_Queue);

            running_Queue = get_Queue_head(&ready_Queue);

            dequeue(&ready_Queue);

    if(num == 1) printf("당첨된 process의 PID : %d\n", running_Queue.pid);

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

 

        printf("\n※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※\n");

 

        

 

    }

 

        TIME += 1;    

 

    }

 

    

 

    int k = 0;

    for(k=terminated_Queue.front+1; k<= terminated_Queue.rear; k++){

 

        PCB temp;

 

        temp = terminated_Queue.buffer[k];

 

        if(terminated_Queue.buffer[k].IO_start_time == 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time;//IO작업이 없으면 순수하게 이것만 고려하면 됨@@@@@

 

        if(terminated_Queue.buffer[k].IO_start_time != 0) terminated_Queue.buffer[k].waiting_time = temp.finish_time - temp.execution_time - temp.arrival_time - (temp.IO_finish_time - temp.IO_start_point);//io작업이 발생하면 io작업 끝난시간에서 io작업 시작한 시간을 뺀 것만큼 더 뺴야함 정말 ㄴㅇㄻㄴㄹ,ㄴㅇㅁ리나ㅡㅁ 

 

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

 

    }                                                                            //이거 수정한게 turnaround 구할 떄 ready queue에 있던 시간 + runningqueue에 ㅇㅆ었던 시가 + waiting queue에 있었던 시간!!!!!!!!!!!!!!! 

 

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

 

        printf("[1. 프로세스 생성]\n");

 

        printf("[2. Scheduling 알고리즘]\n");

 

        printf("[3. 전체 알고리즘 평가]\n");

 

        printf("[4. 종료]\n");

 

        printf("몇 번을 수행하시겠습니까?  :  ");

 

        scanf("%d",&num);

 

        

 

        switch(num){

 

            case(1):

 

                init_result();

 

                process_num = process_count();

 

                init_Queue(&ready_Queue, process_num);

 

                random_process(&ready_Queue);

 

                printf("[생성된 큐 출력]\n");

 

                print_Queue(&ready_Queue);

 

                break;

 

            case(2):

 

                printf("\n[1. FCFS 실행]\n");

 

                printf("[2. Nonpreemptive SJF 실행]]\n");

 

                printf("[3. Nonpreemptive_Priority 실행]\n");

 

                printf("[4. Round Robin 실행]\n");

 

                printf("[5. Preemptive SJF 실행]\n");

 

                printf("[6. Preemptive Priority실행]\n");

 

                printf("[7. Preemptive Priority(aging)실행]\n");

 

                printf("[8. Multilevel Queue 실행]\n");

 

                printf("[9. Rate Monotonic 실행]\n");

 

                printf("[10. Earliest Deadline First실행]\n");

                

                printf("[11. Lottery실행]\n");

 

                printf("몇 번을 수행하시겠습니까?  :  ");

 

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

 

                print_all_result();//일떄 보여준ㄴ거뺴고 다.   

 

                break;

 

            case(4):

 

                return 0;

 

            default:

 

                printf("잘못된 선택입니다. 다시 입력해주세요!\n");

 

                break;

 

        }

 

    }

 

}

 

