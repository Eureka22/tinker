/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *	
 * Author: Nico Blodow (blodow@cs.tum.edu), Julius Kammerl (julius@kammerl.de)
 */

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/openni_grabber.h>
#include <pcl/io/openni_grabber.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/octree/octree.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/console/parse.h>
#include <ros/ros.h>
#include <pcl_ros/point_cloud.h>

enum 
{
  REDDIFF_MODE,
  ONLYDIFF_MODE,
  MODE_COUNT
};

class OpenNIChangeViewer
{
  public:
    OpenNIChangeViewer (double resolution, int mode, int noise_filter)
      : viewer ("PCL OpenNI Viewer")
    {
      octree = new pcl::octree::OctreePointCloudChangeDetector<pcl::PointXYZRGB>(resolution);
      mode_ = mode;
      noise_filter_ = noise_filter;
    }

    void 
    cloud_cb_ (const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &cloud)
    {

      float avg_x = 0;
      float avg_y = 0;
      int num = 0;
      float avg_dep = 0;

      std::cerr << cloud->points.size() << " -- ";

      // assign point cloud to octree
      octree->setInputCloud (cloud);

      // add points from cloud to octree
      octree->addPointsFromInputCloud ();

      std::cerr << octree->getLeafCount() << " -- ";
      boost::shared_ptr<std::vector<int> > newPointIdxVector (new std::vector<int>);

      // get a vector of new points, which did not exist in previous buffer
      octree->getPointIndicesFromNewVoxels (*newPointIdxVector, noise_filter_);

      std::cerr << newPointIdxVector->size() << std::endl;

      pcl::PointCloud<pcl::PointXYZRGB>::Ptr filtered_cloud;

      switch (mode_) 
      {
        case REDDIFF_MODE:
          filtered_cloud.reset (new pcl::PointCloud<pcl::PointXYZRGB> (*cloud));
          filtered_cloud->points.reserve(newPointIdxVector->size());

          for (std::vector<int>::iterator it = newPointIdxVector->begin (); it != newPointIdxVector->end (); it++)
          {
            avg_x += filtered_cloud->points[*it].x;
            avg_y += filtered_cloud->points[*it].y;
            avg_dep += filtered_cloud->points[*it].z;
            num++;
            filtered_cloud->points[*it].rgba = 255<<16;
          }

          if (!viewer.wasStopped())
            viewer.showCloud (filtered_cloud);

          std::cout << "avg_x "  << avg_x / num << " avg_y " << avg_y / num << " agv_dep " << avg_dep / num << std::endl; 

          break;
        case ONLYDIFF_MODE:
          filtered_cloud.reset (new pcl::PointCloud<pcl::PointXYZRGB>);

          filtered_cloud->points.reserve(newPointIdxVector->size());

          for (std::vector<int>::iterator it = newPointIdxVector->begin (); it != newPointIdxVector->end (); it++)
            filtered_cloud->points.push_back(cloud->points[*it]);


          if (!viewer.wasStopped())
            viewer.showCloud (filtered_cloud);
          break;
      }
      
      // switch buffers - reset tree
      octree->switchBuffers ();
    }
    
    void 
    run ()
    {
      //pcl::Grabber* interface = new pcl::OpenNIGrabber();

      //boost::function<void (const pcl::PointCloud<pcl::PointXYZRGBA>::ConstPtr&)> f = 
        //boost::bind (&OpenNIChangeViewer::cloud_cb_, this, _1);

      //boost::signals2::connection c = interface->registerCallback (f);
      
      //interface->start ();
      
      while (!viewer.wasStopped())
      {
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        ros::spinOnce();
      }

      //interface->stop ();
    }

    pcl::octree::OctreePointCloudChangeDetector<pcl::PointXYZRGB> *octree;
    pcl::visualization::CloudViewer viewer;

    int mode_;
    int noise_filter_;
};

int 
main (int argc, char* argv[])
{
  ros::init(argc, argv, "change_detection");
  ros::NodeHandle nh;
  int mode = REDDIFF_MODE;
  int noise_filter = 20;
  double resolution = 0.01;
  OpenNIChangeViewer v (resolution, mode, noise_filter);
  ros::Subscriber sub = nh.subscribe<pcl::PointCloud<pcl::PointXYZRGB> >("/openni/points2", 1, &OpenNIChangeViewer::cloud_cb_, &v);

  std::cout << "Syntax is " << argv[0] << " [-r octree resolution] [-d] [-n noise_filter intensity] \n";


  //pcl::console::parse_argument (argc, argv, "-r", resolution);

  //pcl::console::parse_argument (argc, argv, "-n", noise_filter);

  //if (pcl::console::find_argument (argc, argv, "-d")>0) {
    //mode = ONLYDIFF_MODE;
  //}


  v.run ();
  return 0;
}
