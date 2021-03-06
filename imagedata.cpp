/** @file imagedata.cpp
  * Soubor s tridou ImageData dedici ze tridy QObject obsahujici data nactena z fotografie
  */

#include <QDebug>
#include "imagedata.h"

ImageData::ImageData(QObject *parent) :
    QObject(parent)
{
    isGps = 0;
    gpsSource = 1;
    isDateTimeSaved = 1;
    isGpsSaved = 1;

    latitude = 1000;
    longitude = 1001;
    altitude = -1000;
    dateTime = new QDateTime;
    *dateTime = QDateTime::fromString(QString("0000:00:00 00:00:00"), "yyyy:MM:dd hh:mm:ss");


    isDateTimeChanged = 0;
    scaleSize = QSize(288,212);//maximalni zobrazitelne velikost obrazku
    exifRW = new ExifReaderWriter;
    connect(exifRW, SIGNAL(setGps(double,double, double)), this, SLOT(setGps(double,double, double)));
    connect(exifRW, SIGNAL(setDateTime(QDateTime)), this, SLOT(setDateTime(QDateTime)));
    connect(this, SIGNAL(readExif(QString)), exifRW, SLOT(readExif(QString)));
}

int ImageData::loadData(QString pictureFName) {
    pictureName = pictureFName;

    QByteArray type =  QImageReader::imageFormat(pictureName);
    if (type == "") {    //neni obrazek
        return 1;
    }

    scaleImage(pictureName);
    emit(readExif(pictureName));

    return 0;
}

void ImageData::scaleImage(QString pictureName) {
    QImage img(pictureName);
    if(img.isNull()){   //img je null
        return;
    }
    std::unique_ptr<Exiv2::Image> image;

    bool isExif = false;
    try{
#ifdef _WIN32
        image = Exiv2::ImageFactory::open( pictureName.toStdString() );
#else
        image = Exiv2::ImageFactory::open( pictureName.toStdString() );
#endif
        isExif = true;
    }
    catch (Exiv2::Error& e) {
        qDebug() << pictureName << e.what();
        isExif = false;
    }

    if (isExif) {
        image.get();
        image->readMetadata();

        Exiv2::ExifData &exifData =  image->exifData();
        if (!exifData.empty()) {
            Exiv2::ExifKey key("Exif.Image.Orientation");
            Exiv2::ExifData::iterator pos = exifData.findKey(key);
            QTransform rm;
            if(pos != exifData.end()) {
                QString str = exifData["Exif.Image.Orientation"].toString().data();
                switch(str.toInt()){
                    case 3: //obraz otoceny o 180stupnu
                        img = img.transformed(rm.rotate(180), Qt::SmoothTransformation);
                    break;
                    case 6: //obraz otoceny o 90stupnu
                        img = img.transformed(rm.rotate(90), Qt::SmoothTransformation);
                    break;
                    case 8: //obraz otoceny o 280stupnu
                        img = img.transformed(rm.rotate(280), Qt::SmoothTransformation);
                    break;
                    default:
                    break;
                }

            }
        }
    }

    image_small = new QImage;
    *image_small = img.scaled(scaleSize,Qt::KeepAspectRatio, Qt::SmoothTransformation);

}
/**
  *z EXIF
  */
void ImageData::setDateTime(QDateTime dateTimeNew) {
    *(this->dateTime) = dateTimeNew;
    this->originalDateTime = dateTimeNew;
    this->lastDateTimeSaved = dateTimeNew;
    isDateTimeChanged = 0;
}

void ImageData::setGps(double lat, double lon, double alt) {
    latitude = lat;
    longitude = lon;
    altitude = alt;
    gpsSource = 1;
    isGps = 1;
}
