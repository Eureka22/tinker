// Project      : reading_pointcloud
// File         : sub_xyz.cpp
// Author       : bss
// Creation Date: 2014-07-12
// Last modified: 2014-07-12, 04:14:19
// Description  : convert ros-style pointcloud to pcl-style.
// 

#include <stdio.h>
#include <string>
#include <ros/ros.h>
#include <reading_pointcloud/reading_pointcloud.h>

int main(int argc, char** argv)
{
    ros::init(argc, argv, "test_sub_cloud_xyz");
    ros::NodeHandle n;
    ros::Rate rate(33);

    CloudConverter* cc_ = new CloudConverter();

    while (!cc_->ready_xyz_)
    {
        ros::spinOnce();
        rate.sleep();
        if (!ros::ok())
        {
            printf("Terminated by C-c.\n");
            return -1;
        }
    }

    while (ros::ok())
    {
        pcl::PointCloud<pcl::PointXYZ>::ConstPtr msg = cc_->msg_xyz_;
        printf ("%s, Cloud: width = %d, height = %d\n", msg->header.frame_id.c_str(), msg->width, msg->height);
        rate.sleep();
    }

    delete cc_;
    cc_ = NULL;

    return 0;
}
