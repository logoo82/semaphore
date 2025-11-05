/*3개의 칸이 있는 화장실을 5명의 고객이 이용*/
#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include<string.h>

//화장실 관리 세마포어
sem_t toiletsem;
sem_t mutex;

//성별 enum형
typedef enum 
{
    NONE = 0,
    MALE = 1,
    FEMALE = 2
} Gender;

//현재 화장실을 사용 중인 성별
Gender current_gender = NONE;
//마지막으로 화장실을 사용했던 성별 기억
Gender last_gender = NONE;
int use_cnt = 0;    //현재 화장실을 사용 중인 인원 수
int wait_man = 0;   //대기 중인 남자 수
int wait_woman = 0; //대기 중인 여자 수
int samegender = 0; //같은 성별이 연속으로 입장한 횟수

#define MAX_consecutive 5

//화장실에 들어간 사람의 성별
Gender gender = NONE;

void* guest(void* arg)
{   
    int cnt = -1;
    //앞의 두 글자를 읽고 현재 들어온 쓰레드의 성별 판단
    Gender my_gender = (strstr((char*)arg, "남자") != NULL) ? MALE : FEMALE;

    //화장실 입장 시도 전 대기 인원 등록(성별 구분)
    sem_wait(&mutex);

    if(my_gender == MALE)
        wait_man++;
    else
        wait_woman++;

    sem_post(&mutex);

    //입장 조건이 만족될 때 까지 가능 여부를 확인하고 대기
    /*
    1. 화장실이 비어있는 경우
    2. 같은 성별이 화장실을 쓰고 있고, 빈 칸이 있는 우
        1) 연속 입장 횟수가 6회보다 적을 경우
        2) 연속 입장 횟수를 다 사용했을 경우
            - 내 성별이 남자고 여자가 기다리고 있을 경우 양보
            - 내 성별이 여자고 남자가 기다리고 있을 경우 양보
            - 기다리고 있는 다른 성별이 없을 경우 그냥 입장
    3. 다른 성별이 사용하고 있는 경우
    */

    while(1)
    {
        sem_wait(&mutex);

        //입장 가능 여부 스위치
        int enter_switch = 0;

        //1. 화장실이 비어있을 경우
        if(current_gender == NONE)
        {   
            //같은 성별이 계속 오고, 연속 사용 횟수에 도달했을 경우
            if(last_gender == my_gender && samegender >= MAX_consecutive)
            {
                if(my_gender == MALE && wait_woman > 0)
                {
                    //여자에게 양보
                    enter_switch = 0;
                }
                else if(my_gender == FEMALE && wait_man > 0)
                {
                    //남자에게 양보
                    enter_switch = 0;
                }
                else
                    enter_switch = 1;   //반대 성별에게 양보
                
                //입장 가능할 때 성별이 바뀌면 카운터 초기화
                if(enter_switch && last_gender != my_gender)
                    samegender = 0;
            }
            else
            {
                //연속 제한에 안걸리거나 첫 입장일 경우
                enter_switch = 1;
            }
        }
        //2. 같은 성별이 사용중인 경우 && 빈칸이 있는 경우
        else if(current_gender == my_gender && use_cnt <3)
        {
            //1) 연속 입장 횟수가 6회보다 적을 경우
            if(samegender < MAX_consecutive)
            {
                enter_switch = 1;
            }
            //2) 연속 입장 횟수를 다 사용했을 경우
            else
            {
                //내 성별이 남자고 기다리는 여자가 있을 경우 양보
                if(my_gender == MALE && wait_woman > 0)
                {
                    enter_switch = 0;
                }
                //내 성별이 여자고 기다리는 남자가 있을 경우 양보
                else if(my_gender == FEMALE && wait_man > 0)
                {
                    enter_switch = 0;
                }
                // 나와 다른 성별이 기다리고 있지 않은 경우 그냥 입장 가능
                else
                {
                    enter_switch = 1;
                }
            }
        }
        //3. 다른 성별이 사용중인 경우
        else
        {
            enter_switch = 0;
        }

        //입장 처리
        if(enter_switch)
        {
            //성별별 대기인원 감소
            if(my_gender == MALE)
                wait_man--;
            else   
                wait_woman--;

            //P연산, 화장실 칸 사용
            sem_wait(&toiletsem);

            //성별 상태 업데이트
            if(current_gender != my_gender)
            {
                //직전 성별이 나의 성별과 다를 경우 횟수 초기화
                current_gender = my_gender;
                samegender = 0;
            }
            
            //같은 성별 연속 입장 횟수 증가
            samegender++;
            //사용 중인 인원 증가
            use_cnt++;

            int available;
            sem_getvalue(&toiletsem, &available);
            printf("고객입장: %s, 사용중: %d, 남은칸: %d, 연속: %d, 대기-남: %d, 여: %d \n",
            (char*)arg,  use_cnt, available, samegender, wait_man, wait_woman);

            //입장 완료
            sem_post(&mutex);
            break;

        }
        //입장 실패 시 mutex 해제 및 대기
        sem_post(&mutex);
    }

    //화장실 사용시간
    sleep(1);

    //화장실 퇴장
    sem_wait(&mutex);

    use_cnt--;
    printf("퇴장 / %s 남은 사용자: %d\n", (char*)arg, use_cnt);

    if(use_cnt == 0)
    {
        //화장실이 완전히 비었으니까 성별 및 연속사용횟수 초기화
        last_gender = current_gender;                                                                                                                                                                                                                                                                                                                                       
        current_gender = NONE;

        int same_gender_waiting = 0;
        if(last_gender == MALE && wait_man > 0)
            same_gender_waiting = 1;
        else if(last_gender == FEMALE && wait_woman > 0)
            same_gender_waiting = 1;
        
        if(!same_gender_waiting)
            samegender = 0;
        
    }

    //V연산, 화장실 칸 반환
    sem_post(&toiletsem);
    sem_post(&mutex);

    return NULL;
}

//자식 프로세스와 세마포 공유 X
#define NO 0
//동시에 들어갈 수있는 쓰레드 개수
#define MAX_COUNTER 3
//쓰레드의 개수
#define NUM_THREAD 15

int main()
{
    int counter = -1;
    //남자 고객들의 이름
    char *man_name[] = {"남자1", "남자2", "남자3", "남자4", "남자5", "남자6", 
    "남자7", "남자8", "남자9", "남자10", "남자11", "남자12", "남자13", "남자14", "남자15"};
    //여자 고객들의 이름
    char *woman_name[] = {"여자1", "여자2", "여자3", "여자4", "여자5", "여자6", 
    "여자7", "여자8", "여자9", "여자10", "여자11", "여자12", "여자13", "여자14", "여자15"};


    //남자 쓰레드와 여자 쓰레드 구조체를 각각 생성
    pthread_t man[NUM_THREAD];
    pthread_t woman[NUM_THREAD];

    //세마포 초기화(초기걊 설정)
    //MAX_COUNTER 명이 동시에 사용
    //정상적으로 초기화될 경우 세마포어 초기화 / 그렇지 않을 경우 -1 반환
    if(sem_init(&toiletsem, NO, MAX_COUNTER) == -1)
    {
        printf("semaphore is not surpporetd\n");
        return 0;
    }

    //이진 세마포어(뮤텍스) 초기화
    //한 번에 1개의 쓰레드만 임계구역 진입 가능
    if(sem_init(&mutex, NO, 1) == -1)
    {
        printf("mutex is not surpported\n");
        return 0;
    }

    //세마포의 현재 counter 값 읽기
    sem_getvalue(&toiletsem, &counter);
 
    for(int i = 0; i < NUM_THREAD; i++)
    {
        pthread_create(&man[i], NULL, guest, (void*)man_name[i]);
        pthread_create(&woman[i], NULL, guest, (void*)woman_name[i]);
    }    
        
    
    //모든 남자 고객이 소멸할 때 까지 대기    
    for(int i = 0; i < NUM_THREAD; i++)
        pthread_join(man[i], NULL);
    //모든 여자 고객이 소멸할 때 까지 대기    
    for(int i = 0; i < NUM_THREAD; i++)
        pthread_join(woman[i], NULL);    
    
    //세마포의 현재 counter값 읽기
    sem_getvalue(&toiletsem, &counter);
    printf("세마포 counter = %d\n", counter);
    sem_destroy(&toiletsem);
    sem_destroy(&mutex);

    return 0;
}
