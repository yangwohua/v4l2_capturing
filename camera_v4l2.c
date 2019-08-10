#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>  

#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <sys/time.h>

#include <assert.h>
#include <getopt.h>

#include <sys/types.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <time.h>

#define BUFFER_COUNT 4
#define CLEAR(x) memset(&(x), 0, sizeof(x))

unsigned char 	flame_buffer_0[1024 * 150];
unsigned char 	flame_buffer_1[1024 * 150];
unsigned char 	flame_buffer_2[1024 * 150];

int 			flame_buffer_size_0;
int 			flame_buffer_size_1;
int 			flame_buffer_size_2;

int             fd[3] = {-1, -1, -1};

struct buffer_0 {
	void   *start;
	size_t length;
};
struct buffer_1 {
	void   *start;
	size_t length;
};

static void xioctl(int fh, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

	if (r == -1) {
		fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
void path_check()
{
	char dir[80];
	int camera_Num = 0;
	for (camera_Num; camera_Num < 3; camera_Num++)
	{
		sprintf(dir, "/home/pi/Pictures/camera%d", camera_Num);
		printf("Directory :%s\n", dir);
		if (access(dir, R_OK) == -1)
		{
			printf("%s is not exist!\n", dir);
			mkdir(dir, 0755);
		}
		if (access(dir, R_OK) == 0)
		{
			printf("%s can be read!\n", dir);
		}
	}

}
void camera_init(int dev_video)
{
	struct v4l2_format              fmt;
	struct v4l2_requestbuffers      req;
	fd_set                          fds;
	int 							ret;
	char							temp_name[64];
	char							v_name[10][64];
	char							dir[] = "/home/pi/Pictures/";

	sprintf(temp_name, "/dev/video%d", dev_video);
	strncpy(v_name[dev_video], temp_name, sizeof(temp_name));
	fd[dev_video] = open(v_name[dev_video], O_RDWR | O_NONBLOCK, 0);
	if (fd[dev_video] < 0)
	{
		perror("Cannot open device\n");
		exit(EXIT_FAILURE);
	}

	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	xioctl(fd[dev_video], VIDIOC_S_FMT, &fmt);

	CLEAR(req);
	req.count = BUFFER_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	xioctl(fd[dev_video], VIDIOC_REQBUFS, &req);
}

void picture0(int mode, char *location)
{
	enum v4l2_buf_type              type;
	struct v4l2_buffer              buf;
	fd_set                          fds;
	struct timeval                  tv;
	unsigned int                    i, n_buffers;
	char                            out_name[256];
	FILE                            *fout;
	struct buffer_0                 *buffers;
	int 							ret;
	char							temp_name[64];
	char							v_name[10][64];
	struct tm						*get_time;
	time_t							t;

	buffers = calloc(4, sizeof(*buffers));
	for (n_buffers = 0; n_buffers < BUFFER_COUNT; ++n_buffers) {
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		xioctl(fd[0], VIDIOC_QUERYBUF, &buf);

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL, buf.length,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd[0], buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
		{
			perror("mmap");
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < n_buffers; ++i) {
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		xioctl(fd[0], VIDIOC_QBUF, &buf);
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(fd[0], VIDIOC_STREAMON, &type);
	
	//过滤掉前面几张不清晰的照片
	for (int j = 0; j < 5; j++)
	{
		for (i = 0; i < BUFFER_COUNT; i++)
		{
			buf.index = i;
			xioctl(fd[0], VIDIOC_DQBUF, &buf);
			memcpy(flame_buffer_0, buffers[buf.index].start, buf.bytesused);
			flame_buffer_size_0 = buf.bytesused;
			xioctl(fd[0], VIDIOC_QBUF, &buf);
		}
	}//过滤结束

	while (1) 
	{
		for (i = 0; i < BUFFER_COUNT; i++) 
		{
			buf.index = i;
			xioctl(fd[0], VIDIOC_DQBUF, &buf);
			memcpy(flame_buffer_0, buffers[buf.index].start, buf.bytesused);
			flame_buffer_size_0 = buf.bytesused;

			int ret_write;
			int f_jpg;
			char pic_name[64];
			char pic_path[64];

			//printf("begin\n");
			static int cnt_0 = 0;


			if (mode == 0)
			{
				sprintf(pic_name, "/home/pi/Pictures/camera0/image_%d.jpg", cnt_0++);
			}
			if (mode == 1)
			{

				t = time(NULL);
				get_time = gmtime(&t);

				/*sprintf(pic_name, "/home/pi/Pictures/camera0/image_%d.jpg", cnt_1++);*/
				sprintf(temp_name, "%d-%d-%d-%d-%d>%d.jpg", 1900 + get_time->tm_year, 1 + get_time->tm_mon, \
					get_time->tm_mday, 8 + get_time->tm_hour, get_time->tm_min, cnt_0++);
				strcpy(pic_path, location);
				strcat(pic_path, temp_name);		//location参数：./时间+摄像头号/
				strcpy(pic_name, pic_path);
			}

			remove(pic_name);
			f_jpg = open(pic_name, O_RDWR | O_CREAT, 0666);
			if (f_jpg == -1)
				printf("open error\r\n");
			ret_write = write(f_jpg, flame_buffer_0, flame_buffer_size_0);
			if (ret_write == -1)
				printf("write error\r\n");
			close(f_jpg);
			//printf("camera0 finish\n");

			//close(fd[0]);
			//if (cnt_0 > 200)
				//cnt_0 = 0;

			xioctl(fd[0], VIDIOC_QBUF, &buf);
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	xioctl(fd[0], VIDIOC_STREAMOFF, &type);
	for (i = 0; i < n_buffers; ++i)
		munmap(buffers[i].start, buffers[i].length);
}

void picture1(int mode, char *location)
{
	enum v4l2_buf_type              type;
	struct v4l2_buffer              buf;
	fd_set                          fds;
	struct timeval                  tv;
	unsigned int                    i, n_buffers;
	char                            out_name[256];
	FILE                            *fout;
	struct buffer_1                 *buffers;
	int 							ret;
	char							temp_name[64];
	char							v_name[10][64];
	struct tm						*get_time;
	time_t							t;

	buffers = calloc(4, sizeof(*buffers));
	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		xioctl(fd[1], VIDIOC_QUERYBUF, &buf);

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL, buf.length,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd[1], buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
		{
			perror("mmap");
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < n_buffers; ++i) {
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		xioctl(fd[1], VIDIOC_QBUF, &buf);
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(fd[1], VIDIOC_STREAMON, &type);
	for (i = 0; i < 10; i++) {
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		xioctl(fd[1], VIDIOC_DQBUF, &buf);
		memcpy(flame_buffer_1, buffers[buf.index].start, buf.bytesused);
		flame_buffer_size_1 = buf.bytesused;
		xioctl(fd[1], VIDIOC_QBUF, &buf);
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	xioctl(fd[1], VIDIOC_STREAMOFF, &type);
	for (i = 0; i < n_buffers; ++i)
		munmap(buffers[i].start, buffers[i].length);

	int ret_write;
	int f_jpg;
	char pic_name[64];
	char pic_path[64];

	//printf("begin\n");
	static int cnt_1 = 0;

	if (mode == 0)
	{
		sprintf(pic_name, "/home/pi/Pictures/camera1/image_%d.jpg", cnt_1++);
	}
	if (mode == 1)
	{

		t = time(NULL);
		get_time = gmtime(&t);

		/*sprintf(pic_name, "/home/pi/Pictures/camera0/image_%d.jpg", cnt_1++);*/
		sprintf(temp_name, "%d-%d-%d-%d-%d>%d.jpg", 1900 + get_time->tm_year, 1 + get_time->tm_mon, \
			get_time->tm_mday, 8 + get_time->tm_hour, get_time->tm_min, cnt_1++);
		strcpy(pic_path, location);
		strcat(pic_path, temp_name);		//location参数：./时间+摄像头号/
		strcpy(pic_name, pic_path);
	}

	remove(pic_name);
	f_jpg = open(pic_name, O_RDWR | O_CREAT, 0666);
	if (f_jpg == -1)
		printf("open error\r\n");
	ret_write = write(f_jpg, flame_buffer_1, flame_buffer_size_1);
	if (ret_write == -1)
		printf("write error\r\n");
	close(f_jpg);

	if (cnt_1 > 200)
		cnt_1 = 0;
}

void picture2(int mode, char *location)
{
	enum v4l2_buf_type              type;
	struct v4l2_buffer              buf;
	fd_set                          fds;
	struct timeval                  tv;
	unsigned int                    i, n_buffers;
	char                            out_name[256];
	FILE                            *fout;
	struct buffer_0                 *buffers;
	int 							ret;
	char							temp_name[64];
	char							v_name[10][64];
	struct tm						*get_time;
	time_t							t;

	buffers = calloc(4, sizeof(*buffers));
	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		xioctl(fd[2], VIDIOC_QUERYBUF, &buf);

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL, buf.length,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd[2], buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
		{
			perror("mmap");
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < n_buffers; ++i) {
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		xioctl(fd[2], VIDIOC_QBUF, &buf);
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(fd[2], VIDIOC_STREAMON, &type);
	for (i = 0; i < 10; i++) {
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		xioctl(fd[2], VIDIOC_DQBUF, &buf);
		memcpy(flame_buffer_2, buffers[buf.index].start, buf.bytesused);
		flame_buffer_size_2 = buf.bytesused;
		xioctl(fd[2], VIDIOC_QBUF, &buf);
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	xioctl(fd[2], VIDIOC_STREAMOFF, &type);
	for (i = 0; i < n_buffers; ++i)
		munmap(buffers[i].start, buffers[i].length);

	int ret_write;
	int f_jpg;
	char pic_name[64];
	char pic_path[64];

	//printf("begin\n");

	static int cnt_2 = 0;

	if (mode == 0)
	{
		sprintf(pic_name, "/home/pi/Pictures/camera2/image_%d.jpg", cnt_2++);
	}
	if (mode == 1)
	{

		t = time(NULL);
		get_time = gmtime(&t);

		/*sprintf(pic_name, "/home/pi/Pictures/camera0/image_%d.jpg", cnt_1++);*/
		sprintf(temp_name, "%d-%d-%d-%d-%d>%d.jpg", 1900 + get_time->tm_year, 1 + get_time->tm_mon, \
			get_time->tm_mday, 8 + get_time->tm_hour, get_time->tm_min, cnt_2++);
		strcpy(pic_path, location);
		strcat(pic_path, temp_name);		//location参数：./时间+摄像头号/
		strcpy(pic_name, pic_path);
	}

	remove(pic_name);
	f_jpg = open(pic_name, O_RDWR | O_CREAT, 0666);
	if (f_jpg == -1)
		printf("open error\r\n");
	ret_write = write(f_jpg, flame_buffer_2, flame_buffer_size_2);
	if (ret_write == -1)
		printf("write error\r\n");
	close(f_jpg);
	//printf("camera0 finish\n");

	//close(fd[0]);

	if (cnt_2 > 200)
		cnt_2 = 0;

}

void close_camera()
{
	close(fd[0]);
	close(fd[1]);
	close(fd[2]);
}

int main(void)
{
	//take_pictures_0(0, NULL);//2019-2-24-14-42
	//take_pictures_0(1, "/home/xiaoyang/workspace/2019-2-24-14-42/");			///home/xiaoyang/workspace/2019-2-24-14-42/
	//take_pictures_1(1, 1, "/home/xiaoyang/workspace/2019-2-24-14-42/");
	//take_pictures_2(0, 1, "/home/xiaoyang/workspace/2019-2-24-14-42/");
	printf("Camera init !!!\n");
	path_check();
	camera_init(0);
	camera_init(1);
	camera_init(2);
	//for(int i = 0; i<10; i++)
	//for(int i = 0; i<10; i++)
	//{
	//	picture0(0, NULL);
	//}
	//for (int i = 0; i < 10; i++)
	//{
	//	picture1(0, NULL);
	//}
	for (int i = 0; i < 10; i++)
	{
		picture0(0, NULL);
		//printf("%s\n", flame_buffer_2);
	}
	close_camera();
	/*take_pictures_1(1, 0, NULL);
	take_pictures_2(2, 0, NULL);*/
	return 0;
}

