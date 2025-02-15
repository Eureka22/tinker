#include "Recognizer.h"
#include <iostream>
#include <opencv/highgui.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>
#include <FaceTracker/Tracker.h>
#include <math.h>
#include "SRC/utils.hpp"
#include <ros/package.h>
#include <ros/ros.h>
#include "std_msgs/String.h"
#include <fstream>
#include "reading_image/reading_image.h"
void Recognizer::callBack(const std_msgs::String::ConstPtr& msg)
{
    printf ("%s\n",msg->data.c_str());
    string cmd = msg->data;
    if (cmd == "hello") {
        this->findface = false;
        this->model = false;
        this->state = TESTING;  
        printf("testing\n");
        if (!this->archive) {
            _buildarchive();
            archive = true;
            }
        else this->recognize = false;
        this->count = 0;
        
    }
    else 
    {
        string name = cmd.substr(cmd.find("name is")+8,cmd.length()-6);
        this->guestname = name;
        this->guestpicnum = 0;
        printf("%s\n",name.c_str());
        string echo = "espeak --stdout 'Good evening "+name+" please wait a moment ' | aplay"; 
        printf("%s\n",echo.c_str());
        system(echo.c_str());
        this->findface = false;
        this->model = false;
    }
    
    
    
}


int Recognizer::_buildarchive()
{
	const string train_sample_list_file= this->package_path_+"archive/list_train";
	const size_t n_subject_samples= 2;
	const int sample_width= 80;
	const int sample_height= 100;
	CvSize sample_size= cvSize(sample_width, sample_height);
	const string src_model_file= this->package_path_+"model/sample.sr";
    //TODO:
	vector<string> train_sample_list;
	
	LoadSampleList(train_sample_list_file, &train_sample_list);
	std::cout<<train_sample_list.size()<<std::endl;
	assert(train_sample_list.size() % n_subject_samples == 0);
	std::cout<<"image file loaded"<<std::endl;
	
	SRCModel *src= TrainSRCModel(train_sample_list, sample_size, n_subject_samples);
	
	SaveSRCModel(src, src_model_file);
	//reload
	this->src_model = LoadSRCModel(src_model_file);
	ReleaseSRCModel(&src);	
	printf("saveing model finished\n");
	return 0;
}


void Recognizer::SaveImage(cv::Mat &image,cv::Mat &shape,cv::Mat &con,cv::Mat &tri,cv::Mat &visi,std::string filename)
{
  int i,n = shape.rows/2;  cv::Scalar c;
	cv::Point p[100];
  //draw triangulation
  c = CV_RGB(0,0,0);
  //draw points
  for(i = 0; i < n; i++){    
    if(visi.at<int>(i,0) == 0)continue;
    p[i] = cv::Point(shape.at<double>(i,0),shape.at<double>(i+n,0));
    c = CV_RGB(255,255,0); //cv::circle(image,p[i],2,c);
	char *text = new char[10];
	sprintf(text,"%d",i);
	//cv::putText(image,text,p[i],
	//CV_FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(255,255,255));
		delete []text;
  }
  cv::Point left,right,bottom,leftbrow,rightbrow,nose;
  for (int i = 36;i<42;i++){
	  left.x+=p[i].x;
	  left.y+=p[i].y;
  }
  left.x/=6;
  left.y/=6;
  
  for (int i = 42;i<48;i++){
	  right.x+=p[i].x;
	  right.y+=p[i].y;
  }
  right.x/=6;
  right.y/=6;
  
  for (int i = 7;i<10;i++){
	  bottom.x+=p[i].x;
	  bottom.y+=p[i].y;
  }
  bottom.x/=3;
  bottom.y/=3;
  
  for (int i = 18;i<21;i++){
	  leftbrow.x+=p[i].x;
	  leftbrow.y+=p[i].y;
  }
  leftbrow.x/=3;
  leftbrow.y/=3;
  
  for (int i = 23;i<26;i++){
	  rightbrow.x+=p[i].x;
	  rightbrow.y+=p[i].y;
  }
  rightbrow.x/=3;
  rightbrow.y/=3;
  
  for (int i = 31;i<36;i++){
	  nose.x+=p[i].x;
	  nose.y+=p[i].y;
  }
  nose.x/=5;
  nose.y/=5;
 
  
  /*cv::circle(image,left,4,c);
  cv::circle(image,right,4,c);
  cv::circle(image,bottom,4,c);
  cv::circle(image,leftbrow,4,c);
  cv::circle(image,rightbrow,4,c);
  cv::circle(image,nose,4,c);
  */
  //affine
  /*cv::Point center = nose;
  int dist = right.x - left.x;
  double angle = -atan((double)(right.y-left.y)/(double)(left.x-right.x))/3.1415*180;
  printf("%lf\n",angle);
  double scale = 1.0; // 缩放尺度  
  cv::Mat rotateMat;   
  rotateMat = cv::getRotationMatrix2D(center, angle, scale);  	  
  cv::Mat rotateImg;  
  cv::warpAffine(image, rotateImg, rotateMat, image.size());  
  cv::Mat image_roi = rotateImg(cvRect(nose.x - dist, nose.y - dist*1.3, 2*dist,2*dist));
  */
  cv::Point2f srcTri[3];
  cv::Point2f dstTri[3];

  cv::Mat rot_mat( 2, 3, CV_32FC1 );
  cv::Mat warp_mat( 2, 3, CV_32FC1 );
  cv::Mat  warp_dst, warp_rotate_dst;
  warp_dst = cv::Mat::zeros( image.rows, image.cols, image.type() );
  srcTri[0] = left;
  srcTri[1] = right;
  srcTri[2] = bottom;
  
  dstTri[0] = cv::Point2f( image.cols*0.45, image.rows*0.5 );
  dstTri[1] = cv::Point2f( image.cols*0.55, image.rows*0.5 );
  dstTri[2] = cv::Point2f( image.cols*0.5, image.rows*0.85 );
  
  warp_mat = getAffineTransform( srcTri, dstTri );
  printf("%s\n","Affine");
  warpAffine( image, warp_dst, warp_mat, warp_dst.size() );
  cv::Rect region_of_interest = cv::Rect(dstTri[0].x-50,dstTri[0].y-50,dstTri[1].x-dstTri[0].x+100,dstTri[2].y-dstTri[0].y+50);
  cv::Mat image_roi = warp_dst(region_of_interest);
   printf("%s\n","ROI");
  cv::Mat image_roi_equalization;
  equalizeHist(image_roi,image_roi_equalization);
  imshow( "test", image_roi );
  imshow("test_equ",image_roi_equalization);
  imwrite(filename.c_str(),image_roi_equalization);
  printf("%s\n",filename.c_str());
  return;
}




void Recognizer::Draw(cv::Mat &image,cv::Mat &shape,cv::Mat &con,cv::Mat &tri,cv::Mat &visi,std::string& name)
{
  int i,n = shape.rows/2; cv::Point p1,p2; cv::Scalar c;

  //draw triangulation
  c = CV_RGB(0,0,0);
  for(i = 0; i < tri.rows; i++){
    if(visi.at<int>(tri.at<int>(i,0),0) == 0 ||
       visi.at<int>(tri.at<int>(i,1),0) == 0 ||
       visi.at<int>(tri.at<int>(i,2),0) == 0)continue;
    p1 = cv::Point(shape.at<double>(tri.at<int>(i,0),0),
		   shape.at<double>(tri.at<int>(i,0)+n,0));
    p2 = cv::Point(shape.at<double>(tri.at<int>(i,1),0),
		   shape.at<double>(tri.at<int>(i,1)+n,0));
    cv::line(image,p1,p2,c);
    p1 = cv::Point(shape.at<double>(tri.at<int>(i,0),0),
		   shape.at<double>(tri.at<int>(i,0)+n,0));
    p2 = cv::Point(shape.at<double>(tri.at<int>(i,2),0),
		   shape.at<double>(tri.at<int>(i,2)+n,0));
    cv::line(image,p1,p2,c);
    p1 = cv::Point(shape.at<double>(tri.at<int>(i,2),0),
		   shape.at<double>(tri.at<int>(i,2)+n,0));
    p2 = cv::Point(shape.at<double>(tri.at<int>(i,1),0),
		   shape.at<double>(tri.at<int>(i,1)+n,0));
    cv::line(image,p1,p2,c);
  }
  //draw connections
  c = CV_RGB(0,0,255);
  for(i = 0; i < con.cols; i++){
    if(visi.at<int>(con.at<int>(0,i),0) == 0 ||
       visi.at<int>(con.at<int>(1,i),0) == 0)continue;
    p1 = cv::Point(shape.at<double>(con.at<int>(0,i),0),
		   shape.at<double>(con.at<int>(0,i)+n,0));
    p2 = cv::Point(shape.at<double>(con.at<int>(1,i),0),
		   shape.at<double>(con.at<int>(1,i)+n,0));
    cv::line(image,p1,p2,c,1);
  }
  //draw points
  for(i = 0; i < n; i++){    
    if(visi.at<int>(i,0) == 0)continue;
    p1 = cv::Point(shape.at<double>(i,0),shape.at<double>(i+n,0));
    c = CV_RGB(255,255,0); cv::circle(image,p1,2,c);
	char *text = new char[10];
	//sprintf(text,"%d",i);
		//cv::putText(image,text,p1,
		//CV_FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(255,255,255));
		delete []text;
  }return;
}



bool Recognizer::Init(int argc, char ** argv)
{
 /*std::string package_path_;
 SRCModel *src_model;
 FACETRACKER::Tracker* t_model;
 FACETRACKER::FDet* detector;
 cv::Mat* tri,con;
 char ftFile[256],conFile[256],triFile[256];
 */
 
			   string echo = "espeak --stdout 'hello, i am tinker' | aplay"; 
               printf("%s\n",echo.c_str());
               system(echo.c_str());
 
 
 package_path_ = ros::package::getPath("facerec") + "/";
 string src_model_file= package_path_+"model/sample.sr";
 std::strcpy(ftFile,(package_path_+"model/face2.tracker").c_str());
 std::strcpy(triFile,(package_path_+"model/face.tri").c_str());
 std::strcpy(conFile,(package_path_+"model/face.con").c_str());
 
 
 printf("package path:%s\n",package_path_.c_str());
 printf("src model file path:%s\n",src_model_file.c_str());
 printf("Ftfile:%s\n",ftFile);
 printf("Trifile:%s\n",triFile);
 printf("Confile:%s\n",conFile);
 
 
 detector  = new FACETRACKER::FDet();
 detector->Load((package_path_+"model/haarCascade.dat").c_str());
 
 t_model = new FACETRACKER::Tracker(ftFile);
 this->src_model = LoadSRCModel(src_model_file);
 tri=FACETRACKER::IO::LoadTri(triFile);
 con=FACETRACKER::IO::LoadCon(conFile);
 
 printf("Load ok!\n");
 
 ros::init(argc, argv, "FaceRec");
 n_ = new ros::NodeHandle;
 rate_ = new ros::Rate(33);
 sub_ = n_->subscribe("/speech_rec",1000, &Recognizer::callBack, this );
 ic_ = new ImageConverter();
  while (!ic_->ready)
  {
      ros::spinOnce();
        rate_->sleep();
        if (!ros::ok())
        {
            printf("Terminated by control-C in Init.\n");
            return false;
        }
   } 
     
    //delay
    ros::Duration(1.0).sleep();
 
 
 
 
 
 if (detector && t_model) 
  return true;
  
 else 
  return false;
}
int Recognizer::Run()
{
    string train_sample_list_file= this->package_path_+"archive/list_train";
    ofstream list;
    list.open(train_sample_list_file.c_str(),ios::out | ios::app);
    printf("run\n");
    cv::Mat frame;
    cv::Mat im;
    cv::Mat gray;
    std::vector<cv::Rect*> RectArr;
    findface = false;
    bool failed = true;
	bool show = true;
	model = false;
	 std::vector<int> wSize1(1); wSize1[0] = 7;
	 std::vector<int> wSize2(3); wSize2[0] = 11; wSize2[1] = 9; wSize2[2] = 7;
	  int nIter = 5; double clamp=3,fTol=0.01; 
 	 bool fcheck = false; double scale = 1; int fpd = -1; 
 	 std::string name = "unknown";   
 	 this->guestpicnum = 999;
    while (ros::ok())
    {
        ic_->curr_image.copyTo(frame);
        
        cv::flip(frame,im,1);
        cv::cvtColor(im,gray,CV_BGR2GRAY);
		if (!findface) {findface = detector->Detect(gray,RectArr);
		std::cout<<"facenum"<<RectArr.size()<<std::endl;
		if (findface)   std::cout<<"x:"<<RectArr[0]->x<<"y:"<<RectArr[0]->y<<"width:"<<RectArr[0]->width<<"height:"<<RectArr[0]->height<<std::endl;
		}
		
		//sort
		for (int i = 0;i<RectArr.size();i++)
			for (int j=0;j<i;j++)
		{
			if (RectArr[i]->width*RectArr[i]->height>RectArr[j]->width*RectArr[j]->height) 
			{
				cv::Rect* tmp = RectArr[i];
				RectArr[i] = RectArr[j];
				RectArr[j] = tmp; 
			}
				 
		}
	
	    for (int i=0;i<RectArr.size();i++)
		{
			rectangle(im,cv::Point(RectArr[i]->x,RectArr[i]->y),cv::Point(RectArr[i]->x+RectArr[i]->width,RectArr[i]->y+RectArr[i]->height),cv::Scalar(255,0,0));  
		}
		
		//std::cout<<findface<<std::endl;
	    //track the image
	    std::vector<int> wSize; if(failed)wSize = wSize2; else wSize = wSize1; 
		if (findface)
		if(t_model->Track(gray,wSize,cv::Rect(cv::Point(RectArr[0]->x,RectArr[0]->y),cv::Point(RectArr[0]->x+RectArr[0]->width,RectArr[0]->y+RectArr[0]->height)),fpd,nIter,clamp,fTol,fcheck) == 0){
		 int idx = t_model->_clm.GetViewIdx(); failed = false;
	         Draw(im,t_model->_shape,con,tri,t_model->_clm._visi[idx],name); 
	       }
		   else
		   {
			   std::cout<<"track failed"<<std::endl;
			   if(show){cv::Mat R(im,cvRect(0,0,150,50)); R = cv::Scalar(0,0,255);}
			    t_model->FrameReset(); failed = true;
		   }       
        cv::imshow("test", im);
        char c = cv::waitKey(3);
        if (!model) {
            t_model->FrameReset();
            model = true;
            }
            
            
        if (c == 'd') {
            findface = false;
            model = false;
            }   
        if(char(c) == 'x') model = false;
		if (c == '1') {
		
		    int idx = t_model->_clm.GetViewIdx();failed = false;
		    SaveImage(gray,t_model->_shape,con,tri,t_model->_clm._visi[idx],package_path_+"archive/1/1.jpg"); 
        }
        
         if (c=='b') {
			 _buildarchive();
		 }
		 
		 if ((state == TRAINING)&&(guestpicnum<20)&&(model)&&(findface))
		 {
		    string path = package_path_+"archive/"+"sample"+guestname+"num"+toString(guestpicnum)+".jpg";
		    printf("Saving image %s",path.c_str());
		    
		    int idx = t_model->_clm.GetViewIdx();failed = false;
		    SaveImage(gray,t_model->_shape,con,tri,t_model->_clm._visi[idx],path);
		    ++guestpicnum; 
		    model = false;
		    list<<path<<std::endl;
		 }
		 
		 
		 
		 
		 if ((((state == TESTING) && (!recognize))||(c == 'r'))&&(model)&&(findface)&&(count > 20))
		 {
		     int idx = t_model->_clm.GetViewIdx(); failed = false;
	         string filename = this->package_path_+"recognize/1.jpg";
			 SaveImage(gray,t_model->_shape,con,tri,t_model->_clm._visi[idx],filename); 
	 		 CvMat *y= LoadSample(filename, src_model->sample_size_);
			 printf("%s\n","load ok");
			 name= Recognize(src_model, y, sci_t, 
	 			(filename+".x").c_str(), (filename+".r").c_str());
			 std::cout<<name<<std::endl;
			 //printf("%s\n","rec");
			   string echo = "espeak --stdout 'hello "+name+"' | aplay"; 
               printf("%s\n",echo.c_str());
               system(echo.c_str());
			   recognize = true;
		 }
		 
		  
        
        ic_->ready = false;
        while (!ic_->ready && ros::ok())
        {
            ros::spinOnce();
        }
        rate_->sleep();
        count ++ ;
    }
    list.close();
    return 0;
}
