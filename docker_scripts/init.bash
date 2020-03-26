#!/bin/bash
#export AN_PROJECT_DIR=/root/catkin_ws/src/darknet_ros

#echo "Copying bash aliases..." #{{{
#cp $AN_PROJECT_DIR/docker_scripts/sources/.bash_aliases /root
#}}}

#echo "Installing apt dependencies..." #{{{
#Packages for developing and testing
#apt-get -y install $(cat $AN_PROJECT_DIR/docker_scripts/sources/pkg_dev)
#Packages for deploy
# apt-get -y install $(cat $AN_PROJECT_DIR/docker_scripts/sources/pkg_deps)
#}}}

echo "Setting up ros environment..." #{{{
cd /root/
mkdir -p catkin_ws/src

# Build autonomous_navigation_project
source /opt/ros/melodic/setup.bash
cd /root/catkin_ws && catkin build 
#}}}

