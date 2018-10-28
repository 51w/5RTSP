#include "H264VideoStreamSource.hh"

H264VideoStreamSource*
H264VideoStreamSource::createNew(UsageEnvironment& env)
{
	H264VideoStreamSource* newSource = new H264VideoStreamSource(env);

	return newSource;
}

H264VideoStreamSource::H264VideoStreamSource(UsageEnvironment& env)
	: FramedSource(env)
{//
	parse_h264(dninput);
	npos = 0;
}

H264VideoStreamSource::~H264VideoStreamSource() 
{
	for(int i=0; i<dninput.size(); i++)
		free(dninput[i].data);
	npos = 0;
}

// void H264VideoStreamSource::incomingDataHandler1() {
// 	fInput.getStream((void *)fTo, &fFrameSize, &fPresentationTime, fMaxSize);
// 	afterGetting(this);
// }

void H264VideoStreamSource::doGetNextFrame() 
{
	unsigned int stream_len = 0;

	stream_len = dninput[npos].size;
	if (stream_len > fMaxSize) {
		printf("[1]  drop stream: length=%u, fMaxSize=%d\n", stream_len, fMaxSize);
		stream_len = 0;
		goto out;
	}

	memcpy((void *)fTo, (void *)(dninput[npos].data), stream_len);
	npos++;
	if(npos == dninput.size()) npos = 0;

	gettimeofday(&fPresentationTime, NULL);
	usleep(30*1000);

out:
	fFrameSize = stream_len;

	//afterGetting(this);
	FramedSource::afterGetting(this);
}

int H264VideoStreamSource::parse_h264(std::vector<NaluUnit> &input)
{
	//printf("============cuc_ieschool.264==============\n");
	FILE *fp = fopen("./h264/22.h264", "rb");
	fseek(fp, 0L, SEEK_END);
	int length = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	char *fbuff = (char *)malloc(length);
	fread(fbuff, 1, length, fp);

	
	int Spos = 0;
	int Epos = 0;
	while(Spos < length)
	{
		if(fbuff[Spos++] == 0x00 && fbuff[Spos++] == 0x00)
		{
			if(fbuff[Spos++] == 0x01)
				goto gotnal_head;
			else
			{
				Spos--;
				if(fbuff[Spos++] == 0x00 && fbuff[Spos++] == 0x01)
					goto gotnal_head;
				else
					continue;
			}
		}
		else
		{
			continue;
		}
		
	gotnal_head:
		Epos = Spos;
		//int size = 0;
		NaluUnit NALdata;
		while(Epos < length)
		{
			if(fbuff[Epos++] == 0x00 && fbuff[Epos++] == 0x00)
			{
				if(fbuff[Epos++] == 0x01)
				{
					NALdata.size = (Epos-3)-Spos;
					break;
				}
				else
				{
					Epos--;
					if(fbuff[Epos++] == 0x00 && fbuff[Epos++] == 0x01)
					{	
						NALdata.size = (Epos-4)-Spos;
						break;
					}
				}
			}
		}
		if(Epos >= length)
		{
			NALdata.size = Epos - Spos;
			NALdata.type = fbuff[Spos]&0x1f;
			NALdata.data = (char*)malloc(NALdata.size);
			memcpy(NALdata.data, fbuff+Spos, NALdata.size);
			input.push_back(NALdata);

			break;
		}

		NALdata.type = fbuff[Spos]&0x1f;
		NALdata.data = (char*)malloc(NALdata.size);
		memcpy(NALdata.data, fbuff+Spos, NALdata.size);
		if(NALdata.type != 6)	input.push_back(NALdata);

		Spos = Epos - 4;
	}

	free(fbuff);
	fclose(fp);
	return 0;
}
//************//