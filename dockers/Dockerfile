FROM nvcr.io/nvidia/l4t-tensorflow:r32.4.4-tf2.3-py3

WORKDIR /
RUN apt update && apt install -y --fix-missing make g++ python3-pip 

RUN apt install -y zlib1g-dev zip libjpeg8-dev liblapack-dev libblas-dev && \
    pip3 install --no-cache-dir --upgrade pip  && \
    pip3 install --no-cache-dir --upgrade testresources setuptools cython && \
    apt-get clean

RUN apt install -y \
    sudo \
    git \
    cmake \
    build-essential \
    curl \
    wget \
    libmysqld-dev \
    libcurl4-openssl-dev \
    libev-dev \
    libhiredis-dev \
    unzip


RUN apt-get install -y \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libeigen3-dev \
    libglew-dev \
    libgtk2.0-dev \
    libgtk-3-dev \
    libjpeg-dev \
    libpng-dev \
    libpostproc-dev \
    libswscale-dev \
    libtbb-dev \
    libtiff5-dev \
    libv4l-dev \
    libxvidcore-dev \
    libx264-dev \
    zlib1g-dev \
    pkg-config

ARG OPENCV_VERSION='4.4.0'
ARG GPU_ARCH='5.3'
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
      -D BUILD_TIFF=ON \
      -D WITH_FFMPEG=ON \
      -D WITH_GSTREAMER=ON \
      -D EIGEN_INCLUDE_PATH=/usr/include/eigen3 \
      -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib-${OPENCV_VERSION}/modules/ \
      .. 

WORKDIR /OpenCV/build
RUN make && make install

WORKDIR /app
RUN git clone https://github.com/DoubangoTelecom/ultimateALPR-SDK.git
#RUN git clone https://github.com/chimerakang/redis-plus-plus.git

WORKDIR /app/ultimateALPR-SDK/binaries/jetson_tftrt/aarch64
RUN sh prepare.sh
RUN cp lib* /usr/local/lib
WORKDIR /app/ultimateALPR-SDK/c++
RUN cp *.h /usr/local/include

WORKDIR /app
RUN apt-get install -y nano
RUN git clone https://github.com/chimerakang/redis-plus-plus.git

WORKDIR /app/redis-plus-plus
RUN git checkout master && mkdir build 
WORKDIR /app/redis-plus-plus/build
RUN cmake .. && make

WORKDIR /app/redis-plus-plus/build/camera_server
RUN mkdir -p cfg && cp -rf /app/ultimateALPR-SDK/assets .
RUN cp /app/ultimateALPR-SDK/binaries/jetson_tftrt/aarch64/lib* .

# export setting
EXPOSE 10500
EXPOSE 10501
EXPOSE 10502
EXPOSE 10503
EXPOSE 10504

CMD [ "bash" ]
#CMD ["./redis-camera-client"]
