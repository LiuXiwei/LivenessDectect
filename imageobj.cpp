#include <QPainter>
#include <opencv2/core/core.hpp>
#include <QFontDatabase>
#include <opencv2/imgproc/imgproc.hpp>
#include "imageobj.h"

ImageObj::ImageObj()
: QWidget(NULL)
{
	
}

ImageObj::~ImageObj()
{

}

float ImageObj::countScaleRadio(QPixmap &obj_pixmap, QSize& target_size)
{
	QPixmap temp_pixmap = obj_pixmap;
	float scale_radio = 1.0;
	if (obj_pixmap.width() > target_size.width() || obj_pixmap.height() > target_size.height())
	{
		QSize qsize(target_size.width() - 5, target_size.height() - 5);
		obj_pixmap = obj_pixmap.scaled(qsize, Qt::KeepAspectRatio);

		if (obj_pixmap.width() == qsize.width())
		{
			scale_radio = 1.0 * temp_pixmap.width() / qsize.width();
		}
		else
		{
			scale_radio = 1.0 * temp_pixmap.height() / qsize.height();
		}
	}

	return scale_radio;
}

void ImageObj::drawRect(QPixmap& obj_pixmap, cv_rect_t& rect, float scale_radio)
{
	QPainter painter;
	painter.begin(&obj_pixmap);
	painter.setPen(QPen(QColor("#aee13a"), 2));
	int left = rect.left / scale_radio;
	int top = rect.top / scale_radio;
	int right = rect.right / scale_radio;
	int bottom = rect.bottom / scale_radio;
	int tmpl = left;
	left = 645 - right;
	right = 645 - tmpl;
	top -= 30;
	bottom -= 30;
	// left top
	QPoint leftTopPoint(left, top);
	QPoint leftTopToRightPoint(left + 20, top);
	QPoint leftTopToBottomPoint(left, top + 20);
	painter.drawLine(leftTopPoint, leftTopToRightPoint);
	painter.drawLine(leftTopPoint, leftTopToBottomPoint);

	// right top
	QPoint rightTopPoint(right, top);
	QPoint rightTopToLeftPoint(right - 20, top);
	QPoint rightTopToBottomPoint(right, top + 20);
	painter.drawLine(rightTopPoint, rightTopToLeftPoint);
	painter.drawLine(rightTopPoint, rightTopToBottomPoint);

	// left bottom
	QPoint leftBottomPoint(left, bottom);
	QPoint leftBottomToLeft(left, bottom - 20);
	QPoint leftBottomToRight(left + 20, bottom);
	painter.drawLine(leftBottomPoint, leftBottomToLeft);
	painter.drawLine(leftBottomPoint, leftBottomToRight);

	// right bottom
	QPoint rightBottomPoint(right, bottom);
	QPoint rightBottomToLeft(right, bottom - 20);
	QPoint rightBottomToRight(right - 20, bottom);
	painter.drawLine(rightBottomPoint, rightBottomToLeft);
	painter.drawLine(rightBottomPoint, rightBottomToRight);
}
