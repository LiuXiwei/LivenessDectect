#ifndef LIVENESS_H
#define LIVENESS_H

#include <opencv2/highgui/highgui.hpp>
#include <QtWidgets/QWidget>
#include "ui_liveness.h"
#include "WinSock.h"
#include "cv_finance.h"
#include "time.h"
#include <qjsonobject.h>

#define  MAX_STEP 10

class Liveness : public QWidget
{
	Q_OBJECT

public:
	Liveness(QWidget *parent = 0);
	~Liveness();

private:
	bool init();
	void reinitialize(QString& strInfo);
	int gettimeofday(struct timeval *tp, void *tzp);
	std::string parse(QString type);
	bool init_config();
	int get_index(QString motion);
	void GenerateProtobuf();
	float UploadProtobuf();
	void GenerateImages();
	void set_static_info();

private slots:
	void onOpenCamera();
	void onCloseCamera();
	void onLivenessDetect();
	void onTimeOut();
	void onStartLivenessDetect();

private:
	Ui::LivenessClass ui;
	cv::VideoCapture m_capture;
	int m_frame_width;
	int m_frame_height;
	int m_last_face_id;

	cv_result_t cv_result = CV_OK;
	cv_handle_t handle_liveness = NULL;
	unsigned char* encrypted_blob = NULL;
	unsigned int blob_len = 0;
	
	bool m_bLivenessDetect = false;
	QString m_strText;

	QJsonObject obj;
	int tot = 1;
	int step = 0;
	std::vector<int> motion_list;
	std::string MODEL_PATH;
	std::string API_ID;
	std::string API_SECRET;
	std::string URL;
	int COMPLEXITY;

	double timestamp = 0.0f;
	struct timeval time;

	QTimer* m_timer;

	QTimer* m_timeout_timer;
	int m_timeout_count;

};

#endif // LIVENESS_H
