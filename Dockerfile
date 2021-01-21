from ubuntu:18.04 as build-env

# Install prerequisites
run apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    curl \
    git \
    libcurl3-dev \
    libleptonica-dev \
    liblog4cplus-dev \
    libopencv-dev \
    libtesseract-dev \
    libmysqld-dev \
    libcurl4-openssl-dev \
    libev-dev \
    libhiredis-dev \
    wget

# Copy all data
copy . /srv/netdoalpr

# cp alpr header and lib file
COPY ./libultimate_alpr-sdk.so /usr/local/lib/
COPY ./camera_server/ultimateALPR-SDK-API-PUBLIC.h /usr/local/include/

workdir /src/netdoalpr
run rm -rf build/

# Setup the build directory
workdir /srv/netdoalpr/build


# Setup the compile environment
run cmake .. && \
    make -j2

# alpine mirror
FROM alpine:3.12

RUN mkdir -p /app/netdoalpr

WORKDIR /app/netdoalpr

COPY --from=build-env /srv/netdoalpr/build/camera_server/redis-camera-client .

EXPOSE 10500
EXPOSE 10501
EXPOSE 10502
EXPOSE 10503
EXPOSE 10504

CMD ["./redis-camera-client", "-g", "daemon off;"]
