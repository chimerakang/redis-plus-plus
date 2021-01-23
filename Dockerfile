#from ubuntu:18.04 as build-env
ARG BASE_IMAGE=nvcr.io/nvidia/l4t-base:r32.4.4

FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /workspace

# change the locale from POSIX to UTF-8
RUN locale-gen en_US en_US.UTF-8 && update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
ENV LANG=en_US.UTF-8

ARG OPENCV_VERSION='4.4.0'
ARG GPU_ARCH='6.1'
WORKDIR /opt

# Build tools
RUN apt update && \
    apt install -y \
    sudo \
    tzdata \
    git \
    cmake \
    wget \
    unzip \
    build-essential

# Media I/O:
RUN sudo apt install -y \
    zlib1g-dev \
    libjpeg-dev \
    libwebp-dev \
    libpng-dev \
    libtiff5-dev \
    libopenexr-dev \
    libgdal-dev \
    libgtk2.0-dev

# Video I/O:
RUN sudo apt install -y \
    libdc1394-22-dev \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    libtheora-dev \
    libvorbis-dev \
    libxvidcore-dev \
    libx264-dev \
    yasm \
    libopencore-amrnb-dev \
    libopencore-amrwb-dev \
    libv4l-dev \
    libxine2-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libopencv-highgui-dev \
    ffmpeg

# Parallelism and linear algebra libraries:
RUN sudo apt install -y \
    libtbb-dev \
    libeigen3-dev

# Build OpenCV
RUN wget https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip && \
    unzip ${OPENCV_VERSION}.zip && rm ${OPENCV_VERSION}.zip && \
    mv opencv-${OPENCV_VERSION} OpenCV && \
    cd OpenCV && \
    wget https://github.com/opencv/opencv_contrib/archive/${OPENCV_VERSION}.zip && \
    unzip ${OPENCV_VERSION}.zip && \
    mkdir build && \
    cd build && \
    cmake \
      -D WITH_TBB=ON \
      -D CMAKE_BUILD_TYPE=RELEASE \
      -D BUILD_EXAMPLES=OFF \
      -D WITH_FFMPEG=ON \
      -D WITH_V4L=ON \
      -D WITH_OPENGL=ON \
      -D WITH_CUDA=ON \
      -D CUDA_ARCH_BIN=${GPU_ARCH} \
      -D CUDA_ARCH_PTX=${GPU_ARCH} \
      -D WITH_CUBLAS=ON \
      -D WITH_CUFFT=ON \
      -D WITH_EIGEN=ON \
      -D EIGEN_INCLUDE_PATH=/usr/include/eigen3 \
      -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib-${OPENCV_VERSION}/modules/ \
      .. && \
    make && \
    make install

# Install prerequisites
run apt-get install -y \
#    build-essential \
#    cmake \
    curl \
    git \
    libcurl3-dev \
#    libleptonica-dev \
#    liblog4cplus-dev \
#    libopencv-dev \
#    libtesseract-dev \
    libmysqld-dev \
    libcurl4-openssl-dev \
    libev-dev \
    libhiredis-dev \
    wget

# Copy execute data

workdir /app/netdoalpr
COPY ./build/camera_server/redis-camera-client .
COPY ./build/camera_server/libtensorflow_cc.so .
COPY ./build/camera_server/libtensorflow_cc.so.2 .
COPY ./build/camera_server/libtensorflow_cc.so.2.4.0 .
COPY ./build/camera_server/libtensorflow_framework.so.2 .
COPY ./build/camera_server/libtensorflow_framework.so.2.4.0 .
COPY ./build/camera_server/libultimate_alpr-sdk.so .

#COPY /usr/local/lib/libopencv_videoio.so.4.4 /usr/local/lib
#COPY /usr/local/lib/libultimate_alpr-sdk.so /usr/local/lib
#COPY /usr/local/lib/libhiredis.so.1.0.1-dev /usr/local/lib
#COPY /usr/local/lib/libopencv_imgcodecs.so.4.4 /usr/local/lib
#COPY /usr/local/lib/libopencv_imgproc.so.4.4 /usr/local/lib
#COPY /usr/local/lib/libopencv_core.so.4.4 /usr/local/lib

# copy config.json
RUN mkdir -p /app/netdoalpr/cfg
COPY cfg/config.json cfg/

# export setting
EXPOSE 10500
EXPOSE 10501
EXPOSE 10502
EXPOSE 10503
EXPOSE 10504

RUN ls -l
RUN ldd redis-camera-client
CMD ["./redis-camera-client"]
