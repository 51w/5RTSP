#if __cplusplus
extern "C"{
#endif

typedef enum{
	TYPE_vdieo,
	TYPE_audio
} DataPoolType;

typedef struct{
	char *	sps;
	int 	sps_len;
	char *	pps;
	int 	pps_len;
}H264_mdata;

typedef struct{
	char *	bptr;
	int 	len;
	int 	timestamp;
	int		naltype;
	DataPoolType	datetype;		
}DataPool;  //缓存输入的数据

int bufpool_Getps(H264_mdata *data);

int bufpool_Setps(int type, char *p, int size);

//
int bufpool_put(DataPool *data);

int bufpool_get(int channel, DataPool *data);

int bufpool_init(int size, int channel);//缓存大小 / 消费者个数

void bufpool_exit();

#if __cplusplus
}
#endif