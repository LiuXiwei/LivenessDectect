#ifndef IMAGEOBJ_H
#define IMAGEOBJ_H

#include <QWidget>
#include <opencv2/highgui/highgui.hpp>

#include "cv_finance.h"


class ImageObj : public QWidget
{
	Q_OBJECT

public:
	ImageObj();
	~ImageObj();


public:
	static float countScaleRadio(QPixmap &obj_pixmap, QSize& target_size);
	static void drawRect(QPixmap& obj_pixmap, cv_rect_t& rect, float scale_radio);

private:

};

#endif // IMAGEOBJ_H
