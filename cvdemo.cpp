#include "libfreenect.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>
#include <cv.h>
#include <cxcore.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "beeper.h"

using namespace cv;
using namespace std;


class myMutex {
public:
    myMutex() {
        pthread_mutex_init( &m_mutex, NULL );
    }
    void lock() {
        pthread_mutex_lock( &m_mutex );
    }
    void unlock() {
        pthread_mutex_unlock( &m_mutex );
    }
private:
    pthread_mutex_t m_mutex;
};


class MyFreenectDevice : public Freenect::FreenectDevice {
public:
    MyFreenectDevice(freenect_context *_ctx, int _index)
        : Freenect::FreenectDevice(_ctx, _index), m_buffer_depth(FREENECT_DEPTH_11BIT),
          m_buffer_rgb(FREENECT_VIDEO_RGB), m_gamma(2048), m_new_rgb_frame(false),
          m_new_depth_frame(false), depthMat(Size(640,480),CV_16UC1),
          rgbMat(Size(640,480), CV_8UC3, Scalar(0)),
          ownMat(Size(640,480),CV_8UC3,Scalar(0)) {

        for( unsigned int i = 0 ; i < 2048 ; i++) {
            float v = i/2048.0;
            v = std::pow(v, 3)* 6;
            m_gamma[i] = v*6*256;
        }
    }

    // Do not call directly even in child
    void VideoCallback(void* _rgb, uint32_t timestamp) {
//        std::cout << "RGB callback" << std::endl;
        m_rgb_mutex.lock();
        uint8_t* rgb = static_cast<uint8_t*>(_rgb);
        rgbMat.data = rgb;
        m_new_rgb_frame = true;
        m_rgb_mutex.unlock();
    };

    // Do not call directly even in child
    void DepthCallback(void* _depth, uint32_t timestamp) {
//        std::cout << "Depth callback" << std::endl;
        m_depth_mutex.lock();
        uint16_t* depth = static_cast<uint16_t*>(_depth);
        depthMat.data = (uchar*) depth;
        m_new_depth_frame = true;
        m_depth_mutex.unlock();
    }

    bool getVideo(Mat& output) {
        m_rgb_mutex.lock();
        if(m_new_rgb_frame) {
            cv::cvtColor(rgbMat, output, CV_RGB2BGR);
            m_new_rgb_frame = false;
            m_rgb_mutex.unlock();
            return true;
        } else {
            m_rgb_mutex.unlock();
            return false;
        }
    }

    bool getDepth(Mat& output) {
        m_depth_mutex.lock();
        if(m_new_depth_frame) {
            depthMat.copyTo(output);
            m_new_depth_frame = false;
            m_depth_mutex.unlock();
            return true;
        } else {
            m_depth_mutex.unlock();
            return false;
        }
    }
private:
    std::vector<uint8_t> m_buffer_depth;
    std::vector<uint8_t> m_buffer_rgb;
    std::vector<uint16_t> m_gamma;
    Mat depthMat;
    Mat rgbMat;
    Mat ownMat;
    myMutex m_rgb_mutex;
    myMutex m_depth_mutex;
    bool m_new_rgb_frame;
    bool m_new_depth_frame;
};


int main(int argc, char **argv) {

    string path = "/media/thiago/BigStorage/kinectdata";
    system(("rm -rf " + path + "/depth").c_str());
    system(("rm -rf " + path + "/rgb").c_str());
    system(("mkdir " + path + "/depth").c_str());
    system(("mkdir " + path + "/rgb").c_str());

    bool die(false);
    bool recording = false;

    Mat depthMat(Size(640,480),CV_16UC1);
    Mat depthf (Size(640,480),CV_8UC1);
    Mat rgbMat(Size(640,480),CV_8UC3,Scalar(0));

    // The next two lines must be changed as Freenect::Freenect
    // isn't a template but the method createDevice:
    // Freenect::Freenect<MyFreenectDevice> freenect;
    // MyFreenectDevice& device = freenect.createDevice(0);
    // by these two lines:

    Freenect::Freenect freenect;
    MyFreenectDevice& device = freenect.createDevice<MyFreenectDevice>(0);

    namedWindow("rgb",CV_WINDOW_AUTOSIZE);
    namedWindow("depth",CV_WINDOW_AUTOSIZE);
    device.setDepthFormat(FREENECT_DEPTH_REGISTERED);
    device.setVideoFormat(FREENECT_VIDEO_RGB);
    device.startVideo();
    device.startDepth();
    int frameCount = 0;
    double degrees = 0;
    int max_distance = 800;

    while (!die) {
        device.getVideo(rgbMat);
        device.getDepth(depthMat);
        cv::imshow("rgb", rgbMat);
        threshold(depthMat, depthMat, max_distance, 65535, THRESH_TOZERO_INV);
        depthMat.convertTo(depthf, CV_8UC1, 255.0/2048.0);
        cv::imshow("depth",depthf);
        char k = cvWaitKey(5);
        if( k == 27 ){
            cvDestroyWindow("rgb");
            cvDestroyWindow("depth");
            break;
        }        

        if( k == '+' ) {
            max_distance += 100;
        }

        if( k == '-' ) {
            max_distance -= 100;
        }

        if( k == 'r' ) {
            recording = !recording;
            cerr << recording << endl;
            beepOnce();
        }

        if(k == 'w'){
            degrees += 10;
            device.setTiltDegrees(degrees);
        }

        if(k == 's'){
            degrees -= 10;
            device.setTiltDegrees(degrees);
        }

        if(k == '0'){
            degrees = 0;
            device.setTiltDegrees(degrees);
        }

        if (recording) {            
            std::stringstream ss;
            ss << path + "/depth/";
            ss << setfill('0') << setw(7) << frameCount;
            ss << ".png";
            cv::imwrite(ss.str(), depthMat);

            //limpar o ss
            ss.str(std::string());
            ss.clear();

            ss << path + "/rgb/";
            ss << setfill('0') << setw(7) << frameCount;
            ss << ".png";
            cv::imwrite(ss.str(), rgbMat);
            frameCount++;
        }
    }

    device.stopVideo();
    device.stopDepth();
    return 0;
}
