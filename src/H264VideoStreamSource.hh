#ifndef H264VIDEOSTREAMSOURCE_HH
#define H264VIDEOSTREAMSOURCE_HH

#include "FramedSource.hh"
#include <vector>

typedef struct _NaluUnit
{
	unsigned int size;
	int   type;
	char *data;
}NaluUnit;

class H264VideoStreamSource: public FramedSource {
public:
  static H264VideoStreamSource* createNew(UsageEnvironment& env);

  H264VideoStreamSource(UsageEnvironment& env);

  // called only by createNew()
  virtual ~H264VideoStreamSource();

  int parse_h264(std::vector<NaluUnit> &input);
  std::vector<NaluUnit> dninput;  //h264
  unsigned int npos;

private:
  virtual void doGetNextFrame();

protected:
  Boolean isH264VideoStreamFramer() const { return True; }
  unsigned maxFrameSize()  const { return 150000; }
};

#endif

