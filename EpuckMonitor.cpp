/*
 *  EpuckMonitor.cpp
 *  EPuckMonitor
 *
 *  Created by Stefano Morgani on 10/2/08.
 *
 *	Copyright 2008 GCtronic
 *
 *  This file is part of EPuckMonitor.
 *
 *  EPuckMonitor is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  EPuckMonitor is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EPuckMonitor; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 */


#include "EpuckMonitor.h"
#include <vector>
#include <algorithm>

EpuckMonitor::EpuckMonitor(QMainWindow *parent) : QMainWindow(parent)
{
    ui.setupUi(this);
    motorSpeed=500;
    lblCamera = new QLabel();
    type = 1;
    width = 40;
    height = 40;
    pixNum = 3200;
    zoom = 8;
    commThread=NULL;
    isReceiving = false;

    //disable all the graphical objects before the connection is established
    ui.chkSensors->setEnabled(false);
    ui.btnParameters->setEnabled(false);
    ui.btnImage->setEnabled(false);
    ui.btnUp->setEnabled(false);
    ui.btnDown->setEnabled(false);
    ui.btnRight->setEnabled(false);
    ui.btnLeft->setEnabled(false);
    ui.btnStop->setEnabled(false);
    ui.btn1->setEnabled(false);
    ui.btn2->setEnabled(false);
    ui.btn3->setEnabled(false);
    ui.btn4->setEnabled(false);
    ui.btn5->setEnabled(false);
    ui.checkLed0->setEnabled(false);
    ui.checkLed1->setEnabled(false);
    ui.checkLed2->setEnabled(false);
    ui.checkLed3->setEnabled(false);
    ui.checkLed4->setEnabled(false);
    ui.checkLed5->setEnabled(false);
    ui.checkLed6->setEnabled(false);
    ui.checkLed7->setEnabled(false);
    ui.checkLed8->setEnabled(false);
    ui.checkLed9->setEnabled(false);
    ui.sliderVel->setEnabled(false);
    ui.txtHeight->setEnabled(false);
    ui.txtWidth->setEnabled(false);
    ui.txtZoom->setEnabled(false);
    ui.radioColor->setEnabled(false);
    ui.radioGrayscale->setEnabled(false);
    ui.radioAverage->setEnabled(false);
    ui.radioMedian->setEnabled(false);
    ui.radioNonePreFilter->setEnabled(false);
    ui.btnApplyPreFilter->setEnabled(false);
    ui.radioRedFilter->setEnabled(false);
    ui.radioGreenFilter->setEnabled(false);
    ui.radioBlueFilter->setEnabled(false);
    ui.radioNoneFilter->setEnabled(false);
    ui.btnApplyFilter->setEnabled(false);

    glWidget = new GLWidget;
    ui.layoutOpenGL->addWidget(glWidget);
    QObject::connect(this, SIGNAL(new_x_angle(int)), glWidget, SLOT(setXRotation(int)));
    QObject::connect(this, SIGNAL(new_y_angle(int)), glWidget, SLOT(setYRotation(int)));
    QObject::connect(this, SIGNAL(new_z_angle(int)), glWidget, SLOT(setZRotation(int)));

}

EpuckMonitor::~EpuckMonitor()
{
    disconnect();
    return;
}

void EpuckMonitor::connect() {
    emit connectToRobot(ui.txtPort->text().toLatin1().data());
    return;
}

void EpuckMonitor::getColorsForIndex(int index, int &r, int &g, int &b) {
    r = (int)imgBuffer[index * 2] & 0xF8;
    g = (int)(imgBuffer[index * 2] & 0x07) << 5 | (imgBuffer[index * 2 + 1] & 0xE0) >> 3;
    b = (int)(imgBuffer[index * 2 + 1] & 0x1F) << 3;
}

void EpuckMonitor::setColorsAtIndex(int index, int r, int g, int b) {
    cpImgBuffer[index * 2] = (0xF8 & (char)r) | (0x07 & ((char)g >> 5));
    cpImgBuffer[index * 2 + 1] = (0x1F & ((char)b >> 3)) | (0xE0 & ((char)g << 3));
}

void EpuckMonitor::onColorFilter() {
    switch (filter) {
    case 0: { // red
        memcpy(cpImgBuffer, imgBuffer, 4050);
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                int r, g, b;
                int index = i * width + j;
                getColorsForIndex(index, r, g, b);
                int rN = (r - g) + (r - b);
                if (rN < 0) rN = 0; else if (rN > 255) rN = 255;
                setColorsAtIndex(index, rN, 0, 0);
            }
        }
        memcpy(imgBuffer, cpImgBuffer, 4050);
        break;
    }
    case 1: { // green
        memcpy(cpImgBuffer, imgBuffer, 4050);
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                int r, g, b;
                int index = i * width + j;
                getColorsForIndex(index, r, g, b);
                int gN = (g - r) + (g - b);
                if (gN < 0) gN = 0; else if (gN > 255) gN = 255;
                setColorsAtIndex(index, 0, gN, 0);
            }
        }
        memcpy(imgBuffer, cpImgBuffer, 4050);
        break;
    }
    case 2: { // blue
        memcpy(cpImgBuffer, imgBuffer, 4050);
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                int r, g, b;
                int index = i * width + j;
                getColorsForIndex(index, r, g, b);
                int bN = (b - r) + (b - g);
                if (bN < 0) bN = 0; else if (bN > 255) bN = 255;
                setColorsAtIndex(index, 0, 0, bN);
            }
        }
        memcpy(imgBuffer, cpImgBuffer, 4050);
        break;
    }
    }
}

void EpuckMonitor::onPreFilter() {
    switch (preFilter) {
    case 0: { // Average
        memcpy(cpImgBuffer, imgBuffer, 4050);
        for (int i = 1; i < height - 1; ++i) {
            for (int j = 1; j < width - 1; ++j) {
                int rA = 0, gA = 0, bA = 0;
                int indexes[9];
                int factors[9] = {1, 2, 1, 2, 4, 2, 1, 2, 1};
                indexes[0] = (i - 1) * width + (j - 1);
                indexes[1] = (i - 1) * width + j;
                indexes[2] = (i - 1) * width + (j + 1);
                indexes[3] = i * width + (j - 1);
                indexes[4] = i * width + j;
                indexes[5] = i * width + (j + 1);
                indexes[6] = (i + 1) * width + (j - 1);
                indexes[7] = (i + 1) * width + j;
                indexes[8] = (i + 1) * width + (j + 1);
                int weights = 0;
                for (int k = 0; k < 9; ++k) {
                    int r, g, b;
                    getColorsForIndex(indexes[k], r, g, b);
                    rA += factors[k] * r;
                    gA += factors[k] * g;
                    bA += factors[k] * b;
                    weights += factors[k];
                }
                rA /= weights;
                gA /= weights;
                bA /= weights;
                setColorsAtIndex(indexes[4], rA, gA, bA);
            }
        }
        memcpy(imgBuffer, cpImgBuffer, 4050);
        break;
    }
    case 1: { // Median
        memcpy(cpImgBuffer, imgBuffer, 4050);
        int indexes[9];
        for (int i = 0; i < height - 1; ++i) {
            for (int j = 0; j < width - 1; ++j) {
                int rM, gM, bM;
                indexes[0] = (i - 1) * width + (j - 1);
                indexes[1] = (i - 1) * width + j;
                indexes[2] = (i - 1) * width + (j + 1);
                indexes[3] = i * width + (j - 1);
                indexes[4] = i * width + j;
                indexes[5] = i * width + (j + 1);
                indexes[6] = (i + 1) * width + (j - 1);
                indexes[7] = (i + 1) * width + j;
                indexes[8] = (i + 1) * width + (j + 1);
                std::vector<int> rs, gs, bs;
                for (int k = 0; k < 9; ++k) {
                    int r, g, b;
                    getColorsForIndex(indexes[k], r, g, b);
                    rs.push_back(r);
                    gs.push_back(g);
                    bs.push_back(b);
                }
                std::sort(rs.begin(), rs.end());
                std::sort(gs.begin(), gs.end());
                std::sort(bs.begin(), bs.end());
                rM = rs[4];
                gM = gs[4];
                bM = bs[4];
                setColorsAtIndex(indexes[4], rM, gM, bM);
            }
        }
        memcpy(imgBuffer, cpImgBuffer, 4050);
        break;
    }
    }
}

void EpuckMonitor::cameraUpdate() {

    switch(type) {
        case 0: {	//grayscale
                    commThread->getImg(imgBuffer);
                    img = QImage(width, height, QImage::Format_RGB32);
                    int i=0;
                    for(int y=0; y<height; y++) {
                        for(int x=0; x<width; x++) {
                            int r = (int)imgBuffer[i];
                            int g = (int)imgBuffer[i];
                            int b = (int)imgBuffer[i];
                            img.setPixel(x, y, qRgb (r, g, b));
                            i++;
                        }
                    }
                    //the size of the label is 200*200, the maximum size of the image is 45*45
                    lblCamera->setFixedSize(width*200/45, height*200/45);
                    lblCamera->setScaledContents(true);
                    lblCamera->setPixmap(QPixmap::fromImage(img));
                    ui.scrollCamera->setWidget(lblCamera);
                    break;
                }

        case 1: {	// RGB565 image
                    commThread->getImg(imgBuffer);
                    onPreFilter();
                    onColorFilter();
                    img = QImage(width, height, QImage::Format_RGB16);
                    int i=0;
                    for(int y=0; y<height; y++) {
                        for(int x=0; x<width; x++) {
                            int r = (int)imgBuffer[i*2]&0xF8;
                            int g = (int)(imgBuffer[i*2]&0x07)<<5 | (imgBuffer[i*2+1]&0xE0)>>3;
                            int b = (int)(imgBuffer[i*2+1]&0x1F)<<3;
                            img.setPixel(x, y, qRgb (r, g, b));
                            i++;
                        }
                    }
                    lblCamera->setFixedSize(width*200/45, height*200/45);
                    lblCamera->setScaledContents(true);
                    lblCamera->setPixmap(QPixmap::fromImage(img));
                    ui.scrollCamera->setWidget(lblCamera);
                    break;
                }

        default: break;
    }

    return;

}

void EpuckMonitor::binarySensorsUpdate() {

    //accelerometer
    ui.lcdAcceleration->display(commThread->getAcceleration());
    double orientRad = commThread->getOrientation()/180.0*M_PI;
    emit new_y_angle(-sin(orientRad)*commThread->getInclination());
    emit new_x_angle(cos(orientRad)*commThread->getInclination());

    //emit new_x_angle(int(atan2((double)commThread->getAccZ(),(double)commThread->getAccY())/M_PI*180.0)); //pitch
    //emit new_y_angle(int(atan2((double)commThread->getAccZ(),(double)commThread->getAccX())/M_PI*180.0)); //roll

    //proximity sensors
    ui.progressIR0->setValue(commThread->getIr0());
    ui.progressIR1->setValue(commThread->getIr1());
    ui.progressIR2->setValue(commThread->getIr2());
    ui.progressIR3->setValue(commThread->getIr3());
    ui.progressIR4->setValue(commThread->getIr4());
    ui.progressIR5->setValue(commThread->getIr5());
    ui.progressIR6->setValue(commThread->getIr6());
    ui.progressIR7->setValue(commThread->getIr7());
    //light sensor data
    memset(backgroundColor, 0x0, 100);
    sprintf(backgroundColor, "background-color: rgb(%d, %d, %d);", 255-int((double)commThread->getLight()/4000.0*255.0), 255-int((double)commThread->getLight()/4000.0*255.0), 255-int((double)commThread->getLight()/4000.0*255.0));
    ui.lblLight->setStyleSheet(backgroundColor);
    //microphone data
    ui.progressMic0->setValue(commThread->getMic0());
    ui.progressMic1->setValue(commThread->getMic1());
    ui.progressMic2->setValue(commThread->getMic2());

    return;
}

void EpuckMonitor::asciiSensorsUpdate() {

    //selector data
    ui.lblSelector->setText(commThread->getSelector());
    ui.dialSelector->setValue(atoi(commThread->getSelector()));
    //IR data
    ui.lblIRcheck->setText(commThread->getIrCheck());
    ui.lblIRaddress->setText(commThread->getIrAddress());
    ui.lblIRdata->setText(commThread->getIrData());

    return;
}

void EpuckMonitor::updateParameters() {

    width = ui.txtWidth->text().toInt();
    height = ui.txtHeight->text().toInt();
    zoom = ui.txtZoom->text().toInt();

    if(ui.radioColor->isChecked()) {
        if((width*height)>2025) {	//at most 2025 pixels allowed
            printMessage("Height and Width not allowed, at most 2025 pixels!");
            return;
        } else {
            type = 1;
        }
    }
    else if(ui.radioGrayscale->isChecked()) {
        if((width*height)>2025) {
            printMessage("Height and Width not allowed, at most 2025 pixels!");
            return;
        } else {
            type = 0;
        }
    }

    emit newParameters(type, width, height, zoom);

    return;
}

void EpuckMonitor::applyFilter() {
    if (ui.radioRedFilter->isChecked()) {
        filter = 0;
    } else if (ui.radioGreenFilter->isChecked()) {
        filter = 1;
    } else if (ui.radioBlueFilter->isChecked()) {
        filter = 2;
    } else if (ui.radioNoneFilter->isChecked()) {
        filter = -1;
    }
    return;
}

void EpuckMonitor::applyPreFilter() {
    if (ui.radioAverage->isChecked()) {
        preFilter = 0;
    } else if (ui.radioMedian->isChecked()) {
        preFilter = 1;
    } else if (ui.radioNonePreFilter->isChecked()) {
        preFilter = -1;
    }
    return;
}

void EpuckMonitor::goUp() {

    int speed_left = motorSpeed;
    char high_left = (speed_left>>8) & 0xFF;
    char low_left = speed_left & 0xFF;
    int speed_right = motorSpeed;
    char high_right = (speed_right>>8) & 0xFF;
    char low_right = speed_right & 0xFF;

    emit moveForward(motorSpeed);

    return;
}

void EpuckMonitor::goDown() {

    int speed_left = -motorSpeed;
    char high_left = (speed_left>>8) & 0xFF;
    char low_left = speed_left & 0xFF;
    int speed_right = -motorSpeed;
    char high_right = (speed_right>>8) & 0xFF;
    char low_right = speed_right & 0xFF;

    emit moveBackward(motorSpeed);

    return;
}

void EpuckMonitor::goLeft() {

    int speed_left = -motorSpeed;
    char high_left = (speed_left>>8) & 0xFF;
    char low_left = speed_left & 0xFF;
    int speed_right = motorSpeed;
    char high_right = (speed_right>>8) & 0xFF;
    char low_right = speed_right & 0xFF;

    emit moveLeft(motorSpeed);

    return;
}

void EpuckMonitor::goRight() {

    int speed_left = motorSpeed;
    char high_left = (speed_left>>8) & 0xFF;
    char low_left = speed_left & 0xFF;
    int speed_right = -motorSpeed;
    char high_right = (speed_right>>8) & 0xFF;
    char low_right = speed_right & 0xFF;

    emit moveRight(motorSpeed);

    return;
}

void EpuckMonitor::updateSpeed() {
    motorSpeed = ui.sliderVel->value();
}

void EpuckMonitor::disconnect() {

    commThread->getSensors(false);
    commThread->getCamera(false);
    commThread->abortThread = true;
    commThread->wait();

    //disable all graphical objects
    ui.btnConnect->setEnabled(true);
    ui.btnDisconnect->setEnabled(false);
    ui.chkSensors->setEnabled(false);
    ui.btnParameters->setEnabled(false);
    ui.btnImage->setEnabled(false);
    ui.btnUp->setEnabled(false);
    ui.btnDown->setEnabled(false);
    ui.btnRight->setEnabled(false);
    ui.btnLeft->setEnabled(false);
    ui.btnStop->setEnabled(false);
    ui.btn1->setEnabled(false);
    ui.btn2->setEnabled(false);
    ui.btn3->setEnabled(false);
    ui.btn4->setEnabled(false);
    ui.btn5->setEnabled(false);
    ui.checkLed0->setEnabled(false);
    ui.checkLed1->setEnabled(false);
    ui.checkLed2->setEnabled(false);
    ui.checkLed3->setEnabled(false);
    ui.checkLed4->setEnabled(false);
    ui.checkLed5->setEnabled(false);
    ui.checkLed6->setEnabled(false);
    ui.checkLed7->setEnabled(false);
    ui.checkLed8->setEnabled(false);
    ui.checkLed9->setEnabled(false);
    ui.sliderVel->setEnabled(false);
    ui.txtHeight->setEnabled(false);
    ui.txtWidth->setEnabled(false);
    ui.txtZoom->setEnabled(false);
    ui.radioColor->setEnabled(false);
    ui.radioGrayscale->setEnabled(false);
    ui.radioAverage->setEnabled(false);
    ui.radioMedian->setEnabled(false);
    ui.radioNonePreFilter->setEnabled(false);
    ui.btnApplyPreFilter->setEnabled(false);
    ui.radioRedFilter->setEnabled(false);
    ui.radioGreenFilter->setEnabled(false);
    ui.radioBlueFilter->setEnabled(false);
    ui.radioNoneFilter->setEnabled(false);
    ui.btnApplyFilter->setEnabled(false);
    ui.btnImage->setText("Start receiving images");

    return;
}

void EpuckMonitor::sensorActivation(int state) {

    if(state == Qt::Checked) {
        commThread->getSensors(true);
    } else {
        commThread->getSensors(false);
    }

    return;
}

void EpuckMonitor::getImages() {
    if(isReceiving) {
        isReceiving = false;
        ui.btnImage->setText("Start receiving images");
        commThread->getCamera(false);
    } else {
        isReceiving = true;
        ui.btnImage->setText("Stop receiving images");
        commThread->getCamera(true);
    }

    return;
}

void EpuckMonitor::printMessage(QString s) {
    QMessageBox::critical( this, "E-Puck Monitor", s);
}

void EpuckMonitor::setCommThread(CommThread *thread) {
    commThread = thread;
    QObject::connect(commThread, SIGNAL(newBinaryData()), this, SLOT(binarySensorsUpdate()));
    QObject::connect(commThread, SIGNAL(newAsciiData()), this, SLOT(asciiSensorsUpdate()));
    QObject::connect(commThread, SIGNAL(newImage()), this, SLOT(cameraUpdate()));
}

void EpuckMonitor::portOpened() {
    ui.btnConnect->setEnabled(false);
    ui.btnDisconnect->setEnabled(true);
    ui.chkSensors->setCheckState(Qt::Checked);

    //enable all the graphical objects after the connection was established
    ui.chkSensors->setEnabled(true);
    ui.btnParameters->setEnabled(true);
    ui.btnImage->setEnabled(true);
    ui.btnUp->setEnabled(true);
    ui.btnDown->setEnabled(true);
    ui.btnRight->setEnabled(true);
    ui.btnLeft->setEnabled(true);
    ui.btnStop->setEnabled(true);
    ui.btn1->setEnabled(true);
    ui.btn2->setEnabled(true);
    ui.btn3->setEnabled(true);
    ui.btn4->setEnabled(true);
    ui.btn5->setEnabled(true);
    ui.checkLed0->setEnabled(true);
    ui.checkLed1->setEnabled(true);
    ui.checkLed2->setEnabled(true);
    ui.checkLed3->setEnabled(true);
    ui.checkLed4->setEnabled(true);
    ui.checkLed5->setEnabled(true);
    ui.checkLed6->setEnabled(true);
    ui.checkLed7->setEnabled(true);
    ui.checkLed8->setEnabled(true);
    ui.checkLed9->setEnabled(true);
    ui.sliderVel->setEnabled(true);
    ui.txtHeight->setEnabled(true);
    ui.txtWidth->setEnabled(true);
    ui.txtZoom->setEnabled(true);
    ui.radioColor->setEnabled(true);
    ui.radioGrayscale->setEnabled(true);
    ui.radioAverage->setEnabled(true);
    ui.radioMedian->setEnabled(true);
    ui.radioNonePreFilter->setEnabled(true);
    ui.btnApplyPreFilter->setEnabled(true);
    ui.radioRedFilter->setEnabled(true);
    ui.radioGreenFilter->setEnabled(true);
    ui.radioBlueFilter->setEnabled(true);
    ui.radioNoneFilter->setEnabled(true);
    ui.btnApplyFilter->setEnabled(true);
}


