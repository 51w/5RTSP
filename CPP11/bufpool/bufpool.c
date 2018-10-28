#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#include "bufpool.h"
#include "list.h"

#define MaxChn   5
#define MaxNode  8
static struct list g_list[MaxChn];
static char *_bptr = NULL;    //buff pointer
static int _chn  = 0;  	      //consumer num
static int _bpos = 0;  	      //data position
static int _size = 800*1024;  //buff size
static short _ListNode[MaxChn];

static pthread_mutex_t mutex;
static pthread_cond_t cond;

struct buflist{
    struct list link;
	DataPool	datapool;
	int	start_pos;
	int end_pos;
};

//原始图像信息SPS、PPS
static H264_mdata mdata;
int bufpool_Getps(H264_mdata *data)
{
	*data = mdata;
}

int bufpool_Setps(int type, char *p, int size)
{
	if(type == 7){
		if(mdata.sps == NULL){
			mdata.sps_len = size;
			mdata.sps = (char*)malloc(size);
			memcpy(mdata.sps, p, size);
		}
	}
	else if(type == 8){
		if(mdata.pps == NULL){
			mdata.pps_len = size;
			mdata.pps = (char*)malloc(size);
			memcpy(mdata.pps, p, size);
		}
	}
}

void mdata_init()
{
	mdata.sps_len = 0;
	mdata.pps_len = 0;
	
	mdata.sps = NULL;
	mdata.pps = NULL;
}

void mdata_exit()
{
	mdata.sps_len = 0;
	mdata.pps_len = 0;
	
	if(mdata.sps != NULL){
	  free(mdata.sps);
	  mdata.sps = NULL;
	}
	
	if(mdata.pps != NULL){
	  free(mdata.pps);
	  mdata.pps = NULL;
	}
}

//check 
int check_ANDSET(int a, int len, int x, int y)
{
	if(x >= a+len || y <= a)
		return 0;
	else
		return 1;
}

int insert_buf(int start, int len, DataPool *mdat)
{
	if(start+len > _size) return 0;
	
	struct list *pos, *nn;
	
	int i;
	struct buflist *pm;
	for(i=0; i<_chn; i++){
		list_for_each_safe(pos, nn, &g_list[i])   //;//
		{
			pm = list_entry(pos, struct buflist, link);
			if(check_ANDSET(start, len, pm->start_pos, pm->end_pos))
			{
				list_remove(&(pm->link));
				free(pm);
				pm = NULL;
				_ListNode[i]--;
			}	
		}
	}
	
	memcpy(_bptr+start, mdat->bptr, len);
	
	for(i=0; i<_chn; i++){
		struct buflist *pdata = (struct buflist *)malloc(sizeof(struct buflist));
		pdata->start_pos = start;
		pdata->end_pos = start+len;
		pdata->datapool.len 		= mdat->len;
		pdata->datapool.timestamp 	= mdat->timestamp;
		pdata->datapool.datetype 	= mdat->datetype;
		pdata->datapool.naltype 	= mdat->naltype;
		
		pdata->datapool.bptr 	= _bptr+start;
		
		list_append(&g_list[i], &(pdata->link));
		_ListNode[i]++;
		
		if(_ListNode[i] > MaxNode){
			pm = list_head(&g_list[i], struct buflist, link);
			list_remove(&(pm->link));
			free(pm);
			pm = NULL;
			_ListNode[i]--;
		}
	}
	return 1;
}

int bufpool_put(DataPool *data)
{
	int len = data->len;
	if(_chn<1 || len>_size || len<4) return 0;
	
	//result 0 fail   1 sucess
	int ret = 0, broadcast = 0;
	pthread_mutex_lock(&mutex);
	
	int i;
	for(i=0; i<_chn; i++){
		//broadcast = (g_list[i].next != &g_list[i])?0:1;
		broadcast = (_ListNode[i] != 0)?0:1;
		if(broadcast) break;
	}
	
	//Copy to the position//
	if(len+_bpos > _size){
		ret = insert_buf(0, len, data);
		_bpos = len;
	}
	else if(len+_bpos == _size){
		ret = insert_buf(_bpos, len, data);
		_bpos = 0;
	}
	else{
		ret = insert_buf(_bpos, len, data);
		_bpos = len+_bpos;
	}
	
	if(broadcast)
		pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
	
	return ret;
}


int bufpool_get(int channel, DataPool *data)
{
	//Set channel : 0 ~ _chn-1
	if(channel<0 || channel>(_chn-1))
		return -1;
	
	pthread_mutex_lock(&mutex);
	//while(g_list[channel].next == &g_list[channel])
	while(_ListNode[channel] == 0)
	{
		struct timeval now;
		struct timespec outtime;
		gettimeofday(&now, NULL);
		outtime.tv_sec = now.tv_sec + 2;
		outtime.tv_nsec = now.tv_usec * 1000;
		
		int ret;
		ret = pthread_cond_timedwait(&cond, &mutex, &outtime);
		if(ret != 0){
			pthread_mutex_unlock(&mutex);
			printf("Chn%d timeout\n", channel);
			return 0;
		}	
	}
	
	//output
	struct buflist *head;
	head = list_head(&g_list[channel], struct buflist, link);
	*data = head->datapool;
	
	list_remove(&(head->link));
	free(head);
	_ListNode[channel]--;
	
	pthread_mutex_unlock(&mutex);	
	return 1;
}


//listNode init
void buflist_init(int channel)
{
	_chn = (channel<MaxChn)?channel:MaxChn;
		
	int i;
	for(i=0; i<_chn; i++){
	  list_init(&g_list[i]);
	  _ListNode[i] = 0;
	}
	_bpos = 0;
	
	pthread_mutex_init(&mutex, NULL);	
	pthread_cond_init(&cond, NULL);
}

void buflist_exit()
{
	_chn = 0;
	
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

int bufpool_init(int size, int channel)
{
	_size = (size>_size)?size:_size;
	_bptr = (char*)malloc(_size);
	
	buflist_init(channel);
	
	mdata_init();

	return (_bptr==NULL)?0:1;
}

void bufpool_exit()
{
	mdata_exit();
	
	free(_bptr);
	buflist_exit();
}