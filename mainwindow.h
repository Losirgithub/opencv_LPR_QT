#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QFileDialog>
#include<QMessageBox>
#include<QEvent>
#include<QCloseEvent>
#include<QPixmap>
#include<QTimer>
#include<vector>
#include<algorithm>
#include<windows.h>
#include<iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);
    QImage cvMat2QImage(const Mat& mat);
    Mat frame;//处理帧
    QPixmap FitQPixmap(const QImage& qImg,int h,int w);
    bool canSegChar;
    int w_lic;//车牌显示宽度
    int h_lic;//车牌显示高度
private slots:

    void on_pushButton_input_pressed();

    void on_pushButton_play_clicked();
    void playTimer();
    void on_pushButton_seg_clicked();

    //void on_pushButton_morph_clicked();

    void on_pushButton_charseg_clicked();

    void on_pushButton_template_clicked();

    void on_pushButton_province_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *m_pTimer;
    VideoCapture *m_pVideo;
    Mat *lic;
    Mat *m_morph_lic;
    Mat *m_no_morph_lic;
    Mat mlic_char[7];
};
#endif // MAINWINDOW_H
