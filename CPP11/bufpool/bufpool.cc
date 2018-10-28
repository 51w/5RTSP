#include "bufpool.hh"

Bufpool::Bufpool(int size, int chn)
    : _size(size), _chnn(chn), _bpos(0), _bptr(NULL)
{
    _size = (_size>MinSize) ? _size:MinSize;
    _chnn = (_chnn<MaxChnn) ? _chnn:MaxChnn;
    
    buflist.resize(_chnn);
	_bptr = (char*)malloc(_size);

    pthread_cond_init( &condn, NULL);
    pthread_mutex_init(&mutex, NULL);	
}

Bufpool::~Bufpool()
{
    if(_bptr==NULL)  free(_bptr);

    pthread_cond_destroy( &condn);
	pthread_mutex_destroy(&mutex);
}

int check_ANDSET(int a, int len, int x, int y)
{
	if(x >= a+len || y <= a)
		return 0;
	else
		return 1;
}

int Bufpool::insert_buf(int start, int len, BPool &mdat)
{
    if(start+len > _size) return 0;

    for(int i=0; i<_chnn; i++)
    {
        pthread_mutex_lock(&mutex);
        while(buflist[i].size())
        {
            if(check_ANDSET(start, len, buflist[i].front().start_pos, buflist[i].front().end_pos) )
                buflist[i].pop_front();
            else
                break;
        }
        pthread_mutex_unlock(&mutex);
	}

    memcpy(_bptr+start, mdat.bptr, len);

    for(int i=0; i<_chnn; i++)
    {
        BufList input;
        input.start_pos         = start;
		input.end_pos           = start+len;
		input.datapool.len 		= mdat.len;
		input.datapool.timestamp= mdat.timestamp;
		input.datapool.datetype = mdat.datetype;
		input.datapool.naltype 	= mdat.naltype;
		input.datapool.bptr 	= _bptr+start;

        pthread_mutex_lock(&mutex);
        buflist[i].push_back(input);

        if(buflist[i].size() > MaxNode)
        buflist[i].pop_front();
        pthread_mutex_unlock(&mutex);
    }

    return 1;
}

int Bufpool::push(BPool &data)
{
    int len = data.len;
	if(_chnn<1 || len>_size || len<4) return 0;

    int ret = 0, broadcast = 0;

	for(int i=0; i<_chnn; i++)
    {
		broadcast += (buflist[i].size() != 0)?0:1;
	}

    //Copy to the position//
	if(len+_bpos > _size){
		ret = insert_buf(    0, len, data);
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
	pthread_cond_broadcast(&condn);

    return ret;
}

int Bufpool::pull(BPool &data, int chn)
{
    if(chn<0 || chn>=_chnn)
		return -1;

    pthread_mutex_lock(&mutex);

	while(buflist[chn].size() == 0)
	{
		struct timeval now;
		struct timespec outtime;
		gettimeofday(&now, NULL);
		outtime.tv_sec = now.tv_sec + 2;
		outtime.tv_nsec = now.tv_usec * 1000;
		
		int ret;
		ret = pthread_cond_timedwait(&condn, &mutex, &outtime);
		if(ret != 0) //g++ -lpthread 否则无法正常运行，上面一直循环
        {
			pthread_mutex_unlock(&mutex);
			printf("[CHN%d] timeout\n", chn);
			return 0;
		}
	}

    data = buflist[chn].front().datapool;
    buflist[chn].pop_front();
	
	pthread_mutex_unlock(&mutex);	
	return 1;
}

int main()
{
    Bufpool bufpool;
    printf("***********11\n");
    BPool dd;
    bufpool.pull(dd);
    printf("***********22\n");

    return 0;
}