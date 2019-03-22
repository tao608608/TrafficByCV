// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 VLCSTREAMFROMCAMERA_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// VLCSTREAMFROMCAMERA_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef VLCSTREAMFROMCAMERA_EXPORTS
#define VLCSTREAMFROMCAMERA_API __declspec(dllexport)
#else
#define VLCSTREAMFROMCAMERA_API __declspec(dllimport)
#endif

// 此类是从 VlcStreamFromCamera.dll 导出的
class VLCSTREAMFROMCAMERA_API CVlcStreamFromCamera {
public:
	CVlcStreamFromCamera(void);
	// TODO: 在此添加您的方法。

public:

	//初始化vlc库
	bool    InitilizeCameraStream();

	//释放vlc库
	bool    ReleaseCameraStream();

	void    SetImgBuffer(unsigned char* pImgData, unsigned char* pRealImgData24);
	//连接摄像机的Rtsp流
	int     ConnectCameraStream(char* psz_mrl, int width, int height);
	//断开摄像机的Rtsp流
	bool    DisconnectCameraStream();

	//获取连接状态
	bool   GetConnectstaton();

	//返回图像宽度
	int GetWidth();

	//返回图像高度
	int GetHeight();

	//抓取一帧，返回的IplImage不可手动释放！
	//返回图像数据的为RGB模式
	unsigned  char * QueryFrame();

};

//extern VLCSTREAMFROMCAMERA_API int nVlcStreamFromCamera;
//
//VLCSTREAMFROMCAMERA_API int fnVlcStreamFromCamera(void);
