# Build:
#  docker build -t your_docker_user/yoloros_cpu .

# Get ros full image from dockerhub
FROM ros
LABEL mantainer "Dario Cazzato <dcazzato85@gmail.com> <if you contribute, add your name here>"

ENV ROS_DISTRO melodic

RUN apt-get update && apt-get install -y \
    #ros-melodic-usb-cam \
    apt-utils \
    ros-${ROS_DISTRO}-usb-cam \
    python-rosdep \
    python-rosinstall \
    python-vcstools \
    python-catkin-tools \
    ros-${ROS_DISTRO}-vision-opencv \
    ros-${ROS_DISTRO}-compressed-image-transport \
    ros-${ROS_DISTRO}-tf2 \
    ros-${ROS_DISTRO}-robot \
    ros-${ROS_DISTRO}-perception \
    libboost-all-dev \
    libboost-dev
    #apt-get -y clean &&\
    #apt-get -y purge &&\
    #rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Copy your project to /catkin_ws/src
WORKDIR /root/

# Path for the container
COPY . ./catkin_ws/src/darknet_ros
WORKDIR /root/catkin_ws/src/darknet_ros/

RUN bash -c './docker_scripts/init.bash'

ENTRYPOINT ["/root/catkin_ws/src/darknet_ros/docker_scripts/ros_entrypoint.bash"]
CMD ["bash"]

