#!/bin/bash
export AN_PROJECT_DIR=/root/catkin_ws/src/darknet_ros

#echo "Copying bash aliases..." #{{{
#cp $AN_PROJECT_DIR/docker_scripts/sources/.bash_aliases /root
#}}}

#echo "Installing apt dependencies..." #{{{
#Packages for developing and testing
#apt-get -y install $(cat $AN_PROJECT_DIR/docker_scripts/sources/pkg_dev)
#Packages for deploy
# apt-get -y install $(cat $AN_PROJECT_DIR/docker_scripts/sources/pkg_deps)
#}}}

#script to run with a host network
#export ROS_MASTER_URI=http://$(hostname --ip-address):11311
#export ROS_HOSTNAME=$(hostname --ip-address)

# setup ros environment
source /opt/ros/melodic/setup.bash
source /root/catkin_ws/devel/setup.bash

#exec "$@"
echo "ROS INITIALIZED"
roslaunch darknet_ros darknet_ros.launch
