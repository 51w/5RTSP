#include <vector>
#include <deque>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#define MinSize   100*1024
#define MaxChnn   5
#define SizeVal   2000*1024
#define MaxNode   8

typedef struct
{
	char *bptr;
	int  len;
	int  timestamp;
	int	 naltype;
	int  datetype;	
}BPool;

typedef struct
{
	BPool datapool;
	int	  start_pos;
	int   end_pos;
}BufList;

class Bufpool
{
public:
    Bufpool(int size=SizeVal, int chn=2);
    ~Bufpool();

    int push(BPool &data);

    int pull(BPool &data, int chn=0);

    int insert_buf(int start, int len, BPool &mdat);

private:
    int _size;
    int _chnn;
    int _bpos;

    std::vector<std::deque<BufList> > buflist;
    char *_bptr;
    pthread_mutex_t mutex;
    pthread_cond_t  condn;
};