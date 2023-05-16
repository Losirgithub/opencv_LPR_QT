#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_pTimer = new QTimer(this);
    m_pTimer->setInterval(30);  //定时30毫秒读取一帧数据
    connect(m_pTimer, &QTimer::timeout, this, &MainWindow::playTimer);
    ui->pushButton_play->setEnabled(true);
    canSegChar=false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event){
    //关闭窗口提示
    int result = QMessageBox::warning(this,"Exit",
    "Are you sure you want to close this program?.",
    QMessageBox::Yes,
    QMessageBox::No);
    if(result==QMessageBox::Yes)
        event->accept();
    else event->ignore();


}


void MainWindow::on_pushButton_input_pressed()
{
//    QString fileName = QFileDialog::getOpenFileName (
//                this,
//                "Open Input Image",
//                QDir::currentPath(),
//    "Images (*.jpg *.png *.bmp) ");//选择图片
    QString fileName = QFileDialog::getOpenFileName (
                this,
                "Open Input File",
                QDir::currentPath(),
    "Video(*.avi *.mp4)");//设置可见文件格式，如果是所有种类就用*
    if (QFile::exists(fileName))
    {
        ui->lineEdit->setText (fileName);
    }
    //cv::Mat src = cv::imread(fileName.toStdString());//读取图片
    //cv::imshow(fileName.toStdString(),src);
   // QImage image(src.data,src.cols,src.rows,src.step,QImage::Format_RGB888);
   // ui->label_video->setPixmap(QPixmap::fromImage(image.rgbSwapped()));
   // ui->label_video->resize(image.size());
    //ui->label_video->show();
    //cv::waitKey(0);
    //cv::destroyAllWindows();

    VideoCapture cap(fileName.toStdString());//读取视频

    if(!cap.isOpened())
    {
        QMessageBox::warning(this,"错误",
            "未读取到视频");
    }
    else
    {
        m_pVideo=new VideoCapture(fileName.toStdString());

//        QImage qImg = cvMat2QImage(src);
//        QPixmap pixmap = QPixmap::fromImage(qImg);
//        int width=ui->label_video->width();
//        int height=ui->label_video->height();
//        QPixmap fitpixmap=pixmap.scaled(width,height,Qt::KeepAspectRatio, Qt::SmoothTransformation);
//        ui->label_video->setPixmap(fitpixmap);
    }
}

/************Mat转QImage函数*****************************************************************************************/
/***************************************************************************************************************************************/
QImage MainWindow::cvMat2QImage(const Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage qimage(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        qimage.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            qimage.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = qimage.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return qimage;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        return QImage();
    }
}
//适配label宽高的qpixmap
QPixmap MainWindow::FitQPixmap(const QImage& qImg,int h,int w)
{
    QPixmap pixmap = QPixmap::fromImage(qImg);

    QPixmap fitpixmap=pixmap.scaled(w,h,Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return fitpixmap;

}
//播放按钮
void MainWindow::on_pushButton_play_clicked()
{
    m_pTimer->start();
    ui->pushButton_play->setEnabled(false);
}
//帧处理函数,在QTimer开始后进行调用播放
void MainWindow::playTimer()
{
    Mat frame;

       //从cap中读取一帧数据，存到fram中
       *m_pVideo >> frame;
       if ( frame.empty() )
       {
           ui->pushButton_play->setEnabled(true);
           return;
       }
       //处理关键帧
       // 把frame转换到hsv色彩空间
        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        //设置hsv的蓝色范围
        cv::Vec3b lower_blue(110, 50, 50);
        cv::Vec3b upper_blue(130, 255, 255);

        // 二值化只包含蓝色和黑色部分
        cv::Mat mask;
        cv::inRange(hsv, lower_blue, upper_blue, mask);

        //形态学处理，消除小块噪声
        dilate(mask, mask, getStructuringElement(MORPH_RECT, Size(3, 3)), Point(-1, -1), 2);
        erode(mask, mask, getStructuringElement(MORPH_RECT, Size(3, 3)), Point(-1, -1), 1);

        //显示形态学处理结果
        vector<vector<cv::Point>> vpp;
        vector<Vec4i> hierarchy;
        findContours(mask, vpp, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);//找轮廓

        RotatedRect max_r_rect;//最大矩形
        for (auto& i : vpp)
        {
          RotatedRect r_rect = minAreaRect(i);
          if (r_rect.size.area() > max_r_rect.size.area())
          {
              max_r_rect = r_rect;

          }
          //cout << "面积:--------------" << r_rect.size << endl;
        }


        if (max_r_rect.size.area() > 5500) {//找到车牌
          Mat mask_r;
          Rect mask_rect = max_r_rect.boundingRect();
          Mat lic_r = frame(mask_rect).clone();

          //显示图片
          //显示关键帧
          QImage qImg = cvMat2QImage(frame);
          QPixmap pixmap = QPixmap::fromImage(qImg);
          int width=ui->label_keyframe->width();
          int height=ui->label_keyframe->height();
          QPixmap fitpixmap=pixmap.scaled(width,height,Qt::KeepAspectRatio, Qt::SmoothTransformation);
          ui->label_keyframe->setPixmap(fitpixmap);
          //显示车牌
          QImage qLic = cvMat2QImage(lic_r);
          w_lic=ui->lp_img->width();
          h_lic=ui->lp_img->height();
          ui->lp_img->setPixmap(FitQPixmap(qLic,h_lic,w_lic));

          lic=new Mat(lic_r);
          QMessageBox::information(this,"提示",
              "已定位关键帧和车牌位置");
          canSegChar=true;
          m_pTimer->stop();
        }

        // mask和原图与运算，获取车牌部分
        cv::Mat res;
        cv::bitwise_and(frame, frame, res, mask);

       //显示图片
        QImage qImg = cvMat2QImage(frame);
        int w=ui->label_video->width();
        int h=ui->label_video->height();
        ui->label_video->setPixmap(FitQPixmap(qImg,h,w));
}
//形态学处理和裁切
void MainWindow::on_pushButton_seg_clicked()
{
    //当前路径在debug文件夹下
    //QString fileName = QCoreApplication::applicationDirPath();//显示相对路径

    if(canSegChar==false)
    {
        QMessageBox::warning(this,"error",
                             "未定位车牌，还不能进行分割");
    }
    else
    {

        Mat match_img = (*lic).clone();
        cvtColor(match_img, match_img, COLOR_BGR2GRAY);
        threshold(match_img, match_img, 100, 255, THRESH_BINARY);

        //裁切
        int height = match_img.size().height;
        int width = match_img.size().width;
        Mat clip_bin_lic = match_img(Range(0.1 * height, 0.85 * height), Range(0, width));//裁切的车牌


        //Mat no_morph = clip_bin_lic.clone();//未经过形态学处理的车牌
        m_no_morph_lic=new Mat(clip_bin_lic.clone());
        //imshow("clip", clip_bin_lic);
        dilate(clip_bin_lic, clip_bin_lic, getStructuringElement(MORPH_RECT,Size(2, 2)), Point(-1, -1), 2);
        m_morph_lic=new Mat(clip_bin_lic);//创建共公有变量
        //显示形态学和裁切后的图片
        QImage qImg_morph_clip=cvMat2QImage(*m_morph_lic);
        ui->label_morph->setPixmap(FitQPixmap(qImg_morph_clip,h_lic,w_lic));

    }
}



//字符分割
void MainWindow::on_pushButton_charseg_clicked()
{
    Mat clip_bin_lic=(*m_morph_lic).clone();//此为形态学和裁剪处理后的图片

    //寻找轮廓
           vector<vector<cv::Point>> contours;
           vector<Vec4i> hier;

           findContours(clip_bin_lic, contours, hier, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));   //寻找轮廓

          Mat img=clip_bin_lic.clone();
          cvtColor(img, img, COLOR_GRAY2RGB);

          //绘制轮廓
        // drawContours(img, contours, -1, Scalar(0, 255, 0), 1);
        // imshow("contours",img);
          cout<<"Size================="<<img.size();

          //分割各个字符
          int sum=0;
          for (auto& i : contours) {
              Rect rect = boundingRect(i);
              cout << "面积:" << rect.area() << endl;
              sum += rect.area();
          }

          cout << "平均面积" << sum / contours.size() << endl;
          vector<vector<Point>> char_contours;//字符轮廓
          vector<Rect> vec_rect;//字符的矩形信息
          for (auto& i : contours) {//选择较大的7个
              Rect rect = boundingRect(i);
              if (rect.area() > sum / contours.size()-50)
              {
                  char_contours.push_back(i);
                  vec_rect.push_back(rect);
              }
          }
          drawContours(img, char_contours, -1, Scalar(0, 0, 255), 2);
          //imshow("contours", img);
          for (auto& i : vec_rect)
          {
              cout << "x：" << i.x << "y:" << i.y << endl;
          }

          //对字符矩形排序，根据x坐标从左到右
          std::sort(vec_rect.begin(), vec_rect.end(), [](const Rect& a, const Rect& b)
              {
                  return a.x < b.x;
              });

          cout << "排序后:" << endl;
          for (auto& i : vec_rect)
          {
              cout << "x：" << i.x << "y:" << i.y << endl;
              cout << "宽度:" << i.width << endl;
          }
          Mat no_morph=(*m_morph_lic).clone();

          Mat lic_char[7];
          QImage qlic_char[7];
          QPixmap qfitchar[7];
          int h=ui->lp_char1->height();
          int w=ui->lp_char1->width();
          for(int i=0;i<7;i++)
          {
              lic_char[i]=no_morph(vec_rect[i]);
              mlic_char[i]=lic_char[i];
              imwrite(to_string(i)+".jpg",mlic_char[i]);//保存字符
              qlic_char[i]=cvMat2QImage(lic_char[i]);
              qfitchar[i]=FitQPixmap(qlic_char[i],h,w);
          }

            ui->lp_seg2->setPixmap(qfitchar[1]);
            ui->lp_seg3->setPixmap(qfitchar[2]);
            ui->lp_seg4->setPixmap(qfitchar[3]);
            ui->lp_seg5->setPixmap(qfitchar[4]);
            ui->lp_seg6->setPixmap(qfitchar[5]);
            ui->lp_seg7->setPixmap(qfitchar[6]);
            //对第一个汉字特殊处理
            Point tl=vec_rect[0].tl();
            Point br=vec_rect[0].br();
            tl.y-=6;
            imwrite("0.jpg",clip_bin_lic(Rect(tl,br)));
            qlic_char[0]=cvMat2QImage(clip_bin_lic(Rect(tl,br)));
            qfitchar[0]=FitQPixmap(qlic_char[0],h,w);
            ui->lp_seg1->setPixmap(qfitchar[0]);

            QMessageBox::information(this,"提示",
                                 "分割字符已保存为0.jpg~6.jpg");

            //测试分割是否正确
          //imshow("0", clip_bin_lic(Rect(tl,br)));
          //waitKey(0);
//          imshow("1", clip_bin_lic(vec_rect[1]));
//          waitKey(0);
//          imshow("2", clip_bin_lic(vec_rect[2]));
//          waitKey(0);
//          imshow("3", clip_bin_lic(vec_rect[3]));
//          waitKey(0);
//          imshow("4", clip_bin_lic(vec_rect[4]));
//          waitKey(0);
//          imshow("5", clip_bin_lic(vec_rect[5]));
//          waitKey(0);
//          imshow("6", clip_bin_lic(vec_rect[6]));
//          waitKey(0);

//          imshow("0", no_morph(vec_rect[0]));
//          imshow("1", no_morph(vec_rect[1]));
//          imshow("2", no_morph(vec_rect[2]));
//          imshow("3", no_morph(vec_rect[3]));
//          imshow("4", no_morph(vec_rect[4]));
//          imshow("5", no_morph(vec_rect[5]));
//          imshow("6", no_morph(vec_rect[6]));
}

//识别字母数字
void MainWindow::on_pushButton_template_clicked()
{
    //Mat match_img=imread("2.jpg");
//    cvtColor(match_img, match_img, COLOR_BGR2GRAY);
//    threshold(match_img, match_img, 100, 255, THRESH_BINARY);
    string num_dir_name[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                           "A", "B", "C", "D", "E", "F", "G", "H",  "J",
                           "K", "L", "M", "N", "P", "Q", "R", "S", "T",
                           "U", "V", "W", "X", "Y", "Z" };


    Mat match_imgs[7];
    for(int i=0;i<7;i++)//读取的图片虽然看着是黑白但仍是bgr读取的，需要再进行灰度化和二值化
    {
        match_imgs[i]=imread(to_string(i)+".jpg");
        cvtColor(match_imgs[i], match_imgs[i], COLOR_BGR2GRAY);
        threshold(match_imgs[i], match_imgs[i], 100, 255, THRESH_BINARY);
    }
    QString qlic_char[7];
    for(int i=0;i<7;i++)
    {
        Mat match_img=match_imgs[i].clone();
        vector<float> max_value;
        vector<string> lic_char_name;
        for (auto i :num_dir_name) {
            //cv::String pattern = "province\\"+i+"\\*";
            string pattern="num/"+i+"/*.jpg";
            std::vector<cv::String> fn;
            std::vector<cv::Mat> img_list;
            cv::glob(pattern, fn);

            int count = fn.size(); //当前文件夹中图片个数
            for (int j = 0; j < count;j++)
            {
               /* Mat t_img = imread(fn[j]);
                cvtColor(t_img, t_img, COLOR_BGR2GRAY);
                threshold(t_img, t_img, 100, 255, THRESH_BINARY);*/
                img_list.push_back(imread(fn[j]));//上面是浅拷贝，不能正确处理

            }


            //模板匹配
            vector<float> result;
            for (auto& j : img_list) {

                Size size=j.size();
                Mat r;

               cvtColor(j, j, COLOR_BGR2GRAY);
               threshold(j, j, 100, 255, THRESH_BINARY);
               cv::resize(match_img, match_img, size);//必须尺寸一致

                matchTemplate(match_img, j, r, TM_CCOEFF_NORMED);
                // 找到最佳匹配
                double min_val, max_val;
                Point min_loc, max_loc;
                minMaxLoc(r, &min_val, &max_val, &min_loc, &max_loc);
                //cout << max_val << endl;
                result.push_back(max_val);
            }

            auto max_temp=max_element(result.begin(), result.end());//获取此文件夹内最大匹配值
           // cout << "当前文件夹最大值"<< * max_temp << endl;
            max_value.push_back(*max_temp);
            lic_char_name.push_back(i);//存储文件夹名
        }
        for (auto i : max_value)
        {
            cout << i << endl;
        }
        auto it = std::max_element(std::begin(max_value), std::end(max_value));
        size_t ind = std::distance(std::begin(max_value), it);
        //int ind = distance(max_value.begin(), max_element(max_value.begin(), max_value.end()));
        cout << "最大值下标ind为" << ind << endl;
        cout << "最大值" << *max_element(max_value.begin(), max_value.end()) << "      该字符是" << lic_char_name[ind] << endl;

        qlic_char[i]=QString::fromStdString(lic_char_name[ind]);
    }
    //对汉字单独处理
    ui->lp_char2->setText(qlic_char[1]);
    ui->lp_char3->setText(qlic_char[2]);
    ui->lp_char4->setText(qlic_char[3]);
    ui->lp_char5->setText(qlic_char[4]);
    ui->lp_char6->setText(qlic_char[5]);
    ui->lp_char7->setText(qlic_char[6]);
}

//识别省份
void MainWindow::on_pushButton_province_clicked()
{
    QString Qprovince_dir_name[] = { "京", "津", "冀", "晋", "蒙", "辽", "吉", "黑", "沪", "苏",
                          "浙", "皖", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤", "桂",
                          "琼", "川", "贵", "云", "藏", "陕", "甘", "青", "宁", "新",
                          "渝" };

    std::string province_dir_name[31];

    for(int i=0;i<31;i++)
    {
        QString FileName=Qprovince_dir_name[i];
        province_dir_name[i]=FileName.toLocal8Bit().toStdString();

    }

    Mat match_img=imread("0.jpg");
        cvtColor(match_img, match_img, COLOR_BGR2GRAY);
        threshold(match_img, match_img, 100, 255, THRESH_BINARY);
    QString qlic_char;

        vector<float> max_value;
        vector<string> lic_char_name;
        for (auto i :province_dir_name) {
            //cv::String pattern = "province\\"+i+"\\*";
            string pattern="province/"+i+"/*.jpg";
            std::vector<cv::String> fn;
            std::vector<cv::Mat> img_list;
            cv::glob(pattern, fn);

            int count = fn.size(); //number of png files in images folder
            for (int j = 0; j < count;j++)
            {
               /* Mat t_img = imread(fn[j]);
                cvtColor(t_img, t_img, COLOR_BGR2GRAY);
                threshold(t_img, t_img, 100, 255, THRESH_BINARY);*/
                img_list.push_back(imread(fn[j]));//上面是浅拷贝

            }


            //模板匹配
            vector<float> result;
            for (auto& j : img_list) {

                Size size=j.size();

                Mat r;

               cvtColor(j, j, COLOR_BGR2GRAY);
               threshold(j, j, 100, 255, THRESH_BINARY);
               cv::resize(match_img, match_img, size);//尺寸一致

                matchTemplate(match_img, j, r, TM_CCOEFF_NORMED);
                // 找到最佳匹配
                double min_val, max_val;
                Point min_loc, max_loc;
                minMaxLoc(r, &min_val, &max_val, &min_loc, &max_loc);
                //cout << max_val << endl;
                result.push_back(max_val);
            }

            auto max_temp=max_element(result.begin(), result.end());//获取此文件夹内最大匹配值
           // cout << "当前文件夹最大值"<< * max_temp << endl;
            max_value.push_back(*max_temp);
            lic_char_name.push_back(i);//存储文件夹名
        }
        for (auto i : max_value)
        {
            cout << i << endl;
        }
        auto it = std::max_element(std::begin(max_value), std::end(max_value));
        size_t ind = std::distance(std::begin(max_value), it);
        //int ind = distance(max_value.begin(), max_element(max_value.begin(), max_value.end()));
        cout << "最大值下标ind为" << ind << endl;
        cout << "最大值" << *max_element(max_value.begin(), max_value.end()) << "      该字符是" << Qprovince_dir_name[ind].toStdString() << endl;
        qlic_char=Qprovince_dir_name[ind];
        ui->lp_char1->setText(qlic_char);
}

