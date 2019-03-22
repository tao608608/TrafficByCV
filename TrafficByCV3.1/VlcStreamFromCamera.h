// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� VLCSTREAMFROMCAMERA_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// VLCSTREAMFROMCAMERA_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef VLCSTREAMFROMCAMERA_EXPORTS
#define VLCSTREAMFROMCAMERA_API __declspec(dllexport)
#else
#define VLCSTREAMFROMCAMERA_API __declspec(dllimport)
#endif

// �����Ǵ� VlcStreamFromCamera.dll ������
class VLCSTREAMFROMCAMERA_API CVlcStreamFromCamera {
public:
	CVlcStreamFromCamera(void);
	// TODO: �ڴ�������ķ�����

public:

	//��ʼ��vlc��
	bool    InitilizeCameraStream();

	//�ͷ�vlc��
	bool    ReleaseCameraStream();

	void    SetImgBuffer(unsigned char* pImgData, unsigned char* pRealImgData24);
	//�����������Rtsp��
	int     ConnectCameraStream(char* psz_mrl, int width, int height);
	//�Ͽ��������Rtsp��
	bool    DisconnectCameraStream();

	//��ȡ����״̬
	bool   GetConnectstaton();

	//����ͼ����
	int GetWidth();

	//����ͼ��߶�
	int GetHeight();

	//ץȡһ֡�����ص�IplImage�����ֶ��ͷţ�
	//����ͼ�����ݵ�ΪRGBģʽ
	unsigned  char * QueryFrame();

};

//extern VLCSTREAMFROMCAMERA_API int nVlcStreamFromCamera;
//
//VLCSTREAMFROMCAMERA_API int fnVlcStreamFromCamera(void);
