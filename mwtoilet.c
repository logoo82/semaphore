/*3개의 칸이 있는 화장실을 5명의 고객이 이용*/
#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>

//POSIX 세마포 구조체
//모든 쓰레드에 의해 공유
sem_t toiletsem;

void* guest(void* arg)
{
    int cnt = -1;

    //P연산, 자원 사용 요청, 세마포의 counter 값 1 감소
    sem_wait(&toiletsem);
    sem_getvalue(&toiletsem, &cnt);
    printf("고객%s 화장실에 들어간다,,, 세마포 counter = %d\n", (char*)arg, cnt);

    //1초동안 화장실 사용
    sleep(1);

    printf("고객%s 화장실에서 나온다.\n", (char*)arg);
    //V연산, 화장실 사용을 끝냈음을 알림
    sem_post(&toiletsem);

}

//자식 프로세스와 세마포 공유 X
#define NO 0
//동시에 들어갈 수있는 쓰레드 개수
#define MAX_COUNTER 3

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
    pthread_t man[15];
    pthread_t woman[15];

    //세마포 초기화(초기걊 설정)
    //MAX_COUNTER 명이 동시에 사용
    //정상적으로 초기화될 경우 세마포어 초기화 / 그렇지 않을 경우 -1 반환
    int res = sem_init(&toiletsem, NO, MAX_COUNTER);
    if(res == -1)
    {
        printf("semaphore is not surpporetd\n");
        return 0;
    }

    //세마포의 현재 counter 값 읽기
    sem_getvalue(&toiletsem, &counter);

    for(int i = 0; i < 15; i++)
    //15명의 남자고객 생성
        pthread_create(&man[i], NULL, guest, (void*)man_name[i]);
    for(int i = 0; i < 15; i++)
    //15명의 여자고객 생성
        pthread_create(&woman[i], NULL, guest, (void*)woman_name[i]);
    
    //모든 남자 고객이 소멸할 때 까지 대기    
    for(int i = 0; i < 15; i++)
        pthread_join(man[i], NULL);
    //모든 여자 고객이 소멸할 때 까지 대기    
    for(int i = 0; i < 15; i++)
        pthread_join(woman[i], NULL);    
    
    //세마포의 현재 counter값 읽기
    sem_getvalue(&toiletsem, &counter);
    printf("세마포 counter = %d\n", counter);
    sem_destroy(&toiletsem);

    return 0;
}
