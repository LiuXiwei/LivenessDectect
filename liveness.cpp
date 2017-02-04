#include "imageobj.h"
#include "liveness.h"
#include <QTimer>
#include <QTime>
#include <QDebug>
#include <QMessageBox>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsondocument.h>
#include <qfile.h>
#include "curl/curl.h"  
#include "curl/easy.h"  
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;

#define COUNTDOWN_TIME 3
#define TIMEOUT_COUNT 8
#define SEQ_LENGTH 4
#define LIMIT_SCORE 0.98
cv_finance_motion motion[SEQ_LENGTH] = { CV_LIVENESS_BLINK,
CV_LIVENESS_MOUTH, CV_LIVENESS_HEADYAW, CV_LIVENESS_HEADNOD };

static QString s_actions[4] = { QString::fromLocal8Bit("请眨眼"), QString::fromLocal8Bit("请张嘴"),
								QString::fromLocal8Bit("请左右摇头"),QString::fromLocal8Bit("请上下点头") };

Liveness::Liveness(QWidget *parent)
: QWidget(parent), m_bLivenessDetect(false), m_last_face_id(-1), m_timeout_count(TIMEOUT_COUNT)

{
	ui.setupUi(this);
	m_timer = new QTimer(this);
	m_timeout_timer = new QTimer(this);

	connect(m_timeout_timer, SIGNAL(timeout()), this, SLOT(onTimeOut()));
	connect(ui.pushButton_openCamera, SIGNAL(clicked()), this, SLOT(onOpenCamera()));
	connect(ui.pushButton_closeCamera, SIGNAL(clicked()), this, SLOT(onCloseCamera()));
	connect(ui.pushButton_liveness, SIGNAL(clicked()), this, SLOT(onStartLivenessDetect()));
	connect(m_timer, SIGNAL(timeout()), this, SLOT(onLivenessDetect()));
	
	// 随机种子
	QTime time = QTime::currentTime();
	qsrand(time.msec() + time.second() * 1000);
}
	

int Liveness::gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;

	return (0);
}

int Liveness::get_index(QString motion)
{
	if (motion == QString("BLINK")) return CV_LIVENESS_BLINK;
	if (motion == QString("MOUTH")) return CV_LIVENESS_MOUTH;
	if (motion == QString("HEADYAW")) return CV_LIVENESS_HEADYAW;
	if (motion == QString("HEADNOD")) return CV_LIVENESS_HEADNOD;
	return -1;
}

string Liveness::parse(QString type)
{
		if (obj.contains(type))
		{
			QJsonValue value = obj.take(type);
			if (value.isString())
				return value.toString().toStdString();
			else
			{
				QJsonArray array = value.toArray();
				int size = array.size();
				if (size > MAX_STEP) return "ERROR";
				for (int i = 0; i < size; i++)
				{
					QJsonValue value = array.at(i);
					if (value.isString())
					{
						QString name = value.toString();
						int tmp = get_index(name);
						if (tmp == -1) return false;
						else motion_list.push_back(tmp);
					}
					else
						return "ERROR";
					tot = size;
				}
				return "OK";
			}
		}
		else return "ERROR";
}
bool Liveness::init_config()
{
	API_ID = "ERROR";
	QString val;
	QFile file;

	file.setFileName("config.json");
	file.open(QIODevice::ReadOnly | QIODevice::Text);
	val = file.readAll();
	file.close();

	QJsonParseError json_error;
	QJsonDocument parse_document = QJsonDocument::fromJson(val.toLatin1(), &json_error);
	
	if (json_error.error == QJsonParseError::NoError && parse_document.isObject())
		obj = parse_document.object(); 
	else 
		return false;

	API_ID = parse("API_ID");
	API_SECRET = parse("API_SECRET");
	if (API_ID == "ERROR" || API_SECRET == "ERROR") return false;

	MODEL_PATH = parse("MODEL_PATH");
	if (MODEL_PATH == "ERROR") return false;

	URL = parse("URL");
	if (URL == "ERROR") return false;
	
	if (parse("MOTION_LIST") != "OK") return false;

	string comp = parse("COMPLEXITY");
	COMPLEXITY = -1;
	if (comp == "EASY") COMPLEXITY = WRAPPER_COMPLEXITY_EASY;
	if (comp == "NORMAL") COMPLEXITY = WRAPPER_COMPLEXITY_NORMAL;
	if (comp == "HARD") COMPLEXITY = WRAPPER_COMPLEXITY_HARD;
	if (comp == "HELL") COMPLEXITY = WRAPPER_COMPLEXITY_HELL;
	if (comp == "ERROR" || COMPLEXITY == -1) return false;

	return true;
}
bool Liveness ::init()
{
	step = 0;
	m_timeout_count = TIMEOUT_COUNT;

	if (!init_config())
		{
			if (API_ID == "ERROR" || API_SECRET == "ERROR" || URL == "ERROR")
			{
				QMessageBox::about(NULL, QString::fromLocal8Bit("消息"), QString::fromLocal8Bit("Fail to parse URL or API_ID or API_SECRET!Can't start Dectect!"));
				return false;
			}
			QMessageBox::about(NULL, QString::fromLocal8Bit("消息"), QString::fromLocal8Bit("Fail to parse config file!Using default configuration!"));
			motion_list.clear();
			tot = 4;
			for (int i = 0; i < SEQ_LENGTH; i++)
				motion_list.push_back(i);
			COMPLEXITY = WRAPPER_COMPLEXITY_EASY;
			MODEL_PATH = "models/M_Finance_Composite_General_Liveness_2.0.0.model";
		}

	if (handle_liveness != NULL)
	{
		cv_result = cv_finance_motion_liveness_end(handle_liveness);
		cv_finance_destroy_motion_liveness_handle(handle_liveness);
	}

	cv_result = cv_finance_create_motion_liveness_handle(&handle_liveness,
			MODEL_PATH.data());
	
	if (cv_result != CV_OK)
	{
		QMessageBox::about(NULL, QString::fromLocal8Bit("消息"), QString::fromLocal8Bit("cv_finance_create_motion_liveness_handle failed!!!"));
	}		
	
	cv_result = cv_finance_motion_liveness_begin(handle_liveness,
			COMPLEXITY|
			WRAPPER_OUTPUT_TYPE_MULTI_IMAGE|
			WRAPPER_LOG_LEVEL_SINGLE_FRAME);

	if (cv_result != CV_OK)
	{
		QMessageBox::about(NULL, QString::fromLocal8Bit("消息"), QString::fromLocal8Bit("cv_finance_motion_liveness_begin failed!!!"));
	}		

	cv_finance_motion_liveness_set_motion(
			handle_liveness, motion[motion_list[step]]);		

	if (cv_result != CV_OK)
	{
		QMessageBox::about(NULL, QString::fromLocal8Bit("消息"), QString::fromLocal8Bit("fail to set motion!!!"));
	}		

	return true;
}

Liveness::~Liveness()
{
	m_capture.release();
	delete m_timer;
	delete m_timeout_timer;
	cv_finance_destroy_motion_liveness_handle(handle_liveness);
}

void Liveness::onOpenCamera()
{
	m_capture.open(0);
	m_frame_width = m_capture.get(CV_CAP_PROP_FRAME_WIDTH);
	m_frame_height = m_capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	m_timer->start(20);
}

void Liveness::onCloseCamera()
{
	reinitialize(QString::fromLocal8Bit("-1"));
	m_bLivenessDetect = false;
	m_timer->stop();
	m_capture.release();
	ui.label_showImage->clear();
}

void Liveness::onStartLivenessDetect()
{
	if (!m_capture.isOpened()) return;
	if (init()) m_bLivenessDetect = true;
}

void Liveness::onLivenessDetect()
{
	bool passed = false;
	int face_count = 0;
	float score = 0.0f;
	cv_finance_t *p_face = NULL;
	cv::Mat read_frame, color_frame;

	if (!m_capture.isOpened() || !m_capture.read(read_frame))
		return;

	gettimeofday(&time, NULL);
	timestamp = time.tv_sec + (double)time.tv_usec / 1000000.0f;
	
	if (m_bLivenessDetect)
		cv_finance_motion_liveness_input(handle_liveness,
		read_frame.data, CV_PIX_FMT_BGR888,
		read_frame.cols, read_frame.rows,
		read_frame.step, CV_FINANCE_UP, timestamp,
		&p_face, &face_count, &passed, &score);

	cv::flip(read_frame, read_frame, 1);
	cv::cvtColor(read_frame, color_frame, CV_BGR2BGRA);
	cv::cvtColor(read_frame, read_frame, CV_BGR2RGB);
	QPixmap objPixmap = QPixmap::fromImage(QImage(read_frame.data, read_frame.cols, read_frame.rows,
		read_frame.step, QImage::Format_RGB888));
	float scaleRadio = ImageObj::countScaleRadio(objPixmap, ui.label_showImage->size());

	scaleRadio = ImageObj::countScaleRadio(objPixmap, ui.label_showImage->size());
	
	if (m_bLivenessDetect)
	{
		if (!m_timeout_timer->isActive())
		{
			m_timeout_count = TIMEOUT_COUNT;
			m_timeout_timer->start(1000);
		}

		m_strText = QString("%1  %2").arg(s_actions[motion_list[step]]).arg(QString::number(m_timeout_count));
		// 超时， 重新检测
		if (m_timeout_timer->isActive() && m_timeout_count <= 0)
		{	
			reinitialize(QString::fromLocal8Bit("检测超时，请重新开始检测 !!!"));
			return;
		}
		if (passed)
		{
			step++;
			m_timeout_count = TIMEOUT_COUNT;
		}
	}

	if (step >= tot)
	{
		cv_result = cv_finance_motion_liveness_end(handle_liveness);
		set_static_info();
		GenerateProtobuf();
		score = UploadProtobuf();
		GenerateImages();
		if (score<0 - FLT_EPSILON)
			reinitialize(QString::fromLocal8Bit("网络连接失败!"));
		else
		if (score <= LIMIT_SCORE && score >= FLT_EPSILON)
			reinitialize(QString::fromLocal8Bit("活体检测通过!"));
		else
			reinitialize(QString::fromLocal8Bit("未通过活体检测!"));	// 恢复初始状态
	}

	if (m_bLivenessDetect)
		cv_finance_motion_liveness_set_motion(
			handle_liveness, motion[motion_list[step]]);

	if (p_face != NULL && face_count > 0)
		ImageObj::drawRect(objPixmap, p_face[0].rect, scaleRadio);
	
	cv_finance_motion_liveness_release_frame(p_face, face_count);
	
	if (!m_strText.isEmpty())
		ui.label->setText(m_strText);
	
	ui.label_showImage->setPixmap(objPixmap);
}

void Liveness::GenerateProtobuf()
{
	cv_result = cv_finance_motion_liveness_get_result(handle_liveness,
		&encrypted_blob, &blob_len);
	
	FILE* f = fopen("encrypted.protobuf", "wb");
	fwrite(encrypted_blob, sizeof(char), blob_len, f);
	fclose(f);
	cv_finance_motion_liveness_release_result(encrypted_blob);
}

void Liveness::GenerateImages()
{
	cv_finance_frame* image_list = NULL;
	int image_count = 0;
	FILE* f;
	cv_result = cv_finance_motion_liveness_get_images(handle_liveness, &image_list, &image_count);
	for (int i = 0; i < image_count; ++i) {
		char filename[64];
		sprintf(filename, "%d-motion%d.jpeg", i, image_list[i].motion);
		f = fopen(filename, "wb");
		fwrite(image_list[i].image, sizeof(char), image_list[i].length, f);
		fclose(f);
	}
	cv_finance_motion_liveness_release_images(image_list, image_count);
}

void Liveness::set_static_info()
{
	return;
}

std::string HTTPRESULT;

size_t Write_data(void* buffer, size_t size, size_t nmemb, void* user_p){
	*((std::string *)user_p) += (const char*)buffer;
	return size*nmemb;
}


float Liveness::UploadProtobuf()
{
	float ret = 1.0;
	std::string strResult;
	CURL *curl = curl_easy_init();
	HTTPRESULT = "";
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);

	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;

	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "api_id",
		CURLFORM_COPYCONTENTS, API_ID.data(),
		CURLFORM_END);
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "api_secret",
		CURLFORM_COPYCONTENTS, API_SECRET.data(),
		CURLFORM_END);

	char* file_data = NULL;
	long file_size = 0;

	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "liveness_data_file",
		CURLFORM_FILE, "encrypted.protobuf", CURLFORM_END);

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, URL.data());
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &HTTPRESULT);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,Write_data);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		
		char error[1024];
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, FALSE);

		res = curl_easy_perform(curl);			
		if (res == CURLE_GOT_NOTHING || res == CURLE_OPERATION_TIMEDOUT) return -1;

		ofstream fp("res.json",ios::out);
		fp << HTTPRESULT << endl;		
		if (res != CURLE_OK) fp << endl << error << endl; 
		fp.close();
		int posb = HTTPRESULT.find("score");
		if (posb == -1) return 1.0;
		posb += 7;
		int pose = posb;
		while (HTTPRESULT[pose] != ',') pose++;
		ret = atof(HTTPRESULT.substr(posb, pose - posb).data());
	}
	curl_easy_cleanup(curl);
	curl_formfree(formpost);

	if (file_data != NULL) 
		delete[] file_data;	
	return ret;
}

void Liveness::reinitialize(QString& strInfo)
{
	ui.label->clear();
	step = 0;
	m_timeout_count = TIMEOUT_COUNT;
	m_bLivenessDetect = false;

	if (m_timeout_timer->isActive())
	{
		m_timeout_timer->stop();
	}
	m_strText.clear();

	cv_finance_motion_liveness_begin(handle_liveness,COMPLEXITY |
		WRAPPER_OUTPUT_TYPE_MULTI_IMAGE |
		WRAPPER_LOG_LEVEL_SINGLE_FRAME);

	 if (strInfo != QString::fromLocal8Bit("-1"))
		QMessageBox::about(NULL, QString::fromLocal8Bit("消息"), strInfo);
}

void Liveness::onTimeOut()
{
	m_timeout_count--;
}

