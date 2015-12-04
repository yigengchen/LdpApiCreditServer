FROM stephenwithav/centos7-gcc-hhvm:3.7
COPY . /usr/src/myapp
WORKDIR /usr/src/myapp
RUN yum install -y make && yum clean
RUN make clean && make
EXPOSE 8088
COPY start.sh /usr/src/myapp
CMD ./start.sh

